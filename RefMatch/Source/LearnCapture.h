#pragma once
#include <JuceHeader.h>
#include "SpectrumAnalyser.h"

class LearnCapture
{
public:
    enum Side { none=0,mix=1,reference=2 };
    struct Profile {
        std::array<float,SpectrumAnalyser::bins> db{};
        double seconds=0,sampleRate=48000;
        bool ready=false;
    };
    void start(Side side) { requested.store(side);++revision;const juce::SpinLock::ScopedLockType guard(lock);profiles[side==reference?1:0]={}; }
    void stop() { requested.store(none);++revision; }
    Side active() const { return Side(requested.load()); }
    Profile get(Side side) const {
        const juce::SpinLock::ScopedLockType guard(lock);return profiles[side==reference?1:0];
    }
    void restore(Side side,const Profile& profile) {
        const juce::SpinLock::ScopedLockType guard(lock);profiles[side==reference?1:0]=profile;
    }
    void prepare() { position=0;lastRevision=-1; }
    void push(const juce::AudioBuffer<float>& mixAudio,const juce::AudioBuffer<float>& refAudio,bool gotRef,double sr)
    {
        const int rev=revision.load();const auto side=active();
        if(rev!=lastRevision) {
            lastRevision=rev;position=0;sum.fill(0);frames=0;
            if(side!=none) {const juce::SpinLock::ScopedTryLockType g(lock);if(g.isLocked())profiles[side==reference?1:0]={};}
        }
        if(side==none || (side==reference && !gotRef))return;
        const auto& buffer=side==mix?mixAudio:refAudio;
        for(int i=0;i<buffer.getNumSamples();++i) {
            for(int ch=0;ch<2;++ch)data[ch][position]=buffer.getSample(std::min(ch,buffer.getNumChannels()-1),i);
            if(++position!=SpectrumAnalyser::fftSize)continue;
            position=0;
            std::array<double,SpectrumAnalyser::bins> power{};
            for(int ch=0;ch<2;++ch) {
                auto block=data[ch];window.multiplyWithWindowingTable(block.data(),SpectrumAnalyser::fftSize);
                fft.performFrequencyOnlyForwardTransform(block.data());
                for(int b=1;b<SpectrumAnalyser::bins;++b) {
                    const double v=block[b]/SpectrumAnalyser::fftSize;power[b]+=.5*v*v;
                }
            }
            double total=0;for(auto v:power)total+=v;
            if(total<1.e-10)continue; // Do not learn silence or a disconnected source.
            for(int b=1;b<SpectrumAnalyser::bins;++b)sum[b]+=power[b];
            ++frames;
            Profile profile;profile.sampleRate=sr;profile.seconds=double(frames)*SpectrumAnalyser::fftSize/sr;
            profile.ready=profile.seconds>=.5;
            for(int b=0;b<SpectrumAnalyser::bins;++b)profile.db[b]=float(10*std::log10(std::max(1.e-12,sum[b]/frames)));
            const juce::SpinLock::ScopedTryLockType guard(lock);
            if(guard.isLocked() && revision.load()==rev)profiles[side==reference?1:0]=profile;
        }
    }
private:
    std::atomic<int> requested{none},revision{0};
    int lastRevision=-1,position=0,frames=0;
    std::array<std::array<float,SpectrumAnalyser::fftSize*2>,2> data{};
    std::array<double,SpectrumAnalyser::bins> sum{};
    juce::dsp::FFT fft{SpectrumAnalyser::fftOrder};
    juce::dsp::WindowingFunction<float> window{SpectrumAnalyser::fftSize,juce::dsp::WindowingFunction<float>::hann,true};
    mutable juce::SpinLock lock;
    std::array<Profile,2> profiles{};
};
