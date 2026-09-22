#pragma once
#include <JuceHeader.h>
#include "SystemMediaController.h"
#include "LoopTiming.h"

class ReferenceLoop : private juce::Timer
{
public:
    explicit ReferenceLoop(SystemMediaController& c):controller(c) {startTimer(150);}
    ~ReferenceLoop() override {stopTimer();lifetime.reset();}
    bool setRange(double start,double end) {
        if(!std::isfinite(start)||!std::isfinite(end)||start<0||end-start<.5||end>86400) {
            message="Use valid In/Out times, at least 0.5 s apart";enabled=false;return false;
        }
        if(start==in && end==out)return true;
        in=start;out=end;firstSeek=true;verifySeek=false;message="Loop points set";return true;
    }
    void enable(bool value) {
        enabled=value;track.clear();verifySeek=false;firstSeek=true;
        message=value?"Checking player position...":"Loop off";
    }
    bool seek(double seconds) {
        if(!position.valid || !std::isfinite(seconds))return false;
        seconds=std::clamp(seconds,0.,position.duration>0?position.duration:86400.);
        if(!controller.seekTo(seconds)){message="Player seek unavailable";return false;}
        if(enabled && (seconds<in || seconds>=out)) {enabled=false;message="Loop off - seek outside selection";}
        firstSeek=false;verifySeek=false;position.seconds=seconds;lastManualSeek=juce::Time::getMillisecondCounterHiRes();
        return true;
    }
    bool skip(double delta) {return seek(position.seconds+delta);}
    bool isEnabled() const {return enabled;}
    void setAuditioning(bool value) {auditioning=value;}
    double getIn() const {return in;}
    double getOut() const {return out;}
    SystemMediaController::MediaPosition getPosition() const {return position;}
    juce::String getStatus() const {return message;}
private:
    void timerCallback() override {
        if(pending)return;
        pending=true;
        std::weak_ptr<int> weak=lifetime;
        controller.readPosition([this,weak](SystemMediaController::MediaPosition value) {
            if(weak.expired())return;
            pending=false;
            const double now=juce::Time::getMillisecondCounterHiRes();
            if(!value.valid) {
                position.valid=false; // Never seek using a stale position during the grace interval.
                if(missingSince==0)missingSince=now;
                if(!LoopTiming::missingExpired(now,missingSince)) {
                    position.playbackKnown=value.playbackKnown;position.playing=value.playing;
                    if(value.title.isNotEmpty()) {position.title=value.title;position.artist=value.artist;position.artwork=value.artwork;}
                    if(enabled)message="Waiting for player position...";return;
                }
            } else missingSince=0;
            if(value.valid && now-lastManualSeek<600 && value.track==position.track)value.seconds=position.seconds;
            position=value;
            if(!enabled)return;
            if(!value.valid) {enabled=false;message="Position unavailable - loop disabled; A/B still works";return;}
            if(track.isEmpty())track=value.track;
            if(track.isNotEmpty() && value.track.isNotEmpty() && track!=value.track) {enabled=false;message="Track changed - set new loop points";return;}
            if(value.duration>0 && out>value.duration) {enabled=false;message="Loop Out is beyond this track";return;}
            if(!auditioning || !value.playing) {message="Loop armed - select B to listen";return;}
            if(verifySeek) {
                if(LoopTiming::confirmsSeek(value.seconds,in,out,now-seekAt)) {verifySeek=false;message="Loop active";}
                else if(LoopTiming::missingExpired(now,seekAt)) {enabled=false;message="Player did not confirm seek - loop disabled";}
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
    bool enabled=false,auditioning=false,pending=false,firstSeek=true,verifySeek=false;
    double in=0,out=30,seekAt=0,missingSince=0,lastManualSeek=-10000;
    juce::String track,message="Set In / Out, then enable loop";
};
