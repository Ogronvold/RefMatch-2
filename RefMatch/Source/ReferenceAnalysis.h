#pragma once
#include <JuceHeader.h>
#include "SystemAudioCapture.h"
#include "SpectrumAnalyser.h"
#include "ReferenceAnalysisState.h"

// One consumer owns the system-audio FIFO. No dependency on the host audio callback.
class ReferenceAnalysis : public ReferenceAnalysisState, private juce::Thread {
public:
    ReferenceAnalysis():juce::Thread("RefMatch reference analysis") {startThread();}
    ~ReferenceAnalysis() override {signalThreadShouldExit();notify();stopThread(-1);capture.stop();}
    SystemAudioCapture capture;
private:
    void run() override {
        juce::AudioBuffer<float> audio(2,512);
        double lastPacket=0;
        while(!threadShouldExit()) {
            const auto now=juce::Time::getMillisecondCounterHiRes();
            if(capture.pullAudio(audio,sampleRate)) {
                lastPacket=now;acceptAudio(audio);
            } else if(now-lastPacket>150) {
                present.store(false);peak.store(peak.load()*.95f);rms.store(rms.load()*.98f);
            }
            wait(5);
        }
    }
};
