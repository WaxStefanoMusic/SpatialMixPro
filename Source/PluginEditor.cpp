#include "PluginEditor.h"
using namespace juce;

const uint32 SpatialMixProEditor::kSrcColors[MAX_SOURCES] = {
    0xFFFF9500, 0xFF00D4FF, 0xFF39FF14, 0xFFFF3C3C,
    0xFFC77DFF, 0xFFFF6B9D, 0xFFFFD700, 0xFF4ECDC4
};

const SpatialMixProEditor::HallSec SpatialMixProEditor::kHall[] = {
    { "VIOLINI I",   -6.5f,-5.5f, 3.2f,3.0f, 0xFF00D4FF },
    { "VIOLINI II",  -3.0f,-5.5f, 2.8f,3.0f, 0xFF00D4FF },
    { "VIOLE",        0.3f,-5.5f, 2.5f,3.0f, 0xFF39D4FF },
    { "VIOLONCELLI",  3.0f,-5.5f, 2.8f,3.0f, 0xFF39D4FF },
    { "CONTRABBASSI", 6.0f,-5.0f, 2.0f,3.5f, 0xFF1A8CFF },
    { "FLAUTI",      -4.0f,-1.5f, 2.0f,2.0f, 0xFF39FF14 },
    { "OBOI",        -1.8f,-1.5f, 2.0f,2.0f, 0xFF39FF14 },
    { "CLARINETTI",   0.2f,-1.5f, 2.0f,2.0f, 0xFF5AFF3A },
    { "FAGOTTI",      2.4f,-1.5f, 2.0f,2.0f, 0xFF5AFF3A },
    { "CORNI",       -5.5f, 1.5f, 2.5f,2.5f, 0xFFFF9500 },
    { "TROMBE",      -2.5f, 1.5f, 2.5f,2.5f, 0xFFFFB030 },
    { "TROMBONI",     0.5f, 1.5f, 2.5f,2.5f, 0xFFFFB030 },
    { "TUBA",         3.2f, 1.5f, 1.8f,2.5f, 0xFFFF6010 },
    { "ARPA",        -7.5f,-3.0f, 1.5f,2.5f, 0xFFC77DFF },
    { "TIMPANI",      4.5f, 1.5f, 2.5f,2.5f, 0xFFFF3C3C },
    { "PERCUSSIONI",  6.5f, 1.0f, 1.8f,3.0f, 0xFFFF3C3C },
};
const int SpatialMixProEditor::kHallN = 16;

// ============================================================
SpatialMixProEditor::SpatialMixProEditor(SpatialMixProProcessor& p)
    : AudioProcessorEditor(p), proc(p),
      btnHRTF("HRTF"), btnStereo("STEREO"), btn21("2.1"), btnHelp("?")
{
    setLookAndFeel(&laf);
    buildRows();

    // Selected source sliders
    auto mkH = [&](Slider& s, const String& sfx) {
        s.setSliderStyle(Slider::LinearHorizontal);
        s.setTextBoxStyle(Slider::TextBoxRight,false,55,20);
        s.setTextValueSuffix(sfx);
        addAndMakeVisible(s);
    };
    mkH(sliderX," m"); mkH(sliderZ," m"); mkH(sliderGain,"x");
    sliderY.setSliderStyle(Slider::LinearVertical);
    sliderY.setTextBoxStyle(Slider::TextBoxBelow,false,45,18);
    sliderY.setTextValueSuffix(" m");
    addAndMakeVisible(sliderY);

    sliderMaster.setSliderStyle(Slider::LinearHorizontal);
    sliderMaster.setTextBoxStyle(Slider::TextBoxRight,false,50,18);
    sliderMaster.setTextValueSuffix("x");
    addAndMakeVisible(sliderMaster);
    attMaster = std::make_unique<SliderParameterAttachment>(*proc.paramMasterGain, sliderMaster);

    for (auto* b : {&btnHRTF,&btnStereo,&btn21,&btnHelp}) addAndMakeVisible(*b);
    btnHRTF  .onClick=[this]{ proc.paramMode->setValueNotifyingHost(0.f); updateGlobalButtons(); };
    btnStereo.onClick=[this]{ proc.paramMode->setValueNotifyingHost(0.5f);updateGlobalButtons(); };
    btn21    .onClick=[this]{ proc.paramMode->setValueNotifyingHost(1.f); updateGlobalButtons(); };
    btnHelp  .onClick=[this]{ showHelp(); };

    selectSource(0);
    updateGlobalButtons();

    setResizable(true,false);
    setResizeLimits(960,540,3840,2160);
    getConstrainer()->setFixedAspectRatio(16.0/9.0);
    setSize(1600,900);
    startTimerHz(30);
}

