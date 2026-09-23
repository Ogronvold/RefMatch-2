#pragma once
#include <JuceHeader.h>
#include "LoopSelection.h"

class LoopTimeline : public juce::Component
{
public:
    std::function<void(double,double)> onRange;
    std::function<void(double)> onSeek;

    void setZoom(double value) { zoom=std::clamp(value,1.0,6.0); repaint(); }
    double getZoom() const { return zoom; }

    void update(double time,double length,double start,double end,bool active,bool valid,const juce::String& identity) {
        if(dragging && identity!=track)dragging=false;
        track=identity;if(!dragging || mode!=3)cursor=time;duration=length;enabled=active;available=valid && length>=.5;
        if(!dragging){const auto range=LoopSelection::drag(start,end,length);in=range.start;out=range.end;}
        repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto bounds=getLocalBounds().toFloat();
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff111927),bounds.getTopLeft(),juce::Colour(0xff0b121d),bounds.getBottomRight(),false));
        g.fillRoundedRectangle(bounds,9.f);
        g.setColour(juce::Colour(0xff273348).withAlpha(.85f));g.drawRoundedRectangle(bounds.reduced(.6f),8.4f,1.f);

        auto r=bounds.reduced(12.f,18.f); r.setBottom(bounds.getBottom()-24.f);
        g.setColour(juce::Colour(0xff131d2c));g.fillRoundedRectangle(r,6.f);
        g.setColour(juce::Colour(0xff334058).withAlpha(.7f));g.drawRoundedRectangle(r,6.f,1.f);

        if(duration>0) {
            const auto window=visibleWindow();
            const auto x=[&](double value){return r.getX()+float((value-window.first)/(window.second-window.first))*r.getWidth();};
            const float left=std::clamp(x(in),r.getX(),r.getRight());
            const float right=std::clamp(x(out),r.getX(),r.getRight());

            // Decorative pseudo-waveform. The envelope is derived from absolute track time,
            // not the screen index, so zooming into the timeline visibly stretches the
            // waveform detail instead of redrawing the same pattern at every zoom level.
            const int bars=juce::jlimit(96,260,int(118.0*std::sqrt(zoom)));
            for(int i=0;i<bars;++i) {
                const float frac=bars>1?float(i)/float(bars-1):0.f;
                const float px=r.getX()+5.f+(r.getWidth()-10.f)*frac;
                const double t=window.first+(window.second-window.first)*double(frac);
                const double phase=t*2.0*juce::MathConstants<double>::pi;
                const double envelope=.48
                    + .22*std::sin(phase/7.3 + .4)
                    + .14*std::sin(phase/2.15 + 1.2)
                    + .08*std::sin(phase*.73 + 2.1)
                    + .05*std::sin(phase*1.91 + .7);
                const float a=juce::jlimit(5.f,24.f,7.f+18.f*float(std::abs(envelope)));
                const bool selected=t>=in && t<=out;
                auto c=selected?juce::Colour(0xffa65cf0):juce::Colour(0xff66738a);
                g.setColour(c.withAlpha(selected?.66f:.30f));
                g.drawLine(px,r.getCentreY()-a,px,r.getCentreY()+a,selected?1.35f:1.f);
            }

            if(right>left) {
                g.setGradientFill(juce::ColourGradient(juce::Colour(0xffffad42).withAlpha(.12f),left,r.getCentreY(),juce::Colour(0xffa64df2).withAlpha(.22f),right,r.getCentreY(),false));
                g.fillRect(juce::Rectangle<float>(left,r.getY(),right-left,r.getHeight()));
            }

            const auto orange=juce::Colour(0xffffad42), purple=juce::Colour(0xffa64df2);
            auto drawHandle=[&](float xx,juce::Colour c){
                g.setColour(c.withAlpha(.14f));g.fillEllipse(xx-10.f,r.getCentreY()-10.f,20.f,20.f);
                g.setColour(c);g.fillRoundedRectangle(xx-1.5f,r.getY()-7.f,3.f,r.getHeight()+14.f,1.5f);
                g.fillEllipse(xx-6.f,r.getCentreY()-6.f,12.f,12.f);
            };
            drawHandle(left,orange);drawHandle(right,purple);

            if(cursor>=window.first && cursor<=window.second) {
                const float cx=x(cursor);g.setColour(juce::Colours::white.withAlpha(.88f));g.fillRoundedRectangle(cx-.75f,r.getY()-3.f,1.5f,r.getHeight()+6.f,.75f);
            }

            g.setColour(juce::Colour(0xff8c96a8));g.setFont(juce::Font(juce::FontOptions(10.f)));
            for(int i=0;i<=4;++i) {
                const double sec=window.first+(window.second-window.first)*i/4.0;
                const juce::String time=juce::String(int(sec)/60)+":"+juce::String(int(sec)%60).paddedLeft('0',2);
                const float tx=r.getX()+r.getWidth()*i/4.f;
                g.setColour(juce::Colour(0xff708099));g.drawVerticalLine(int(tx),r.getBottom()+5.f,r.getBottom()+11.f);
                const int labelX=int(std::clamp(tx-22.f,0.f,float(getWidth()-44)));
                g.setColour(juce::Colour(0xff8c96a8));g.drawText(time,labelX,int(r.getBottom()+10.f),44,14,juce::Justification::centred);
            }
        }
        if(!available) {g.setColour(juce::Colour(0xff8b95a6));g.setFont(juce::Font(juce::FontOptions(11.f)));g.drawText("Waiting for player position",r,juce::Justification::centred);}
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if(!available)return;
        dragging=true;downX=e.position.x;anchor=at(e.position.x);originalIn=in;originalOut=out;
        auto r=getLocalBounds().toFloat().reduced(12.f,18.f); r.setBottom(float(getHeight())-24.f);
        const auto window=visibleWindow();
        const auto px=[&](double v){return r.getX()+float((v-window.first)/(window.second-window.first))*r.getWidth();};
        const float leftDistance=std::abs(e.position.x-px(in));
        const float rightDistance=std::abs(e.position.x-px(out));
        const float cursorDistance=std::abs(e.position.x-px(cursor));
        mode=LoopSelection::dragMode(cursorDistance,leftDistance,rightDistance);
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if(!dragging || !available)return;
        const double point=at(e.position.x);
        if(mode==3)cursor=point;
        else if(mode==1)in=std::clamp(point,0.,std::max(0.,out-.5));
        else if(mode==2)out=std::clamp(point,std::min(duration,in+.5),duration);
        else {const auto range=LoopSelection::drag(anchor,point,duration);in=range.start;out=range.end;}
        repaint();
    }
    void mouseUp(const juce::MouseEvent& e) override {
        if(!dragging)return;
        dragging=false;
        if(!available){in=originalIn;out=originalOut;repaint();return;}
        if(mode==3) {if(onSeek)onSeek(at(e.position.x));}
        else if(mode==0 && std::abs(e.position.x-downX)<4) {in=originalIn;out=originalOut;if(onSeek)onSeek(at(e.position.x));}
        else if(onRange)onRange(in,out);
        repaint();
    }
private:
    std::pair<double,double> visibleWindow() const {
        if(duration<=0)return {0.,1.};
        const double span=std::max(.5,duration/zoom);
        if(zoom<=1.001)return {0.,duration};
        double centre=(out>in)?(in+out)*.5:cursor;
        centre=std::clamp(centre,span*.5,std::max(span*.5,duration-span*.5));
        double start=std::clamp(centre-span*.5,0.,std::max(0.,duration-span));
        return {start,std::min(duration,start+span)};
    }
    double at(float x) const {
        auto r=getLocalBounds().toFloat().reduced(12.f,18.f); r.setBottom(float(getHeight())-24.f);
        const auto window=visibleWindow();
        const double norm=std::clamp(double((x-r.getX())/std::max(1.f,r.getWidth())),0.,1.);
        return window.first+norm*(window.second-window.first);
    }
    double cursor=0,duration=0,in=0,out=30,anchor=0,originalIn=0,originalOut=30,zoom=1.0;
    float downX=0;int mode=0;bool enabled=false,available=false,dragging=false;
    juce::String track;
};
