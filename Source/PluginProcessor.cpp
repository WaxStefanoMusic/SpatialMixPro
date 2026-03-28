#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;

// ── Build 8 stereo input buses + 1 stereo output ─────────────
BusesProperties SpatialMixProProcessor::buildBuses()
{
    BusesProperties bp;
    const char* names[] = {
        "Source 1","Source 2","Source 3","Source 4",
        "Source 5","Source 6","Source 7","Source 8"
    };
    // First bus is always enabled; rest start disabled (FL Studio enables them when you route)
    bp = bp.withInput(names[0], AudioChannelSet::stereo(), true);
    for (int i = 1; i < MAX_SOURCES; ++i)
        bp = bp.withInput(names[i], AudioChannelSet::stereo(), false);
    bp = bp.withOutput("Master Out", AudioChannelSet::stereo(), true);
    return bp;
}

SpatialMixProProcessor::SpatialMixProProcessor()
    : AudioProcessor(buildBuses())
{
    addParameter(paramMode = new AudioParameterChoice({"mode",1},"Output Mode",
                 StringArray{"HRTF Binaural","Stereo","2.1 Sim"}, 0));
    addParameter(paramMasterGain = new AudioParameterFloat({"mg",1},"Master Gain", 0.f, 2.f, 1.f));

    // Default source names matching orchestral layout
    const char* defNames[] = {
        "Violini I","Violini II","Viole","Violoncelli",
        "Contrabbassi","Legni","Ottoni","Percussioni"
    };
    // Default positions (orchestra layout, metres)
    float defX[] = { -3.f, -1.f,  1.f,  3.f,  6.f, -1.f,  0.f,  6.5f };
    float defY[] = {  1.4f, 1.4f, 1.4f, 1.4f, 1.5f, 1.6f, 1.7f,  1.6f };
    float defZ[] = { -5.f, -5.f, -5.f, -5.f, -4.f, -2.f,  1.f,   1.5f };

    for (int i = 0; i < MAX_SOURCES; ++i)
    {
        auto& s = sources[i];
        s.name = defNames[i];
        String pfx = "s" + String(i);
        addParameter(s.paramX     = new AudioParameterFloat ({pfx+"x", 1}, s.name+" X",    -8.f, 8.f, defX[i]));
        addParameter(s.paramY     = new AudioParameterFloat ({pfx+"y", 1}, s.name+" Y",     0.f, 6.f, defY[i]));
        addParameter(s.paramZ     = new AudioParameterFloat ({pfx+"z", 1}, s.name+" Z",    -8.f, 8.f, defZ[i]));
        addParameter(s.paramGain  = new AudioParameterFloat ({pfx+"g", 1}, s.name+" Vol",   0.f, 2.f, 1.f));
        addParameter(s.paramMono  = new AudioParameterBool  ({pfx+"m", 1}, s.name+" Mono",  false));
        addParameter(s.paramPhase = new AudioParameterBool  ({pfx+"p", 1}, s.name+" Phase", false));
        addParameter(s.paramActive= new AudioParameterBool  ({pfx+"a", 1}, s.name+" Active",true));
    }
}

SpatialMixProProcessor::~SpatialMixProProcessor() {}

// ============================================================
void SpatialMixProProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    dsp::ProcessSpec spec{sampleRate,(uint32)samplesPerBlock,1};

    for (int i = 0; i < MAX_SOURCES; ++i)
    {
        auto& s = sources[i];
        s.dBufSize = jmax(64,(int)(0.0013*sampleRate)+4);
        s.dBufL.assign(s.dBufSize,0.f);
        s.dBufR.assign(s.dBufSize,0.f);
        s.dWritePos = 0;

        for (auto* f : {&s.elevFL,&s.elevFR,&s.rearFL,&s.rearFR})
        { f->prepare(spec); f->reset(); }

        auto rearC = dsp::IIR::Coefficients<float>::makeLowPass(sampleRate,3500.f,0.707f);
        s.rearFL.coefficients = rearC; s.rearFR.coefficients = rearC;

        const double sm = 0.05;
        s.smX   .reset(sampleRate,sm); s.smX   .setCurrentAndTargetValue(s.paramX->get());
        s.smY   .reset(sampleRate,sm); s.smY   .setCurrentAndTargetValue(s.paramY->get());
        s.smZ   .reset(sampleRate,sm); s.smZ   .setCurrentAndTargetValue(s.paramZ->get());
        s.smGain.reset(sampleRate,sm); s.smGain.setCurrentAndTargetValue(s.paramGain->get());
        s.smRear.reset(sampleRate,0.1f); s.smRear.setCurrentAndTargetValue(0.f);
    }

    subFilterL.prepare(spec); subFilterR.prepare(spec);
    subFilterL.reset(); subFilterR.reset();
    auto subC = dsp::IIR::Coefficients<float>::makeLowPass(sampleRate,120.f,0.707f);
    subFilterL.coefficients = subC; subFilterR.coefficients = subC;

    smMaster.reset(sampleRate,0.05);
    smMaster.setCurrentAndTargetValue(paramMasterGain->get());
}