SpatialMixProEditor::~SpatialMixProEditor()
{ stopTimer(); setLookAndFeel(nullptr); }

void SpatialMixProEditor::buildRows()
{
    for (int i = 0; i < MAX_SOURCES; ++i)
    {
        auto& r  = rows[i];
        auto& s  = proc.sources[i];
        if (!r.built)
        {
            r.nameLabel.setFont(Font("Courier New",12.f,Font::bold));
            r.nameLabel.setJustificationType(Justification::centredLeft);
            r.nameLabel.setEditable(false,true);
            r.nameLabel.onTextChange=[this,i]{ proc.sources[i].name=rows[i].nameLabel.getText(); };
            r.btnActive.setClickingTogglesState(false);
            r.btnActive.setButtonText("ON");
            r.btnMono  .setButtonText("M");
            r.btnPhase .setButtonText("\xe2\x88\x85");
            for (auto* b:{&r.btnActive,&r.btnMono,&r.btnPhase}) addAndMakeVisible(*b);
            addAndMakeVisible(r.nameLabel);
            r.built=true;
        }
        r.nameLabel.setText(s.name, dontSendNotification);
        r.btnActive.onClick=[this,i]{
            auto& sp=proc.sources[i];
            sp.paramActive->setValueNotifyingHost(sp.paramActive->get()?0.f:1.f);
        };
        r.btnMono.onClick=[this,i]{
            auto& sp=proc.sources[i];
            sp.paramMono->setValueNotifyingHost(sp.paramMono->get()?0.f:1.f);
        };
        r.btnPhase.onClick=[this,i]{
            auto& sp=proc.sources[i];
            sp.paramPhase->setValueNotifyingHost(sp.paramPhase->get()?0.f:1.f);
        };
    }
}

void SpatialMixProEditor::selectSource(int idx)
{
    selectedSrc = idx;
    attX.reset(); attY.reset(); attZ.reset(); attGain.reset();
    auto& s = proc.sources[idx];
    attX    = std::make_unique<SliderParameterAttachment>(*s.paramX,    sliderX);
    attY    = std::make_unique<SliderParameterAttachment>(*s.paramY,    sliderY);
    attZ    = std::make_unique<SliderParameterAttachment>(*s.paramZ,    sliderZ);
    attGain = std::make_unique<SliderParameterAttachment>(*s.paramGain, sliderGain);
}

void SpatialMixProEditor::updateGlobalButtons()
{
    int m=proc.paramMode->getIndex();
    btnHRTF  .setToggleState(m==0,dontSendNotification);
    btnStereo.setToggleState(m==1,dontSendNotification);
    btn21    .setToggleState(m==2,dontSendNotification);
}

void SpatialMixProEditor::timerCallback()
{
    // Update row button states
    for (int i=0;i<MAX_SOURCES;i++) {
        auto& r=rows[i]; auto& s=proc.sources[i];
        r.btnActive.setToggleState(s.paramActive->get(),dontSendNotification);
        r.btnMono  .setToggleState(s.paramMono->get(),  dontSendNotification);
        r.btnPhase .setToggleState(s.paramPhase->get(), dontSendNotification);
        if (r.nameLabel.getText()!=s.name)
            r.nameLabel.setText(s.name,dontSendNotification);
    }
    repaint(rTop); repaint(rFront); repaint(rBottom); repaint(rSourceList);
    updateGlobalButtons();
}

