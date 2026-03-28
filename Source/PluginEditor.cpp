#include "PluginEditor.h"
using namespace juce;

const SpatialMixProEditor::HallSection SpatialMixProEditor::kSections[] = {
    { "VIOLINI I",    -6.5f, -5.5f, 3.2f, 3.0f, 0xFF00D4FF },
    { "VIOLINI II",   -3.0f, -5.5f, 2.8f, 3.0f, 0xFF00D4FF },
    { "VIOLE",         0.3f, -5.5f, 2.5f, 3.0f, 0xFF39D4FF },
    { "VIOLONCELLI",   3.0f, -5.5f, 2.8f, 3.0f, 0xFF39D4FF },
    { "CONTRABBASSI",  6.0f, -5.0f, 2.0f, 3.5f, 0xFF1A8CFF },
    { "FLAUTI",       -4.0f, -1.5f, 2.0f, 2.0f, 0xFF39FF14 },
    { "OBOI",         -1.8f, -1.5f, 2.0f, 2.0f, 0xFF39FF14 },
    { "CLARINETTI",    0.2f, -1.5f, 2.0f, 2.0f, 0xFF5AFF3A },
    { "FAGOTTI",       2.4f, -1.5f, 2.0f, 2.0f, 0xFF5AFF3A },
    { "CORNI",        -5.5f, -0.5f, 2.5f, 2.5f, 0xFFFF9500 },
    { "TROMBE",       -2.5f, -0.5f, 2.5f, 2.5f, 0xFFFFB030 },
    { "TROMBONI",      0.5f, -0.5f, 2.5f, 2.5f, 0xFFFFB030 },
    { "TUBA",          3.2f, -0.5f, 1.8f, 2.5f, 0xFFFF6010 },
    { "TIMPANI",       4.5f, -0.5f, 2.5f, 2.5f, 0xFFFF3C3C },
    { "PERCUSSIONI",   6.5f, -0.5f, 1.8f, 3.0f, 0xFFFF3C3C },
};
const int SpatialMixProEditor::kNumSections = 16;

