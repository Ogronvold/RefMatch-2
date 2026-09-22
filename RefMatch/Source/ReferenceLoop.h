#pragma once
#include <JuceHeader.h>
#include "SystemMediaController.h"

class ReferenceLoop : private juce::Timer
{
public:
    explicit ReferenceLoop(SystemMediaController& c):controller(c) {startTimer(150);}
    ~ReferenceLoop() override {stopTimer();lifetime.reset();}
    bool setRange(double start,double end) {
        if(!std::isfinite(start)||!std::isfinite(end)||start<0||end-start<.5||end>86400) {
            message="Use valid In/Out times, at least 0.5 s apart";enabled=false;return false;
        }
        in=start;out=end;enabled=false;message="Loop points set";return true;
    }
    void enable(bool value) {
        enabled=value;track.clear();verifySeek=false;firstSeek=true;
        message=value?"Checking player position...":"Loop off";
    }
    bool isEnabled() const {return enabled;}
    void setTimed(bool value) {timed=value;firstSeek=true;verifySeek=false;}
    bool isTimed() const {return timed;}
    void setAuditioning(bool value) {auditioning=value;}
    double getIn() const {return in;}
    double getOut() const {return out;}
    SystemMediaController::MediaPosition getPosition() const {return position;}
    juce::String getStatus() const {return message;}
private:
    void timerCallback() override {
        if(enabled && timed) {
            if(!auditioning) {firstSeek=true;message="Timed loop armed - select B";}
            else {
                const double now=juce::Time::getMillisecondCounterHiRes();
                if(firstSeek || now-seekAt>=(out-in)*1000) {
                    firstSeek=false;seekAt=now;
                    if(!controller.seekTo(in)) {enabled=false;message="Seek unavailable - loop disabled";}
                    else message="Timed loop - position / seek unverified";
                }
            }
        }
        if(pending)return;
        pending=true;
        std::weak_ptr<int> weak=lifetime;
        controller.readPosition([this,weak](SystemMediaController::MediaPosition value) {
            if(weak.expired())return;
            pending=false;position=value;
            if(!enabled || timed)return;
            if(!value.valid) {enabled=false;message="Position unavailable - loop disabled; A/B still works";return;}
            if(track.isEmpty())track=value.track;
            if(track!=value.track) {enabled=false;message="Track changed - set new loop points";return;}
            if(value.duration>0 && out>value.duration) {enabled=false;message="Loop Out is beyond this track";return;}
            if(!auditioning || !value.playing) {message="Loop armed - select B to listen";return;}
            const auto now=juce::Time::getMillisecondCounterHiRes();
            if(verifySeek) {
                if(value.seconds>=in-.6 && value.seconds<=in+1.0) {verifySeek=false;message="Loop active";}
                else if(now-seekAt>1500) {enabled=false;message="Player did not confirm seek - loop disabled";}
                return;
            }
            if(firstSeek || value.seconds>=out) {
                firstSeek=false;
                if(!controller.seekTo(in)) {enabled=false;message="Seek unavailable - loop disabled";return;}
                verifySeek=true;seekAt=now;message="Returning to Loop In...";
            } else message="Loop active";
        });
    }
    SystemMediaController& controller;
    std::shared_ptr<int> lifetime=std::make_shared<int>(0);
    SystemMediaController::MediaPosition position;
    bool timed=false,enabled=false,auditioning=false,pending=false,firstSeek=true,verifySeek=false;
    double in=0,out=30,seekAt=0;
    juce::String track,message="Set In / Out, then enable loop";
};