void SpatialMixProProcessor::releaseResources() {}

// ============================================================
void SpatialMixProProcessor::updateElevFilter(float elevDeg, SpatialSource& src)
{
    float gainLin = 1.f + jmax(0.f,(elevDeg-10.f)/80.f)*2.f;
    auto c = dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate,8000.f,0.707f,gainLin);
    src.elevFL.coefficients = c; src.elevFR.coefficients = c;
}

// ── Per-source spatialization ─────────────────────────────────
void SpatialMixProProcessor::processSource(int idx,
    const float* inL, const float* inR,
    float* outL, float* outR, int numSamples, int mode)
{
    auto& s = sources[idx];

    // Update elevation filter once per block
    float cx=s.smX.getTargetValue(), cy=s.smY.getTargetValue(), cz=s.smZ.getTargetValue();
    float hd=std::sqrt(cx*cx+cz*cz);
    float el=std::atan2(cy,jmax(hd,0.001f))*(180.f/MathConstants<float>::pi);
    updateElevFilter(el, s);

    float azTarget = std::atan2(cx,-cz);
    s.smRear.setTargetValue(jmax(0.f,-std::cos(azTarget)));

    s.smX.setTargetValue(s.paramX->get());
    s.smY.setTargetValue(s.paramY->get());
    s.smZ.setTargetValue(s.paramZ->get());
    s.smGain.setTargetValue(s.paramGain->get());

    const bool isMono  = s.paramMono->get();
    const bool isPhase = s.paramPhase->get();

    for (int i = 0; i < numSamples; ++i)
    {
        const float x    = s.smX.getNextValue();
        const float gain = s.smGain.getNextValue();
        const float rear = s.smRear.getNextValue();
        const float z    = s.smZ.getNextValue();

        const float dist = jmax(0.1f, std::sqrt(x*x + s.smY.getCurrentValue()*s.smY.getCurrentValue() + z*z));
        const float dg   = 1.f/dist;
        const float az   = std::atan2(x,-z);
        const float sinA = std::sin(az);
        const float panL = std::sqrt(jmax(0.f,0.5f*(1.f-sinA)));
        const float panR = std::sqrt(jmax(0.f,0.5f*(1.f+sinA)));

        float mono = (inL[i]+inR[i])*0.5f * gain * dg;
        float sL = mono*panL, sR = mono*panR;

        float rL,rR;
        if (mode == 0)
        {
            const float itdSec  = (0.215f/343.f)*std::sin(az);
            const int   itdSamp = jlimit(0,s.dBufSize-1,(int)(std::abs(itdSec)*currentSampleRate));
            s.dBufL[s.dWritePos]=sL; s.dBufR[s.dWritePos]=sR;
            int rLi=s.dWritePos, rRi=s.dWritePos;
            if (az>0.f) rLi=(s.dWritePos-itdSamp+s.dBufSize)%s.dBufSize;
            else        rRi=(s.dWritePos-itdSamp+s.dBufSize)%s.dBufSize;
            float eL=s.elevFL.processSample(s.dBufL[rLi]);
            float eR=s.elevFR.processSample(s.dBufR[rRi]);
            float rrL=s.rearFL.processSample(eL);
            float rrR=s.rearFR.processSample(eR);
            rL=eL*(1.f-rear)+rrL*rear;
            rR=eR*(1.f-rear)+rrR*rear;
            s.dWritePos=(s.dWritePos+1)%s.dBufSize;
        }
        else { rL=sL; rR=sR; }

        if (isMono)  { rL=rR=(rL+rR)*0.5f; }
        if (isPhase) { rL=-rL; rR=-rR; }

        outL[i] += rL;
        outR[i] += rR;
    }
}