SpatialMixProEditor::SpatialMixProEditor(SpatialMixProProcessor& p)
    : AudioProcessorEditor(p), proc(p),
      btnHRTF("HRTF"), btnStereo("STEREO"), btn21("2.1"),
      btnMono("MONO"), btnPhase("PHASE"), btnDual("DUAL"), btnHelp("?")
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

    for (auto* b : { &btnHRTF, &btnStereo, &btn21, &btnMono, &btnPhase, &btnDual, &btnHelp })
        addAndMakeVisible(*b);

    btnHRTF  .onClick = [this] { proc.paramMode->setValueNotifyingHost(0.0f);  updateModeButtons(); };
    btnStereo.onClick = [this] { proc.paramMode->setValueNotifyingHost(0.5f);  updateModeButtons(); };
    btn21    .onClick = [this] { proc.paramMode->setValueNotifyingHost(1.0f);  updateModeButtons(); };
    btnMono  .onClick = [this] { proc.paramMono ->setValueNotifyingHost(proc.paramMono->get()  ? 0.0f : 1.0f); updateOptionButtons(); };
    btnPhase .onClick = [this] { proc.paramPhase->setValueNotifyingHost(proc.paramPhase->get() ? 0.0f : 1.0f); updateOptionButtons(); };
    btnDual  .onClick = [this] { proc.paramDual ->setValueNotifyingHost(proc.paramDual->get()  ? 0.0f : 1.0f); updateOptionButtons(); resized(); };
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
    btnMono .setToggleState(proc.paramMono->get(),  dontSendNotification);
    btnPhase.setToggleState(proc.paramPhase->get(), dontSendNotification);
    btnDual .setToggleState(proc.paramDual->get(),  dontSendNotification);
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
    const int hH = jmax(46, b.getHeight() / 20);
    const int stH = jmax(34, b.getHeight() / 26);
    const int lW  = jmax(210, b.getWidth() / 7);
    const int fW  = jmax(260, b.getWidth() / 6);

    rHeader = b.removeFromTop(hH);
    rBottom = b.removeFromBottom(stH);
    rLeft   = b.removeFromLeft(lW);
    rFront  = b.removeFromRight(fW);
    rTop    = b;

    // Help button in header
    btnHelp.setBounds(rHeader.getRight() - 44, rHeader.getY() + 6, 34, hH - 12);

    // Left panel
    auto lp = rLeft.reduced(8, 8);
    nameLabel.setBounds(lp.removeFromTop(jmax(22, hH - 14)));
    lp.removeFromTop(6);
    const int rowH = jmax(24, lp.getHeight() / 12);

    sliderX.setBounds(lp.removeFromTop(rowH));   lp.removeFromTop(3);
    sliderZ.setBounds(lp.removeFromTop(rowH));   lp.removeFromTop(3);
    sliderGain.setBounds(lp.removeFromTop(rowH)); lp.removeFromTop(8);

    bool dual = proc.paramDual->get();
    if (dual)
    {
        sliderX2.setBounds(lp.removeFromTop(rowH));    lp.removeFromTop(3);
        sliderZ2.setBounds(lp.removeFromTop(rowH));    lp.removeFromTop(3);
        sliderGain2.setBounds(lp.removeFromTop(rowH)); lp.removeFromTop(8);
    }

    lp.removeFromTop(4);
    auto row1 = lp.removeFromTop(rowH);
    int bw1 = row1.getWidth() / 3;
    btnHRTF  .setBounds(row1.removeFromLeft(bw1).reduced(2, 0));
    btnStereo.setBounds(row1.removeFromLeft(bw1).reduced(2, 0));
    btn21    .setBounds(row1.reduced(2, 0));
    lp.removeFromTop(4);
    auto row2 = lp.removeFromTop(rowH);
    int bw2 = row2.getWidth() / 3;
    btnMono .setBounds(row2.removeFromLeft(bw2).reduced(2, 0));
    btnPhase.setBounds(row2.removeFromLeft(bw2).reduced(2, 0));
    btnDual .setBounds(row2.reduced(2, 0));

    // Y slider in front panel
    sliderY.setBounds(rFront.removeFromRight(52).reduced(4, 20));

    sliderX2.setVisible(dual);
    sliderZ2.setVisible(dual);
    sliderGain2.setVisible(dual);
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
    auto fr = rFront; fr.removeFromRight(52);
    auto r = fr.toFloat().reduced(18.0f);
    return { r.getX() + (z + 8.0f) / 16.0f * r.getWidth(),
             r.getBottom() - y / 6.0f * r.getHeight() };
}
void SpatialMixProEditor::frontToYZ(Point<float> pt, float& y, float& z) const
{
    auto fr = rFront; fr.removeFromRight(52);
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

    for (int i = 0; i < kNumSections; ++i)
    {
        const auto& s = kSections[i];
        auto ptTL = topPt(s.x,       s.z);
        auto ptBR = topPt(s.x + s.w, s.z + s.d);
        Rectangle<float> sr(ptTL.x, ptTL.y, ptBR.x - ptTL.x, ptBR.y - ptTL.y);
        g.setColour(Colour(s.col).withAlpha(0.08f)); g.fillRect(sr);
        g.setColour(Colour(s.col).withAlpha(0.35f)); g.drawRect(sr, 1.0f);
        g.setFont(Font("Courier New", jmax(7.0f, (float)rTop.getWidth() / 120.0f), Font::bold));
        g.setColour(Colour(s.col).withAlpha(0.8f));
        g.drawText(s.name, (int)sr.getX(), (int)sr.getY(), (int)sr.getWidth(), (int)sr.getHeight(),
                   Justification::centred, true);
    }

    g.setColour(Colour(0xFF222222)); g.drawRect(rv, 2.0f);

    float dotR = jmax(6.0f, (float)rTop.getWidth() / 70.0f);

    // VISTA TOP label
    g.setFont(Font("Courier New", fs * 1.5f, Font::bold));
    g.setColour(Colour(C_AMBER));
    g.drawText("VISTA TOP", rTop.getX(), rTop.getY() + 4, rTop.getWidth() - 4, (int)(fs * 2),
               Justification::centredRight, false);

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
    auto fr = rFront; fr.removeFromRight(52);
    g.setColour(Colour(0xFF050505)); g.fillRect(rFront);
    g.setColour(Colour(C_BORDER));
    g.drawVerticalLine(rFront.getX(), (float)rFront.getY(), (float)rFront.getBottom());

    auto rv = fr.toFloat().reduced(18.0f);
    g.setColour(Colour(0xFF080808)); g.fillRect(rv);

    g.setColour(Colour(0xFF0F0F0F));
    for (int i = 0; i <= 16; ++i)
    {
        float xr = rv.getX() + i * rv.getWidth() / 16.0f;
        g.drawVerticalLine((int)xr, rv.getY(), rv.getBottom());
    }
    for (int i = 0; i <= 6; ++i)
    {
        float yr = rv.getBottom() - i * rv.getHeight() / 6.0f;
        g.drawHorizontalLine((int)yr, rv.getX(), rv.getRight());
    }
    g.setColour(Colour(0xFF1E1E1E)); g.drawRect(rv, 1.5f);

    float fs   = jmax(8.0f, (float)fr.getWidth() / 28.0f);
    float dotR = jmax(5.0f, (float)fr.getWidth() / 55.0f);

    // VISTA FRONTALE label
    g.setFont(Font("Courier New", fs * 1.3f, Font::bold));
    g.setColour(Colour(C_AMBER));
    g.drawText("VISTA FRONTALE", fr.getX() + 2, fr.getY() + 4, fr.getWidth() - 4, (int)(fs * 2),
               Justification::centredLeft, false);

    g.setFont(Font("Courier New", fs * 0.75f, Font::plain));
    g.setColour(Colour(C_DIM));
    g.drawText("PALCO", fr.getX(), fr.getY() + (int)(fs * 2), fr.getWidth() / 2, (int)fs + 2, Justification::centredLeft, false);
    g.drawText("RETRO", fr.getX() + fr.getWidth() / 2, fr.getY() + (int)(fs * 2), fr.getWidth() / 2, (int)fs + 2, Justification::centredRight, false);
    g.drawText("SUOLO", fr.getX(), fr.getBottom() - (int)fs - 2, fr.getWidth(), (int)fs + 2, Justification::centredLeft, false);

    g.setFont(Font("Courier New", fs, Font::bold));
    g.setColour(Colour(C_GREEN));
    g.drawText("Y", rFront.getRight() - 48, rFront.getCentreY() - 10, 16, 20, Justification::centred, false);

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