// ============================================================
void SpatialMixProEditor::resized()
{
    auto b = getLocalBounds();
    const int hH  = jmax(44,b.getHeight()/20);
    const int stH = jmax(30,b.getHeight()/30);
    const int lW  = jmax(220,b.getWidth()/7);
    const int cH  = jmax(100,b.getHeight()/9);
    const int fW  = jmax(240,b.getWidth()/6);
    const int yW  = 54;

    rHeader = b.removeFromTop(hH);
    rBottom = b.removeFromBottom(stH);
    rSourceList = b.removeFromLeft(lW);
    rControls = rSourceList.removeFromBottom(cH);
    rFront = b.removeFromRight(fW);
    rTop   = b;

    // Header
    {
        auto hb = rHeader.reduced(4,4);
        btnHelp.setBounds(hb.removeFromRight(38));
    }

    // Source list rows
    {
        auto lb = rSourceList.reduced(4,4);
        const int rowH = jmax(20, lb.getHeight()/MAX_SOURCES);
        for (int i=0;i<MAX_SOURCES;i++) {
            auto row = lb.removeFromTop(rowH);
            rows[i].btnActive.setBounds(row.removeFromRight(28).reduced(1,1));
            rows[i].btnPhase .setBounds(row.removeFromRight(22).reduced(1,1));
            rows[i].btnMono  .setBounds(row.removeFromRight(22).reduced(1,1));
            rows[i].nameLabel.setBounds(row.reduced(2,1));
        }
    }

    // Selected source controls
    {
        auto cb = rControls.reduced(6,4);
        const int rowH = jmax(18, cb.getHeight()/4);
        sliderX   .setBounds(cb.removeFromTop(rowH).reduced(0,1));
        sliderZ   .setBounds(cb.removeFromTop(rowH).reduced(0,1));
        sliderGain.setBounds(cb.removeFromTop(rowH).reduced(0,1));
        // mode + master in last row
        auto lastRow = cb.removeFromTop(rowH);
        int bw = lastRow.getWidth()/5;
        btnHRTF  .setBounds(lastRow.removeFromLeft(bw).reduced(1,1));
        btnStereo.setBounds(lastRow.removeFromLeft(bw).reduced(1,1));
        btn21    .setBounds(lastRow.removeFromLeft(bw).reduced(1,1));
        sliderMaster.setBounds(lastRow.reduced(2,1));
    }

    // Front view Y slider
    {
        auto fr = rFront;
        sliderY.setBounds(fr.removeFromRight(yW).reduced(4,22));
    }
}

// ============================================================
// COORDINATE MAPPING
// ============================================================
Point<float> SpatialMixProEditor::topPt(float x, float z) const
{
    auto r = rTop.toFloat().reduced(20.f);
    return { r.getX()+(x+8.f)/16.f*r.getWidth(),
             r.getY()+(z+8.f)/16.f*r.getHeight() };
}
void SpatialMixProEditor::topToXZ(Point<float> pt, float& x, float& z) const
{
    auto r = rTop.toFloat().reduced(20.f);
    x=((pt.x-r.getX())/r.getWidth())*16.f-8.f;
    z=((pt.y-r.getY())/r.getHeight())*16.f-8.f;
    x=jlimit(-8.f,8.f,x); z=jlimit(-8.f,8.f,z);
}
Point<float> SpatialMixProEditor::frontPt(float y, float z) const
{
    auto fr=rFront; fr.removeFromRight(54);
    auto r=fr.toFloat().reduced(18.f);
    return { r.getX()+(z+8.f)/16.f*r.getWidth(),
             r.getBottom()-y/6.f*r.getHeight() };
}
void SpatialMixProEditor::frontToYZ(Point<float> pt, float& y, float& z) const
{
    auto fr=rFront; fr.removeFromRight(54);
    auto r=fr.toFloat().reduced(18.f);
    z=((pt.x-r.getX())/r.getWidth())*16.f-8.f;
    y=(r.getBottom()-pt.y)/r.getHeight()*6.f;
    y=jlimit(0.f,6.f,y); z=jlimit(-8.f,8.f,z);
}

// ============================================================
// PAINT
// ============================================================
void SpatialMixProEditor::paint(Graphics& g)
{
    g.fillAll(Colour(C_BG));
    drawHeader(g);
    drawSourceList(g);
    drawControls(g);
    drawTopView(g);
    drawFrontView(g);
    drawBottom(g);
}

