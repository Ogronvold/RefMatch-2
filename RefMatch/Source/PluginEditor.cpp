#include "PluginEditor.h"
namespace {
const juce::Colour black(0xff08090c),panel(0xff101217),line(0xff242830),text(0xffeef1f7),muted(0xff808897),cyan(0xff76e5d1),violet(0xffb3a0ff);
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
    g.setColour(colour.withMultipliedBrightness(down?.8f:over?1.2f:1.f));
    g.fillRoundedRectangle(r,7);
    g.setColour(button.getToggleState()?cyan.withAlpha(.7f):line);g.drawRoundedRectangle(r,7,1);
}
RefMatchAudioProcessorEditor::RefMatchAudioProcessorEditor(RefMatchAudioProcessor& p):AudioProcessorEditor(&p),processor(p)
{
    setLookAndFeel(&look);setResizable(false,false);
    for(juce::Component* c:std::initializer_list<juce::Component*>{&a,&b,&switchButton,&eqTab,&loopTab,&play,&meters,&autoGain,
        &recordMix,&recordRef,&match,&reset,&eqOn,&gain,&amount,&search,&find,&inTime,&outTime,&setIn,&setOut,&loopOn,&timedLoop,
        &status,&mixProfile,&refProfile,&position})addAndMakeVisible(c);
    a.onClick=[this]{processor.selectSource(false);};b.onClick=[this]{processor.selectSource(true);};
    switchButton.onClick=[this]{processor.switchWithSystemMedia();};
    eqTab.onClick=[this]{setPage(1);};loopTab.onClick=[this]{setPage(2);};
    play.onClick=[this]{transport(SystemMediaController::Command::playPause);};
    autoGain.onClick=[this]{processor.autoGainMatch();};
    meters.onClick=[this]{processor.startReferenceCapture();};
    recordMix.onClick=[this]{processor.recordProfile(LearnCapture::mix);};
    recordRef.onClick=[this]{processor.recordProfile(LearnCapture::reference);};
    match.onClick=[this]{processor.learnMatch();};reset.onClick=[this]{processor.clearMatch();};
    for(auto* slider:{&gain,&amount}) {slider->setSliderStyle(juce::Slider::LinearHorizontal);slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,66,24);}
    gain.setTextValueSuffix(" dB");amount.setTextValueSuffix(" %");
    gainAttach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"sourcegain",gain);
    amountAttach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"matchamount",amount);
    eqAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"matchenabled",eqOn);
    search.setTextToShowWhenEmpty("Search Spotify...",muted);
    find.onClick=[this]{processor.getMediaController().openSearch(search.getText(),message);};search.onReturnKey=find.onClick;
    inTime.setText(timeText(p.getLoop().getIn()));outTime.setText(timeText(p.getLoop().getOut()));
    inTime.onReturnKey=[this]{updateLoopRange();};outTime.onReturnKey=inTime.onReturnKey;
    inTime.onFocusLost=inTime.onReturnKey;outTime.onFocusLost=inTime.onReturnKey;
    setIn.onClick=[this]{const auto v=processor.getLoop().getPosition();if(v.valid){inTime.setText(timeText(v.seconds));updateLoopRange();}};
    setOut.onClick=[this]{const auto v=processor.getLoop().getPosition();if(v.valid){outTime.setText(timeText(v.seconds));updateLoopRange();}};
    timedLoop.onClick=[this]{processor.getLoop().setTimed(timedLoop.getToggleState());};
    timedLoop.setTooltip("Use the entered interval without player position. Seek is unverified; disable if you change track or pause outside RefMatch.");
    loopOn.onClick=[this]{if(loopOn.getToggleState()) {const auto start=parseTime(inTime.getText()),end=parseTime(outTime.getText());if(processor.getLoop().setRange(start,end))processor.getLoop().enable(true);}else processor.getLoop().enable(false);};
    label(status,11);label(mixProfile,11,cyan);label(refProfile,11,violet);label(position,13,text);
    a.setTooltip("Listen to your mix. Pauses the active media player.");b.setTooltip("Listen to reference. Mutes MIX and sends system PLAY.");
    recordMix.setTooltip("Record the incoming MIX spectrum before EQ. Click again to finish.");
    recordRef.setTooltip("Record system-reference spectrum. Requires capture permission and host audio processing. Click again to finish.");
    match.setTooltip("Calculate EQ from the two captured profiles and enable it on MIX.");
    meters.setTooltip("Optional ScreenCaptureKit analysis. A/B works without it.");
    loopOn.setTooltip("Loop the active source between In and Out. Requires player position and seek support.");
    setPage(1);processor.startReferenceCapture();startTimerHz(15);
}
RefMatchAudioProcessorEditor::~RefMatchAudioProcessorEditor(){stopTimer();setLookAndFeel(nullptr);}
void RefMatchAudioProcessorEditor::setPage(int value)
{
    page=value;
    for(auto* c:std::initializer_list<juce::Component*>{&recordMix,&recordRef,&match,&reset,&eqOn,&amount,&mixProfile,&refProfile})c->setVisible(page==1);
    for(auto* c:std::initializer_list<juce::Component*>{&inTime,&outTime,&setIn,&setOut,&loopOn,&timedLoop,&position})c->setVisible(page==2);
    search.setVisible(true);find.setVisible(true);
    setSize(640,page==1?550:390);
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
    mixProfile.setText(processor.recording()==LearnCapture::mix?"RECORDING  "+juce::String(m.seconds,1)+" s":m.ready?"CAPTURED  "+juce::String(m.seconds,1)+" s":"No MIX recording",juce::dontSendNotification);
    refProfile.setText(processor.recording()==LearnCapture::reference?"RECORDING  "+juce::String(r.seconds,1)+" s":r.ready?"CAPTURED  "+juce::String(r.seconds,1)+" s":"No REF recording",juce::dontSendNotification);
    match.setEnabled(m.ready&&r.ready);eqOn.setEnabled(processor.hasMatch());
    juce::String info=page==1?processor.getLearningStatus():page==2?processor.getLoop().getStatus():"A = your mix   /   B = system reference";
    if(page==1 && processor.recording()==LearnCapture::reference && !processor.hasReferenceAudio())info=processor.getReferenceCaptureStatus()+" - waiting for audio";
    if(processor.getTransportError().isNotEmpty())info=processor.getTransportError();
    if(message.isNotEmpty())info=message;
    status.setText(info,juce::dontSendNotification);status.setTooltip(info);
    loopOn.setToggleState(processor.getLoop().isEnabled(),juce::dontSendNotification);
    timedLoop.setToggleState(processor.getLoop().isTimed(),juce::dontSendNotification);
    const auto p=processor.getLoop().getPosition();position.setText(p.valid?timeText(p.seconds)+"  /  "+timeText(p.duration):"Position unavailable",juce::dontSendNotification);
    play.setToggleState(p.playbackKnown && p.playing,juce::dontSendNotification);
    play.setButtonText(p.playbackKnown?(p.playing?"PAUSE":"PLAY"):"PLAY / PAUSE");
    play.setTooltip(p.playbackKnown?"Current player playback state":"Playback status unavailable; click to toggle playback");
    search.setTooltip(p.title.isNotEmpty()?p.title+" - "+p.artist:"Search opens Spotify");
    autoGain.setEnabled(processor.hasReferenceAudio());
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
        const auto curve=processor.getMatchCurveDb();float scale=12.f;for(auto db:curve)scale=std::max(scale,std::ceil(std::abs(db)));juce::Path path;
        for(size_t i=0;i<curve.size();++i) {const float x=plot.getX()+float(i)/float(curve.size()-1)*plot.getWidth();const float y=plot.getCentreY()-curve[i]/(2*scale)*plot.getHeight();if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}
        g.setColour(eqOn.getToggleState()?cyan:muted);g.strokePath(path,juce::PathStrokeType(2));
        g.setFont(juce::Font(juce::FontOptions(10)));g.setColour(muted);g.drawText(juce::String("+/- ")+juce::String(scale,0)+" dB   "+(eqOn.getToggleState()?"APPLIED EQ RESPONSE":"EQ BYPASSED / STORED CURVE"),r.reduced(12).removeFromTop(14),juce::Justification::left);
    }else{
        for(int side=0;side<2;++side){auto values=side?processor.getReferenceSpectrum():processor.getSourceSpectrum();juce::Path path;
            for(int i=0;i<180;++i){const double hz=20*std::pow(1000.,i/179.);const int index=std::clamp(int(hz*SpectrumAnalyser::fftSize/processor.getSampleRateForDisplay()),1,SpectrumAnalyser::bins-1);const float x=plot.getX()+i/179.f*plot.getWidth(),y=plot.getBottom()-std::clamp((values[index]+100)/100.f,0.f,1.f)*plot.getHeight();if(!i)path.startNewSubPath(x,y);else path.lineTo(x,y);}
            g.setColour(side?violet:cyan);g.strokePath(path,juce::PathStrokeType(1.5f));}
    }
    g.setFont(juce::Font(juce::FontOptions(9)));g.setColour(muted);g.drawText("20 Hz                                             1 kHz                                            20 kHz",r.reduced(12,2).removeFromBottom(14),juce::Justification::centred);
}
void RefMatchAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(black);g.setColour(text);g.setFont(juce::Font(juce::FontOptions(23,juce::Font::bold)));g.drawText("RefMatch",20,12,170,30,juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(10)));g.setColour(muted);g.drawText("0.5.1   /   STREAM",455,18,164,20,juce::Justification::right);
    for(auto r:{juce::Rectangle<float>(20,60,250,114),juce::Rectangle<float>(370,60,250,114)}){g.setColour(panel);g.fillRoundedRectangle(r,12);g.setColour(line);g.drawRoundedRectangle(r,12,1);}
    g.setColour(text);g.setFont(juce::Font(juce::FontOptions(13,juce::Font::bold)));g.drawText("YOUR MIX",87,75,150,20,juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(11)));g.setColour(muted);g.drawText("Gain",36,123,40,24,juce::Justification::left);
    const auto media=processor.getLoop().getPosition();
    g.drawText(media.title.isNotEmpty()?media.title:"Track info unavailable",437,78,172,17,juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(9)));
    g.drawText(media.artist,437,96,172,14,juce::Justification::left);
    g.drawText(juce::String(processor.getSourcePeakDb(),1)+" dB",87,97,68,16,juce::Justification::left);
    auto meter=[&](float x,float y,float width,float db,juce::Colour colour) {
        g.setColour(line);g.fillRoundedRectangle(x,y,width,3,1.5f);
        g.setColour(colour);g.fillRoundedRectangle(x,y,width*std::clamp((db+60.f)/60.f,0.f,1.f),3,1.5f);
    };
    meter(36,158,220,processor.getSourcePeakDb(),cyan);
    meter(384,111,228,processor.getReferencePeakDb(),violet);
    if(page==1) {
        meter(20,294,180,processor.getSourcePeakDb(),cyan);
        meter(212,294,180,processor.getReferencePeakDb(),violet);
    }
    if(page==0)drawSpectrum(g,{20,258,600,68},false);
    if(page==1){
        for(int side=0;side<2;++side) {
            const juce::Rectangle<float> box(side?328.f:20.f,302,292,62);
            g.setColour(panel);g.fillRoundedRectangle(box,6);
            const auto snapshot=processor.profile(side?LearnCapture::reference:LearnCapture::mix);
            const auto live=side?processor.getReferenceSpectrum():processor.getSourceSpectrum();
            const auto colour=side?violet:cyan;
            for(int stored=0;stored<2;++stored) {
                if(stored && !snapshot.ready)continue;
                const auto& data=stored?snapshot.db:live;
                const double rate=stored?snapshot.sampleRate:processor.getSampleRateForDisplay();
                juce::Path path;
                for(int i=0;i<140;++i) {
                    const double hz=20*std::pow(1000.,i/139.);
                    const int index=std::clamp(int(hz*SpectrumAnalyser::fftSize/std::max(1.,rate)),1,SpectrumAnalyser::bins-1);
                    const float x=box.getX()+6+i/139.f*(box.getWidth()-12);
                    const float y=box.getBottom()-5-std::clamp((data[index]+100)/100.f,0.f,1.f)*36;
                    if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);
                }
                g.setColour(colour.withAlpha(stored?1.f:.35f));g.strokePath(path,juce::PathStrokeType(stored?1.7f:1.f));
            }
            g.setFont(juce::Font(juce::FontOptions(9)));g.setColour(muted);
            g.drawText("LIVE (dim) / RECORDING OR CAPTURED (bright)",box.reduced(6).removeFromTop(12),juce::Justification::left);
        }
        drawSpectrum(g,{20,370,600,94},true);g.setColour(muted);g.drawText("Amount",22,476,55,24,juce::Justification::left);}
    if(page==2){g.setColour(muted);g.drawText("IN",20,234,30,20,juce::Justification::left);g.drawText("OUT",238,234,34,20,juce::Justification::left);g.drawText("Loop follows the active player. A/B works without position access.",20,318,600,20,juce::Justification::left);}
}
void RefMatchAudioProcessorEditor::resized()
{
    a.setBounds(34,76,42,38);b.setBounds(384,76,42,38);switchButton.setBounds(280,94,80,38);
    gain.setBounds(75,120,180,28);autoGain.setBounds(160,97,96,21);play.setBounds(437,117,108,26);meters.setBounds(551,117,60,26);
    eqTab.setBounds(20,190,292,28);loopTab.setBounds(328,190,292,28);
    search.setBounds(380,147,174,21);find.setBounds(560,147,52,21);
    recordMix.setBounds(20,234,180,32);recordRef.setBounds(212,234,180,32);match.setBounds(404,234,102,32);reset.setBounds(516,234,104,32);
    mixProfile.setBounds(20,267,180,24);refProfile.setBounds(212,267,180,24);eqOn.setBounds(520,269,100,24);
    amount.setBounds(76,475,540,28);
    inTime.setBounds(52,232,92,28);setIn.setBounds(152,232,70,28);outTime.setBounds(278,232,92,28);setOut.setBounds(378,232,70,28);loopOn.setBounds(480,232,120,28);position.setBounds(20,280,420,28);timedLoop.setBounds(480,280,120,28);
    status.setBounds(20,getHeight()-30,600,24);
}
