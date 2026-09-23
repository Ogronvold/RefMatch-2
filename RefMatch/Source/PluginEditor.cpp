#include "PluginEditor.h"
namespace {
const juce::Colour black(0xff070b11), panel(0xff101722), panelRaised(0xff151d29), line(0xff283344),
                   text(0xfff2f4f8), muted(0xff7f8999), cyan(0xffffad45), violet(0xffa85df5);

void label(juce::Label& l,float size,juce::Colour colour=muted) {
    l.setFont(juce::Font(juce::FontOptions(size)));l.setColour(juce::Label::textColourId,colour);
}

void glowRounded(juce::Graphics& g, juce::Rectangle<float> r, float radius, juce::Colour colour, float strength=.16f)
{
    // Cheap bloom made from layered translucent shapes. It stays lightweight in
    // JUCE while giving selected cards/controls a soft neon halo.
    for(int i=5;i>=1;--i) {
        const float spread=float(i)*2.2f;
        g.setColour(colour.withAlpha(strength*(6.f-float(i))/32.f));
        g.fillRoundedRectangle(r.expanded(spread),radius+spread);
    }
}

void panelCard(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour accent, bool selected=false)
{
    if(selected) glowRounded(g,r,12.f,accent,.22f);
    g.setGradientFill(juce::ColourGradient(panelRaised.withAlpha(.96f),r.getTopLeft(),panel.darker(.18f),r.getBottomRight(),false));
    g.fillRoundedRectangle(r,12.f);
    g.setColour(juce::Colours::white.withAlpha(.025f));
    g.drawRoundedRectangle(r.reduced(.75f),11.3f,1.f);
    g.setColour(accent.withAlpha(selected?.82f:.30f));
    g.drawRoundedRectangle(r,12.f,1.1f);
}
}
RefMatchLookAndFeel::RefMatchLookAndFeel()
{
    setColour(juce::TextButton::buttonColourId,panel);
    setColour(juce::TextButton::textColourOffId,text);
    setColour(juce::TextButton::textColourOnId,black);
    setColour(juce::TextButton::buttonOnColourId,cyan);
    setColour(juce::Slider::thumbColourId,cyan);
    setColour(juce::Slider::trackColourId,cyan.withAlpha(.5f));
    setColour(juce::Slider::backgroundColourId,line);
    setColour(juce::Slider::textBoxTextColourId,text);
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId,panel);
    setColour(juce::ComboBox::textColourId,text);
    setColour(juce::ComboBox::outlineColourId,line);
    setColour(juce::ComboBox::arrowColourId,violet);
    setColour(juce::PopupMenu::backgroundColourId,panel);
    setColour(juce::PopupMenu::textColourId,text);
    setColour(juce::ToggleButton::textColourId,text);
    setColour(juce::ToggleButton::tickColourId,cyan);
    setColour(juce::TextEditor::backgroundColourId,panel);
    setColour(juce::TextEditor::textColourId,text);
    setColour(juce::TextEditor::outlineColourId,line);
    setColour(juce::TextEditor::focusedOutlineColourId,cyan);
}
void RefMatchLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& button,const juce::Colour& colour,bool over,bool down)
{
    auto r=button.getLocalBounds().toFloat().reduced(.75f);
    const bool on=button.getToggleState();
    const bool activeWhenOff=bool(button.getProperties()["activeWhenOff"]);
    const bool visualOn=activeWhenOff?!on:on;
    const bool compact=bool(button.getProperties()["compactType"]);
    const bool dual=bool(button.getProperties()["dualAccent"]);
    const bool glow=bool(button.getProperties()["glow"]);
    const bool roundSwitch=bool(button.getProperties()["roundSwitch"]);

    if(roundSwitch) {
        auto circle=r.withSizeKeepingCentre(std::min(r.getWidth(),r.getHeight()),std::min(r.getWidth(),r.getHeight()));
        glowRounded(g,circle, circle.getWidth()*.5f, violet, over?.18f:.11f);
        g.setGradientFill(juce::ColourGradient(panelRaised.brighter(over?.10f:.04f),circle.getTopLeft(),black.brighter(.06f),circle.getBottomRight(),false));
        g.fillEllipse(circle);
        g.setColour(juce::Colours::white.withAlpha(.035f));g.drawEllipse(circle.reduced(1.f),1.f);
        g.setColour(line.brighter(.12f).withAlpha(.95f));g.drawEllipse(circle,1.1f);
        const auto icon=text.withAlpha(down?.65f:over?1.f:.92f);
        g.setColour(icon);
        const float cx=circle.getCentreX(), cy=circle.getCentreY();
        juce::Path top,bottom;
        top.startNewSubPath(cx-10.f,cy-5.f);top.lineTo(cx+8.f,cy-5.f);top.lineTo(cx+4.f,cy-9.f);
        bottom.startNewSubPath(cx+10.f,cy+5.f);bottom.lineTo(cx-8.f,cy+5.f);bottom.lineTo(cx-4.f,cy+9.f);
        g.strokePath(top,juce::PathStrokeType(1.8f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        g.strokePath(bottom,juce::PathStrokeType(1.8f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        return;
    }

    if((glow && visualOn) || visualOn) glowRounded(g,r,compact?6.f:8.f,dual?violet:colour,compact?.20f:.17f);

    if(compact) {
        const auto fill=panelRaised.withMultipliedBrightness(down?.88f:over?1.10f:1.f);
        g.setColour(fill);g.fillRoundedRectangle(r,6.f);
        g.setColour((visualOn?colour:line).withAlpha(visualOn?.95f:.62f));g.drawRoundedRectangle(r,6.f,visualOn?1.35f:1.f);
        return;
    }

    if((visualOn || bool(button.getProperties()["forceDual"])) && dual)
        g.setGradientFill(juce::ColourGradient(cyan,r.getTopLeft(),violet,r.getTopRight(),false));
    else if(visualOn)
        g.setGradientFill(juce::ColourGradient(colour.brighter(.08f),r.getTopLeft(),colour.darker(.08f),r.getBottomRight(),false));
    else {
        const auto fill=panelRaised.withMultipliedBrightness(down?.88f:over?1.10f:1.f);
        g.setGradientFill(juce::ColourGradient(fill.brighter(.04f),r.getTopLeft(),fill.darker(.08f),r.getBottomRight(),false));
    }
    g.fillRoundedRectangle(r,8.f);
    g.setColour((visualOn?colour:line).withAlpha(visualOn?.90f:.72f));g.drawRoundedRectangle(r,8.f,visualOn?1.2f:1.f);
    g.setColour(juce::Colours::white.withAlpha(over?.055f:.025f));g.drawRoundedRectangle(r.reduced(1.f),7.f,.8f);
}
void RefMatchLookAndFeel::drawButtonText(juce::Graphics& g,juce::TextButton& button,bool over,bool down)
{
    if(bool(button.getProperties()["trashIcon"])) {
        const auto r=button.getLocalBounds().toFloat();
        const auto c=text.withAlpha(down?.68f:over?1.f:.88f);
        const float x=r.getX()+15.f,cy=r.getCentreY();
        g.setColour(c);
        g.drawRoundedRectangle(x-5.f,cy-5.f,10.f,12.f,1.5f,1.2f);
        g.drawLine(x-7.f,cy-7.f,x+7.f,cy-7.f,1.2f);
        g.drawLine(x-3.f,cy-9.f,x+3.f,cy-9.f,1.2f);
        g.setFont(juce::Font(juce::FontOptions(11.f,juce::Font::bold)));
        g.drawText(button.getButtonText(),juce::Rectangle<float>(x+12.f,r.getY(),r.getRight()-(x+15.f),r.getHeight()),juce::Justification::centredLeft);
        return;
    }
    if(bool(button.getProperties()["recordIcon"])) {
        const auto r=button.getLocalBounds().toFloat();
        const bool on=button.getToggleState();
        const auto accent=button.findColour(juce::TextButton::buttonOnColourId);
        const float iconX=r.getX()+18.f, cy=r.getCentreY();
        if(on) glowRounded(g,{iconX-8.f,cy-8.f,16.f,16.f},8.f,accent,.24f);
        g.setColour(accent.withAlpha(on?1.f:.82f));
        g.drawEllipse(iconX-6.f,cy-6.f,12.f,12.f,1.4f);
        g.fillEllipse(iconX-2.5f,cy-2.5f,5.f,5.f);
        g.setFont(juce::Font(juce::FontOptions(12.f,juce::Font::bold)));
        g.setColour(text.withAlpha(down?.70f:over?1.f:.94f));
        g.drawText(button.getButtonText(),juce::Rectangle<float>(iconX+13.f,r.getY(),r.getRight()-(iconX+18.f),r.getHeight()),juce::Justification::centredLeft);
        return;
    }
    juce::LookAndFeel_V4::drawButtonText(g,button,over,down);
}

void RefMatchLookAndFeel::drawLinearSlider(juce::Graphics& g,int x,int y,int width,int height,float position,float,float,juce::Slider::SliderStyle,juce::Slider& slider)
{
    const float mid=y+height*.5f;
    g.setColour(line.darker(.08f));g.fillRoundedRectangle(float(x),mid-2,float(width),4,2);
    const auto accent=slider.findColour(juce::Slider::trackColourId);
    g.setColour(juce::Colours::white.withAlpha(.025f));g.fillRoundedRectangle(float(x),mid-2,float(width),1.2f,1.f);
    g.setGradientFill(juce::ColourGradient(bool(slider.getProperties()["dualAccent"])?cyan:accent,float(x),mid,bool(slider.getProperties()["dualAccent"])?violet:accent,float(x+width),mid,false));
    g.fillRoundedRectangle(float(x),mid-2,std::max(0.f,position-x),4,2);
    g.setColour(accent.withAlpha(.12f));g.fillEllipse(position-11,mid-11,22,22);
    g.setColour(accent.withAlpha(.18f));g.fillEllipse(position-8,mid-8,16,16);
    g.setColour(text.withAlpha(.95f));g.fillEllipse(position-5.2f,mid-5.2f,10.4f,10.4f);
}

void RefMatchLookAndFeel::drawToggleButton(juce::Graphics& g,juce::ToggleButton& button,bool over,bool down)
{
    if(bool(button.getProperties()["pillToggle"])) {
        auto r=button.getLocalBounds().toFloat();
        const float h=18.f,w=36.f;
        auto pill=juce::Rectangle<float>(r.getX()+2.f,r.getCentreY()-h*.5f,w,h);
        const bool on=button.getToggleState();
        const auto accent=button.findColour(juce::ToggleButton::tickColourId);
        if(on) glowRounded(g,pill,9.f,accent,.18f);
        g.setColour(on?accent.withAlpha(.28f):line.withAlpha(.8f));g.fillRoundedRectangle(pill,9.f);
        g.setColour(on?accent.withAlpha(.80f):muted.withAlpha(.45f));g.drawRoundedRectangle(pill,9.f,1.f);
        const float cx=on?pill.getRight()-9.f:pill.getX()+9.f;
        g.setColour(on?text:muted);g.fillEllipse(cx-5.5f,pill.getCentreY()-5.5f,11.f,11.f);
        g.setFont(juce::Font(juce::FontOptions(11.f,juce::Font::bold)));g.setColour(text.withAlpha(down?.7f:over?1.f:.92f));
        g.drawText(button.getButtonText(),juce::Rectangle<float>(pill.getRight()+8.f,r.getY(),std::max(0.f,r.getWidth()-pill.getWidth()-10.f),r.getHeight()),juce::Justification::centredLeft);
        return;
    }
    juce::LookAndFeel_V4::drawToggleButton(g,button,over,down);
}
RefMatchAudioProcessorEditor::RefMatchAudioProcessorEditor(RefMatchAudioProcessor& p):AudioProcessorEditor(&p),processor(p)
{
    setLookAndFeel(&look);setResizable(false,false);
    for(juce::Component* c:std::initializer_list<juce::Component*>{&a,&b,&switchButton,&eqTab,&loopTab,&play,&toneButton,&toneReset,&toneOn,&graphRange,&quickLoop,&matchState,&lowType,&highType,&midQ,
        &recordMix,&recordRef,&match,&reset,&eqOn,&gain,&amount,&smooth,&back,&forward,&timeline,&inTime,&outTime,&setIn,&setOut,&clearLoop,&zoomMinus,&zoomPlus,&loopZoom,
        &status,&mixProfile,&refProfile,&position})addAndMakeVisible(c);
    a.onClick=[this]{processor.selectSource(false);};b.onClick=[this]{processor.selectSource(true);};
    switchButton.onClick=[this]{processor.switchWithSystemMedia();};
    quickLoop.onClick=[this]{
        if(quickLoop.getToggleState()) {
            if(processor.getLoop().getOut()>processor.getLoop().getIn()) processor.getLoop().enable(true);
            else {quickLoop.setToggleState(false,juce::dontSendNotification);message="Set a loop range once in LOOP first";}
        } else processor.getLoop().enable(false);
    };
    eqTab.onClick=[this]{setPage(1);};loopTab.onClick=[this]{setPage(2);};
    play.onClick=[this]{
        if(!processor.isReferenceSelected())processor.selectSource(true);
        else transport(processor.getMediaController().playbackState()==1?SystemMediaController::Command::pause:SystemMediaController::Command::play);
        timerCallback();
    };
    back.onClick=[this]{processor.getLoop().skip(-5);};forward.onClick=[this]{processor.getLoop().skip(5);};
    back.setTooltip("Back 5 seconds");forward.setTooltip("Forward 5 seconds");
    timeline.onRange=[this](double start,double end){
        inTime.setText(rangeText(start));outTime.setText(rangeText(end));
        if(processor.getLoop().setRange(start,end))processor.getLoop().enable(true);
    };
    timeline.onSeek=[this](double seconds){processor.getLoop().seek(seconds);};
    toneButton.onClick=[this]{showTone=true;};
    toneReset.onClick=[this]{for(int i=0;i<3;++i) {
        const auto id="tone"+juce::String(i)+"gain";
        auto* p=processor.apvts.getParameter(id);p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(0));p->endChangeGesture();
    }};
    recordMix.onClick=[this]{processor.recordProfile(LearnCapture::mix);};
    recordRef.onClick=[this]{processor.recordProfile(LearnCapture::reference);};
    match.onClick=[this]{processor.learnMatch();matchFlashUntil=juce::Time::getMillisecondCounterHiRes()+850.0;message="Match applied";match.setButtonText("MATCHED");repaint();};reset.onClick=[this]{processor.clearMatch();};
    for(auto* slider:{&gain,&amount,&smooth,&midQ}) {slider->setSliderStyle(juce::Slider::LinearHorizontal);slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,66,24);}
    gain.setTextValueSuffix(" dB");amount.setTextValueSuffix(" %");smooth.setTextValueSuffix(" %");midQ.setTextValueSuffix(" Q");
    gainAttach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"sourcegain",gain);
    amountAttach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"matchamount",amount);
    smoothAttach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"smooth",smooth);
    midQAttach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"tone1q",midQ);
    smooth.setTooltip("Fine to broad correction. Recalculates from captured profiles without recording again.");
    amount.getProperties().set("dualAccent",true);smooth.setColour(juce::Slider::trackColourId,violet);
    for(int i=0;i<6;++i) {
        addAndMakeVisible(tone[i]);tone[i].setSliderStyle(juce::Slider::LinearHorizontal);
        tone[i].setTextBoxStyle(juce::Slider::TextBoxRight,false,70,22);
        tone[i].setTextValueSuffix(i%2?" Hz":" dB");tone[i].setColour(juce::Slider::trackColourId,violet);
        toneAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"tone"+juce::String(i/2)+(i%2?"freq":"gain"),tone[i]);
    }
    for(auto* button:{&b,&play,&recordRef})button->setColour(juce::TextButton::buttonOnColourId,violet);
    for(auto* button:{&play,&recordMix,&recordRef,&match,&reset})button->getProperties().set("glow",true);
    match.getProperties().set("dualAccent",true);match.getProperties().set("forceDual",true);
    recordMix.getProperties().set("recordIcon",true);recordRef.getProperties().set("recordIcon",true);
    switchButton.getProperties().set("roundSwitch",true);
    switchButton.setButtonText("");
    switchButton.setTooltip("Switch between your mix and the system reference.");
    lowShelfAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"tone0shelf",lowType);
    highShelfAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"tone2shelf",highType);
    for(auto* typeButton:{&lowType,&highType}) {
        typeButton->setClickingTogglesState(true);
        typeButton->getProperties().set("compactType",true);
        typeButton->setColour(juce::TextButton::textColourOffId,text);
        typeButton->setColour(juce::TextButton::textColourOnId,text);
    }
    lowType.setColour(juce::TextButton::buttonColourId,cyan);
    lowType.setColour(juce::TextButton::buttonOnColourId,cyan);
    highType.setColour(juce::TextButton::buttonColourId,violet);
    highType.setColour(juce::TextButton::buttonOnColourId,violet);
    matchState.setClickingTogglesState(true);
    matchState.getProperties().set("compactType",true);
    matchState.getProperties().set("glow",true);
    matchState.getProperties().set("activeWhenOff",true);
    matchState.setColour(juce::TextButton::buttonColourId,cyan);
    matchState.setColour(juce::TextButton::buttonOnColourId,violet);
    matchState.onClick=[this]{
        if(auto* parameter=processor.apvts.getParameter("processingafter")) {
            const bool bypass=matchState.getToggleState();
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(bypass?0.0f:1.0f);
            parameter->endChangeGesture();
        }
    };
    matchState.setTooltip("MATCH ON = Match EQ + Tone EQ active. BYPASSED = those stages are bypassed while A gain remains active.");
    quickLoop.getProperties().set("pillToggle",true);eqOn.getProperties().set("pillToggle",true);toneOn.getProperties().set("pillToggle",true);
    quickLoop.setColour(juce::ToggleButton::tickColourId,violet);eqOn.setColour(juce::ToggleButton::tickColourId,violet);toneOn.setColour(juce::ToggleButton::tickColourId,violet);
    quickLoop.setTooltip("Toggle the most recently defined loop without opening the LOOP page.");
    eqTab.getProperties().set("dualAccent",true);loopTab.getProperties().set("dualAccent",true);
    eqTab.getProperties().set("glow",true);loopTab.getProperties().set("glow",true);toneButton.getProperties().set("glow",true);
    toneOnAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"toneenabled",toneOn);
    toneOn.setTooltip("Bypass only the three Tone bands. Keeps their settings and leaves Match EQ active.");
    graphRange.addItem("+/- 12 dB",12);graphRange.addItem("+/- 24 dB",24);graphRange.addItem("+/- 48 dB",48);graphRange.addItem("+/- 96 dB",96);
    const int savedRange=int(p.apvts.state.getProperty("graphRange",24));
    graphScale=float(savedRange==12 || savedRange==48 || savedRange==96?savedRange:24);
    graphRange.setSelectedId(int(graphScale),juce::dontSendNotification);
    graphRange.setTooltip("Fixed graph range. Changes only when you choose a different range here.");
    graphRange.onChange=[this]{graphScale=float(graphRange.getSelectedId());processor.apvts.state.setProperty("graphRange",int(graphScale),nullptr);repaint();};
    eqAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"matchenabled",eqOn);
    inTime.setText(rangeText(p.getLoop().getIn()));outTime.setText(rangeText(p.getLoop().getOut()));
    inTime.onReturnKey=[this]{updateLoopRange();};outTime.onReturnKey=inTime.onReturnKey;
    inTime.onFocusLost=inTime.onReturnKey;outTime.onFocusLost=inTime.onReturnKey;
    setIn.onClick=[this]{const auto v=processor.getLoop().getPosition();if(v.valid){inTime.setText(rangeText(v.seconds));updateLoopRange();}};
    setOut.onClick=[this]{const auto v=processor.getLoop().getPosition();if(v.valid){outTime.setText(rangeText(v.seconds));updateLoopRange();}};
    clearLoop.onClick=[this]{processor.getLoop().clear();quickLoop.setToggleState(false,juce::dontSendNotification);message="Loop cleared";};
    clearLoop.setTooltip("Clear the current loop region and turn looping off.");
    clearLoop.getProperties().set("trashIcon",true);
    zoomMinus.onClick=[this]{loopZoom.setValue(std::max(1.0,loopZoom.getValue()-.5));};
    zoomPlus.onClick=[this]{loopZoom.setValue(std::min(6.0,loopZoom.getValue()+.5));};
    loopZoom.setRange(1.0,6.0,.1);loopZoom.setValue(1.0);loopZoom.setSliderStyle(juce::Slider::LinearHorizontal);loopZoom.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    loopZoom.onValueChange=[this]{timeline.setZoom(loopZoom.getValue());};
    label(status,11);label(mixProfile,11,cyan);label(refProfile,11,violet);label(position,13,text);
    a.setTooltip("Listen to your mix. Pauses the active media player.");b.setTooltip("Listen to reference. Mutes MIX and sends system PLAY.");
    recordMix.setTooltip("Record the incoming MIX spectrum before EQ. Click again to finish.");
    recordRef.setTooltip("Record system-reference spectrum. Requires capture permission and host audio processing. Click again to finish.");
    match.setTooltip("Calculate EQ from the two captured profiles and enable it on MIX.");
    setPage(1);processor.startReferenceCapture();startTimerHz(15);
}
RefMatchAudioProcessorEditor::~RefMatchAudioProcessorEditor(){stopTimer();setLookAndFeel(nullptr);}
void RefMatchAudioProcessorEditor::setPage(int value)
{
    page=value;
    const bool main=page==1;
    for(auto* c:std::initializer_list<juce::Component*>{&recordMix,&recordRef,&match,&reset,&eqOn,&amount,&smooth,&toneButton,&toneOn,&graphRange,&mixProfile,&refProfile,&toneReset,&lowType,&highType,&midQ})c->setVisible(main);
    for(auto& control:tone)control.setVisible(main);
    for(auto* c:std::initializer_list<juce::Component*>{&timeline,&position,&clearLoop,&zoomMinus,&zoomPlus,&loopZoom})c->setVisible(page==2);
    for(auto* c:std::initializer_list<juce::Component*>{&inTime,&outTime,&setIn,&setOut})c->setVisible(false);
    // MATCH EQ acts as a back button only on the dedicated loop page.
    eqTab.setVisible(page==2);
    loopTab.setVisible(true);
    quickLoop.setVisible(true);
    setSize(960,page==1?650:560);
    resized();repaint();
}
void RefMatchAudioProcessorEditor::transport(SystemMediaController::Command c)
{
    if(processor.isTransportPending()||processor.getMediaController().isBusy())return;
    juce::Component::SafePointer<RefMatchAudioProcessorEditor> safe(this);
    processor.getMediaController().request(c,[safe](bool ok,SystemMediaInfo,juce::String error){if(safe && !ok)safe->message=error;});
}
juce::String RefMatchAudioProcessorEditor::timeText(double seconds)
{
    const int s=std::max(0,int(seconds));return juce::String(s/60)+":"+juce::String(s%60).paddedLeft('0',2);
}
juce::String RefMatchAudioProcessorEditor::rangeText(double seconds)
{
    const int millis=int(std::round(std::max(0.,seconds)*1000.));
    return juce::String(millis/60000)+":"+juce::String((millis%60000)/1000).paddedLeft('0',2)+"."+juce::String(millis%1000).paddedLeft('0',3);
}
double RefMatchAudioProcessorEditor::parseTime(const juce::String& input)
{
    const auto t=input.trim();
    if(t.isEmpty() || !t.containsOnly("0123456789:."))return -1;
    const auto pieces=juce::StringArray::fromTokens(t,":","");
    if(pieces.size()<1||pieces.size()>3)return -1;
    double result=0;
    for(int i=0;i<pieces.size();++i) {
        const auto part=pieces[i];
        if(part.isEmpty() || part=="." || part.retainCharacters(".").length()>1
            || (i<pieces.size()-1 && part.containsChar('.')))return -1;
        const double v=part.getDoubleValue();
        if(v<0||(i>0&&v>=60))return -1;
        result=result*60+v;
    }return result;
}