void SpatialMixProEditor::drawHeader(Graphics& g)
{
    g.setColour(Colour(C_PANEL)); g.fillRect(rHeader);
    g.setColour(Colour(C_BORDER));
    g.drawHorizontalLine(rHeader.getBottom(),(float)rHeader.getX(),(float)rHeader.getRight());
    float fs=(float)rHeader.getHeight()*.46f;
    g.setFont(Font("Courier New",fs,Font::bold));
    g.setColour(Colour(C_AMBER));
    g.drawText("SPATIALMIX PRO",rHeader.withWidth(260),Justification::centredLeft,false);
    g.setColour(Colour(C_DIM));
    g.setFont(Font("Courier New",fs*.55f,Font::plain));
    g.drawText("MULTI-SOURCE ATMOS SIMULATOR  |  8x STEREO IN  |  64-BIT VST3",
               rHeader.withTrimmedLeft(265).withTrimmedRight(50),Justification::centredLeft,false);
}

void SpatialMixProEditor::drawSourceList(Graphics& g)
{
    g.setColour(Colour(C_PANEL)); g.fillRect(rSourceList);
    g.setColour(Colour(C_BORDER));
    g.drawVerticalLine(rSourceList.getRight(),(float)rSourceList.getY(),(float)rSourceList.getBottom());

    float fs=jmax(8.f,(float)rSourceList.getHeight()/52.f);

    // Header
    g.setFont(Font("Courier New",fs*.85f,Font::bold));
    g.setColour(Colour(C_DIM));
    g.drawText("SORGENTI  (doppio click = rinomina)",
               rSourceList.getX()+4,rSourceList.getY()+2,
               rSourceList.getWidth()-4,(int)fs+4,Justification::centredLeft,false);

    // Active bus count indicator
    int activeBuses = proc.numActiveBuses;
    g.setColour(activeBuses>1 ? Colour(C_GREEN) : Colour(C_DIM));
    g.drawText("BUS ATTIVI: "+String(activeBuses),
               rSourceList.getX()+4,rSourceList.getY()+(int)fs+4,
               rSourceList.getWidth()-4,(int)fs+2,Justification::centredLeft,false);

    // Highlight selected row
    {
        auto lb=rSourceList.reduced(4,4);
        const int rowH=jmax(20,lb.getHeight()/MAX_SOURCES);
        auto selRow=lb.withTop(lb.getY()+selectedSrc*rowH).withHeight(rowH);
        g.setColour(Colour(kSrcColors[selectedSrc]).withAlpha(0.08f));
        g.fillRect(selRow);
        g.setColour(Colour(kSrcColors[selectedSrc]).withAlpha(0.4f));
        g.drawRect(selRow,1.f);
    }

    // Colour dots per row
    {
        auto lb=rSourceList.reduced(4,4);
        const int rowH=jmax(20,lb.getHeight()/MAX_SOURCES);
        for (int i=0;i<MAX_SOURCES;i++) {
            int yy=lb.getY()+i*rowH;
            g.setColour(Colour(kSrcColors[i]));
            g.fillEllipse(lb.getX()+1.f,(float)yy+rowH/2.f-4.f,8.f,8.f);
            // dim if inactive
            if (!proc.sources[i].paramActive->get()) {
                g.setColour(Colour(0x88000000));
                g.fillRect(lb.getX(),yy,lb.getWidth(),rowH);
            }
        }
    }
}

