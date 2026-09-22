#pragma once
#include <JuceHeader.h>
#include "LoopSelection.h"
class LoopTimeline : public juce::Component
{
public:
    std::function<void(double,double)> onRange;
    std::function<void(double)> onSeek;
    void update(double time,double length,double start,double end,bool active,bool valid,const juce::String& identity) {
        if(dragging && identity!=track)dragging=false;
        track=identity;cursor=time;duration=length;enabled=active;available=valid && length>=.5;
        if(!dragging){const auto range=LoopSelection::drag(start,end,length);in=range.start;out=range.end;}
        repaint();
    }
    void paint(juce::Graphics& g) override {
        const auto r=getLocalBounds().toFloat().reduced(8,18);
        g.setColour(juce::Colour(0xff101217));g.fillRoundedRectangle(getLocalBounds().toFloat(),8);
        g.setColour(juce::Colour(0xff242830));g.fillRoundedRectangle(r,4);
        if(duration>0) {
            const auto x=[&](double value){return r.getX()+float(std::clamp(value/duration,0.,1.))*r.getWidth();};
            const float left=x(in),right=x(out);
            g.setColour(juce::Colour(0xff76e5d1).withAlpha(enabled?.35f:.14f));
            g.fillRect(juce::Rectangle<float>(left,r.getY(),std::max(0.f,right-left),r.getHeight()));
            g.setColour(juce::Colour(0xff76e5d1));
            g.fillRect(left-2,r.getY(),4.f,r.getHeight());g.fillRect(right-2,r.getY(),4.f,r.getHeight());
            g.setColour(juce::Colour(0xffeef1f7));g.fillRect(x(cursor)-1,r.getY()-3,2.f,r.getHeight()+6);
            g.setColour(juce::Colour(0xff808897));g.setFont(juce::Font(juce::FontOptions(10)));
            for(int i=0;i<=4;++i) {
                const int sec=int(duration*i/4);const juce::String time=juce::String(sec/60)+":"+juce::String(sec%60).paddedLeft('0',2);
                const int labelX=int(std::clamp(x(duration*i/4)-20,0.f,float(getWidth()-40)));
                g.drawText(time,labelX,getHeight()-16,40,14,juce::Justification::centred);
            }
        }
        if(!available) {g.setColour(juce::Colour(0xff808897));g.setFont(juce::Font(juce::FontOptions(11)));g.drawText("Waiting for player position",getLocalBounds(),juce::Justification::centred);}
    }
    void mouseDown(const juce::MouseEvent& e) override {
        if(!available)return;
        dragging=true;downX=e.position.x;anchor=at(e.position.x);originalIn=in;originalOut=out;
        const float width=float(getWidth()-16);
        const float leftDistance=std::abs(e.position.x-(8+float(in/duration)*width));
        const float rightDistance=std::abs(e.position.x-(8+float(out/duration)*width));
        mode=std::min(leftDistance,rightDistance)<8?(leftDistance<=rightDistance?1:2):0;
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if(!dragging || !available)return;
        const double point=at(e.position.x);
        if(mode==1)in=std::clamp(point,0.,std::max(0.,out-.5));
        else if(mode==2)out=std::clamp(point,std::min(duration,in+.5),duration);
        else {const auto range=LoopSelection::drag(anchor,point,duration);in=range.start;out=range.end;}
        repaint();
    }
    void mouseUp(const juce::MouseEvent& e) override {
        if(!dragging)return;
        dragging=false;
        if(!available){in=originalIn;out=originalOut;repaint();return;}
        if(mode==0 && std::abs(e.position.x-downX)<4) {
            in=originalIn;out=originalOut;if(onSeek)onSeek(at(e.position.x));
        } else if(onRange)onRange(in,out);
        repaint();
    }
private:
    double at(float x) const {return LoopSelection::seconds(x-8,float(getWidth()-16),duration);}
    double cursor=0,duration=0,in=0,out=30,anchor=0,originalIn=0,originalOut=30;
    float downX=0;int mode=0;bool enabled=false,available=false,dragging=false;
    juce::String track;
};
