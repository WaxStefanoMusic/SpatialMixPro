#include "PluginEditor.h"
using namespace juce;

const SpatialMixProEditor::HallSection SpatialMixProEditor::kSections[] = {{ "", 0,0,0,0,0 }};
const int SpatialMixProEditor::kNumSections = 0;

SpatialMixProEditor::SpatialMixProEditor(SpatialMixProProcessor& p)
    : AudioProcessorEditor(p), proc(p),
      btnHRTF("HRTF"), btnStereo("STEREO"), btn21("2.1"),
      btnMono("MONO"), btnPhase("PHASE"), btnDual("DUAL"), btnHelp("?"),
      btnMono2("MONO 2"), btnPhase2("FASE 2")
{
    setLookAndFeel(&laf);

    nameLabel.setText(proc.sourceName, dontSendNotification);
    nameLabel.setFont(Font("Courier New", 16.0f, Font::bold));
    nameLabel.setJustificationType(Justification::centredLeft);
    nameLabel.setEditable(false, true);
    nameLabel.onTextChange = [this] { proc.sourceName = nameLabel.getText(); };
    addAndMakeVisible(nameLabel);

    // All sliders are horizontal
    auto mkH = [&](Slider& s, const String& suffix) {
        s.setSliderStyle(Slider::LinearHorizontal);
        s.setTextBoxStyle(Slider::TextBoxRight, false, 55, 20);
        s.setTextValueSuffix(suffix);
        addAndMakeVisible(s);
    };
    mkH(sliderX,     " m");
    mkH(sliderY,     " m");   // Y is now horizontal too
    mkH(sliderZ,     " m");
    mkH(sliderGain,  "x");
    mkH(sliderX2,    " m");
    mkH(sliderY2,    " m");
    mkH(sliderZ2,    " m");
    mkH(sliderGain2, "x");

    // sliderY2 reuse sliderGain2 slot — no, we keep sliderY for spk1 Y
    // For spk2 Y we need sliderY2 — but it's not in the header.
    // We'll use sliderGain for V and sliderY for Y, layout them properly.

    attX    = std::make_unique<SliderParameterAttachment>(*proc.paramX,     sliderX);
    attY    = std::make_unique<SliderParameterAttachment>(*proc.paramY,     sliderY);
    attZ    = std::make_unique<SliderParameterAttachment>(*proc.paramZ,     sliderZ);
    attGain = std::make_unique<SliderParameterAttachment>(*proc.paramGain,  sliderGain);
    attX2   = std::make_unique<SliderParameterAttachment>(*proc.paramX2,    sliderX2);
    attY2   = std::make_unique<SliderParameterAttachment>(*proc.paramY2,    sliderY2);
    attZ2   = std::make_unique<SliderParameterAttachment>(*proc.paramZ2,    sliderZ2);
    attGain2= std::make_unique<SliderParameterAttachment>(*proc.paramGain2, sliderGain2);

    for (auto* b : { &btnHRTF, &btnStereo, &btn21, &btnMono, &btnPhase, &btnDual, &btnHelp, &btnMono2, &btnPhase2 })
        addAndMakeVisible(*b);

    btnHRTF  .onClick = [this] { proc.paramMode->setValueNotifyingHost(0.0f);  updateModeButtons(); };
    btnStereo.onClick = [this] { proc.paramMode->setValueNotifyingHost(0.5f);  updateModeButtons(); };
    btn21    .onClick = [this] { proc.paramMode->setValueNotifyingHost(1.0f);  updateModeButtons(); };
    btnMono  .onClick = [this] { proc.paramMono ->setValueNotifyingHost(proc.paramMono->get()   ? 0.0f : 1.0f); updateOptionButtons(); };
    btnPhase .onClick = [this] { proc.paramPhase->setValueNotifyingHost(proc.paramPhase->get()  ? 0.0f : 1.0f); updateOptionButtons(); };
    btnDual  .onClick = [this] { proc.paramDual ->setValueNotifyingHost(proc.paramDual->get()   ? 0.0f : 1.0f); updateOptionButtons(); resized(); };
    btnMono2 .onClick = [this] { proc.paramMono2 ->setValueNotifyingHost(proc.paramMono2->get() ? 0.0f : 1.0f); updateOptionButtons(); };
    btnPhase2.onClick = [this] { proc.paramPhase2->setValueNotifyingHost(proc.paramPhase2->get()? 0.0f : 1.0f); updateOptionButtons(); };
    btnHelp  .onClick = [this] { showHelp(); };

    updateModeButtons();
    updateOptionButtons();

    setResizable(true, false);
    setResizeLimits(960, 540, 3840, 2160);
    getConstrainer()->setFixedAspectRatio(16.0 / 9.0);
    setSize(1600, 900);
    startTimerHz(30);
}