void SpatialMixProEditor::drawControls(Graphics& g)
{
    g.setColour(Colour(0xFF0A0A0A)); g.fillRect(rControls);
    g.setColour(Colour(C_BORDER));
    g.drawHorizontalLine(rControls.getY(),(float)rControls.getX(),(float)rControls.getRight());

    float fs=jmax(9.f,(float)rControls.getHeight()/6.f);
    auto cb=rControls.reduced(6,4);
    int rowH=jmax(18,cb.getHeight()/4);

    // Axis labels
    g.setFont(Font("Courier New",fs,Font::bold));
    int yy=cb.getY();
    auto drawLbl=[&](const String& s,Colour c){ g.setColour(c); g.drawText(s,cb.getX(),yy,16,rowH,Justification::centredLeft,false); yy+=rowH+1; };
    drawLbl("X",Colour(C_AMBER)); drawLbl("Z",Colour(C_AMBER)); drawLbl("V",Colour(C_BLUE));

    // Source name in controls header
    g.setFont(Font("Courier New",fs*.9f,Font::bold));
    g.setColour(Colour(kSrcColors[selectedSrc]));
    g.drawText("▶ "+proc.sources[selectedSrc].name,
               cb.getX()+20,cb.getY(),cb.getWidth()-20,rowH,Justification::centredLeft,false);

    // Master label
    g.setColour(Colour(C_DIM));
    g.setFont(Font("Courier New",fs*.75f,Font::plain));
    g.drawText("MASTER",cb.getX()+cb.getWidth()*3/5,cb.getY()+rowH*3,cb.getWidth()/5,rowH,Justification::centredLeft,false);
}

void SpatialMixProEditor::drawTopView(Graphics& g)
{
    g.setColour(Colour(0xFF050505)); g.fillRect(rTop);
    auto rv=rTop.toFloat().reduced(20.f);
    g.setColour(Colour(0xFF090909)); g.fillRect(rv);

    // Grid
    g.setColour(Colour(0xFF0F0F0F));
    for(int i=0;i<=16;i++){
        float xr=rv.getX()+i*rv.getWidth()/16.f;
        float yr=rv.getY()+i*rv.getHeight()/16.f;
        g.drawVerticalLine((int)xr,rv.getY(),rv.getBottom());
        g.drawHorizontalLine((int)yr,rv.getX(),rv.getRight());
    }

    // Hall sections
    for(int i=0;i<kHallN;i++){
        const auto& s=kHall[i];
        auto ptTL=topPt(s.x,s.z), ptBR=topPt(s.x+s.w,s.z+s.d);
        Rectangle<float> sr(ptTL.x,ptTL.y,ptBR.x-ptTL.x,ptBR.y-ptTL.y);
        g.setColour(Colour(s.col).withAlpha(0.07f)); g.fillRect(sr);
        g.setColour(Colour(s.col).withAlpha(0.3f));  g.drawRect(sr,1.f);
        float fs2=jmax(6.f,(float)rTop.getWidth()/130.f);
        g.setFont(Font("Courier New",fs2,Font::bold));
        g.setColour(Colour(s.col).withAlpha(0.75f));
        g.drawText(s.nm,(int)sr.getX(),(int)sr.getY(),(int)sr.getWidth(),(int)sr.getHeight(),Justification::centred,true);
    }
    g.setColour(Colour(0xFF1E1E1E)); g.drawRect(rv,2.f);

    float fs=jmax(9.f,(float)rTop.getHeight()/50.f);
    float dotR=jmax(5.f,(float)rTop.getWidth()/75.f);

    // VISTA TOP label
    g.setFont(Font("Courier New",fs*1.4f,Font::bold));
    g.setColour(Colour(C_AMBER));
    g.drawText("VISTA TOP",rTop.getX(),rTop.getY()+4,rTop.getWidth()-4,(int)fs*2,Justification::centredRight,false);

    g.setFont(Font("Courier New",fs*.8f,Font::plain));
    g.setColour(Colour(C_DIM));
    g.drawText("PALCO / FRONTE",rTop.getX(),rTop.getY()+2,rTop.getWidth(),(int)fs+2,Justification::centred,false);
    g.drawText("RETRO",rTop.getX(),rTop.getBottom()-(int)fs-4,rTop.getWidth(),(int)fs+2,Justification::centred,false);
    g.drawText("SX",rTop.getX()+2,rTop.getCentreY()-(int)fs,(int)fs*2,(int)fs*2,Justification::centred,false);
    g.drawText("DX",rTop.getRight()-(int)fs*2-2,rTop.getCentreY()-(int)fs,(int)fs*2,(int)fs*2,Justification::centred,false);

    // Listener
    {
        auto pt=topPt(0.f,0.f);
        g.setColour(Colour(C_BLUE).withAlpha(0.15f));
        g.fillEllipse(pt.x-dotR*3,pt.y-dotR*3,dotR*6,dotR*6);
        g.setColour(Colour(C_BLUE));
        g.fillEllipse(pt.x-dotR*.7f,pt.y-dotR*.7f,dotR*1.4f,dotR*1.4f);
    }

    // All sources
    for(int i=0;i<MAX_SOURCES;i++){
        auto& s=proc.sources[i];
        auto* bus=proc.getBus(true,i);
        bool busOn=bus&&bus->isEnabled();
        if(!busOn&&!s.paramActive->get()) continue; // skip fully inactive

        auto pt=topPt(s.paramX->get(),s.paramZ->get());
        Colour col=Colour(kSrcColors[i]);
        float alpha=s.paramActive->get()?1.f:0.35f;
        bool isSel=(i==selectedSrc);

        if(isSel){
            ColourGradient gw(col.withAlpha(0.3f),pt.x,pt.y,col.withAlpha(0.f),pt.x+dotR*5,pt.y,true);
            g.setGradientFill(gw); g.fillEllipse(pt.x-dotR*4,pt.y-dotR*4,dotR*8,dotR*8);
        }
        g.setColour(col.withAlpha(alpha));
        g.fillEllipse(pt.x-dotR*(isSel?1.2f:0.8f),pt.y-dotR*(isSel?1.2f:0.8f),dotR*(isSel?2.4f:1.6f),dotR*(isSel?2.4f:1.6f));

        // Bus indicator dot
        if(busOn){
            g.setColour(Colour(C_GREEN).withAlpha(0.9f));
            g.fillEllipse(pt.x+dotR*.6f,pt.y-dotR*1.4f,dotR*.6f,dotR*.6f);
        }

        // Label
        g.setColour(col.withAlpha(alpha*.9f));
        g.setFont(Font("Courier New",jmax(7.f,fs*.8f),isSel?Font::bold:Font::plain));
        g.drawText(s.name,(int)(pt.x-55),(int)(pt.y-dotR*2.5f-fs-1),110,(int)(fs+2),Justification::centred,false);
    }
}

