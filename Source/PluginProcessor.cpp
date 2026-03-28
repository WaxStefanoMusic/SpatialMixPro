#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

static AudioProcessor::BusesProperties getDefaultBuses()
{
    return AudioProcessor::BusesProperties()
           .withInput ("Input",  AudioChannelSet::stereo(), true)
           .withOutput("Output", AudioChannelSet::stereo(), true);
}

SpatialMixProProcessor::SpatialMixProProcessor()
    : AudioProcessor(getDefaultBuses())
{
    addParameter(paramX     = new AudioParameterFloat ({"x",  1}, "Pos X",    -8.f,  8.f,  0.f));
    addParameter(paramY     = new AudioParameterFloat ({"y",  1}, "Pos Y",     0.f,  6.f,  1.5f));
    addParameter(paramZ     = new AudioParameterFloat ({"z",  1}, "Pos Z",    -8.f,  8.f, -3.f));
    addParameter(paramGain  = new AudioParameterFloat ({"g",  1}, "Volume",    0.f,  2.f,  1.f));
    addParameter(paramMode  = new AudioParameterChoice({"md", 1}, "Mode",
                              StringArray{"HRTF Binaural","Stereo","2.1 Sim"}, 0));
    addParameter(paramMono  = new AudioParameterBool  ({"mn", 1}, "Mono",   false));
    addParameter(paramPhase = new AudioParameterBool  ({"ph", 1}, "Phase",  false));
    addParameter(paramDual  = new AudioParameterBool  ({"dl", 1}, "Dual",   false));
    addParameter(paramX2    = new AudioParameterFloat ({"x2", 1}, "Pos X2", -8.f,  8.f,  2.f));
    addParameter(paramY2    = new AudioParameterFloat ({"y2", 1}, "Pos Y2",  0.f,  6.f,  1.5f));
    addParameter(paramZ2    = new AudioParameterFloat ({"z2", 1}, "Pos Z2", -8.f,  8.f, -3.f));
    addParameter(paramGain2 = new AudioParameterFloat ({"g2", 1}, "Volume2", 0.f,  2.f,  1.f));
}

SpatialMixProProcessor::~SpatialMixProProcessor() {}

void SpatialMixProProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    dBufSize = jmax(64, (int)(0.0013 * sampleRate) + 4);
    dBufL1.assign(dBufSize, 0.f); dBufR1.assign(dBufSize, 0.f);
    dBufL2.assign(dBufSize, 0.f); dBufR2.assign(dBufSize, 0.f);
    dWritePos1 = dWritePos2 = 0;

    dsp::ProcessSpec spec { sampleRate, (uint32)samplesPerBlock, 1 };
    for (auto* f : { &elevFL1, &elevFR1, &elevFL2, &elevFR2,
                     &rearFL1, &rearFR1, &rearFL2, &rearFR2,
                     &subFilterL, &subFilterR })
    { f->prepare(spec); f->reset(); }

    auto subC = dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 120.f, 0.707f);
    subFilterL.coefficients = subC; subFilterR.coefficients = subC;

    auto rearC = dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 3500.f, 0.707f);
    rearFL1.coefficients = rearC; rearFR1.coefficients = rearC;
    rearFL2.coefficients = rearC; rearFR2.coefficients = rearC;

    const double sm = 0.05;
    smX   .reset(sampleRate, sm); smX   .setCurrentAndTargetValue(paramX->get());
    smY   .reset(sampleRate, sm); smY   .setCurrentAndTargetValue(paramY->get());
    smZ   .reset(sampleRate, sm); smZ   .setCurrentAndTargetValue(paramZ->get());
    smGain.reset(sampleRate, sm); smGain.setCurrentAndTargetValue(paramGain->get());
    smX2  .reset(sampleRate, sm); smX2  .setCurrentAndTargetValue(paramX2->get());
    smY2  .reset(sampleRate, sm); smY2  .setCurrentAndTargetValue(paramY2->get());
    smZ2  .reset(sampleRate, sm); smZ2  .setCurrentAndTargetValue(paramZ2->get());
    smGain2.reset(sampleRate,sm); smGain2.setCurrentAndTargetValue(paramGain2->get());
    smRear1.reset(sampleRate, 0.1); smRear1.setCurrentAndTargetValue(0.f);
    smRear2.reset(sampleRate, 0.1); smRear2.setCurrentAndTargetValue(0.f);
}

void SpatialMixProProcessor::releaseResources() {}

void SpatialMixProProcessor::updateElevFilter(float elevDeg,
    dsp::IIR::Filter<float>& fl, dsp::IIR::Filter<float>& fr)
{
    float gainLin = 1.f + jmax(0.f, (elevDeg - 10.f) / 80.f) * 2.f;
    auto c = dsp::IIR::Coefficients<float>::makeHighShelf(
                 currentSampleRate, 8000.f, 0.707f, gainLin);
    fl.coefficients = c; fr.coefficients = c;
}

void SpatialMixProProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 2) return;

    const int  numSamples = buffer.getNumSamples();
    const int  mode       = paramMode->getIndex();
    const bool isDual     = paramDual->get();
    const bool isMono     = paramMono->get();
    const bool isPhase    = paramPhase->get();

    auto* chL = buffer.getWritePointer(0);
    auto* chR = buffer.getWritePointer(1);

    // Update elevation filters once per block
    {
        float cx = smX.getTargetValue(), cy = smY.getTargetValue(), cz = smZ.getTargetValue();
        float hd = std::sqrt(cx*cx + cz*cz);
        float el = std::atan2(cy, jmax(hd, 0.001f)) * (180.f / MathConstants<float>::pi);
        updateElevFilter(el, elevFL1, elevFR1);

        if (isDual) {
            float cx2 = smX2.getTargetValue(), cy2 = smY2.getTargetValue(), cz2 = smZ2.getTargetValue();
            float hd2 = std::sqrt(cx2*cx2 + cz2*cz2);
            float el2 = std::atan2(cy2, jmax(hd2, 0.001f)) * (180.f / MathConstants<float>::pi);
            updateElevFilter(el2, elevFL2, elevFR2);
        }
    }

    // Update rear targets
    {
        float az1 = std::atan2(paramX->get(), -(paramZ->get()));
        smRear1.setTargetValue(jmax(0.f, -std::cos(az1)));
        if (isDual) {
            float az2 = std::atan2(paramX2->get(), -(paramZ2->get()));
            smRear2.setTargetValue(jmax(0.f, -std::cos(az2)));
        }
    }

    smX.setTargetValue(paramX->get());
    smY.setTargetValue(paramY->get());
    smZ.setTargetValue(paramZ->get());
    smGain.setTargetValue(paramGain->get());
    smX2.setTargetValue(paramX2->get());
    smY2.setTargetValue(paramY2->get());
    smZ2.setTargetValue(paramZ2->get());
    smGain2.setTargetValue(paramGain2->get());

    for (int i = 0; i < numSamples; ++i)
    {
        const float x     = smX.getNextValue();
        const float y     = smY.getNextValue();
        const float z     = smZ.getNextValue();
        const float gain  = smGain.getNextValue();
        const float rear1 = smRear1.getNextValue();

        const float monoIn = (chL[i] + chR[i]) * 0.5f;

        // ── Speaker 1 ──────────────────────────────────────────
        const float dist1  = jmax(0.1f, std::sqrt(x*x + y*y + z*z));
        const float dGain1 = 1.f / dist1;
        const float az1    = std::atan2(x, -z);
        const float sinAz1 = std::sin(az1);
        const float panL1  = std::sqrt(jmax(0.f, 0.5f * (1.f - sinAz1)));
        const float panR1  = std::sqrt(jmax(0.f, 0.5f * (1.f + sinAz1)));

        float sig1  = monoIn * gain * dGain1;
        float sigL1 = sig1 * panL1;
        float sigR1 = sig1 * panR1;

        float outL1, outR1;
        if (mode == 0)
        {
            const float itdSec  = (0.215f / 343.f) * std::sin(az1);
            const int   itdSamp = jlimit(0, dBufSize - 1, (int)(std::abs(itdSec) * currentSampleRate));
            dBufL1[dWritePos1] = sigL1;
            dBufR1[dWritePos1] = sigR1;
            int rL1 = dWritePos1, rR1 = dWritePos1;
            if (az1 > 0.f) rL1 = (dWritePos1 - itdSamp + dBufSize) % dBufSize;
            else            rR1 = (dWritePos1 - itdSamp + dBufSize) % dBufSize;
            float eL1 = elevFL1.processSample(dBufL1[rL1]);
            float eR1 = elevFR1.processSample(dBufR1[rR1]);
            float rlL1 = rearFL1.processSample(eL1);
            float rlR1 = rearFR1.processSample(eR1);
            outL1 = eL1 * (1.f - rear1) + rlL1 * rear1;
            outR1 = eR1 * (1.f - rear1) + rlR1 * rear1;
            dWritePos1 = (dWritePos1 + 1) % dBufSize;
        }
        else { outL1 = sigL1; outR1 = sigR1; }

        float outL = outL1, outR = outR1;

        // ── Speaker 2 (dual mode) ───────────────────────────────
        if (isDual)
        {
            const float x2    = smX2.getNextValue();
            const float y2    = smY2.getNextValue();
            const float z2    = smZ2.getNextValue();
            const float g2    = smGain2.getNextValue();
            const float rear2 = smRear2.getNextValue();

            const float dist2  = jmax(0.1f, std::sqrt(x2*x2 + y2*y2 + z2*z2));
            const float dGain2 = 1.f / dist2;
            const float az2    = std::atan2(x2, -z2);
            const float sinAz2 = std::sin(az2);
            const float panL2  = std::sqrt(jmax(0.f, 0.5f * (1.f - sinAz2)));
            const float panR2  = std::sqrt(jmax(0.f, 0.5f * (1.f + sinAz2)));

            float sig2 = monoIn * g2 * dGain2;
            float sL2  = sig2 * panL2;
            float sR2  = sig2 * panR2;
            float outL2, outR2;

            if (mode == 0)
            {
                const float itdSec2 = (0.215f / 343.f) * std::sin(az2);
                const int   itdS2   = jlimit(0, dBufSize - 1, (int)(std::abs(itdSec2) * currentSampleRate));
                dBufL2[dWritePos2] = sL2;
                dBufR2[dWritePos2] = sR2;
                int rL2 = dWritePos2, rR2 = dWritePos2;
                if (az2 > 0.f) rL2 = (dWritePos2 - itdS2 + dBufSize) % dBufSize;
                else            rR2 = (dWritePos2 - itdS2 + dBufSize) % dBufSize;
                float eL2  = elevFL2.processSample(dBufL2[rL2]);
                float eR2  = elevFR2.processSample(dBufR2[rR2]);
                float rlL2 = rearFL2.processSample(eL2);
                float rlR2 = rearFR2.processSample(eR2);
                outL2 = eL2 * (1.f - rear2) + rlL2 * rear2;
                outR2 = eR2 * (1.f - rear2) + rlR2 * rear2;
                dWritePos2 = (dWritePos2 + 1) % dBufSize;
            }
            else { outL2 = sL2; outR2 = sR2; }

            outL = (outL1 + outL2) * 0.5f;
            outR = (outR1 + outR2) * 0.5f;
        }

        // ── Options ────────────────────────────────────────────
        if (isMono)  { outL = outR = (outL + outR) * 0.5f; }
        if (isPhase) { outL = -outL; outR = -outR; }

        if (mode == 2) {
            float sub = (subFilterL.processSample(outL) + subFilterR.processSample(outR)) * 0.3f;
            outL += sub; outR += sub;
        }

        chL[i] = outL;
        chR[i] = outR;
    }
}