// ============================================================
void SpatialMixProProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    const int mode = paramMode->getIndex();
    const int numSamples = buffer.getNumSamples();
    smMaster.setTargetValue(paramMasterGain->get());

    // Count active (enabled+connected) buses
    numActiveBuses = 0;
    for (int b = 0; b < MAX_SOURCES; ++b)
        if (getBus(true,b) && getBus(true,b)->isEnabled()) numActiveBuses++;

    // Output accumulator
    std::vector<float> mixL(numSamples,0.f), mixR(numSamples,0.f);

    // Get output channels (we write to them at the end)
    int outCh0 = -1, outCh1 = -1;
    {
        auto* outBus = getBus(false,0);
        if (outBus && outBus->isEnabled()) {
            int start = outBus->getChannelIndexInProcessBlockBuffer(0);
            outCh0 = start; outCh1 = start+1;
        }
    }

    // Accumulate each active input bus
    for (int b = 0; b < MAX_SOURCES; ++b)
    {
        auto* bus = getBus(true,b);
        if (!bus || !bus->isEnabled()) continue;
        if (!sources[b].paramActive->get()) continue;

        int ch0 = bus->getChannelIndexInProcessBlockBuffer(0);
        int ch1 = bus->getChannelIndexInProcessBlockBuffer(1);
        if (ch0 >= buffer.getNumChannels() || ch1 >= buffer.getNumChannels()) continue;

        const float* inL = buffer.getReadPointer(ch0);
        const float* inR = buffer.getReadPointer(ch1);

        processSource(b, inL, inR, mixL.data(), mixR.data(), numSamples, mode);
    }

    // Apply master gain + 2.1 sub + write output
    for (int i = 0; i < numSamples; ++i)
    {
        float mg = smMaster.getNextValue();
        float oL = mixL[i]*mg;
        float oR = mixR[i]*mg;

        if (mode == 2) {
            float sub=(subFilterL.processSample(oL)+subFilterR.processSample(oR))*0.28f;
            oL+=sub; oR+=sub;
        }

        if (outCh0>=0) buffer.setSample(outCh0,i,oL);
        if (outCh1>=0) buffer.setSample(outCh1,i,oR);
    }

    // Clear all input bus channels that are not output
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        if (ch != outCh0 && ch != outCh1)
            buffer.clear(ch,0,numSamples);
}

// ============================================================
AudioProcessorEditor* SpatialMixProProcessor::createEditor()
{ return new SpatialMixProEditor(*this); }

void SpatialMixProProcessor::getStateInformation(MemoryBlock& destData)
{
    ValueTree st("SMP3");
    st.setProperty("mode", paramMode->getIndex(), nullptr);
    st.setProperty("mg",   paramMasterGain->get(), nullptr);
    for (int i = 0; i < MAX_SOURCES; ++i)
    {
        auto& s = sources[i];
        ValueTree sv("SRC"); sv.setProperty("i",i,nullptr);
        sv.setProperty("name", s.name,            nullptr);
        sv.setProperty("x",    s.paramX->get(),   nullptr);
        sv.setProperty("y",    s.paramY->get(),   nullptr);
        sv.setProperty("z",    s.paramZ->get(),   nullptr);
        sv.setProperty("g",    s.paramGain->get(),nullptr);
        sv.setProperty("mn",   (bool)s.paramMono->get(),  nullptr);
        sv.setProperty("ph",   (bool)s.paramPhase->get(), nullptr);
        sv.setProperty("ac",   (bool)s.paramActive->get(),nullptr);
        st.addChild(sv,-1,nullptr);
    }
    MemoryOutputStream os(destData,true); st.writeToStream(os);
}

void SpatialMixProProcessor::setStateInformation(const void* data, int sz)
{
    auto st = ValueTree::readFromData(data,(size_t)sz);
    if (!st.isValid()) return;
    paramMode->setValueNotifyingHost((int)st.getProperty("mode",0)/2.f);
    *paramMasterGain = (float)st.getProperty("mg",1.f);
    for (auto child : st)
    {
        int i = (int)child.getProperty("i",-1);
        if (i<0||i>=MAX_SOURCES) continue;
        auto& s = sources[i];
        s.name = child.getProperty("name",s.name).toString();
        *s.paramX    = (float)child.getProperty("x",s.paramX->get());
        *s.paramY    = (float)child.getProperty("y",s.paramY->get());
        *s.paramZ    = (float)child.getProperty("z",s.paramZ->get());
        *s.paramGain = (float)child.getProperty("g",1.f);
        s.paramMono ->setValueNotifyingHost((bool)child.getProperty("mn",false)?1.f:0.f);
        s.paramPhase->setValueNotifyingHost((bool)child.getProperty("ph",false)?1.f:0.f);
        s.paramActive->setValueNotifyingHost((bool)child.getProperty("ac",true)?1.f:0.f);
    }
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{ return new SpatialMixProProcessor(); }
