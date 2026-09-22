#pragma once
#include <algorithm>
#include <cmath>
#include "LoopTiming.h"

// Only explicit enable(false) clears the user's loop selection.
class PersistentLoop {
public:
    enum class State { off,waiting,active,seeking };
    bool setRange(double a,double b) {
        if(!std::isfinite(a)||!std::isfinite(b)||a<0||b-a<.5||b>86400)return false;
        if(a!=in || b!=out){in=a;out=b;restart();}return true;
    }
    void enable(bool value){enabled=value;restart();state=value?State::waiting:State::off;}
    void restart(){first=true;verifying=false;retryAt=0;}
    bool isEnabled() const{return enabled;}
    double getIn() const{return in;}
    double getOut() const{return out;}
    State getState() const{return state;}
    bool update(bool valid,bool auditioning,bool playing,double position,double duration,double now) {
        if(!enabled){state=State::off;return false;}
        if(!valid || !auditioning || !playing || (duration>0 && out>duration)) {
            state=State::waiting;verifying=false;return false;
        }
        if(verifying) {
            if(LoopTiming::confirmsSeek(position,in,out,now-seekAt)) {verifying=false;first=false;}
            else if(LoopTiming::missingExpired(now,seekAt)) {verifying=false;first=true;retryAt=now+1000;}
            state=verifying?State::seeking:State::waiting;
            if(verifying)return false;
        }
        if(first || position>=out || position<in-.1) {
            state=State::waiting;
            if(now<retryAt)return false;
            state=State::seeking;return true;
        }
        state=State::active;return false;
    }
    void seekResult(bool delivered,double now) {
        if(!enabled)return;
        verifying=delivered;seekAt=now;first=!delivered;
        if(!delivered)retryAt=now+1000;
        state=delivered?State::seeking:State::waiting;
    }
    double manualTarget(double seconds,double duration) const {
        if(enabled && (duration<=0 || out<=duration))return std::clamp(seconds,in,std::max(in,out-.05));
        return std::clamp(seconds,0.,duration>0?duration:86400.);
    }
    void manualSeek(){verifying=false;first=false;retryAt=0;state=enabled?State::waiting:State::off;}
private:
    bool enabled=false,first=true,verifying=false;
    double in=0,out=30,seekAt=0,retryAt=0;
    State state=State::off;
};
