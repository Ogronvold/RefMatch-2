#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LoopTimeline.h"

class RefMatchLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RefMatchLookAndFeel();
    void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
    void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
};
class RefMatchAudioProcessorEditor : public juce::AudioProcessorEditor,private juce::Timer
{
public:
    explicit RefMatchAudioProcessorEditor(RefMatchAudioProcessor&);
    ~RefMatchAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    void setPage(int);
    void drawSpectrum(juce::Graphics&,juce::Rectangle<float>,bool eq);
    void transport(SystemMediaController::Command);
    void updateLoopRange();
    void updateMatchHandle(float x);
    float hzToGraphX(float hz) const;
    float graphXToHz(float x) const;
    static double parseTime(const juce::String&);
    static juce::String timeText(double);
    static juce::String rangeText(double);
    RefMatchAudioProcessor& processor;
    RefMatchLookAndFeel look;
    juce::TextButton a{"A"},b{"B"},switchButton{"SWITCH"};
    juce::TextButton eqTab{"MATCH EQ"},loopTab{"LOOP"};
    juce::TextButton clearLoop{"CLEAR"},zoomMinus{"-"},zoomPlus{"+"};
    juce::TextButton play{"PLAY"},toneButton{"TONE EQ"},toneReset{"RESET TONE"};
    juce::TextButton recordMix{"RECORD MIX"},recordRef{"RECORD REF"},match{"MATCH"},reset{"RESET"};
    juce::TextButton lowType{"SHELF"},highType{"SHELF"};
    juce::ToggleButton eqOn{"EQ ON"},toneOn{"TONE ON"},quickLoop{"LOOP"};
    juce::TextButton matchState{"MATCH ON"};
    juce::ComboBox graphRange;
    float graphScale=24.f;
    juce::Slider gain,amount,smooth,midQ,loopZoom;
    std::array<juce::Slider,6> tone;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,6> toneAttachments;
    juce::TextEditor inTime,outTime;
    juce::TextButton back{"-5 s"},forward{"+5 s"},setIn{"SET IN"},setOut{"SET OUT"};
    LoopTimeline timeline;
    juce::Label status,mixProfile,refProfile,position;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttach,amountAttach,smoothAttach,midQAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> eqAttach,toneOnAttach,lowShelfAttach,highShelfAttach;
    juce::TooltipWindow tips{this,650};
    juce::String message;
    int page=1;
    bool showTone=true;
    double matchFlashUntil=0.0;
    enum class MatchDrag { none, low, high } matchDrag=MatchDrag::none;
};
