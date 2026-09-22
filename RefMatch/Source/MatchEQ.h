#pragma once
#include <JuceHeader.h>
#include "SpectrumAnalyser.h"
#include "EQDesign.h"

class MatchEQ
{
public:
    void prepare(double sampleRate,int,int);
    void reset(); // Publish flat target; audio filter states remain audio-owned.
    void setAmount(float value) { amount.store(value); }
    void setMaxCorrectionDb(float value) { limit.store(value); }
    void setSmoothing(float value) { smoothing.store(value); }
    void learn(const std::array<float,SpectrumAnalyser::bins>&,
               const std::array<float,SpectrumAnalyser::bins>&,double sampleRate);
    void process(juce::AudioBuffer<float>&);
    std::vector<float> getCurveDb() const;
    EQDesign::Gains getGains() const;
    void restoreGains(const EQDesign::Gains&);
    void refresh(); // Message-thread coefficient design, never called in process.
private:
    struct Stage { double z1=0,z2=0; };
    std::array<std::array<Stage,EQDesign::bands>,2> states{};
    std::array<EQDesign::Coeff,EQDesign::bands> current{},target{},published{};
    EQDesign::Gains learned{};
    std::atomic<float> amount{.6f},limit{4},smoothing{.35f};
    std::atomic<double> rate{48000};
    mutable juce::SpinLock lock;
    bool dirty=true;
    int rampRemaining=0;
};
