#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LoopTimeline.h"

class RefMatchLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RefMatchLookAndFeel();
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
};
class RefMatchAudioProcessorEditor : public juce::AudioProcessorEditor,private juce::Timer
{
public:
    explicit RefMatchAudioProcessorEditor(RefMatchAudioProcessor&);
    ~RefMatchAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void setPage(int);
    void drawSpectrum(juce::Graphics&,juce::Rectangle<float>,bool eq);
    void transport(SystemMediaController::Command);
    void updateLoopRange();
    static double parseTime(const juce::String&);
    static juce::String timeText(double);
    static juce::String rangeText(double);
    RefMatchAudioProcessor& processor;
    RefMatchLookAndFeel look;
    juce::TextButton a{"A"},b{"B"},switchButton{"SWITCH"};
    juce::TextButton eqTab{"MATCH EQ"},loopTab{"LOOP"};
    juce::TextButton play{"PLAY"},meters{"METERS"},autoGain{"AUTO GAIN"};
    juce::TextButton recordMix{"RECORD MIX"},recordRef{"RECORD REF"},match{"MATCH"},reset{"RESET"};
    juce::ToggleButton eqOn{"EQ ON"};
    juce::Slider gain,amount;
    juce::TextEditor inTime,outTime;
    juce::TextButton back{"-5 s"},forward{"+5 s"},setIn{"SET IN"},setOut{"SET OUT"};
    juce::ToggleButton loopOn{"LOOP"};
    LoopTimeline timeline;
    juce::Label status,mixProfile,refProfile,position;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttach,amountAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> eqAttach;
    juce::TooltipWindow tips{this,650};
    juce::String message;
    int page=1;
};