void SpatialMixProEditor::drawFrontView(Graphics& g)
{
    auto fr=rFront; fr.removeFromRight(54);
    g.setColour(Colour(0xFF050505)); g.fillRect(rFront);
    g.setColour(Colour(C_BORDER));
    g.drawVerticalLine(rFront.getX(),(float)rFront.getY(),(float)rFront.getBottom());

    auto rv=fr.toFloat().reduced(18.f);
    g.setColour(Colour(0xFF080808)); g.fillRect(rv);

    g.setColour(Colour(0xFF0F0F0F));
    for(int i=0;i<=16;i++){float xr=rv.getX()+i*rv.getWidth()/16.f; g.drawVerticalLine((int)xr,rv.getY(),rv.getBottom());}
    for(int i=0;i<=6;i++){ float yr=rv.getBottom()-i*rv.getHeight()/6.f; g.drawHorizontalLine((int)yr,rv.getX(),rv.getRight());}
    g.setColour(Colour(0xFF1E1E1E)); g.drawRect(rv,1.5f);

    float fs=jmax(8.f,(float)fr.getWidth()/28.f);
    float dotR=jmax(4.f,(float)fr.getWidth()/60.f);

    g.setFont(Font("Courier New",fs*1.3f,Font::bold));
    g.setColour(Colour(C_AMBER));
    g.drawText("VISTA FRONTALE",fr.getX()+2,fr.getY()+4,fr.getWidth()-4,(int)fs*2,Justification::centredLeft,false);

    g.setFont(Font("Courier New",fs*.7f,Font::plain));
    g.setColour(Colour(C_DIM));
    g.drawText("SUOLO",fr.getX(),fr.getBottom()-(int)fs-2,fr.getWidth(),(int)fs+2,Justification::centredLeft,false);

    // Y axis label
    g.setFont(Font("Courier New",fs,Font::bold));
    g.setColour(Colour(C_GREEN));
    g.drawText("Y",rFront.getRight()-50,rFront.getCentreY()-10,16,20,Justification::centred,false);

    // Listener
    {
        auto pt=frontPt(0.f,0.f);
        g.setColour(Colour(C_BLUE).withAlpha(0.25f));
        g.fillEllipse(pt.x-dotR*2,pt.y-dotR*2,dotR*4,dotR*4);
        g.setColour(Colour(C_BLUE)); g.fillEllipse(pt.x-dotR*.6f,pt.y-dotR*.6f,dotR*1.2f,dotR*1.2f);
    }

    // All sources
    for(int i=0;i<MAX_SOURCES;i++){
        auto& s=proc.sources[i];
        auto* bus=proc.getBus(true,i);
        bool busOn=bus&&bus->isEnabled();
        if(!busOn&&!s.paramActive->get()) continue;
        bool isSel=(i==selectedSrc);
        float alpha=s.paramActive->get()?1.f:0.35f;
        Colour col=Colour(kSrcColors[i]);
        auto pt=frontPt(s.paramY->get(),s.paramZ->get());
        g.setColour(col.withAlpha(alpha));
        float r=dotR*(isSel?1.2f:0.75f);
        g.fillEllipse(pt.x-r,pt.y-r,r*2,r*2);
        if(isSel){g.setColour(col.withAlpha(0.5f));g.drawEllipse(pt.x-r*1.8f,pt.y-r*1.8f,r*3.6f,r*3.6f,1.f);}
    }
}

