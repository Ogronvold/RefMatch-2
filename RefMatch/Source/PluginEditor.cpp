#include "PluginEditor.h"
namespace {
const juce::Colour black(0xff090e16),panel(0xff111824),line(0xff2a3445),text(0xffeef1f7),muted(0xff808897),cyan(0xffffad42),violet(0xffb16af3);
void label(juce::Label& l,float size,juce::Colour colour=muted) {
    l.setFont(juce::Font(juce::FontOptions(size)));l.setColour(juce::Label::textColourId,colour);
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
    auto r=button.getLocalBounds().toFloat().reduced(.5f);
    const auto fill=colour.withMultipliedBrightness(down?.8f:over?1.2f:1.f);
    if(button.getToggleState() && bool(button.getProperties()["dualAccent"]))
        g.setGradientFill(juce::ColourGradient(cyan,r.getTopLeft(),violet,r.getTopRight(),false));
    else g.setGradientFill(juce::ColourGradient(fill.brighter(.10f),r.getTopLeft(),fill.darker(.10f),r.getBottomLeft(),false));
    g.fillRoundedRectangle(r,7);
    g.setColour(button.getToggleState()?colour.withAlpha(.7f):line);g.drawRoundedRectangle(r,7,1);
}
void RefMatchLookAndFeel::drawLinearSlider(juce::Graphics& g,int x,int y,int width,int height,float position,float,float,juce::Slider::SliderStyle,juce::Slider& slider)
{
    const float mid=y+height*.5f;
    g.setColour(line);g.fillRoundedRectangle(float(x),mid-2,float(width),4,2);
    const auto accent=slider.findColour(juce::Slider::trackColourId);
    g.setGradientFill(juce::ColourGradient(bool(slider.getProperties()["dualAccent"])?cyan:accent,float(x),mid,bool(slider.getProperties()["dualAccent"])?violet:accent,float(x+width),mid,false));
    g.fillRoundedRectangle(float(x),mid-2,std::max(0.f,position-x),4,2);
    g.setColour(accent.withAlpha(.15f));g.fillEllipse(position-8,mid-8,16,16);
    g.setColour(text);g.fillEllipse(position-5,mid-5,10,10);
}
RefMatchAudioProcessorEditor::RefMatchAudioProcessorEditor(RefMatchAudioProcessor& p):AudioProcessorEditor(&p),processor(p)
{
    setLookAndFeel(&look);setResizable(false,false);
    for(juce::Component* c:std::initializer_list<juce::Component*>{&a,&b,&switchButton,&eqTab,&loopTab,&play,&toneButton,&toneReset,&toneOn,&graphRange,
        &recordMix,&recordRef,&match,&reset,&eqOn,&gain,&amount,&smooth,&back,&forward,&timeline,&inTime,&outTime,&setIn,&setOut,&loopOn,
        &status,&mixProfile,&refProfile,&position})addAndMakeVisible(c);
    a.onClick=[this]{processor.selectSource(false);};b.onClick=[this]{processor.selectSource(true);};
    switchButton.onClick=[this]{processor.switchWithSystemMedia();};
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
    toneButton.onClick=[this]{showTone=!showTone;setPage(page);};
    toneReset.onClick=[this]{for(int i=0;i<3;++i) {
        const auto id="tone"+juce::String(i)+"gain";
        auto* p=processor.apvts.getParameter(id);p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(0));p->endChangeGesture();
    }};
    recordMix.onClick=[this]{processor.recordProfile(LearnCapture::mix);};
    recordRef.onClick=[this]{processor.recordProfile(LearnCapture::reference);};
    match.onClick=[this]{processor.learnMatch();};reset.onClick=[this]{processor.clearMatch();};
    for(auto* slider:{&gain,&amount,&smooth}) {slider->setSliderStyle(juce::Slider::LinearHorizontal);slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,66,24);}
    gain.setTextValueSuffix(" dB");amount.setTextValueSuffix(" %");smooth.setTextValueSuffix(" %");
    gainAttach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"sourcegain",gain);
    amountAttach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"matchamount",amount);
    smoothAttach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"smooth",smooth);
    smooth.setTooltip("Fine to broad correction. Recalculates from captured profiles without recording again.");
    amount.getProperties().set("dualAccent",true);smooth.setColour(juce::Slider::trackColourId,violet);
    for(int i=0;i<6;++i) {
        addAndMakeVisible(tone[i]);tone[i].setSliderStyle(juce::Slider::LinearHorizontal);
        tone[i].setTextBoxStyle(juce::Slider::TextBoxRight,false,70,22);
        tone[i].setTextValueSuffix(i%2?" Hz":" dB");tone[i].setColour(juce::Slider::trackColourId,violet);
        toneAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"tone"+juce::String(i/2)+(i%2?"freq":"gain"),tone[i]);
    }
    for(auto* button:{&b,&play,&recordRef})button->setColour(juce::TextButton::buttonOnColourId,violet);
    eqTab.getProperties().set("dualAccent",true);loopTab.getProperties().set("dualAccent",true);
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
    loopOn.onClick=[this]{if(loopOn.getToggleState()) {const auto start=parseTime(inTime.getText()),end=parseTime(outTime.getText());if(processor.getLoop().setRange(start,end))processor.getLoop().enable(true);}else processor.getLoop().enable(false);};
    label(status,11);label(mixProfile,11,cyan);label(refProfile,11,violet);label(position,13,text);
    a.setTooltip("Listen to your mix. Pauses the active media player.");b.setTooltip("Listen to reference. Mutes MIX and sends system PLAY.");
    recordMix.setTooltip("Record the incoming MIX spectrum before EQ. Click again to finish.");
    recordRef.setTooltip("Record system-reference spectrum. Requires capture permission and host audio processing. Click again to finish.");
    match.setTooltip("Calculate EQ from the two captured profiles and enable it on MIX.");
    loopOn.setTooltip("Loop the active source between In and Out. Requires player position and seek support.");
    setPage(1);processor.startReferenceCapture();startTimerHz(15);
}
RefMatchAudioProcessorEditor::~RefMatchAudioProcessorEditor(){stopTimer();setLookAndFeel(nullptr);}
void RefMatchAudioProcessorEditor::setPage(int value)
{
    page=value;
    for(auto* c:std::initializer_list<juce::Component*>{&recordMix,&recordRef,&match,&reset,&eqOn,&amount,&smooth,&toneButton,&toneOn,&graphRange,&mixProfile,&refProfile})c->setVisible(page==1);
    for(auto* c:std::initializer_list<juce::Component*>{&inTime,&outTime,&setIn,&setOut,&loopOn,&timeline,&position})c->setVisible(page==2);
    toneButton.setToggleState(showTone,juce::dontSendNotification);
    toneReset.setVisible(page==1 && showTone);
    for(auto& control:tone)control.setVisible(page==1 && showTone);
    setSize(640,page==1?(showTone?660:580):430);
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
    switchButton.setButtonText(processor.isTransportPending()?"CANCEL":"SWITCH");
    const auto m=processor.profile(LearnCapture::mix),r=processor.profile(LearnCapture::reference);
    recordMix.setButtonText(processor.recording()==LearnCapture::mix?"STOP MIX":"RECORD MIX");
    recordMix.setToggleState(processor.recording()==LearnCapture::mix,juce::dontSendNotification);
    recordRef.setToggleState(processor.recording()==LearnCapture::reference,juce::dontSendNotification);
    recordRef.setButtonText(processor.recording()==LearnCapture::reference?"STOP REF":"RECORD REF");
    mixProfile.setText(processor.recording()==LearnCapture::mix?"RECORDING  "+juce::String(m.seconds,1)+" s":m.ready?"CAPTURED  "+juce::String(m.seconds,1)+" s":"MIX",juce::dontSendNotification);
    refProfile.setText(processor.recording()==LearnCapture::reference?"RECORDING  "+juce::String(r.seconds,1)+" s":r.ready?"CAPTURED  "+juce::String(r.seconds,1)+" s":"REF",juce::dontSendNotification);
    match.setEnabled(m.ready&&r.ready);eqOn.setEnabled(processor.hasMatch());
    juce::String info=page==1?processor.getLearningStatus():page==2?processor.getLoop().getStatus():"A = your mix   /   B = system reference";
    if(page==1 && processor.recording()==LearnCapture::reference && !processor.hasReferenceAudio())info=processor.getReferenceCaptureStatus()+" - waiting for audio";
    if(processor.getTransportError().isNotEmpty())info=processor.getTransportError();
    if(message.isNotEmpty())info=message;
    status.setText(info,juce::dontSendNotification);status.setTooltip(info);
    loopOn.setToggleState(processor.getLoop().isEnabled(),juce::dontSendNotification);
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
    g.setColour(panel);g.fillRoundedRectangle(r,9);auto plot=r.reduced(12,20);
    g.setColour(line);
    for(int i=0;i<=4;++i){const float y=plot.getY()+plot.getHeight()*i/4;g.drawHorizontalLine(int(y),plot.getX(),plot.getRight());}
    for(double hz:{100.,1000.,10000.}) {const float x=plot.getX()+float(std::log(hz/20.)/std::log(1000.))*plot.getWidth();g.drawVerticalLine(int(x),plot.getY(),plot.getBottom());}
    if(eq) {
        const auto curve=processor.getMatchCurveDb();const float scale=graphScale;bool outside=false;for(auto db:curve)outside=outside || std::abs(db)>scale;juce::Path path;
        for(size_t i=0;i<curve.size();++i) {const float x=plot.getX()+float(i)/float(curve.size()-1)*plot.getWidth();const float y=plot.getCentreY()-std::clamp(curve[i],-scale,scale)/(2*scale)*plot.getHeight();if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}
        g.setColour(eqOn.getToggleState()?text:muted);g.strokePath(path,juce::PathStrokeType(2));
        g.setFont(juce::Font(juce::FontOptions(10)));g.setColour(muted);g.drawText(juce::String("+/- ")+juce::String(scale,0)+" dB   "+(eqOn.getToggleState()?"APPLIED EQ RESPONSE":"EQ BYPASSED / STORED CURVE")+(outside?"  (choose wider range)":""),r.reduced(12).removeFromTop(14),juce::Justification::left);
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
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff172238),0,0,black,640,250,false));g.fillRect(getLocalBounds());g.setColour(text);g.setFont(juce::Font(juce::FontOptions(23,juce::Font::bold)));g.drawText("RefMatch",20,12,170,30,juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(10)));g.setColour(muted);g.drawText("0.5.5   /   STREAM",455,18,164,20,juce::Justification::right);
    for(int side=0;side<2;++side) {
        const juce::Rectangle<float> r(side?370.f:20.f,60,250,114);
        g.setGradientFill(juce::ColourGradient(panel.brighter(.12f),r.getTopLeft(),panel.darker(.12f),r.getBottomRight(),false));g.fillRoundedRectangle(r,12);
        g.setColour((side?violet:cyan).withAlpha((side==int(processor.isReferenceSelected()))?.65f:.22f));g.drawRoundedRectangle(r,12,1);
    }
    g.setColour(muted);g.setFont(juce::Font(juce::FontOptions(10)));g.drawText("Match your sound.",21,41,180,14,juce::Justification::left);
    g.setColour(text);g.setFont(juce::Font(juce::FontOptions(13,juce::Font::bold)));g.drawText("YOUR MIX",87,75,150,20,juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(11)));g.setColour(muted);g.drawText("Gain",36,123,40,24,juce::Justification::left);
    const auto media=processor.getLoop().getPosition();
    const juce::Rectangle<float> cover(435,76,36,36);
    if(media.artwork.isValid())g.drawImageWithin(media.artwork,435,76,36,36,juce::RectanglePlacement::centred);
    else {g.setColour(line);g.fillRoundedRectangle(cover,4);g.setColour(muted);g.drawEllipse(445,86,16,16,1);}
    g.setColour(text);g.setFont(juce::Font(juce::FontOptions(11)));
    g.drawText(media.title.isNotEmpty()?media.title:"REFERENCE",479,77,128,17,juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(9)));
    g.drawText(media.artist,479,96,128,14,juce::Justification::left);
    g.drawText(juce::String(processor.getSourcePeakDb(),1)+" dB",87,97,68,16,juce::Justification::left);
    auto meter=[&](float x,float y,float width,float db,juce::Colour colour) {
        g.setColour(line);g.fillRoundedRectangle(x,y,width,3,1.5f);
        g.setColour(colour);g.fillRoundedRectangle(x,y,width*std::clamp((db+60.f)/60.f,0.f,1.f),3,1.5f);
    };
    meter(36,158,220,processor.getSourcePeakDb(),cyan);
    meter(386,158,220,processor.getReferencePeakDb(),violet);
    if(page==1) {
        meter(20,294,180,processor.getSourcePeakDb(),cyan);
        meter(212,294,180,processor.getReferencePeakDb(),violet);
    }
    if(page==0)drawSpectrum(g,{20,258,600,68},false);
    if(page==1){
        for(int side=0;side<2;++side) {
            const juce::Rectangle<float> box(side?328.f:20.f,302,292,62);
            g.setColour(panel);g.fillRoundedRectangle(box,6);
            g.setColour(line.withAlpha(.5f));for(int row=0;row<3;++row)g.drawHorizontalLine(int(box.getY()+24+row*14),box.getX()+6,box.getRight()-6);
            const auto snapshot=processor.profile(side?LearnCapture::reference:LearnCapture::mix);
            const auto live=side?processor.getReferenceSpectrum():processor.getSourceSpectrum();
            const auto colour=side?violet:cyan;
            for(int stored=0;stored<2;++stored) {
                if(stored && !snapshot.ready)continue;
                const auto& data=stored?snapshot.db:live;
                const double rate=stored?snapshot.sampleRate:side?ReferenceAnalysis::sampleRate:processor.getSampleRateForDisplay();
                juce::Path path;
                for(int i=0;i<140;++i) {
                    const double hz=20*std::pow(1000.,i/139.);
                    const int index=std::clamp(int(hz*SpectrumAnalyser::fftSize/std::max(1.,rate)),1,SpectrumAnalyser::bins-1);
                    const float x=box.getX()+6+i/139.f*(box.getWidth()-12);
                    const float y=box.getBottom()-5-std::clamp((data[index]+100)/100.f,0.f,1.f)*36;
                    if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);
                }
                if(stored) {
                    auto fill=path;fill.lineTo(box.getRight()-6,box.getBottom()-5);fill.lineTo(box.getX()+6,box.getBottom()-5);fill.closeSubPath();
                    g.setGradientFill(juce::ColourGradient(colour.withAlpha(.22f),box.getTopLeft(),colour.withAlpha(0.f),box.getBottomLeft(),false));g.fillPath(fill);
                }
                g.setColour(colour.withAlpha(stored?1.f:.35f));g.strokePath(path,juce::PathStrokeType(stored?1.7f:1.f));
            }
            g.setFont(juce::Font(juce::FontOptions(9)));g.setColour(muted);
            g.drawText(side?"REFERENCE    LIVE / CAPTURED":"YOUR MIX    LIVE / CAPTURED",box.reduced(6).removeFromTop(12),juce::Justification::left);
        }
        drawSpectrum(g,{20,370,600,94},true);g.setColour(muted);g.drawText("Amount",22,476,55,24,juce::Justification::left);g.drawText("Smooth",326,476,54,24,juce::Justification::left);
        g.drawText("Fine",386,499,45,14,juce::Justification::left);g.drawText("Broad",563,499,54,14,juce::Justification::right);
        if(showTone)for(int i=0;i<3;++i) {
            const int x=20+i*204;g.setColour(panel);g.fillRoundedRectangle(float(x),542,192,76,7);
            g.setColour(violet);g.drawText(i==0?"LOW":i==1?"MID":"HIGH",x+8,542,160,16,juce::Justification::left);
        }}
    if(processor.getLoop().isEnabled()) {
        const bool active=processor.getLoop().isActive();
        const float pulse=active ? .35f+.65f*float(.5+.5*std::sin(juce::Time::getMillisecondCounterHiRes()*.008)) : 1.f;
        g.setColour((active?violet:cyan).withAlpha(pulse));g.fillEllipse(300,25,6,6);
        g.setFont(juce::Font(juce::FontOptions(10)));g.drawText(active?"LOOP ACTIVE":"LOOP WAITING",312,18,124,20,juce::Justification::left);
    }
    if(page==2){g.setColour(muted);g.drawText("IN",20,316,30,20,juce::Justification::left);g.drawText("OUT",238,316,34,20,juce::Justification::left);g.drawText("Drag the white playhead to seek. Drag a region to loop.",20,370,600,20,juce::Justification::left);}
}
void RefMatchAudioProcessorEditor::resized()
{
    a.setBounds(34,76,42,38);b.setBounds(384,76,42,38);switchButton.setBounds(280,94,80,38);
    gain.setBounds(75,120,180,28);back.setBounds(384,123,48,26);play.setBounds(439,123,112,26);forward.setBounds(558,123,48,26);
    eqTab.setBounds(20,190,292,28);loopTab.setBounds(328,190,292,28);
    recordMix.setBounds(20,234,180,32);recordRef.setBounds(212,234,180,32);match.setBounds(404,234,102,32);reset.setBounds(516,234,104,32);
    mixProfile.setBounds(20,267,180,24);refProfile.setBounds(212,267,180,24);eqOn.setBounds(520,269,100,24);
    amount.setBounds(76,475,232,28);smooth.setBounds(380,475,238,28);
    toneButton.setBounds(20,517,112,25);toneOn.setBounds(146,517,114,25);graphRange.setBounds(366,517,126,25);toneReset.setBounds(504,517,116,25);
    for(int i=0;i<3;++i){tone[2*i].setBounds(26+i*204,560,180,24);tone[2*i+1].setBounds(26+i*204,588,180,24);}
    timeline.setBounds(20,230,600,72);inTime.setBounds(52,314,92,28);setIn.setBounds(152,314,70,28);outTime.setBounds(278,314,92,28);setOut.setBounds(378,314,70,28);loopOn.setBounds(480,314,120,28);position.setBounds(20,345,600,22);
    status.setBounds(20,getHeight()-30,600,24);
}
