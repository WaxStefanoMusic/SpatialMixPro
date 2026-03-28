#pragma once
#include <JuceHeader.h>

class SpatialMixProProcessor : public juce::AudioProcessor
{
public:
    SpatialMixProProcessor();
    ~SpatialMixProProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "SpatialMix Pro"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.02; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioParameterFloat*  paramX;
    juce::AudioParameterFloat*  paramY;
    juce::AudioParameterFloat*  paramZ;
    juce::AudioParameterFloat*  paramGain;
    juce::AudioParameterChoice* paramMode;
    juce::AudioParameterBool*   paramMono;
    juce::AudioParameterBool*   paramPhase;
    juce::AudioParameterBool*   paramDual;
    juce::AudioParameterFloat*  paramX2;
    juce::AudioParameterFloat*  paramY2;
    juce::AudioParameterFloat*  paramZ2;
    juce::AudioParameterFloat*  paramGain2;

    juce::String sourceName { "Object 1" };

private:
    double currentSampleRate = 44100.0;
    std::vector<float> dBufL1, dBufR1, dBufL2, dBufR2;
    int dBufSize = 256, dWritePos1 = 0, dWritePos2 = 0;
    juce::dsp::IIR::Filter<float> elevFL1, elevFR1, elevFL2, elevFR2;
    juce::dsp::IIR::Filter<float> rearFL1, rearFR1, rearFL2, rearFR2;
    juce::dsp::IIR::Filter<float> subFilterL, subFilterR;
    juce::SmoothedValue<float> smX, smY, smZ, smGain;
    juce::SmoothedValue<float> smX2, smY2, smZ2, smGain2;
    juce::SmoothedValue<float> smRear1, smRear2;
    void updateElevFilter(float elevDeg,
                          juce::dsp::IIR::Filter<float>& fl,
                          juce::dsp::IIR::Filter<float>& fr);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialMixProProcessor)
};