AudioProcessorEditor* SpatialMixProProcessor::createEditor()
{
    return new SpatialMixProEditor(*this);
}

void SpatialMixProProcessor::getStateInformation(MemoryBlock& destData)
{
    ValueTree st("SMP2");
    st.setProperty("name",  sourceName,             nullptr);
    st.setProperty("x",     paramX->get(),           nullptr);
    st.setProperty("y",     paramY->get(),            nullptr);
    st.setProperty("z",     paramZ->get(),            nullptr);
    st.setProperty("g",     paramGain->get(),         nullptr);
    st.setProperty("mode",  paramMode->getIndex(),    nullptr);
    st.setProperty("mono",  (bool)paramMono->get(),   nullptr);
    st.setProperty("phase", (bool)paramPhase->get(),  nullptr);
    st.setProperty("dual",  (bool)paramDual->get(),   nullptr);
    st.setProperty("x2",    paramX2->get(),           nullptr);
    st.setProperty("y2",    paramY2->get(),            nullptr);
    st.setProperty("z2",    paramZ2->get(),            nullptr);
    st.setProperty("g2",    paramGain2->get(),         nullptr);
    MemoryOutputStream os(destData, true);
    st.writeToStream(os);
}

void SpatialMixProProcessor::setStateInformation(const void* data, int sz)
{
    auto st = ValueTree::readFromData(data, (size_t)sz);
    if (!st.isValid()) return;
    sourceName = st.getProperty("name", "Object 1").toString();
    *paramX     = (float)st.getProperty("x",    0.f);
    *paramY     = (float)st.getProperty("y",    1.5f);
    *paramZ     = (float)st.getProperty("z",   -3.f);
    *paramGain  = (float)st.getProperty("g",    1.f);
    paramMode->setValueNotifyingHost((int)st.getProperty("mode", 0) / 2.f);
    paramMono ->setValueNotifyingHost((bool)st.getProperty("mono",  false) ? 1.f : 0.f);
    paramPhase->setValueNotifyingHost((bool)st.getProperty("phase", false) ? 1.f : 0.f);
    paramDual ->setValueNotifyingHost((bool)st.getProperty("dual",  false) ? 1.f : 0.f);
    *paramX2    = (float)st.getProperty("x2",   2.f);
    *paramY2    = (float)st.getProperty("y2",   1.5f);
    *paramZ2    = (float)st.getProperty("z2",  -3.f);
    *paramGain2 = (float)st.getProperty("g2",   1.f);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SpatialMixProProcessor(); }