SpatialMixProEditor::~SpatialMixProEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void SpatialMixProEditor::timerCallback()
{
    repaint(rTop); repaint(rFront); repaint(rBottom);
    if (nameLabel.getText() != proc.sourceName)
        nameLabel.setText(proc.sourceName, dontSendNotification);
    bool dual = proc.paramDual->get();
    sliderX2.setVisible(dual);
    sliderY2.setVisible(dual);
    sliderZ2.setVisible(dual);
    sliderGain2.setVisible(dual);
    btnMono2.setVisible(dual);
    btnPhase2.setVisible(dual);
}

void SpatialMixProEditor::updateModeButtons()
{
    int m = proc.paramMode->getIndex();
    btnHRTF  .setToggleState(m == 0, dontSendNotification);
    btnStereo.setToggleState(m == 1, dontSendNotification);
    btn21    .setToggleState(m == 2, dontSendNotification);
}

void SpatialMixProEditor::updateOptionButtons()
{
    btnMono .setToggleState(proc.paramMono->get(),   dontSendNotification);
    btnPhase.setToggleState(proc.paramPhase->get(),  dontSendNotification);
    btnDual .setToggleState(proc.paramDual->get(),   dontSendNotification);
    btnMono2 .setToggleState(proc.paramMono2->get(), dontSendNotification);
    btnPhase2.setToggleState(proc.paramPhase2->get(),dontSendNotification);
    bool dual = proc.paramDual->get();
    btnMono2 .setVisible(dual);
    btnPhase2.setVisible(dual);
}

void SpatialMixProEditor::showHelp()
{
    String msg;
    msg << "SPATIALMIX PRO - COMANDI\n\n"
        << "VISTA TOP (dall'alto)\n"
        << "  Trascina: muove X (sin/dx) e Z (profondita)\n"
        << "  Pallino piu grande = cassa a 1.2m altezza orecchie\n\n"
        << "VISTA FRONTALE\n"
        << "  Trascina verticalmente: muove Y (altezza)\n"
        << "  Pallino piu grande = cassa vicina all'ascoltatore (Z=0)\n"
        << "  Pallino piu piccolo = cassa lontana (avanti o dietro)\n\n"
        << "SLIDER PANNELLO SINISTRA\n"
        << "  X = sinistra/destra (-8..+8 m)\n"
        << "  Y = altezza (0..6 m)\n"
        << "  Z = profondita (-8=fronte, 0=ascoltatore, +8=retro)\n"
        << "  V = volume cassa (0..2x)\n\n"
        << "MODALITA OUTPUT\n"
        << "  HRTF   = simulazione 3D binaurale (cuffie)\n"
        << "  STEREO = downmix stereo\n"
        << "  2.1    = stereo + sub 120 Hz\n\n"
        << "OPZIONI\n"
        << "  MONO  = collassa L+R in mono\n"
        << "  PHASE = inversione fase 180 gradi\n"
        << "  DUAL  = sdoppia cassa (due sorgenti indipendenti)\n\n"
        << "NOME: doppio click per rinominare";
    AlertWindow::showMessageBoxAsync(MessageBoxIconType::InfoIcon,
                                     "SpatialMix Pro - Guida", msg, "OK");
}

