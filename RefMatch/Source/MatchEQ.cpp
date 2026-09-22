#include "MatchEQ.h"
void MatchEQ::prepare(double sr,int,int)
{
    rate.store(sr);states={};current={};target={};rampRemaining=0;
    {const juce::SpinLock::ScopedLockType guard(lock);dirty=true;}
    // Do not erase learned gains on prepareToPlay/sample-rate changes.
}
void MatchEQ::reset() { restoreGains({}); }
EQDesign::Gains MatchEQ::getGains() const
{
    const juce::SpinLock::ScopedLockType guard(lock);return learned;
}
void MatchEQ::restoreGains(const EQDesign::Gains& gains)
{
    {const juce::SpinLock::ScopedLockType guard(lock);learned=gains;}refresh();
}
void MatchEQ::learn(const std::array<float,SpectrumAnalyser::bins>& mix,
                   const std::array<float,SpectrumAnalyser::bins>& ref,double sr)
{
    EQDesign::Gains a{},b{};
    for(int band=0;band<EQDesign::bands;++band) {
        const double lo=EQDesign::centre(band)/1.18,hi=EQDesign::centre(band)*1.18;
        int count=0;
        for(int i=1;i<SpectrumAnalyser::bins;++i) {
            const double hz=i*sr/SpectrumAnalyser::fftSize;
            if(hz>=lo && hz<=hi) {a[band]+=mix[i];b[band]+=ref[i];++count;}
        }
        if(count) {a[band]/=count;b[band]/=count;}
        else {const int i=std::clamp(int(EQDesign::centre(band)*SpectrumAnalyser::fftSize/sr),1,SpectrumAnalyser::bins-1);a[band]=mix[i];b[band]=ref[i];}
    }
    restoreGains(EQDesign::fit(a,b,sr,smoothing.load()));
}
void MatchEQ::refresh()
{
    const auto sr=rate.load();
    const auto gains=EQDesign::scaled(getGains(),amount.load(),1000.,sr);
    std::array<EQDesign::Coeff,stages> coeff{};
    for(int b=0;b<EQDesign::bands;++b)coeff[b]=EQDesign::peak(sr,EQDesign::centre(b),gains[b]);
    const juce::SpinLock::ScopedLockType guard(lock);
    for(int i=0;i<3;++i)coeff[EQDesign::bands+i]=EQDesign::peak(sr,tone[2*i+1],tone[2*i],.75);
    published=coeff;dirty=true;
}
void MatchEQ::process(juce::AudioBuffer<float>& buffer)
{
    {const juce::SpinLock::ScopedTryLockType guard(lock);
     if(guard.isLocked() && dirty) {target=published;dirty=false;rampRemaining=std::max(1,int(rate.load()*.020));}}
    for(int i=0;i<buffer.getNumSamples();++i) {
        if(rampRemaining>0) {
            const double step=1./rampRemaining;
            for(int b=0;b<stages;++b) {
                auto& c=current[b];const auto& t=target[b];
                c.b0+=(t.b0-c.b0)*step;c.b1+=(t.b1-c.b1)*step;c.b2+=(t.b2-c.b2)*step;
                c.a1+=(t.a1-c.a1)*step;c.a2+=(t.a2-c.a2)*step;
            }--rampRemaining;
        }
        for(int ch=0;ch<std::min(2,buffer.getNumChannels());++ch) {
            double x=buffer.getSample(ch,i);
            for(int b=0;b<stages;++b) {
                const auto& c=current[b];auto& st=states[ch][b];
                const double y=c.b0*x+st.z1;
                st.z1=c.b1*x-c.a1*y+st.z2;st.z2=c.b2*x-c.a2*y;x=y;
            }
            buffer.setSample(ch,i,float(x));
        }
    }
}
std::vector<float> MatchEQ::getCurveDb(float displayAmount) const
{
    const auto sr=rate.load();const auto gains=EQDesign::scaled(getGains(),displayAmount<0?amount.load():displayAmount,1000.,sr);
    std::array<float,6> manual;{const juce::SpinLock::ScopedLockType guard(lock);manual=tone;}
    std::vector<float> result(180);
    for(int i=0;i<180;++i) {
        const double hz=20*std::pow(std::min(20000.,sr*.45)/20.,i/179.);
        double db=0;for(int b=0;b<EQDesign::bands;++b)db+=EQDesign::response(EQDesign::peak(sr,EQDesign::centre(b),gains[b]),hz,sr);
        for(int band=0;band<3;++band)db+=EQDesign::response(EQDesign::peak(sr,manual[2*band+1],manual[2*band],.75),hz,sr);
        result[i]=float(db);
    }return result;
}
