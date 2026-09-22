#pragma once
// Message-thread model: show a requested transport change immediately, then reconcile
// with player metadata. Never treat command acceptance as confirmed playback.
class PlaybackFeedback {
public:
    void request(bool playing,double now) {wanted=playing?1:0;requestedAt=now;}
    void failed() {wanted=-1;}
    void report(int value,double now) {
        reported=value;reportedAt=now;
        if(value==wanted)wanted=-1;
    }
    bool pending(double now) const {return wanted>=0 && now-requestedAt<5000;}
    int state(double now) const {
        if(pending(now))return wanted;
        return now-reportedAt<3000?reported:-1;
    }
private:
    int wanted=-1,reported=-1;
    double requestedAt=-10000,reportedAt=-10000;
};