void RefMatchAudioProcessorEditor::updateLoopRange()
{
    processor.getLoop().setRange(parseTime(inTime.getText()),parseTime(outTime.getText()));
}
void RefMatchAudioProcessorEditor::timerCallback()
{
    const bool ref=processor.isReferenceSelected();a.setToggleState(!ref,juce::dontSendNotification);b.setToggleState(ref,juce::dontSendNotification);
    eqTab.setToggleState(page==1,juce::dontSendNotification);loopTab.setToggleState(page==2,juce::dontSendNotification);
    switchButton.setButtonText("");
    switchButton.getProperties().set("transportPending",processor.isTransportPending());
    const auto m=processor.profile(LearnCapture::mix),r=processor.profile(LearnCapture::reference);
    recordMix.setButtonText(processor.recording()==LearnCapture::mix?"STOP MIX":"RECORD MIX");
    recordMix.setToggleState(processor.recording()==LearnCapture::mix,juce::dontSendNotification);
    recordRef.setToggleState(processor.recording()==LearnCapture::reference,juce::dontSendNotification);
    recordRef.setButtonText(processor.recording()==LearnCapture::reference?"STOP REF":"RECORD REF");
    mixProfile.setText(processor.recording()==LearnCapture::mix?"RECORDING  "+juce::String(m.seconds,1)+" s":m.ready?"CAPTURED  "+juce::String(m.seconds,1)+" s":"MIX",juce::dontSendNotification);
    refProfile.setText(processor.recording()==LearnCapture::reference?"RECORDING  "+juce::String(r.seconds,1)+" s":r.ready?"CAPTURED  "+juce::String(r.seconds,1)+" s":"REF",juce::dontSendNotification);
    match.setEnabled(m.ready&&r.ready);eqOn.setEnabled(processor.hasMatch());
    if(juce::Time::getMillisecondCounterHiRes()>=matchFlashUntil && match.getButtonText()!="MATCH")match.setButtonText("MATCH");
    juce::String info=page==1?processor.getLearningStatus():page==2?processor.getLoop().getStatus():"A = your mix   /   B = system reference";
    if(page==1 && processor.recording()==LearnCapture::reference && !processor.hasReferenceAudio())info=processor.getReferenceCaptureStatus()+" - waiting for audio";
    if(processor.getTransportError().isNotEmpty())info=processor.getTransportError();
    if(message.isNotEmpty())info=message;
    status.setText(info,juce::dontSendNotification);status.setTooltip(info);
    quickLoop.setToggleState(processor.getLoop().isEnabled(),juce::dontSendNotification);
    quickLoop.setButtonText(processor.getLoop().isEnabled()?"ON":"OFF");
    const bool processingAfter=processor.apvts.getRawParameterValue("processingafter")->load()>.5f;
    matchState.setToggleState(!processingAfter,juce::dontSendNotification);
    matchState.setButtonText(processingAfter?"MATCH ACTIVE":"BYPASSED");
    lowType.setButtonText(lowType.getToggleState()?"SHELF":"BELL");highType.setButtonText(highType.getToggleState()?"SHELF":"BELL");
    const auto p=processor.getLoop().getPosition();position.setText(p.valid?timeText(p.seconds)+"  /  "+timeText(p.duration):"Position unavailable",juce::dontSendNotification);
    const auto playback=processor.getMediaController().playbackState();
    play.setToggleState(playback==1,juce::dontSendNotification);
    play.setButtonText(processor.isReferenceSelected() && playback==1?"PAUSE":"PLAY");
    play.setTooltip(processor.getMediaController().playbackPending()?"Command sent - waiting for player status":playback<0?"Player status unavailable; click to play":"Current player playback state");
    play.setEnabled(!processor.isTransportPending() && !processor.getMediaController().isBusy());
    back.setEnabled(p.valid);forward.setEnabled(p.valid);
    timeline.update(p.seconds,p.duration,processor.getLoop().getIn(),processor.getLoop().getOut(),processor.getLoop().isEnabled(),p.valid,p.track);
    setIn.setEnabled(p.valid);setOut.setEnabled(p.valid);
    repaint();
}
void RefMatchAudioProcessorEditor::drawSpectrum(juce::Graphics& g,juce::Rectangle<float> r,bool eq)
{
    if(eq) glowRounded(g,r,9.f,violet,.07f);
    g.setGradientFill(juce::ColourGradient(panelRaised.withAlpha(.94f),r.getTopLeft(),panel.darker(.15f),r.getBottomRight(),false));
    g.fillRoundedRectangle(r,9.f);
    g.setColour(juce::Colours::white.withAlpha(.025f));g.drawRoundedRectangle(r.reduced(.7f),8.4f,1.f);
    auto plot=r.reduced(12,20);
    // Denser, studio-style logarithmic grid. Major divisions are brighter, while
    // intermediate frequency guides add depth without competing with the curve.
    g.setColour(line.withAlpha(.42f));
    for(int i=0;i<=8;++i){const float y=plot.getY()+plot.getHeight()*i/8.f;g.drawHorizontalLine(int(y),plot.getX(),plot.getRight());}
    const auto graphX=[&](double hz){return plot.getX()+float(std::log(hz/20.)/std::log(1000.))*plot.getWidth();};
    for(double hz:{30.,40.,50.,70.,100.,200.,300.,500.,700.,1000.,2000.,3000.,5000.,7000.,10000.,16000.,20000.}) {
        const bool major=hz==100. || hz==1000. || hz==10000.;
        g.setColour(line.withAlpha(major?.72f:.26f));
        g.drawVerticalLine(int(graphX(hz)),plot.getY(),plot.getBottom());
    }
    g.setColour(juce::Colours::white.withAlpha(.08f));
    g.drawHorizontalLine(int(plot.getCentreY()),plot.getX(),plot.getRight());
    if(eq) {
        // Soft live metric behind the EQ response: before processing vs after processing.
        for(int side=0;side<2;++side) {
            const auto values=side?processor.getAfterEffectSpectrum():processor.getBeforeEffectSpectrum();
            juce::Path spectrum;
            for(int i=0;i<180;++i) {
                const double hz=20*std::pow(1000.,i/179.);
                const int index=std::clamp(int(hz*SpectrumAnalyser::fftSize/processor.getSampleRateForDisplay()),1,SpectrumAnalyser::bins-1);
                const float x=plot.getX()+i/179.f*plot.getWidth();
                const float norm=std::clamp((values[index]+90.f)/80.f,0.f,1.f);
                const float y=plot.getBottom()-norm*plot.getHeight();
                if(i==0)spectrum.startNewSubPath(x,y);else spectrum.lineTo(x,y);
            }
            const auto specColour=side?violet:cyan;
            auto specFill=spectrum;specFill.lineTo(plot.getRight(),plot.getBottom());specFill.lineTo(plot.getX(),plot.getBottom());specFill.closeSubPath();
            g.setColour(specColour.withAlpha(side?.028f:.022f));g.fillPath(specFill);
            g.setColour(specColour.withAlpha(side?.25f:.16f));
            g.strokePath(spectrum,juce::PathStrokeType(side?1.1f:.9f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        }
        const auto curve=processor.getMatchCurveDb();
        const auto fullCurve=processor.getMatchCurveDbAtAmount(1.0f);
        const float scale=graphScale;bool outside=false;for(auto db:curve)outside=outside || std::abs(db)>scale;
        auto makeCurvePath=[&](const std::vector<float>& values){juce::Path path;for(size_t i=0;i<values.size();++i){const float x=plot.getX()+float(i)/float(values.size()-1)*plot.getWidth();const float y=plot.getCentreY()-std::clamp(values[i],-scale,scale)/(2*scale)*plot.getHeight();if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}return path;};
        // Dim 100% target underneath; bright white is the correction currently
        // being applied, so moving Amount visibly morphs toward/away from target.
        auto targetPath=makeCurvePath(fullCurve);
        // A translucent target-area fill makes the graph feel less flat while the
        // actual applied response remains the visual focus.
        if(!fullCurve.empty()) {
            auto targetFill=targetPath;
            targetFill.lineTo(plot.getRight(),plot.getCentreY());
            targetFill.lineTo(plot.getX(),plot.getCentreY());
            targetFill.closeSubPath();
            g.setGradientFill(juce::ColourGradient(violet.withAlpha(.08f),plot.getTopLeft(),violet.withAlpha(.015f),plot.getBottomLeft(),false));
            g.fillPath(targetFill);
        }
        g.setColour(violet.withAlpha(.11f));g.strokePath(targetPath,juce::PathStrokeType(5.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        g.setColour(violet.withAlpha(.34f));g.strokePath(targetPath,juce::PathStrokeType(1.25f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));

        // Draw the actually-applied response as a magnitude-sensitive colour line.
        // Near 0 dB it stays almost white; stronger corrections move through mint
        // into cyan/blue so it is immediately obvious where Match EQ is working.
        if(curve.size()>1) {
            const auto nearWhite=eqOn.getToggleState()?text:muted;
            const juce::Colour mint(0xff73e6b1), blue(0xff55a7ff);
            for(size_t i=1;i<curve.size();++i) {
                const float x1=plot.getX()+float(i-1)/float(curve.size()-1)*plot.getWidth();
                const float x2=plot.getX()+float(i)/float(curve.size()-1)*plot.getWidth();
                const float y1=plot.getCentreY()-std::clamp(curve[i-1],-scale,scale)/(2*scale)*plot.getHeight();
                const float y2=plot.getCentreY()-std::clamp(curve[i],-scale,scale)/(2*scale)*plot.getHeight();
                const float magnitude=.5f*(std::abs(curve[i-1])+std::abs(curve[i]));
                const float strength=std::clamp(magnitude/4.f,0.f,1.f);
                juce::Colour colour=nearWhite.interpolatedWith(mint,std::min(1.f,strength*1.6f));
                if(strength>.35f) colour=colour.interpolatedWith(blue,std::clamp((strength-.35f)/.65f,0.f,1.f));
                if(!eqOn.getToggleState()) colour=colour.withMultipliedAlpha(.55f);
                // soft bloom underneath each segment, then the crisp coloured line
                g.setColour(colour.withAlpha(.10f));
                g.drawLine(x1,y1,x2,y2,7.0f);
                g.setColour(colour.withAlpha(.30f));
                g.drawLine(x1,y1,x2,y2,4.2f);
                g.setColour(colour);
                g.drawLine(x1,y1,x2,y2,2.0f);
            }
        }

        const float low=processor.apvts.getRawParameterValue("matchlow")->load();
        const float high=processor.apvts.getRawParameterValue("matchhigh")->load();
        const auto fx=[&](float hz){return plot.getX()+float(std::log(hz/20.f)/std::log(1000.f))*plot.getWidth();};
        const float lx=low<=20.01f?plot.getX():fx(low),hx=high>=19999.f?plot.getRight():fx(high);
        g.setColour(black.withAlpha(.48f));g.fillRect(plot.getX(),plot.getY(),std::max(0.f,lx-plot.getX()),plot.getHeight());g.fillRect(hx,plot.getY(),std::max(0.f,plot.getRight()-hx),plot.getHeight());
        g.setColour(cyan.withAlpha(.9f));g.drawVerticalLine(int(lx),plot.getY(),plot.getBottom());
        g.setColour(violet.withAlpha(.9f));g.drawVerticalLine(int(hx),plot.getY(),plot.getBottom());
        g.fillEllipse(lx-4,plot.getCentreY()-4,8,8);g.fillEllipse(hx-4,plot.getCentreY()-4,8,8);
        g.setFont(juce::Font(juce::FontOptions(9)));g.setColour(muted);
        const auto hzText=[](float hz){return hz>=1000.f?juce::String(hz/1000.f,hz<10000?1:0)+"k":juce::String(int(hz));};
        g.drawText("LOW "+hzText(low)+" Hz",juce::Rectangle<float>(lx+5,plot.getY(),76,14),juce::Justification::left);
        g.drawText("HIGH "+hzText(high)+" Hz",juce::Rectangle<float>(hx-82,plot.getY(),78,14),juce::Justification::right);
        g.setFont(juce::Font(juce::FontOptions(10)));g.setColour(muted);
        // Keep the graph legend clearly above the plotted response so it never
        // sits on top of the low-frequency curve/handle labels.
        auto legend=r.reduced(12);legend.setY(r.getY()+4);legend.setHeight(12);
        g.drawText(juce::String("+/- ")+juce::String(scale,0)+" dB   "+(eqOn.getToggleState()?"APPLIED (COLOUR) / 100% TARGET (PURPLE)":"EQ BYPASSED / STORED CURVE")+(outside?"  (choose wider range)":""),legend,juce::Justification::left);
    }else{
        for(int side=0;side<2;++side){auto values=side?processor.getReferenceSpectrum():processor.getSourceSpectrum();juce::Path path;
            for(int i=0;i<180;++i){const double hz=20*std::pow(1000.,i/179.);const int index=std::clamp(int(hz*SpectrumAnalyser::fftSize/processor.getSampleRateForDisplay()),1,SpectrumAnalyser::bins-1);const float x=plot.getX()+i/179.f*plot.getWidth(),y=plot.getBottom()-std::clamp((values[index]+100)/100.f,0.f,1.f)*plot.getHeight();if(!i)path.startNewSubPath(x,y);else path.lineTo(x,y);}
            g.setColour(side?violet:cyan);g.strokePath(path,juce::PathStrokeType(1.5f));}
    }
    g.setFont(juce::Font(juce::FontOptions(9)));g.setColour(muted);for(double hz:{20.,100.,1000.,10000.,20000.}) {
        const float x=plot.getX()+float(std::log(hz/20.)/std::log(1000.))*plot.getWidth();
        g.drawText(hz<1000?juce::String(int(hz))+" Hz":juce::String(int(hz/1000))+" kHz",juce::Rectangle<float>(std::clamp(x-22,r.getX()+4,r.getRight()-48),r.getBottom()-16,44,14),juce::Justification::centred);
    }
}
void RefMatchAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(black);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff111827),0,0,black,960,500,false));
    g.fillRect(getLocalBounds());
    g.setColour(cyan.withAlpha(.022f));g.fillEllipse(-170.f,-55.f,520.f,380.f);
    g.setColour(violet.withAlpha(.024f));g.fillEllipse(650.f,-60.f,440.f,360.f);

    g.setColour(text);g.setFont(juce::Font(juce::FontOptions(24.f,juce::Font::bold)));
    g.drawText("RefMatch",44,18,180,30,juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(10.f)));g.setColour(muted);
    g.drawText("Match your sound.",44,48,180,16,juce::Justification::left);
    g.drawText("v0.5.19    /    STREAM",744,24,150,20,juce::Justification::right);

    // Source cards
    const juce::Rectangle<float> mixCard(44,72,360,104), refCard(542,72,374,104);
    panelCard(g,mixCard,cyan,!processor.isReferenceSelected());
    panelCard(g,refCard,violet,processor.isReferenceSelected());
    g.setColour(text);g.setFont(juce::Font(juce::FontOptions(13.f,juce::Font::bold)));
    g.drawText("YOUR MIX",116,87,110,18,juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(10.f)));g.setColour(muted);
    g.drawText(juce::String(processor.getSourcePeakDb(),1)+" dB",116,108,85,16,juce::Justification::left);
    g.drawText("Gain",116,131,42,18,juce::Justification::left);

    const auto media=processor.getLoop().getPosition();
    const juce::Rectangle<float> cover(628,86,44,44);
    if(media.artwork.isValid())g.drawImageWithin(media.artwork,628,86,44,44,juce::RectanglePlacement::centred);
    else {g.setColour(line);g.fillRoundedRectangle(cover,5.f);g.setColour(violet.withAlpha(.8f));g.fillEllipse(642,100,16,16);}
    g.setColour(text);g.setFont(juce::Font(juce::FontOptions(11.5f,juce::Font::bold)));
    g.drawText(media.title.isNotEmpty()?media.title:"REFERENCE",684,88,160,18,juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(10.f)));g.setColour(text.withAlpha(.82f));
    g.drawText(media.artist,684,109,160,16,juce::Justification::left);
    // tiny decorative player waveform like the reference design
    g.setColour(violet.withAlpha(.18f));
    for(int i=0;i<50;++i){float h=2.f+6.f*std::abs(std::sin(i*.47f)+.35f*std::sin(i*1.37f));g.fillRoundedRectangle(684.f+i*2.4f,135.f-h*.5f,1.4f,h,.7f);}

    if(page==1) {
        // Main graph card
        const juce::Rectangle<float> graphCard(44,248,872,244);
        g.setGradientFill(juce::ColourGradient(panelRaised.withAlpha(.97f),graphCard.getTopLeft(),panel.darker(.14f),graphCard.getBottomRight(),false));
        g.fillRoundedRectangle(graphCard,12.f);g.setColour(line.withAlpha(.88f));g.drawRoundedRectangle(graphCard,12.f,1.f);
        g.setColour(text);g.setFont(juce::Font(juce::FontOptions(13.f,juce::Font::bold)));g.drawText("MATCH EQ",58,260,120,20,juce::Justification::left);
        // legend
        auto dot=[&](float x,juce::Colour c,const juce::String& t){g.setColour(c);g.fillEllipse(x,269,7,7);g.setColour(text.withAlpha(.76f));g.setFont(juce::Font(juce::FontOptions(9.f)));g.drawText(t,int(x+14),263,96,18,juce::Justification::left);};
        dot(412,cyan,"Your Mix");dot(505,violet,"Reference");dot(606,juce::Colour(0xffffb5ff),"Matched (Applied)");
        drawSpectrum(g,{58,282,844,150},true);
        const float low=processor.apvts.getRawParameterValue("matchlow")->load();
        const float high=processor.apvts.getRawParameterValue("matchhigh")->load();
        g.setFont(juce::Font(juce::FontOptions(9.f)));g.setColour(muted);
        g.drawText("20 Hz",58,435,50,18,juce::Justification::left);g.drawText("50 Hz",162,435,48,18,juce::Justification::centred);
        g.drawText("100 Hz",238,435,55,18,juce::Justification::centred);g.drawText("200 Hz",316,435,55,18,juce::Justification::centred);
        g.drawText("500 Hz",410,435,55,18,juce::Justification::centred);g.drawText("1 kHz",500,435,50,18,juce::Justification::centred);
        g.drawText("2 kHz",578,435,50,18,juce::Justification::centred);g.drawText("5 kHz",673,435,50,18,juce::Justification::centred);
        g.drawText("10 kHz",760,435,58,18,juce::Justification::centred);g.drawText("20 kHz",844,435,58,18,juce::Justification::right);
        // controls row inside graph card
        g.setColour(muted);g.setFont(juce::Font(juce::FontOptions(10.f)));g.drawText("Amount",58,459,58,20,juce::Justification::left);g.drawText("Smooth",430,459,58,20,juce::Justification::left);
        g.drawText("Target Range",706,459,82,20,juce::Justification::left);
        auto smallBox=[&](float x,const juce::String& t){g.setColour(panel);g.fillRoundedRectangle(x,457,58,24,5);g.setColour(line);g.drawRoundedRectangle(x,457,58,24,5,1);g.setColour(text.withAlpha(.86f));g.setFont(juce::Font(juce::FontOptions(9.f)));g.drawText(t,int(x),457,58,24,juce::Justification::centred);};
        smallBox(788,low<1000?juce::String(int(std::round(low)))+" Hz":juce::String(low/1000.f,1)+" kHz");
        g.setColour(muted);g.drawText("-",848,457,18,24,juce::Justification::centred);
        smallBox(862,high<1000?juce::String(int(std::round(high)))+" Hz":juce::String(high/1000.f,high>=10000?0:1)+" kHz");

        // Tone EQ section, deliberately flatter and cleaner than the old cards.
        const juce::Rectangle<float> toneCard(44,508,872,112);
        g.setGradientFill(juce::ColourGradient(panelRaised.withAlpha(.93f),toneCard.getTopLeft(),panel.darker(.20f),toneCard.getBottomRight(),false));g.fillRoundedRectangle(toneCard,11.f);
        g.setColour(line.withAlpha(.78f));g.drawRoundedRectangle(toneCard,11.f,1.f);g.drawHorizontalLine(542,58,902);
        g.setColour(text);g.setFont(juce::Font(juce::FontOptions(12.5f,juce::Font::bold)));g.drawText("TONE EQ",60,517,86,20,juce::Justification::left);
        g.setFont(juce::Font(juce::FontOptions(9.f)));g.setColour(muted);g.drawText("Reset All",846,517,56,20,juce::Justification::right);
        const char* names[3]={"LOW","MID","HIGH"}; const char* ranges[3]={"30 - 300 Hz","200 Hz - 6 kHz","3 - 20 kHz"};
        for(int i=0;i<3;++i){const float x=60.f+i*286.f;const auto accent=i==0?cyan:violet;g.setColour(accent);g.fillEllipse(x,554,10,10);g.setFont(juce::Font(juce::FontOptions(10.5f,juce::Font::bold)));g.drawText(names[i],int(x+18),548,52,22,juce::Justification::left);g.setFont(juce::Font(juce::FontOptions(8.8f)));g.setColour(muted);g.drawText(ranges[i],int(x+70),550,98,18,juce::Justification::left);g.drawText("Gain",int(x),576,34,18,juce::Justification::left);g.drawText("Freq",int(x),600,34,18,juce::Justification::left);if(i<2){g.setColour(line.withAlpha(.45f));g.drawVerticalLine(int(x+272),552,614);}}
    } else {
        g.setColour(text);g.setFont(juce::Font(juce::FontOptions(14.f,juce::Font::bold)));g.drawText("LOOP REGION",50,220,150,22,juce::Justification::left);
        const auto& lp=processor.getLoop(); const double length=std::max(0.0,lp.getOut()-lp.getIn()); const int ms=int(std::round(length*1000.));
        const juce::String len=juce::String(ms/60000)+":"+juce::String((ms%60000)/1000).paddedLeft('0',2)+"."+juce::String(ms%1000).paddedLeft('0',3);
        g.setFont(juce::Font(juce::FontOptions(10.f)));g.setColour(muted);g.drawText("ZOOM",52,452,60,20,juce::Justification::left);g.drawText("LOOP LENGTH",720,452,100,20,juce::Justification::right);
        g.setFont(juce::Font(juce::FontOptions(17.f,juce::Font::bold)));g.setColour(text);g.drawText(len,828,448,88,26,juce::Justification::right);
    }

    if(page==1 && juce::Time::getMillisecondCounterHiRes()<matchFlashUntil) {
        const double remain=std::clamp((matchFlashUntil-juce::Time::getMillisecondCounterHiRes())/850.0,0.0,1.0);
        const float pulse=float(.35+.65*std::sin((1.0-remain)*juce::MathConstants<double>::pi));
        auto rr=match.getBounds().toFloat().expanded(8.f,6.f);glowRounded(g,rr,11.f,violet,.15f+.32f*pulse);g.setColour(cyan.withAlpha(.20f+.25f*pulse));g.drawRoundedRectangle(rr,11.f,1.5f);
    }
}

