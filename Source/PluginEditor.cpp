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

    auto mkH = [&](Slider& s, const String& suffix) {
        s.setSliderStyle(Slider::LinearHorizontal);
        s.setTextBoxStyle(Slider::TextBoxRight, false, 55, 20);
        s.setTextValueSuffix(suffix);
        addAndMakeVisible(s);
    };
    mkH(sliderX,     " m");
    mkH(sliderZ,     " m");
    mkH(sliderGain,  "x");
    mkH(sliderX2,    " m");
    mkH(sliderZ2,    " m");
    mkH(sliderGain2, "x");

    sliderY.setSliderStyle(Slider::LinearVertical);
    sliderY.setTextBoxStyle(Slider::TextBoxBelow, false, 45, 18);
    sliderY.setTextValueSuffix(" m");
    addAndMakeVisible(sliderY);

    attX    = std::make_unique<SliderParameterAttachment>(*proc.paramX,     sliderX);
    attZ    = std::make_unique<SliderParameterAttachment>(*proc.paramZ,     sliderZ);
    attGain = std::make_unique<SliderParameterAttachment>(*proc.paramGain,  sliderGain);
    attX2   = std::make_unique<SliderParameterAttachment>(*proc.paramX2,    sliderX2);
    attZ2   = std::make_unique<SliderParameterAttachment>(*proc.paramZ2,    sliderZ2);
    attGain2= std::make_unique<SliderParameterAttachment>(*proc.paramGain2, sliderGain2);
    attY    = std::make_unique<SliderParameterAttachment>(*proc.paramY,     sliderY);

    for (auto* b : { &btnHRTF, &btnStereo, &btn21, &btnMono, &btnPhase, &btnDual, &btnHelp, &btnMono2, &btnPhase2 })
        addAndMakeVisible(*b);

    btnHRTF  .onClick = [this] { proc.paramMode->setValueNotifyingHost(0.0f);  updateModeButtons(); };
    btnStereo.onClick = [this] { proc.paramMode->setValueNotifyingHost(0.5f);  updateModeButtons(); };
    btn21    .onClick = [this] { proc.paramMode->setValueNotifyingHost(1.0f);  updateModeButtons(); };
    btnMono  .onClick = [this] { proc.paramMono ->setValueNotifyingHost(proc.paramMono->get()  ? 0.0f : 1.0f); updateOptionButtons(); };
    btnPhase .onClick = [this] { proc.paramPhase->setValueNotifyingHost(proc.paramPhase->get() ? 0.0f : 1.0f); updateOptionButtons(); };
    btnDual  .onClick = [this] { proc.paramDual ->setValueNotifyingHost(proc.paramDual->get()  ? 0.0f : 1.0f); updateOptionButtons(); resized(); };
    btnMono2 .onClick = [this] { proc.paramMono2 ->setValueNotifyingHost(proc.paramMono2->get()  ? 0.0f : 1.0f); updateOptionButtons(); };
    btnPhase2.onClick = [this] { proc.paramPhase2->setValueNotifyingHost(proc.paramPhase2->get() ? 0.0f : 1.0f); updateOptionButtons(); };
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
    sliderZ2.setVisible(dual);
    sliderGain2.setVisible(dual);
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
        << "VISTE\n"
        << "  Vista Top: trascina il punto per muovere la cassa (X/Z)\n"
        << "  Vista Frontale: trascina per altezza e profondita (Y/Z)\n\n"
        << "SLIDER SINISTRA\n"
        << "  X = sinistra / destra (-8..+8 m)\n"
        << "  Z = profondita fronte / retro (-8..+8 m)\n"
        << "  V = volume della cassa (0..2x)\n\n"
        << "SLIDER FRONTALE (destra)\n"
        << "  Y = altezza (0..6 m)\n\n"
        << "MODALITA OUTPUT\n"
        << "  HRTF   = simulazione 3D binaurale in cuffia\n"
        << "  STEREO = downmix stereo standard\n"
        << "  2.1    = stereo + rinforzo sub 120 Hz\n\n"
        << "OPZIONI\n"
        << "  MONO  = collassa L+R in mono\n"
        << "  PHASE = inversione di fase 180 gradi\n"
        << "  DUAL  = sdoppia la cassa in due sorgenti indipendenti\n\n"
        << "NOME\n"
        << "  Doppio click sul nome per rinominare\n\n"
        << "NOTA FL STUDIO\n"
        << "  Inserisci una istanza per ogni traccia da posizionare.";
    AlertWindow::showMessageBoxAsync(MessageBoxIconType::InfoIcon,
                                     "SpatialMix Pro - Guida", msg, "OK");
}

