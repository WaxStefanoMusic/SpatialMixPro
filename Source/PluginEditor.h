#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SpatialMixProEditor : public juce::AudioProcessorEditor, public juce::Timer
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
        C_BG=0xFF070707,C_PANEL=0xFF0D0D0D,C_BORDER=0xFF1A1A1A,
        C_AMBER=0xFFFF9500,C_GREEN=0xFF39FF14,C_BLUE=0xFF00D4FF,
        C_RED=0xFFFF3C3C,C_TEXT=0xFFC8C8C8,C_DIM=0xFF404040,C_BRT=0xFFF0F0F0;

    // Per-source colours
    static const uint32 kSrcColors[MAX_SOURCES];

    struct DarkLAF : juce::LookAndFeel_V4 {
        DarkLAF() {
            setColour(juce::Slider::thumbColourId,     juce::Colour(0xFFFF9500));
            setColour(juce::Slider::trackColourId,     juce::Colour(0xFF3A2000));
            setColour(juce::Slider::backgroundColourId,juce::Colour(0xFF181818));
            setColour(juce::Label::textColourId,       juce::Colour(0xFFC8C8C8));
            setColour(juce::Label::backgroundColourId, juce::Colour(0xFF0D0D0D));
            setColour(juce::Label::outlineColourId,    juce::Colour(0xFF1A1A1A));
            setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xFF111111));
            setColour(juce::TextEditor::textColourId,      juce::Colour(0xFFF0F0F0));
            setColour(juce::TextEditor::outlineColourId,   juce::Colour(0xFFFF9500));
            setColour(juce::TextButton::buttonColourId,  juce::Colour(0xFF111111));
            setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xFF3A2000));
            setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF505050));
            setColour(juce::TextButton::textColourOnId,  juce::Colour(0xFFFF9500));
            setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xFF111111));
            setColour(juce::ComboBox::textColourId,      juce::Colour(0xFFC8C8C8));
            setColour(juce::ComboBox::outlineColourId,   juce::Colour(0xFF2A2A2A));
        }
    } laf;

    // ── Source list (left panel) ──────────────────────────────
    struct SourceRow {
        juce::Label       nameLabel;
        juce::TextButton  btnActive, btnMono, btnPhase;
        bool built = false;
    };
    SourceRow rows[MAX_SOURCES];

    // ── Selected source controls ──────────────────────────────
    int selectedSrc = 0;
    juce::Slider sliderX, sliderY, sliderZ, sliderGain;
    std::unique_ptr<juce::SliderParameterAttachment> attX, attY, attZ, attGain;

    // ── Global controls ───────────────────────────────────────
    juce::TextButton btnHRTF, btnStereo, btn21, btnHelp;
    juce::Slider     sliderMaster;
    std::unique_ptr<juce::SliderParameterAttachment> attMaster;

    // Layout
    juce::Rectangle<int> rHeader, rSourceList, rControls, rTop, rFront, rBottom;

    // Drag
    int dragSrc = -1;
    enum DragView { DragNone, DragTop, DragFront };
    DragView dragView = DragNone;

    // Helpers
    void buildRows();
    void selectSource(int idx);
    void updateGlobalButtons();
    void drawHeader(juce::Graphics&);
    void drawSourceList(juce::Graphics&);
    void drawControls(juce::Graphics&);
    void drawTopView(juce::Graphics&);
    void drawFrontView(juce::Graphics&);
    void drawBottom(juce::Graphics&);
    void showHelp();

    juce::Point<float> topPt(float x, float z) const;
    void topToXZ(juce::Point<float>, float& x, float& z) const;
    juce::Point<float> frontPt(float y, float z) const;
    void frontToYZ(juce::Point<float>, float& y, float& z) const;

    // Orchestra hall sections (for top view background)
    struct HallSec { const char* nm; float x,z,w,d; uint32 col; };
    static const HallSec kHall[];
    static const int kHallN;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpatialMixProEditor)
};