void SpatialMixProEditor::resized()
{
    auto b = getLocalBounds();
    const int hH  = jmax(46, b.getHeight() / 20);
    const int stH = jmax(34, b.getHeight() / 26);
    const int lW  = jmax(230, b.getWidth() / 7);
    const int lblW = 20; // label width for X/Y/Z/V

    rHeader = b.removeFromTop(hH);
    rBottom = b.removeFromBottom(stH);
    rLeft   = b.removeFromLeft(lW);

    // Top and front same width, stacked vertically
    rTop   = b.removeFromTop((int)(b.getHeight() * 0.62f));
    rFront = b;

    // Help button
    btnHelp.setBounds(rHeader.getRight() - 44, rHeader.getY() + 6, 34, hH - 12);

    // ── LEFT PANEL ──────────────────────────────────────────
    auto lp = rLeft.reduced(8, 8);
    const int rowH = jmax(22, lp.getHeight() / 16);

    // Name label
    nameLabel.setBounds(lp.removeFromTop(jmax(22, hH - 14)));
    lp.removeFromTop(6);

    // MONO / PHASE / DUAL
    {
        auto row = lp.removeFromTop(rowH);
        int bw = row.getWidth() / 3;
        btnMono .setBounds(row.removeFromLeft(bw).reduced(2, 0));
        btnPhase.setBounds(row.removeFromLeft(bw).reduced(2, 0));
        btnDual .setBounds(row.reduced(2, 0));
    }
    lp.removeFromTop(8);

    // Spk1 sliders with X/Y/Z/V labels
    auto placeLabeledSlider = [&](Slider& s, const String& /*lbl*/) {
        // We draw the label in paint(), just place slider offset by lblW
        auto row = lp.removeFromTop(rowH);
        row.removeFromLeft(lblW);
        s.setBounds(row);
        lp.removeFromTop(3);
    };
    placeLabeledSlider(sliderX,    "X");
    placeLabeledSlider(sliderY,    "Y");
    placeLabeledSlider(sliderZ,    "Z");
    placeLabeledSlider(sliderGain, "V");
    lp.removeFromTop(8);

    bool dual = proc.paramDual->get();
    if (dual)
    {
        // MONO2 / PHASE2
        auto row2 = lp.removeFromTop(rowH);
        int bw2 = row2.getWidth() / 2;
        btnMono2 .setBounds(row2.removeFromLeft(bw2).reduced(2, 0));
        btnPhase2.setBounds(row2.reduced(2, 0));
        lp.removeFromTop(6);

        // Spk2 sliders
        placeLabeledSlider(sliderX2,    "X2");
        placeLabeledSlider(sliderY2,    "Y2");
        placeLabeledSlider(sliderZ2,    "Z2");
        placeLabeledSlider(sliderGain2, "V2");
        lp.removeFromTop(8);
    }

    // Mode buttons HRTF/STEREO/2.1
    {
        auto row = lp.removeFromTop(rowH);
        int bw = row.getWidth() / 3;
        btnHRTF  .setBounds(row.removeFromLeft(bw).reduced(2, 0));
        btnStereo.setBounds(row.removeFromLeft(bw).reduced(2, 0));
        btn21    .setBounds(row.reduced(2, 0));
    }

    sliderX2.setVisible(dual);
    sliderY2.setVisible(dual);
    sliderZ2.setVisible(dual);
    sliderGain2.setVisible(dual);
    btnMono2.setVisible(dual);
    btnPhase2.setVisible(dual);
}

// ── COORDINATE HELPERS ──────────────────────────────────────

Point<float> SpatialMixProEditor::topPt(float x, float z) const
{
    auto r = rTop.toFloat().reduced(20.0f);
    return { r.getX() + (x + 8.0f) / 16.0f * r.getWidth(),
             r.getY() + (z + 8.0f) / 16.0f * r.getHeight() };
}
void SpatialMixProEditor::topToXZ(Point<float> pt, float& x, float& z) const
{
    auto r = rTop.toFloat().reduced(20.0f);
    x = ((pt.x - r.getX()) / r.getWidth())  * 16.0f - 8.0f;
    z = ((pt.y - r.getY()) / r.getHeight()) * 16.0f - 8.0f;
    x = jlimit(-8.0f, 8.0f, x);
    z = jlimit(-8.0f, 8.0f, z);
}

// Front view: X horizontal, Y vertical. Z = dot size only.
// Uses full rFront area (no Y-slider strip).
Point<float> SpatialMixProEditor::frontPt(float y, float z) const
{
    (void)z;
    auto r = rFront.toFloat().reduced(20.0f);
    float x = proc.paramX->get();
    return { r.getX() + (x + 8.0f) / 16.0f * r.getWidth(),
             r.getBottom() - y / 6.0f * r.getHeight() };
}
void SpatialMixProEditor::frontToYZ(Point<float> pt, float& y, float& z) const
{
    auto r = rFront.toFloat().reduced(20.0f);
    y = (r.getBottom() - pt.y) / r.getHeight() * 6.0f;
    y = jlimit(0.0f, 6.0f, y);
    z = proc.paramZ->get(); // Z unchanged by front drag
}

// ── PAINT ────────────────────────────────────────────────────

void SpatialMixProEditor::paint(Graphics& g)
{
    g.fillAll(Colour(C_BG));
    drawHeader(g);
    drawLeftPanel(g);
    drawTopView(g);
    drawFrontView(g);
    drawBottom(g);
}