void SpatialMixProEditor::resized()
{
    auto b = getLocalBounds();
    const int hH  = jmax(46, b.getHeight() / 20);
    const int stH = jmax(34, b.getHeight() / 26);
    const int lW  = jmax(220, b.getWidth() / 7);
    const int yW  = 68;

    rHeader = b.removeFromTop(hH);
    rBottom = b.removeFromBottom(stH);
    rLeft   = b.removeFromLeft(lW);

    // Views stacked vertically, same width
    rTop   = b.removeFromTop((int)(b.getHeight() * 0.62f));
    rFront = b;

    // Help button
    btnHelp.setBounds(rHeader.getRight() - 44, rHeader.getY() + 6, 34, hH - 12);

    // Left panel
    auto lp = rLeft.reduced(8, 8);
    const int rowH = jmax(22, lp.getHeight() / 14);

    // 1. Name label
    nameLabel.setBounds(lp.removeFromTop(jmax(22, hH - 14)));
    lp.removeFromTop(4);

    // 2. MONO / PHASE / DUAL buttons (spk1)
    {
        auto row = lp.removeFromTop(rowH);
        int bw = row.getWidth() / 3;
        btnMono .setBounds(row.removeFromLeft(bw).reduced(2, 0));
        btnPhase.setBounds(row.removeFromLeft(bw).reduced(2, 0));
        btnDual .setBounds(row.reduced(2, 0));
    }
    lp.removeFromTop(4);

    // 3. Spk1 sliders X/Z/V
    sliderX.setBounds(lp.removeFromTop(rowH));    lp.removeFromTop(2);
    sliderZ.setBounds(lp.removeFromTop(rowH));    lp.removeFromTop(2);
    sliderGain.setBounds(lp.removeFromTop(rowH)); lp.removeFromTop(6);

    bool dual = proc.paramDual->get();
    if (dual)
    {
        // 4. MONO2 / PHASE2 for spk2
        auto row2 = lp.removeFromTop(rowH);
        int bw2 = row2.getWidth() / 2;
        btnMono2 .setBounds(row2.removeFromLeft(bw2).reduced(2, 0));
        btnPhase2.setBounds(row2.reduced(2, 0));
        lp.removeFromTop(4);

        // 5. Spk2 sliders
        sliderX2.setBounds(lp.removeFromTop(rowH));    lp.removeFromTop(2);
        sliderZ2.setBounds(lp.removeFromTop(rowH));    lp.removeFromTop(2);
        sliderGain2.setBounds(lp.removeFromTop(rowH)); lp.removeFromTop(6);
    }

    // 6. Mode buttons HRTF/STEREO/2.1
    lp.removeFromTop(4);
    {
        auto row = lp.removeFromTop(rowH);
        int bw = row.getWidth() / 3;
        btnHRTF  .setBounds(row.removeFromLeft(bw).reduced(2, 0));
        btnStereo.setBounds(row.removeFromLeft(bw).reduced(2, 0));
        btn21    .setBounds(row.reduced(2, 0));
    }

    // Y slider on right edge of front view
    sliderY.setBounds(rFront.getRight() - yW, rFront.getY() + 10, yW - 4, rFront.getHeight() - 20);

    sliderX2.setVisible(dual);
    sliderZ2.setVisible(dual);
    sliderGain2.setVisible(dual);
    btnMono2.setVisible(dual);
    btnPhase2.setVisible(dual);
}

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
Point<float> SpatialMixProEditor::frontPt(float y, float z) const
{
    auto fr = rFront; fr.removeFromRight(68);
    auto r = fr.toFloat().reduced(18.0f);
    return { r.getX() + (z + 8.0f) / 16.0f * r.getWidth(),
             r.getBottom() - y / 6.0f * r.getHeight() };
}
void SpatialMixProEditor::frontToYZ(Point<float> pt, float& y, float& z) const
{
    auto fr = rFront; fr.removeFromRight(68);
    auto r = fr.toFloat().reduced(18.0f);
    z = ((pt.x - r.getX()) / r.getWidth())  * 16.0f - 8.0f;
    y = (r.getBottom() - pt.y) / r.getHeight() * 6.0f;
    y = jlimit(0.0f, 6.0f, y);
    z = jlimit(-8.0f, 8.0f, z);
}

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
    float fs = jmax(9.0f, (float)rLeft.getHeight() / 52.0f);
    int rowH = jmax(24, lp.getHeight() / 12);
    int yy = lp.getY() + jmax(22, rHeader.getHeight() - 14) + 6;

    g.setFont(Font("Courier New", fs, Font::bold));
    auto drawLbl = [&](const String& s, Colour col) {
        g.setColour(col);
        g.drawText(s, lp.getX(), yy, 20, rowH, Justification::centredLeft, false);
        yy += rowH + 3;
    };
    drawLbl("X", Colour(C_AMBER));
    drawLbl("Z", Colour(C_AMBER));
    drawLbl("V", Colour(C_BLUE));

    if (proc.paramDual->get())
    {
        yy += 8;
        g.setColour(Colour(C_BLUE).withAlpha(0.7f));
        g.setFont(Font("Courier New", fs * 0.85f, Font::plain));
        g.drawText("CASSA 2", lp.getX(), yy, lp.getWidth(), rowH, Justification::centredLeft, false);
        yy += rowH - 2;
        drawLbl("X2", Colour(C_BLUE));
        drawLbl("Z2", Colour(C_BLUE));
        drawLbl("V2", Colour(C_GREEN));
    }

    float cx = proc.paramX->get(), cy = proc.paramY->get(), cz = proc.paramZ->get();
    float hd = std::sqrt(cx * cx + cz * cz);
    float az = std::atan2(cx, -cz) * (180.0f / MathConstants<float>::pi);
    float el = std::atan2(cy, jmax(hd, 0.001f)) * (180.0f / MathConstants<float>::pi);
    float dist = std::sqrt(cx * cx + cy * cy + cz * cz);

    int by = rLeft.getBottom() - (int)(rLeft.getHeight() * 0.22f);
    g.setFont(Font("Courier New", fs * 0.88f, Font::plain));
    g.setColour(Colour(C_DIM));
    g.drawText("AZIMUTH", lp.getX(), by,        lp.getWidth() / 2, rowH, Justification::centredLeft, false);
    g.drawText("ELEV",    lp.getX(), by + rowH,  lp.getWidth() / 2, rowH, Justification::centredLeft, false);
    g.drawText("DIST",    lp.getX(), by + rowH*2, lp.getWidth() / 2, rowH, Justification::centredLeft, false);
    g.setColour(Colour(C_AMBER));
    g.drawText(String(az,   1) + " deg", lp.getX(), by,        lp.getWidth(), rowH, Justification::centredRight, false);
    g.drawText(String(el,   1) + " deg", lp.getX(), by + rowH,  lp.getWidth(), rowH, Justification::centredRight, false);
    g.setColour(Colour(C_BLUE));
    g.drawText(String(dist, 2) + " m",   lp.getX(), by + rowH*2, lp.getWidth(), rowH, Justification::centredRight, false);
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

    float fs = jmax(9.0f, (float)rTop.getHeight() / 50.0f);

    g.setColour(Colour(0xFF222222)); g.drawRect(rv, 2.0f);

    float dotR = jmax(6.0f, (float)rTop.getWidth() / 70.0f);

    // VISTA TOP label — centered at top
    {
        int lblH = (int)(fs * 1.8f);
        g.setColour(Colour(0xCC000000));
        g.fillRect(rTop.getCentreX() - 80, rTop.getY() + 4, 160, lblH);
        g.setFont(Font("Courier New", fs * 1.5f, Font::bold));
        g.setColour(Colour(C_AMBER));
        g.drawText("VISTA TOP", rTop.getX(), rTop.getY() + 4, rTop.getWidth(), lblH,
                   Justification::centred, false);
    }

    g.setFont(Font("Courier New", fs * 0.8f, Font::plain));
    g.setColour(Colour(C_DIM));
    g.drawText("PALCO/FRONTE", rTop.getX(), rTop.getY() + 2, rTop.getWidth(), (int)fs + 2, Justification::centred, false);
    g.drawText("RETRO", rTop.getX(), rTop.getBottom() - (int)fs - 4, rTop.getWidth(), (int)fs + 2, Justification::centred, false);
    g.drawText("SX", rTop.getX() + 2, rTop.getCentreY() - (int)fs, (int)fs * 2, (int)fs * 2, Justification::centred, false);
    g.drawText("DX", rTop.getRight() - (int)fs * 2 - 2, rTop.getCentreY() - (int)fs, (int)fs * 2, (int)fs * 2, Justification::centred, false);

    // Listener
    {
        auto pt = topPt(0.0f, 0.0f);
        g.setColour(Colour(C_BLUE).withAlpha(0.18f));
        g.fillEllipse(pt.x - dotR * 4, pt.y - dotR * 4, dotR * 8, dotR * 8);
        g.setColour(Colour(C_BLUE));
        g.fillEllipse(pt.x - dotR * 0.8f, pt.y - dotR * 0.8f, dotR * 1.6f, dotR * 1.6f);
        g.setColour(Colour(C_BLUE).withAlpha(0.6f));
        g.setFont(Font("Courier New", fs * 0.75f, Font::bold));
        g.drawText("ASCOLTATORE", (int)(pt.x - 50), (int)(pt.y + dotR + 2), 100, (int)(fs + 2), Justification::centred, false);
    }

    // Speaker 1 (amber)
    {
        float sx = proc.paramX->get(), sz = proc.paramZ->get();
        auto pt = topPt(sx, sz);
        g.setColour(Colour(C_AMBER).withAlpha(0.3f));
        g.fillEllipse(pt.x - dotR * 4, pt.y - dotR * 4, dotR * 8, dotR * 8);
        g.setColour(Colour(C_AMBER));
        g.fillEllipse(pt.x - dotR, pt.y - dotR, dotR * 2, dotR * 2);
        g.setColour(Colour(C_BRT));
        g.fillEllipse(pt.x - dotR * 0.4f, pt.y - dotR * 0.4f, dotR * 0.8f, dotR * 0.8f);
        g.setColour(Colour(C_AMBER));
        g.setFont(Font("Courier New", fs, Font::bold));
        g.drawText(proc.sourceName, (int)(pt.x - 60), (int)(pt.y - dotR * 2.5f - fs - 2), 120, (int)(fs + 2), Justification::centred, false);
    }

    // Speaker 2 (blue, dual)
    if (proc.paramDual->get())
    {
        float sx2 = proc.paramX2->get(), sz2 = proc.paramZ2->get();
        auto pt = topPt(sx2, sz2);
        g.setColour(Colour(0xFF3399FF).withAlpha(0.25f));
        g.fillEllipse(pt.x - dotR * 4, pt.y - dotR * 4, dotR * 8, dotR * 8);
        g.setColour(Colour(0xFF3399FF));
        g.fillEllipse(pt.x - dotR, pt.y - dotR, dotR * 2, dotR * 2);
        g.setColour(Colour(C_BRT));
        g.fillEllipse(pt.x - dotR * 0.4f, pt.y - dotR * 0.4f, dotR * 0.8f, dotR * 0.8f);
        g.setColour(Colour(0xFF3399FF));
        g.setFont(Font("Courier New", fs, Font::bold));
        g.drawText(proc.sourceName + " [2]", (int)(pt.x - 65), (int)(pt.y - dotR * 2.5f - fs - 2), 130, (int)(fs + 2), Justification::centred, false);
    }
}

