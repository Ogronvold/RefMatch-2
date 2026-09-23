#pragma once
#include <JuceHeader.h>
#include "SystemMediaController.h"
#include "PersistentLoop.h"

class ReferenceLoop : private juce::Timer {
public:
    explicit ReferenceLoop(SystemMediaController& c):controller(c){startTimer(150);}
    ~ReferenceLoop() override {stopTimer();lifetime.reset();}
    bool setRange(double start,double end) {
        if(!loop.setRange(start,end)){message="Use valid In/Out times, at least 0.5 s apart";return false;}
        message="Loop points set";return true;
    }
    void enable(bool value){loop.enable(value);message=value?"LOOP WAITING - checking player":"Loop off";}
    void clear(){loop.clear();message="Loop cleared";}
    bool seek(double seconds) {
        if(!position.valid || !std::isfinite(seconds))return false;
        seconds=loop.manualTarget(seconds,position.duration);
        if(!controller.seekTo(seconds)){message="Seek unavailable - loop remains armed";return false;}
        loop.manualSeek();position.seconds=seconds;lastManualSeek=juce::Time::getMillisecondCounterHiRes();return true;
    }
    bool skip(double delta){return seek(position.seconds+delta);}
    bool isEnabled() const{return loop.isEnabled();}
    bool isActive() const{return loop.getState()==PersistentLoop::State::active;}
    void setAuditioning(bool value){auditioning=value;}
    double getIn() const{return loop.getIn();}
    double getOut() const{return loop.getOut();}
    SystemMediaController::MediaPosition getPosition() const{return position;}
    juce::String getStatus() const{return message;}
private:
    void timerCallback() override {
        if(pending)return;pending=true;
        std::weak_ptr<int> weak=lifetime;
        controller.readPosition([this,weak](SystemMediaController::MediaPosition value) {
            if(weak.expired())return;pending=false;
            const auto now=juce::Time::getMillisecondCounterHiRes();
            if(value.valid) {
                if(value.track!=track){track=value.track;loop.restart();}
                if(now-lastManualSeek<600 && value.track==position.track)value.seconds=position.seconds;
                position=value;
            }else {
                position.valid=false;position.playbackKnown=value.playbackKnown;position.playing=value.playing;
                if(value.title.isNotEmpty()){position.title=value.title;position.artist=value.artist;position.artwork=value.artwork;}
            }
            const bool seekNow=loop.update(value.valid,auditioning,value.playing,value.seconds,value.duration,now);
            if(!loop.isEnabled())return;
            if(seekNow) {
                const bool delivered=controller.seekTo(loop.getIn());loop.seekResult(delivered,now);
                message=delivered?"LOOP WAITING - returning to In":"LOOP WAITING - retrying player seek";
            }else if(!value.valid)message="LOOP WAITING - player position unavailable";
            else if(value.duration>0 && loop.getOut()>value.duration)message="LOOP WAITING - adjust Out for this track";
            else if(!auditioning || !value.playing)message="LOOP WAITING - select B and play";
            else message=isActive()?"LOOP ACTIVE":"LOOP WAITING - confirming player seek";
        });
    }
    SystemMediaController& controller;
    PersistentLoop loop;
    std::shared_ptr<int> lifetime=std::make_shared<int>(0);
    SystemMediaController::MediaPosition position;
    bool auditioning=false,pending=false;
    double lastManualSeek=-10000;
    juce::String track,message="Set In / Out, then enable loop";
};