void SpatialMixProEditor::drawBottom(Graphics& g)
{
    g.setColour(Colour(0xFF090909)); g.fillRect(rBottom);
    g.setColour(Colour(C_BORDER));
    g.drawHorizontalLine(rBottom.getY(),(float)rBottom.getX(),(float)rBottom.getRight());
    float fs=jmax(9.f,(float)rBottom.getHeight()*.42f);
    g.setFont(Font("Courier New",fs,Font::plain));
    g.setColour(Colour(C_DIM));
    g.drawText("SpatialMix Pro v3.0 — MULTI-SOURCE",rBottom.getX()+8,rBottom.getY(),300,rBottom.getHeight(),Justification::centredLeft,false);
    static const char* mn[]={"HRTF BINAURAL","STEREO","2.1 SIM"};
    g.setColour(Colour(C_AMBER));
    g.drawText(String("MODE: ")+mn[proc.paramMode->getIndex()],rBottom.getX()+320,rBottom.getY(),180,rBottom.getHeight(),Justification::centredLeft,false);
    auto& s=proc.sources[selectedSrc];
    g.setColour(Colour(C_TEXT));
    g.drawText(String::formatted("SEL: %s | X:%.1f Y:%.1f Z:%.1f V:%.2fx",
               s.name.toRawUTF8(),s.paramX->get(),s.paramY->get(),s.paramZ->get(),s.paramGain->get()),
               rBottom.getRight()-450,rBottom.getY(),440,rBottom.getHeight(),Justification::centredRight,false);
}