void SpatialMixProEditor::drawHeader(Graphics& g)
{
    g.setColour(Colour(C_PANEL)); g.fillRect(rHeader);
    g.setColour(Colour(C_BORDER));
    g.drawHorizontalLine(rHeader.getBottom(), (float)rHeader.getX(), (float)rHeader.getRight());
    float fs = (float)rHeader.getHeight() * 0.48f;
    g.setFont(Font("Courier New", fs, Font::bold));
    g.setColour(Colour(C_AMBER));
    g.drawText("SPATIALMIX PRO", rHeader.withWidth(300), Justification::centredLeft, false);
    g.setColour(Colour(C_DIM));
    g.setFont(Font("Courier New", fs * 0.55f, Font::plain));
    g.drawText("ATMOS SPATIAL SIMULATOR  |  64-BIT VST3  |  HRTF + DUAL",
               rHeader.withTrimmedLeft(295).withTrimmedRight(50), Justification::centredLeft, false);
}

void SpatialMixProEditor::drawLeftPanel(Graphics& g)
{
    g.setColour(Colour(C_PANEL)); g.fillRect(rLeft);
    g.setColour(Colour(C_BORDER));
    g.drawVerticalLine(rLeft.getRight(), (float)rLeft.getY(), (float)rLeft.getBottom());

    auto lp = rLeft.reduced(8, 8);
    float fs   = jmax(9.0f, (float)rLeft.getHeight() / 50.0f);
    int   rowH = jmax(22, lp.getHeight() / 16);
    const int hH = rHeader.getHeight();

    // Starting Y for slider labels — after name + gap + buttons row + gap
    int yy = lp.getY() + jmax(22, hH - 14) + 6 + rowH + 8;

    // Draw X/Y/Z/V labels to the left of each slider
    g.setFont(Font("Courier New", fs, Font::bold));
    auto drawAxisLbl = [&](const String& lbl, Colour col) {
        g.setColour(col);
        g.drawText(lbl, lp.getX(), yy, 18, rowH, Justification::centredLeft, false);
        yy += rowH + 3;
    };
    drawAxisLbl("X", Colour(C_AMBER));
    drawAxisLbl("Y", Colour(C_GREEN));
    drawAxisLbl("Z", Colour(C_AMBER));
    drawAxisLbl("V", Colour(C_BLUE));

    if (proc.paramDual->get())
    {
        yy += 8 + rowH + 6; // skip MONO2/PHASE2 row + gap
        g.setColour(Colour(C_BLUE).withAlpha(0.55f));
        g.setFont(Font("Courier New", fs * 0.8f, Font::plain));
        g.drawText("X2", lp.getX(), yy, 18, rowH, Justification::centredLeft, false); yy += rowH + 3;
        g.setColour(Colour(C_GREEN).withAlpha(0.55f));
        g.drawText("Y2", lp.getX(), yy, 18, rowH, Justification::centredLeft, false); yy += rowH + 3;
        g.setColour(Colour(C_BLUE).withAlpha(0.55f));
        g.drawText("Z2", lp.getX(), yy, 18, rowH, Justification::centredLeft, false); yy += rowH + 3;
        g.setColour(Colour(C_GREEN).withAlpha(0.55f));
        g.drawText("V2", lp.getX(), yy, 18, rowH, Justification::centredLeft, false);
    }

    // Coordinate readout at bottom
    float cx = proc.paramX->get(), cy = proc.paramY->get(), cz = proc.paramZ->get();
    float hd   = std::sqrt(cx * cx + cz * cz);
    float az   = std::atan2(cx, -cz) * (180.0f / MathConstants<float>::pi);
    float el   = std::atan2(cy, jmax(hd, 0.001f)) * (180.0f / MathConstants<float>::pi);
    float dist = std::sqrt(cx * cx + cy * cy + cz * cz);

    int by = rLeft.getBottom() - (int)(rLeft.getHeight() * 0.20f);
    g.setFont(Font("Courier New", fs * 0.85f, Font::plain));
    g.setColour(Colour(C_DIM));
    g.drawText("AZIMUTH", lp.getX(), by,         lp.getWidth() / 2, rowH, Justification::centredLeft, false);
    g.drawText("ELEV",    lp.getX(), by + rowH,   lp.getWidth() / 2, rowH, Justification::centredLeft, false);
    g.drawText("DIST",    lp.getX(), by + rowH*2, lp.getWidth() / 2, rowH, Justification::centredLeft, false);
    g.setColour(Colour(C_AMBER));
    g.drawText(String(az,  1) + " deg", lp.getX(), by,         lp.getWidth(), rowH, Justification::centredRight, false);
    g.drawText(String(el,  1) + " deg", lp.getX(), by + rowH,   lp.getWidth(), rowH, Justification::centredRight, false);
    g.setColour(Colour(C_BLUE));
    g.drawText(String(dist,2) + " m",   lp.getX(), by + rowH*2, lp.getWidth(), rowH, Justification::centredRight, false);
}