void SpatialMixProEditor::drawFrontView(Graphics& g)
{
    auto fr = rFront; fr.removeFromRight(68);
    g.setColour(Colour(0xFF050505)); g.fillRect(rFront);
    g.setColour(Colour(C_BORDER));
    g.drawHorizontalLine(rFront.getY(), (float)rFront.getX(), (float)rFront.getRight());

    auto rv = fr.toFloat().reduced(18.0f);
    g.setColour(Colour(0xFF0D0D0D)); g.fillRect(rv);

    // Grid lines
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
        // Height labels on grid lines
        if (i > 0) {
            float fs2 = jmax(8.0f, (float)fr.getWidth() / 80.0f);
            g.setFont(Font("Courier New", fs2, Font::plain));
            g.setColour(Colour(0xFF555555));
            g.drawText(String(i) + "m", (int)rv.getX() + 2, (int)yr - (int)fs2 - 1,
                       30, (int)fs2 + 1, Justification::centredLeft, false);
        }
    }
    g.setColour(Colour(0xFF303030)); g.drawRect(rv, 1.5f);

    float fs   = jmax(10.0f, (float)fr.getHeight() / 18.0f);
    float dotR = jmax(6.0f,  (float)fr.getWidth()  / 50.0f);

    // VISTA FRONTALE label — large, with background for contrast
    {
        String lbl = "VISTA FRONTALE";
        int lblW = fr.getWidth() - 80;
        int lblH = (int)(fs * 1.8f);
        int lblX = fr.getX() + 4;
        int lblY = fr.getY() + 4;
        g.setColour(Colour(0xCC000000));
        g.fillRect(lblX - 2, lblY - 2, lblW + 4, lblH + 4);
        g.setFont(Font("Courier New", fs * 1.5f, Font::bold));
        g.setColour(Colour(C_AMBER));
        g.drawText(lbl, lblX, lblY, lblW, lblH, Justification::centredLeft, false);
    }

    // Direction labels with dark background boxes for contrast
    auto drawContrastLabel = [&](const String& txt, int x, int y, int w, int h, Colour col) {
        g.setColour(Colour(0xBB000000));
        g.fillRect(x, y, w, h);
        g.setColour(col);
        g.drawText(txt, x, y, w, h, Justification::centred, false);
    };

    int lblH2 = (int)(fs * 1.1f);
    g.setFont(Font("Courier New", fs, Font::bold));
    drawContrastLabel("PALCO",  (int)rv.getX(),                  (int)rv.getY() + 2,       80, lblH2, Colour(C_GREEN));
    drawContrastLabel("RETRO",  (int)rv.getRight() - 80,         (int)rv.getY() + 2,       80, lblH2, Colour(C_GREEN));
    drawContrastLabel("SUOLO",  (int)rv.getCentreX() - 40,       (int)rv.getBottom() - lblH2 - 2, 80, lblH2, Colour(0xFFAAAAAA));
    drawContrastLabel("SOFFITTO",(int)rv.getCentreX() - 50,      (int)rv.getY() + 2,      100, lblH2, Colour(0xFFAAAAAA));

    // Big Y label with background
    {
        int yLblSize = (int)(rFront.getHeight() * 0.18f);
        int yLblX = rFront.getRight() - 66;
        int yLblY = rFront.getCentreY() - yLblSize / 2;
        g.setColour(Colour(0xCC000000));
        g.fillRoundedRectangle((float)yLblX, (float)yLblY, 60.0f, (float)yLblSize, 4.0f);
        g.setColour(Colour(C_GREEN));
        g.setFont(Font("Courier New", (float)yLblSize * 0.75f, Font::bold));
        g.drawText("Y", yLblX, yLblY, 60, yLblSize, Justification::centred, false);
        g.setFont(Font("Courier New", (float)yLblSize * 0.25f, Font::plain));
        g.setColour(Colour(0xFF888888));
        g.drawText("ALTEZZA", yLblX, yLblY + (int)(yLblSize * 0.72f), 60, (int)(yLblSize * 0.28f), Justification::centred, false);
    }

    // Listener
    {
        auto pt = frontPt(1.2f, 0.0f);
        g.setColour(Colour(C_BLUE).withAlpha(0.25f));
        g.fillEllipse(pt.x - dotR * 2, pt.y - dotR * 2, dotR * 4, dotR * 4);
        g.setColour(Colour(C_BLUE));
        g.fillEllipse(pt.x - dotR * 0.7f, pt.y - dotR * 0.7f, dotR * 1.4f, dotR * 1.4f);
    }

    // Speaker 1
    {
        float sy = proc.paramY->get(), sz = proc.paramZ->get();
        auto pt = frontPt(sy, sz);
        g.setColour(Colour(C_AMBER));
        g.fillEllipse(pt.x - dotR, pt.y - dotR, dotR * 2, dotR * 2);
        g.setColour(Colour(C_BRT));
        g.fillEllipse(pt.x - dotR * 0.4f, pt.y - dotR * 0.4f, dotR * 0.8f, dotR * 0.8f);
    }

    // Speaker 2 (dual)
    if (proc.paramDual->get())
    {
        float sy2 = proc.paramY2->get(), sz2 = proc.paramZ2->get();
        auto pt = frontPt(sy2, sz2);
        g.setColour(Colour(0xFF3399FF));
        g.fillEllipse(pt.x - dotR, pt.y - dotR, dotR * 2, dotR * 2);
        g.setColour(Colour(C_BRT));
        g.fillEllipse(pt.x - dotR * 0.4f, pt.y - dotR * 0.4f, dotR * 0.8f, dotR * 0.8f);
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
            auto p2 = frontPt(proc.paramY2->get(), proc.paramZ2->get());
            if (pt.getDistanceFrom(p2) < 22.0f) { dragging = Spk2Front; return; }
        }
        dragging = Spk1Front;
        float y, z; frontToYZ(pt, y, z);
        proc.paramY->setValueNotifyingHost(proc.paramY->convertTo0to1(y));
        proc.paramZ->setValueNotifyingHost(proc.paramZ->convertTo0to1(z));
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
        proc.paramZ->setValueNotifyingHost(proc.paramZ->convertTo0to1(z));
    }
    else if (dragging == Spk2Front)
    {
        float y, z; frontToYZ(pt, y, z);
        proc.paramY2->setValueNotifyingHost(proc.paramY2->convertTo0to1(y));
        proc.paramZ2->setValueNotifyingHost(proc.paramZ2->convertTo0to1(z));
    }
}

void SpatialMixProEditor::mouseUp(const MouseEvent&) { dragging = None; }
