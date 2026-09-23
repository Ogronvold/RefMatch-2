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
    void setTone(const std::array<float,6>& value) {const juce::SpinLock::ScopedLockType guard(lock);tone=value;}
    void setToneEnabled(bool enabled) {toneEnabled.store(enabled);}
    void setToneTypes(bool lowIsShelf, bool highIsShelf) { lowShelf.store(lowIsShelf); highShelf.store(highIsShelf); }
    void setMidQ(float value) { midQ.store(value); }
    void setMatchRange(float lowHz, float highHz) { matchLow.store(lowHz); matchHigh.store(highHz); }
    void setSmoothing(float value) { smoothing.store(value); }
    void learn(const std::array<float,SpectrumAnalyser::bins>&,
               const std::array<float,SpectrumAnalyser::bins>&,double sampleRate);
    void process(juce::AudioBuffer<float>&);
    std::vector<float> getCurveDb(float displayAmount=-1.f) const;
    EQDesign::Gains getGains() const;
    void restoreGains(const EQDesign::Gains&);
    void refresh(); // Message-thread coefficient design, never called in process.
private:
    static constexpr int stages=EQDesign::bands+3;
    std::array<float,6> tone{{0,120,0,1000,0,8000}};
    struct Stage { double z1=0,z2=0; };
    std::array<std::array<Stage,stages>,2> states{};
    std::array<EQDesign::Coeff,stages> current{},target{},published{};
    EQDesign::Gains learned{};
    std::atomic<bool> toneEnabled{true}, lowShelf{true}, highShelf{true};
    std::atomic<float> amount{.6f},limit{4},smoothing{.35f},midQ{.75f},matchLow{20.f},matchHigh{20000.f};
    std::atomic<double> rate{48000};
    mutable juce::SpinLock lock;
    bool dirty=true;
    int rampRemaining=0;

    double matchWeight(double hz) const;
    EQDesign::Coeff toneCoeff(int band,double sr,float gain,float freq) const;
};
