#pragma once
#include <JuceHeader.h>
#include "SpectrumAnalyser.h"

class MatchEQ
{
public:
    struct Zone { float lowHz = 20.0f, highHz = 20000.0f; bool enabled = true; };

    void prepare(double sampleRate, int maximumBlockSize, int channels);
    void reset();
    void setAmount(float newAmount) { amount.store(juce::jlimit(0.0f, 1.0f, newAmount)); }
    void setMaxCorrectionDb(float db) { maxCorrectionDb.store(juce::jlimit(0.1f, 12.0f, db)); }
    void setZones(std::vector<Zone> z);
    void learn(const std::array<float, SpectrumAnalyser::bins>& source,
               const std::array<float, SpectrumAnalyser::bins>& reference,
               double sampleRate);
    void process(juce::AudioBuffer<float>& buffer);
    const std::vector<float>& getCurveDb() const { return curveDb; }

private:
    bool frequencyAllowed(float hz) const;
    void updateFilters();

    double sr = 44100.0;
    std::atomic<float> amount { 0.5f };
    std::atomic<float> maxCorrectionDb { 3.0f };
    std::vector<Zone> zones {{20.0f, 20000.0f, true}};
    std::vector<float> curveDb;

    static constexpr int numBands = 12;
    std::array<float, numBands> centres { 32, 48, 75, 120, 190, 300, 480, 760, 1200, 2400, 5200, 11000 };
    std::array<float, numBands> bandGainsDb {};
    std::vector<std::array<juce::dsp::IIR::Filter<float>, numBands>> filters;
};