void SpatialMixProEditor::drawTopView(Graphics& g)
{
    g.setColour(Colour(0xFF050505)); g.fillRect(rTop);
    auto rv = rTop.toFloat().reduced(20.0f);
    g.setColour(Colour(0xFF0A0A0A)); g.fillRect(rv);

    g.setColour(Colour(0xFF111111));
    for (int i = 0; i <= 16; ++i)
    {
        float xr = rv.getX() + i * rv.getWidth()  / 16.0f;
        float yr = rv.getY() + i * rv.getHeight() / 16.0f;
        g.drawVerticalLine  ((int)xr, rv.getY(), rv.getBottom());
        g.drawHorizontalLine((int)yr, rv.getX(), rv.getRight());
    }
    g.setColour(Colour(0xFF222222)); g.drawRect(rv, 2.0f);

    float fs   = jmax(9.0f,  (float)rTop.getHeight() / 50.0f);
    float dotR = jmax(6.0f,  (float)rTop.getWidth()  / 70.0f);

    // VISTA TOP — centered, same size style as VISTA FRONTALE
    {
        int lblH = (int)(fs * 2.0f);
        g.setColour(Colour(0xCC000000));
        g.fillRect(rTop.getCentreX() - 90, rTop.getY() + 4, 180, lblH);
        g.setFont(Font("Courier New", fs * 1.5f, Font::bold));
        g.setColour(Colour(C_AMBER));
        g.drawText("VISTA TOP", rTop.getX(), rTop.getY() + 4, rTop.getWidth(), lblH,
                   Justification::centred, false);
    }

    // Direction labels
    g.setFont(Font("Courier New", fs * 0.8f, Font::plain));
    g.setColour(Colour(C_DIM));
    g.drawText("FRONTE", rTop.getX(), rTop.getY() + 2, rTop.getWidth(), (int)fs + 2, Justification::centred, false);
    g.drawText("RETRO",  rTop.getX(), rTop.getBottom() - (int)fs - 4, rTop.getWidth(), (int)fs + 2, Justification::centred, false);
    g.drawText("SX", rTop.getX() + 4, rTop.getCentreY() - (int)fs, (int)fs * 2, (int)fs * 2, Justification::centred, false);
    g.drawText("DX", rTop.getRight() - (int)fs * 2 - 4, rTop.getCentreY() - (int)fs, (int)fs * 2, (int)fs * 2, Justification::centred, false);

    // Listener (fixed at 0,0)
    {
        auto pt = topPt(0.0f, 0.0f);
        g.setColour(Colour(C_BLUE).withAlpha(0.18f));
        g.fillEllipse(pt.x - dotR * 4, pt.y - dotR * 4, dotR * 8, dotR * 8);
        g.setColour(Colour(C_BLUE));
        g.fillEllipse(pt.x - dotR * 0.8f, pt.y - dotR * 0.8f, dotR * 1.6f, dotR * 1.6f);
        g.setColour(Colour(C_BLUE).withAlpha(0.7f));
        g.setFont(Font("Courier New", fs * 0.75f, Font::bold));
        g.drawText("ASCOLTATORE", (int)(pt.x - 50), (int)(pt.y + dotR + 2), 100, (int)(fs + 2), Justification::centred, false);
    }

    // Dot size = proximity to ear height 1.2m
    auto yToRadius = [&](float y) -> float {
        float diff  = std::abs(y - 1.2f);
        float scale = 1.0f - jmin(0.55f, diff / 4.0f);
        return dotR * scale;
    };

    // Speaker 1
    {
        float r  = yToRadius(proc.paramY->get());
        auto  pt = topPt(proc.paramX->get(), proc.paramZ->get());
        g.setColour(Colour(C_AMBER).withAlpha(0.22f));
        g.fillEllipse(pt.x - r * 3.5f, pt.y - r * 3.5f, r * 7.0f, r * 7.0f);
        g.setColour(Colour(C_AMBER));
        g.fillEllipse(pt.x - r, pt.y - r, r * 2.0f, r * 2.0f);
        g.setColour(Colour(C_BRT));
        g.fillEllipse(pt.x - r * 0.38f, pt.y - r * 0.38f, r * 0.76f, r * 0.76f);
        g.setColour(Colour(C_AMBER));
        g.setFont(Font("Courier New", fs, Font::bold));
        g.drawText(proc.sourceName, (int)(pt.x - 60), (int)(pt.y - r * 2.5f - fs - 2), 120, (int)(fs + 2), Justification::centred, false);
    }

    // Speaker 2 (dual)
    if (proc.paramDual->get())
    {
        float r2 = yToRadius(proc.paramY2->get());
        auto  pt = topPt(proc.paramX2->get(), proc.paramZ2->get());
        g.setColour(Colour(0xFF3399FF).withAlpha(0.18f));
        g.fillEllipse(pt.x - r2 * 3.5f, pt.y - r2 * 3.5f, r2 * 7.0f, r2 * 7.0f);
        g.setColour(Colour(0xFF3399FF));
        g.fillEllipse(pt.x - r2, pt.y - r2, r2 * 2.0f, r2 * 2.0f);
        g.setColour(Colour(C_BRT));
        g.fillEllipse(pt.x - r2 * 0.38f, pt.y - r2 * 0.38f, r2 * 0.76f, r2 * 0.76f);
        g.setColour(Colour(0xFF3399FF));
        g.setFont(Font("Courier New", fs, Font::bold));
        g.drawText(proc.sourceName + " [2]", (int)(pt.x - 65), (int)(pt.y - r2 * 2.5f - fs - 2), 130, (int)(fs + 2), Justification::centred, false);
    }
}

