#include <JuceHeader.h>
#include "MatchEQ.h"
#include "LearnCapture.h"
#include "ReferenceAnalysisState.h"
#include <iostream>
#include <cstdlib>
void require(bool value,const char* message){if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
int main()
{
    constexpr double sr=48000;
    ReferenceAnalysisState independentReference;
    independentReference.learning.start(LearnCapture::reference);
    juce::AudioBuffer<float> external(2,512);
    for(int block=0;block<100;++block) {
        for(int i=0;i<512;++i)for(int ch=0;ch<2;++ch)external.setSample(ch,i,float(.2*std::sin(2*juce::MathConstants<double>::pi*1000*(block*512+i)/sr)));
        independentReference.acceptAudio(external);
    }
    require(independentReference.learning.get(LearnCapture::reference).ready,"REF profile records with no host processBlock calls");
    require(independentReference.peak.load()>.15f && independentReference.present.load(),"REF meter updates independently of host playback");
    LearnCapture capture;
    juce::AudioBuffer<float> mix(2,512),ref(2,512);
    auto feed=[&](bool colour,int blocks) {
        for(int block=0;block<blocks;++block) {
            for(int i=0;i<512;++i) {
                const double t=(block*512+i)/sr;
                const float lo=float(std::sin(2*juce::MathConstants<double>::pi*300*t));
                const float hi=float(std::sin(2*juce::MathConstants<double>::pi*6000*t));
                for(int ch=0;ch<2;++ch){mix.setSample(ch,i,.2f*(lo+hi));ref.setSample(ch,i,(ch? -1.f:1.f)*.2f*((colour?.3f:1.f)*lo+hi));}
            }
            capture.push(mix,ref,true,sr);
        }
    };
    capture.start(LearnCapture::mix);feed(false,100);capture.stop();
    const auto a=capture.get(LearnCapture::mix);
    require(a.ready,"MIX capture completes");
    capture.start(LearnCapture::reference);feed(true,100);capture.stop();
    const auto b=capture.get(LearnCapture::reference);
    require(b.ready,"out-of-phase stereo reference still captures power");
    require(capture.get(LearnCapture::mix).db==a.db,"recording REF preserves frozen MIX");
    capture.start(LearnCapture::reference);mix.clear();ref.clear();
    for(int i=0;i<100;++i)capture.push(mix,ref,true,sr);
    capture.stop();require(!capture.get(LearnCapture::reference).ready,"silence never creates usable profile");
    capture.restore(LearnCapture::reference,b);
    MatchEQ eq;eq.prepare(sr,512,2);eq.learn(a.db,b.db,sr);
    auto learned=eq.getGains();double maximum=0;for(auto gain:learned)maximum=std::max(maximum,std::abs(gain));
    require(maximum>.5,"distinct recorded spectra produce nonflat EQ");
    eq.prepare(sr,512,2);require(eq.getGains()==learned,"prepare preserves learned match");
    // Exercise actual audio processing, not just a displayed target curve.
    EQDesign::Gains one{};one[10]=3;
    eq.setAmount(1);eq.setMaxCorrectionDb(.5f);eq.restoreGains(one);
    const double frequency=EQDesign::centre(10);double inputEnergy=0,outputEnergy=0;
    for(int block=0;block<200;++block) {
        for(int i=0;i<512;++i)for(int ch=0;ch<2;++ch)mix.setSample(ch,i,float(.1*std::sin(2*juce::MathConstants<double>::pi*frequency*(block*512+i)/sr)));
        if(block>=100)for(int i=0;i<512;++i)inputEnergy+=mix.getSample(0,i)*mix.getSample(0,i);
        eq.process(mix);
        for(int i=0;i<512;++i)require(std::isfinite(mix.getSample(0,i)),"EQ output finite");
        if(block>=100)for(int i=0;i<512;++i)outputEnergy+=mix.getSample(0,i)*mix.getSample(0,i);
    }
    const double measured=10*std::log10(outputEnergy/inputEnergy);
    require(std::abs(measured-3)<.15,"100% Amount applies full 3 dB correction despite legacy saved limit");
    const auto fullScaleCurve=eq.getCurveDb(1.f);
    eq.setAmount(.5f);const auto halfCurve=eq.getCurveDb();
    require(eq.getCurveDb(1.f)==fullScaleCurve,"graph full-scale response does not change with Amount");
    eq.setAmount(.75f);const auto threeQuarterCurve=eq.getCurveDb();
    eq.setAmount(1.f);const auto fullCurve=eq.getCurveDb();
    float halfMax=0,threeQuarterMax=0,fullMax=0;
    for(size_t i=0;i<fullCurve.size();++i){halfMax=std::max(halfMax,halfCurve[i]);threeQuarterMax=std::max(threeQuarterMax,threeQuarterCurve[i]);fullMax=std::max(fullMax,fullCurve[i]);}
    require(halfMax<threeQuarterMax && threeQuarterMax<fullMax,"applied graph continues moving from 50 through 75 to 100 percent");
    eq.setAmount(0);eq.refresh();
    for(int block=0;block<100;++block){mix.clear();eq.process(mix);}
    for(int i=0;i<512;++i)mix.setSample(0,i,float(.1*std::sin(i*.2)));
    juce::AudioBuffer<float> original;original.makeCopyOf(mix);eq.process(mix);
    for(int i=0;i<512;++i)require(std::abs(mix.getSample(0,i)-original.getSample(0,i))<.0001,"zero amount is transparent");
    eq.setAmount(0);eq.setTone({{3,1000,0,2000,0,8000}});eq.refresh();
    const auto manual=eq.getCurveDb();float manualPeak=0;for(auto db:manual)manualPeak=std::max(manualPeak,db);
    require(manualPeak>2.9,"manual Tone EQ remains active at zero Match Amount");
    inputEnergy=0;outputEnergy=0;
    for(int block=0;block<150;++block) {
        for(int i=0;i<512;++i)for(int ch=0;ch<2;++ch)mix.setSample(ch,i,float(.1*std::sin(2*juce::MathConstants<double>::pi*1000*(block*512+i)/sr)));
        if(block>50)for(int i=0;i<512;++i)inputEnergy+=mix.getSample(0,i)*mix.getSample(0,i);
        eq.process(mix);
        if(block>50)for(int i=0;i<512;++i)outputEnergy+=mix.getSample(0,i)*mix.getSample(0,i);
    }
    require(std::abs(10*std::log10(outputEnergy/inputEnergy)-3)<.15,"manual Tone EQ applies 3 dB to actual audio after matching");
    const auto enabledToneCurve=eq.getCurveDb();
    eq.setToneEnabled(false);eq.refresh();
    for(auto db:eq.getCurveDb())require(std::abs(db)<.0001,"Tone OFF removes manual response at zero Match Amount");
    for(int block=0;block<100;++block){mix.clear();eq.process(mix);}
    for(int i=0;i<512;++i)mix.setSample(0,i,float(.1*std::sin(i*.2)));
    original.makeCopyOf(mix);eq.process(mix);
    for(int i=0;i<512;++i)require(std::abs(mix.getSample(0,i)-original.getSample(0,i))<.0001,"Tone OFF passes actual audio after ramp settles");
    eq.setAmount(1);
    float matchOnlyPeak=0;for(auto db:eq.getCurveDb())matchOnlyPeak=std::max(matchOnlyPeak,db);
    require(matchOnlyPeak>2.9,"Tone OFF leaves the learned Match EQ active");
    eq.setAmount(0);eq.setToneEnabled(true);eq.refresh();
    require(eq.getCurveDb()==enabledToneCurve,"Tone ON restores the exact retained settings");
    std::cout<<"PASS: profile capture, silence, stereo power, frozen profiles, persistence on prepare, audible EQ and zero amount\n";
}
