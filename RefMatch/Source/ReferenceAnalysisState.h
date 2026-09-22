#pragma once
#include <JuceHeader.h>
#include "SpectrumAnalyser.h"
#include "LearnCapture.h"
class ReferenceAnalysisState {
public:
    LearnCapture learning;
    SpectrumAnalyser analyser;
    std::atomic<float> peak{0},rms{0};
    std::atomic<bool> present{false};
    static constexpr double sampleRate=48000.;
    void acceptAudio(const juce::AudioBuffer<float>& audio) {
        present.store(true);analyser.pushBlock(audio);learning.push(audio,audio,true,sampleRate);
        const int n=audio.getNumSamples();
        float p=0,r=0;
        for(int ch=0;ch<audio.getNumChannels();++ch){p=std::max(p,audio.getMagnitude(ch,0,n));r+=audio.getRMSLevel(ch,0,n);}
        r/=std::max(1,audio.getNumChannels());
        peak.store(std::max(p,peak.load()*.93f));rms.store(rms.load()*.985f+r*.015f);
    }
};