void SpatialMixProEditor::drawFrontView(Graphics& g)
{
    // Full width — same as rTop, no Y-slider strip
    g.setColour(Colour(0xFF050505)); g.fillRect(rFront);
    g.setColour(Colour(C_BORDER));
    g.drawHorizontalLine(rFront.getY(), (float)rFront.getX(), (float)rFront.getRight());

    auto rv = rFront.toFloat().reduced(20.0f);
    g.setColour(Colour(0xFF0D0D0D)); g.fillRect(rv);

    // Grid
    g.setColour(Colour(0xFF1A1A1A));
    for (int i = 0; i <= 16; ++i)
    {
        float xr = rv.getX() + i * rv.getWidth() / 16.0f;
        g.drawVerticalLine((int)xr, rv.getY(), rv.getBottom());
    }
    for (int i = 0; i <= 6; ++i)
    {
        float yr = rv.getBottom() - i * rv.getHeight() / 6.0f;
        g.drawHorizontalLine((int)yr, rv.getX(), rv.getRight());
        if (i > 0)
        {
            float fs2 = jmax(8.0f, (float)rv.getWidth() / 100.0f);
            g.setFont(Font("Courier New", fs2, Font::plain));
            g.setColour(Colour(0xFF555555));
            g.drawText(String(i) + "m", (int)rv.getX() + 3, (int)yr + 2, 26, (int)fs2 + 2,
                       Justification::centredLeft, false);
        }
    }

    // Ear height reference line 1.2m
    {
        float earY = rv.getBottom() - 1.2f / 6.0f * rv.getHeight();
        g.setColour(Colour(C_BLUE).withAlpha(0.22f));
        g.drawHorizontalLine((int)earY, rv.getX(), rv.getRight());
        float fs2 = jmax(8.0f, (float)rv.getWidth() / 100.0f);
        g.setFont(Font("Courier New", fs2, Font::plain));
        g.setColour(Colour(C_BLUE).withAlpha(0.5f));
        g.drawText("1.2m", (int)rv.getRight() - 36, (int)earY + 2, 34, (int)fs2 + 2,
                   Justification::centredRight, false);
    }

    g.setColour(Colour(0xFF2A2A2A)); g.drawRect(rv, 1.5f);

    float fs      = jmax(9.0f,  (float)rFront.getHeight() / 22.0f);
    float dotBase = jmax(7.0f,  (float)rFront.getWidth()  / 55.0f);

    // SX / DX labels
    g.setFont(Font("Courier New", fs * 0.85f, Font::bold));
    g.setColour(Colour(0xFF555555));
    g.drawText("SX", (int)rv.getX() + 2, (int)rv.getBottom() - (int)fs - 2, 24, (int)fs, Justification::centredLeft, false);
    g.drawText("DX", (int)rv.getRight() - 26, (int)rv.getBottom() - (int)fs - 2, 24, (int)fs, Justification::centredRight, false);

    // VISTA FRONTALE — same size and style as VISTA TOP
    {
        int lblH = (int)(fs * 2.0f);
        int lblY = rFront.getBottom() - lblH - 4;
        g.setColour(Colour(0xCC000000));
        g.fillRect(rFront.getCentreX() - 100, lblY - 2, 200, lblH + 4);
        g.setFont(Font("Courier New", fs * 1.5f, Font::bold));
        g.setColour(Colour(C_AMBER));
        g.drawText("VISTA FRONTALE", rFront.getX(), lblY, rFront.getWidth(), lblH,
                   Justification::centred, false);
    }

    // Dot radius = biggest at Z=0 (listener), smaller going front OR back
    // Z range: -8=front 0=listener +8=back
    auto zToRadius = [&](float z) -> float {
        float dist = std::abs(z);                          // 0..8, 0=at listener
        float scale = 1.0f - jmin(0.60f, dist / 9.5f);   // 1.0 at Z=0, ~0.4 at Z=8
        return dotBase * scale;
    };

    // Listener — FIXED at centre X=0, Y=1.2m, regardless of speaker
    {
        auto r2a = rFront.toFloat().reduced(20.0f);
        float lx = r2a.getX() + (0.0f + 8.0f) / 16.0f * r2a.getWidth();  // X=0 always
        float ly = r2a.getBottom() - 1.2f / 6.0f * r2a.getHeight();
        float r  = dotBase * 1.15f;
        g.setColour(Colour(C_BLUE).withAlpha(0.18f));
        g.fillEllipse(lx - r * 2.6f, ly - r * 2.6f, r * 5.2f, r * 5.2f);
        g.setColour(Colour(C_BLUE));
        g.fillEllipse(lx - r, ly - r, r * 2.0f, r * 2.0f);
        g.setColour(Colour(0xFF000000));
        g.setFont(Font("Courier New", jmax(7.0f, r * 0.60f), Font::bold));
        g.drawText("ASCOLTO", (int)(lx - r * 2.8f), (int)(ly - r * 0.45f),
                   (int)(r * 5.6f), (int)(r * 0.9f), Justification::centred, false);
    }

    // Speaker 1: X from paramX, Y vertical, Z as dot size
    {
        float sy = proc.paramY->get();
        float sz = proc.paramZ->get();
        float sx = proc.paramX->get();
        auto  r2a = rFront.toFloat().reduced(20.0f);
        float px = r2a.getX() + (sx + 8.0f) / 16.0f * r2a.getWidth();
        float py = r2a.getBottom() - sy / 6.0f * r2a.getHeight();
        float rad = zToRadius(sz);
        g.setColour(Colour(C_AMBER).withAlpha(0.22f));
        g.fillEllipse(px - rad * 3.0f, py - rad * 3.0f, rad * 6.0f, rad * 6.0f);
        g.setColour(Colour(C_AMBER));
        g.fillEllipse(px - rad, py - rad, rad * 2.0f, rad * 2.0f);
        g.setColour(Colour(C_BRT));
        g.fillEllipse(px - rad * 0.35f, py - rad * 0.35f, rad * 0.7f, rad * 0.7f);
    }

    // Speaker 2 (dual)
    if (proc.paramDual->get())
    {
        float sy2 = proc.paramY2->get();
        float sz2 = proc.paramZ2->get();
        float sx2 = proc.paramX2->get();
        auto  r2a = rFront.toFloat().reduced(20.0f);
        float px2 = r2a.getX() + (sx2 + 8.0f) / 16.0f * r2a.getWidth();
        float py2 = r2a.getBottom() - sy2 / 6.0f * r2a.getHeight();
        float rad2 = zToRadius(sz2);
        g.setColour(Colour(0xFF3399FF).withAlpha(0.18f));
        g.fillEllipse(px2 - rad2 * 3.0f, py2 - rad2 * 3.0f, rad2 * 6.0f, rad2 * 6.0f);
        g.setColour(Colour(0xFF3399FF));
        g.fillEllipse(px2 - rad2, py2 - rad2, rad2 * 2.0f, rad2 * 2.0f);
        g.setColour(Colour(C_BRT));
        g.fillEllipse(px2 - rad2 * 0.35f, py2 - rad2 * 0.35f, rad2 * 0.7f, rad2 * 0.7f);
    }
}

