#pragma once
#include <JuceHeader.h>

// ============================================================
//  SpatialMix Pro — Multi-Source Atmos Processor
//  Up to 8 stereo inputs, each spatialized independently
// ============================================================

static constexpr int MAX_SOURCES = 8;

struct SpatialSource
{
    // Per-source automatable parameters
    juce::AudioParameterFloat* paramX    = nullptr;  // L/R  -8..+8 m
    juce::AudioParameterFloat* paramY    = nullptr;  // H     0..6 m
    juce::AudioParameterFloat* paramZ    = nullptr;  // D    -8..+8 m
    juce::AudioParameterFloat* paramGain = nullptr;  // vol   0..2
    juce::AudioParameterBool*  paramMono = nullptr;
    juce::AudioParameterBool*  paramPhase= nullptr;
    juce::AudioParameterBool*  paramActive=nullptr;  // mute/solo

    juce::String name;

    // DSP (one per source)
    juce::dsp::IIR::Filter<float> elevFL, elevFR, rearFL, rearFR;
    std::vector<float> dBufL, dBufR;
    int dBufSize = 256, dWritePos = 0;

    juce::SmoothedValue<float> smX, smY, smZ, smGain, smRear;
};

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
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.02; }

    int  getNumPrograms()                         override { return 1; }
    int  getCurrentProgram()                      override { return 0; }
    void setCurrentProgram(int)                   override {}
    const juce::String getProgramName(int)        override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int)   override;

    // Global parameters
    juce::AudioParameterChoice* paramMode;   // HRTF/Stereo/2.1
    juce::AudioParameterFloat*  paramMasterGain;

    // Per-source data (public so editor can access)
    SpatialSource sources[MAX_SOURCES];

    // How many buses are actually connected (detected each block)
    int numActiveBuses = 1;

private:
    double currentSampleRate = 44100.0;
    juce::dsp::IIR::Filter<float> subFilterL, subFilterR;
    juce::SmoothedValue<float> smMaster;

    void processSource(int srcIdx, const float* inL, const float* inR,
                       float* outL, float* outR, int numSamples, int mode);
    void updateElevFilter(float elevDeg, SpatialSource& src);
    static juce::BusesProperties buildBuses();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialMixProProcessor)
};
