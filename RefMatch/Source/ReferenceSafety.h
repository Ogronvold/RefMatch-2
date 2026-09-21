#pragma once
// Audio-thread owned. Failures latch until MIX is selected, so a dead capture
// cannot repeatedly mute the DAW. Silence is valid; missing packets are not.
class ReferenceSafety
{
public:
    bool update(bool requested, bool running, bool freshPackets, int frames, double rate)
    {
        if (!requested) { failed = false; missingFrames = 0; return false; }
        if (!running) failed = true;
        if (freshPackets) missingFrames = 0;
        else missingFrames += frames;
        if (missingFrames > rate) failed = true;
        return !failed;
    }
    void reset() { failed = false; missingFrames = 0; }
private:
    bool failed = false;
    double missingFrames = 0;
};
