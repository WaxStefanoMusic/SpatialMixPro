#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SpatialMixProEditor : public juce::AudioProcessorEditor,
                             public juce::Timer
{
public:
    explicit SpatialMixProEditor(SpatialMixProProcessor&);
    ~SpatialMixProEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    SpatialMixProProcessor& proc;

    static constexpr uint32
        C_BG    = 0xFF070707, C_PANEL  = 0xFF0D0D0D, C_BORDER = 0xFF1A1A1A,
        C_AMBER = 0xFFFF9500, C_AMBDIM = 0xFF5A3500,
        C_GREEN = 0xFF39FF14, C_BLUE   = 0xFF00D4FF,
        C_TEXT  = 0xFFC8C8C8, C_DIM   = 0xFF404040, C_BRT = 0xFFF0F0F0;

    struct DarkLAF : juce::LookAndFeel_V4 {
        DarkLAF() {
            setColour(juce::Slider::thumbColourId,          juce::Colour(0xFFFF9500));
            setColour(juce::Slider::trackColourId,          juce::Colour(0xFF3A2000));
            setColour(juce::Slider::backgroundColourId,     juce::Colour(0xFF181818));
            setColour(juce::Label::textColourId,            juce::Colour(0xFFC8C8C8));
            setColour(juce::Label::backgroundColourId,      juce::Colour(0xFF0D0D0D));
            setColour(juce::Label::outlineColourId,         juce::Colour(0xFF1A1A1A));
            setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF111111));
            setColour(juce::TextEditor::textColourId,       juce::Colour(0xFFF0F0F0));
            setColour(juce::TextEditor::outlineColourId,    juce::Colour(0xFFFF9500));
            setColour(juce::TextButton::buttonColourId,     juce::Colour(0xFF111111));
            setColour(juce::TextButton::buttonOnColourId,   juce::Colour(0xFF3A2000));
            setColour(juce::TextButton::textColourOffId,    juce::Colour(0xFF505050));
            setColour(juce::TextButton::textColourOnId,     juce::Colour(0xFFFF9500));
        }
    } laf;

    juce::Label      nameLabel;
    juce::Label      nameLabel2;
    juce::Label      nameLabel2;
    juce::Label      nameLabel2;
    juce::Slider     sliderX, sliderZ, sliderGain;
    juce::Slider     sliderX2, sliderY2, sliderZ2, sliderGain2;
    juce::Slider     sliderY;
    juce::TextButton btnHRTF, btnStereo, btn21;
    juce::TextButton btnMono, btnPhase, btnDual, btnHelp;
    juce::TextButton btnMono2, btnPhase2;

    std::unique_ptr<juce::SliderParameterAttachment> attX, attY, attZ, attGain;
    std::unique_ptr<juce::SliderParameterAttachment> attX2, attY2, attZ2, attGain2;

    juce::Rectangle<int> rHeader, rLeft, rTop, rFront, rBottom;

    enum DragTarget { None, Spk1Top, Spk1Front, Spk2Top, Spk2Front };
    DragTarget dragging = None;

    struct HallSection { const char* name; float x, z, w, d; uint32 col; };
    static const HallSection kSections[];
    static const int kNumSections;

    void drawHeader(juce::Graphics&);
    void drawLeftPanel(juce::Graphics&);
    void drawTopView(juce::Graphics&);
    void drawFrontView(juce::Graphics&);
    void drawBottom(juce::Graphics&);
    void showHelp();
    void updateModeButtons();
    void updateOptionButtons();

    juce::Point<float> topPt(float x, float z) const;
    void topToXZ(juce::Point<float> pt, float& x, float& z) const;
    juce::Point<float> frontPt(float y, float z) const;
    void frontToYZ(juce::Point<float> pt, float& y, float& z) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialMixProEditor)
};