void SpatialMixProEditor::drawBottom(Graphics& g)
{
    g.setColour(Colour(0xFF090909)); g.fillRect(rBottom);
    g.setColour(Colour(C_BORDER));
    g.drawHorizontalLine(rBottom.getY(), (float)rBottom.getX(), (float)rBottom.getRight());

    float fs = jmax(9.0f, (float)rBottom.getHeight() * 0.42f);
    g.setFont(Font("Courier New", fs, Font::plain));
    g.setColour(Colour(C_DIM));
    g.drawText("SpatialMix Pro v2.0", rBottom.getX() + 8, rBottom.getY(), 200, rBottom.getHeight(), Justification::centredLeft, false);

    static const char* modeN[] = { "HRTF BINAURAL", "STEREO", "2.1 SIM" };
    g.setColour(Colour(C_AMBER));
    g.drawText(String("MODE: ") + modeN[proc.paramMode->getIndex()],
               rBottom.getX() + 220, rBottom.getY(), 200, rBottom.getHeight(), Justification::centredLeft, false);

    String opts;
    if (proc.paramMono->get())  opts += " MONO";
    if (proc.paramPhase->get()) opts += " PHASE";
    if (proc.paramDual->get())  opts += " DUAL";
    g.setColour(Colour(C_GREEN));
    g.drawText(opts, rBottom.getX() + 430, rBottom.getY(), 200, rBottom.getHeight(), Justification::centredLeft, false);

    g.setColour(Colour(C_TEXT));
    g.drawText(String::formatted("X:%.2f  Y:%.2f  Z:%.2f  V:%.2fx",
               proc.paramX->get(), proc.paramY->get(), proc.paramZ->get(), proc.paramGain->get()),
               rBottom.getRight() - 380, rBottom.getY(), 370, rBottom.getHeight(), Justification::centredRight, false);
}

