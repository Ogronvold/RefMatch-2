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

double MatchEQ::matchWeight(double hz) const
{
    const double low=std::max(20.0,double(matchLow.load()));
    const double high=std::max(low*1.01,double(matchHigh.load()));
    if(hz<=low/1.5 || hz>=high*1.5) return 0.0;
    auto smoothStep=[](double x){x=std::clamp(x,0.0,1.0);return x*x*(3.0-2.0*x);};
    double w=1.0;
    if(hz<low) w*=smoothStep(std::log(hz/(low/1.5))/std::log(1.5));
    if(hz>high) w*=smoothStep(std::log((high*1.5)/hz)/std::log(1.5));
    return std::clamp(w,0.0,1.0);
}

EQDesign::Coeff MatchEQ::toneCoeff(int band,double sr,float gain,float freq) const
{
    if(band==0 && lowShelf.load()) return EQDesign::lowShelf(sr,freq,gain);
    if(band==2 && highShelf.load()) return EQDesign::highShelf(sr,freq,gain);
    return EQDesign::peak(sr,freq,gain,band==1?midQ.load():.75f);
}

void MatchEQ::refresh()
{
    const auto sr=rate.load();
    auto gains=EQDesign::scaled(getGains(),amount.load(),limit.load(),sr);
    for(int b=0;b<EQDesign::bands;++b)gains[b]*=matchWeight(EQDesign::centre(b));
    std::array<EQDesign::Coeff,stages> coeff{};
    for(int b=0;b<EQDesign::bands;++b)coeff[b]=EQDesign::peak(sr,EQDesign::centre(b),gains[b]);
    const juce::SpinLock::ScopedLockType guard(lock);
    for(int i=0;i<3;++i)coeff[EQDesign::bands+i]=toneCoeff(i,sr,toneEnabled.load()?tone[2*i]:0.f,tone[2*i+1]);
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
    const auto sr=rate.load();auto gains=EQDesign::scaled(getGains(),displayAmount<0?amount.load():displayAmount,limit.load(),sr);
    for(int b=0;b<EQDesign::bands;++b)gains[b]*=matchWeight(EQDesign::centre(b));
    std::array<float,6> manual;{const juce::SpinLock::ScopedLockType guard(lock);manual=tone;}
    if(!toneEnabled.load())for(int i=0;i<3;++i)manual[2*i]=0;
    std::vector<float> result(180);
    for(int i=0;i<180;++i) {
        const double hz=20*std::pow(std::min(20000.,sr*.45)/20.,i/179.);
        double db=0;for(int b=0;b<EQDesign::bands;++b)db+=EQDesign::response(EQDesign::peak(sr,EQDesign::centre(b),gains[b]),hz,sr);
        for(int band=0;band<3;++band)db+=EQDesign::response(toneCoeff(band,sr,manual[2*band],manual[2*band+1]),hz,sr);
        result[i]=float(db);
    }return result;
}
