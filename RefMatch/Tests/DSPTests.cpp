#include <JuceHeader.h>
#include "MatchEQ.h"
#include "LearnCapture.h"
#include <iostream>
#include <cstdlib>
void require(bool value,const char* message){if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
int main()
{
    constexpr double sr=48000;
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
    eq.setAmount(0);eq.refresh();
    for(int block=0;block<100;++block){mix.clear();eq.process(mix);}
    for(int i=0;i<512;++i)mix.setSample(0,i,float(.1*std::sin(i*.2)));
    juce::AudioBuffer<float> original;original.makeCopyOf(mix);eq.process(mix);
    for(int i=0;i<512;++i)require(std::abs(mix.getSample(0,i)-original.getSample(0,i))<.0001,"zero amount is transparent");
    std::cout<<"PASS: profile capture, silence, stereo power, frozen profiles, persistence on prepare, audible EQ and zero amount\n";
}