float RefMatchAudioProcessorEditor::hzToGraphX(float hz) const
{
    const float left=70.f,width=820.f;
    if(hz<=20.01f)return left;
    if(hz>=19999.f)return left+width;
    return left+float(std::log(std::clamp(hz,20.f,20000.f)/20.f)/std::log(1000.f))*width;
}
float RefMatchAudioProcessorEditor::graphXToHz(float x) const
{
    const float norm=std::clamp((x-70.f)/820.f,0.f,1.f);
    return 20.f*std::pow(1000.f,norm);
}
void RefMatchAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if(page!=1 || e.position.y<300.f || e.position.y>420.f)return;
    const float low=processor.apvts.getRawParameterValue("matchlow")->load();
    const float high=processor.apvts.getRawParameterValue("matchhigh")->load();
    const float dl=std::abs(e.position.x-hzToGraphX(low)),dh=std::abs(e.position.x-hzToGraphX(high));
    if(std::min(dl,dh)>14.f)return;
    matchDrag=dl<dh?MatchDrag::low:MatchDrag::high;updateMatchHandle(e.position.x);
}
void RefMatchAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if(matchDrag!=MatchDrag::none)updateMatchHandle(e.position.x);
}
void RefMatchAudioProcessorEditor::mouseUp(const juce::MouseEvent&) { matchDrag=MatchDrag::none; }
void RefMatchAudioProcessorEditor::updateMatchHandle(float x)
{
    float hz=graphXToHz(x);
    auto* lowP=dynamic_cast<juce::AudioParameterFloat*>(processor.apvts.getParameter("matchlow"));
    auto* highP=dynamic_cast<juce::AudioParameterFloat*>(processor.apvts.getParameter("matchhigh"));
    if(!lowP||!highP)return;
    const float low=processor.apvts.getRawParameterValue("matchlow")->load(),high=processor.apvts.getRawParameterValue("matchhigh")->load();
    if(matchDrag==MatchDrag::low){hz=std::min(hz,high/1.25f);hz=std::clamp(hz,20.f,1000.f);lowP->setValueNotifyingHost(lowP->convertTo0to1(hz));}
    if(matchDrag==MatchDrag::high){hz=std::max(hz,low*1.25f);hz=std::clamp(hz,1000.f,20000.f);highP->setValueNotifyingHost(highP->convertTo0to1(hz));}
    repaint();
}