void SpatialMixProEditor::showHelp()
{
    String m;
    m << "SPATIALMIX PRO v3 — GUIDA MULTI-SOURCE\n\n"
      << "CONCETTO CHIAVE — ROUTING IN FL STUDIO\n"
      << "  Questo plugin ha 8 ingressi stereo separati.\n"
      << "  Invece di usarlo su ogni singola traccia,\n"
      << "  lo metti UNA SOLA VOLTA su un bus master:\n\n"
      << "  1. Crea un bus nel mixer di FL Studio\n"
      << "  2. Inserisci SpatialMix Pro come effetto sul bus\n"
      << "  3. Nelle impostazioni del bus, abilita il routing\n"
      << "     multi-canale (fino a 8 coppie stereo)\n"
      << "  4. Manda la traccia 'Violino' al canale 1-2 del bus\n"
      << "  5. Manda la traccia 'Tuba' al canale 3-4 del bus\n"
      << "  6. etc. per ogni sorgente\n"
      << "  Il plugin vede ogni sorgente separata e la spazializza!\n\n"
      << "  I pallini verdi nelle viste indicano i bus attivi.\n\n"
      << "LISTA SORGENTI (sinistra)\n"
      << "  Click = seleziona la sorgente\n"
      << "  Doppio click sul nome = rinomina\n"
      << "  ON  = attiva/disattiva la sorgente\n"
      << "  M   = mono\n"
      << "  Phase = inversione di fase 180deg\n\n"
      << "VISTE\n"
      << "  VISTA TOP: trascina il punto colorato = muovi X/Z\n"
      << "  VISTA FRONTALE: trascina = muovi Y/Z\n"
      << "  Slider Y (destra) = altezza della sorgente selezionata\n\n"
      << "PARAMETRI\n"
      << "  X = sinistra/destra (-8..+8m)\n"
      << "  Z = fronte/retro (-8..+8m)\n"
      << "  V = volume sorgente (0..2x)\n"
      << "  Y = altezza (0..6m)\n"
      << "  MASTER = volume globale in uscita\n\n"
      << "OUTPUT\n"
      << "  HRTF = 3D binaurale in cuffia\n"
      << "  STEREO = downmix stereo\n"
      << "  2.1 = stereo + rinforzo sub 120Hz";
    AlertWindow::showMessageBoxAsync(MessageBoxIconType::InfoIcon,"SpatialMix Pro — Guida",m,"OK");
}

// ============================================================
// MOUSE
// ============================================================
void SpatialMixProEditor::mouseDown(const MouseEvent& e)
{
    auto pt=e.position;
    dragSrc=-1; dragView=DragNone;

    // Source list click to select
    if (rSourceList.toFloat().contains(pt))
    {
        auto lb=rSourceList.reduced(4,4);
        int rowH=jmax(20,lb.getHeight()/MAX_SOURCES);
        int idx=(int)((pt.y-lb.getY())/rowH);
        if(idx>=0&&idx<MAX_SOURCES){ selectSource(idx); return; }
    }

    if (rTop.toFloat().contains(pt))
    {
        // Find closest source
        float best=30.f; int bestIdx=-1;
        for(int i=0;i<MAX_SOURCES;i++){
            auto dp=topPt(proc.sources[i].paramX->get(),proc.sources[i].paramZ->get());
            float d=pt.getDistanceFrom(dp);
            if(d<best){best=d;bestIdx=i;}
        }
        if(bestIdx>=0){dragSrc=bestIdx;dragView=DragTop;selectSource(bestIdx);}
        else{dragSrc=selectedSrc;dragView=DragTop;}
        float x,z; topToXZ(pt,x,z);
        proc.sources[dragSrc].paramX->setValueNotifyingHost(proc.sources[dragSrc].paramX->convertTo0to1(x));
        proc.sources[dragSrc].paramZ->setValueNotifyingHost(proc.sources[dragSrc].paramZ->convertTo0to1(z));
    }
    else if (rFront.toFloat().contains(pt))
    {
        dragSrc=selectedSrc; dragView=DragFront;
        float y,z; frontToYZ(pt,y,z);
        proc.sources[dragSrc].paramY->setValueNotifyingHost(proc.sources[dragSrc].paramY->convertTo0to1(y));
        proc.sources[dragSrc].paramZ->setValueNotifyingHost(proc.sources[dragSrc].paramZ->convertTo0to1(z));
    }
}

void SpatialMixProEditor::mouseDrag(const MouseEvent& e)
{
    if(dragSrc<0) return;
    auto pt=e.position;
    auto& s=proc.sources[dragSrc];
    if(dragView==DragTop){
        float x,z; topToXZ(pt,x,z);
        s.paramX->setValueNotifyingHost(s.paramX->convertTo0to1(x));
        s.paramZ->setValueNotifyingHost(s.paramZ->convertTo0to1(z));
    } else if(dragView==DragFront){
        float y,z; frontToYZ(pt,y,z);
        s.paramY->setValueNotifyingHost(s.paramY->convertTo0to1(y));
        s.paramZ->setValueNotifyingHost(s.paramZ->convertTo0to1(z));
    }
}

void SpatialMixProEditor::mouseUp(const MouseEvent&)
{ dragSrc=-1; dragView=DragNone; }