// ── MOUSE ────────────────────────────────────────────────────

void SpatialMixProEditor::mouseDown(const MouseEvent& e)
{
    auto pt = e.position;
    dragging = None;
    bool dual = proc.paramDual->get();

    if (rTop.toFloat().contains(pt))
    {
        if (dual)
        {
            auto p2 = topPt(proc.paramX2->get(), proc.paramZ2->get());
            if (pt.getDistanceFrom(p2) < 22.0f) { dragging = Spk2Top; return; }
        }
        dragging = Spk1Top;
        float x, z; topToXZ(pt, x, z);
        proc.paramX->setValueNotifyingHost(proc.paramX->convertTo0to1(x));
        proc.paramZ->setValueNotifyingHost(proc.paramZ->convertTo0to1(z));
    }
    else if (rFront.toFloat().contains(pt))
    {
        if (dual)
        {
            // Spk2 front position based on paramX2/Y2
            auto r2a = rFront.toFloat().reduced(20.0f);
            float px2 = r2a.getX() + (proc.paramX2->get() + 8.0f) / 16.0f * r2a.getWidth();
            float py2 = r2a.getBottom() - proc.paramY2->get() / 6.0f * r2a.getHeight();
            if (pt.getDistanceFrom(Point<float>(px2, py2)) < 22.0f) { dragging = Spk2Front; return; }
        }
        dragging = Spk1Front;
        float y, z; frontToYZ(pt, y, z);
        proc.paramY->setValueNotifyingHost(proc.paramY->convertTo0to1(y));
    }
}

void SpatialMixProEditor::mouseDrag(const MouseEvent& e)
{
    auto pt = e.position;
    if (dragging == Spk1Top)
    {
        float x, z; topToXZ(pt, x, z);
        proc.paramX->setValueNotifyingHost(proc.paramX->convertTo0to1(x));
        proc.paramZ->setValueNotifyingHost(proc.paramZ->convertTo0to1(z));
    }
    else if (dragging == Spk2Top)
    {
        float x, z; topToXZ(pt, x, z);
        proc.paramX2->setValueNotifyingHost(proc.paramX2->convertTo0to1(x));
        proc.paramZ2->setValueNotifyingHost(proc.paramZ2->convertTo0to1(z));
    }
    else if (dragging == Spk1Front)
    {
        float y, z; frontToYZ(pt, y, z);
        proc.paramY->setValueNotifyingHost(proc.paramY->convertTo0to1(y));
        // Z unchanged — use slider or top view
    }
    else if (dragging == Spk2Front)
    {
        float y, z; frontToYZ(pt, y, z);
        proc.paramY2->setValueNotifyingHost(proc.paramY2->convertTo0to1(y));
        // Z2 unchanged
    }
}

void SpatialMixProEditor::mouseUp(const MouseEvent&) { dragging = None; }