void RefMatchAudioProcessorEditor::resized()
{
    // Top source cards
    a.setBounds(58,86,48,42); b.setBounds(560,86,48,42); switchButton.setBounds(438,78,66,66); matchState.setBounds(294,82,94,24);
    gain.setBounds(158,124,224,30); back.setBounds(780,132,48,28); play.setBounds(834,132,44,28); forward.setBounds(884,132,44,28);

    // Main action row mirrors the supplied visual reference.
    recordMix.setBounds(44,184,170,38); recordRef.setBounds(228,184,170,38); match.setBounds(414,184,170,38); reset.setBounds(596,184,108,38);
    loopTab.setBounds(716,184,64,38); quickLoop.setBounds(778,184,78,38); eqOn.setBounds(866,184,76,38);
    mixProfile.setBounds(62,219,150,18); refProfile.setBounds(246,219,150,18);
    eqTab.setBounds(44,184,120,36);

    // Graph controls
    amount.setBounds(118,452,278,34); smooth.setBounds(488,452,208,34); graphRange.setBounds(822,254,80,24);

    // Tone EQ
    toneButton.setBounds(60,514,92,24); toneOn.setBounds(150,514,76,24); toneReset.setButtonText("Reset All"); toneReset.setBounds(838,514,64,24);
    tone[0].setBounds(98,572,180,24); tone[1].setBounds(98,596,180,24);
    tone[2].setBounds(384,572,180,24); tone[3].setBounds(384,596,180,24);
    tone[4].setBounds(670,572,180,24); tone[5].setBounds(670,596,180,24);
    lowType.setBounds(242,548,82,20); midQ.setBounds(518,546,104,24); highType.setBounds(812,548,82,20);

    // Loop page
    timeline.setBounds(48,250,868,170); position.setBounds(50,424,260,20); clearLoop.setBounds(800,214,116,28);
    zoomMinus.setBounds(116,448,32,28); loopZoom.setBounds(154,448,146,28); zoomPlus.setBounds(306,448,32,28);
    inTime.setBounds(0,0,0,0);setIn.setBounds(0,0,0,0);outTime.setBounds(0,0,0,0);setOut.setBounds(0,0,0,0);
    status.setBounds(46,getHeight()-26,868,20);
}

