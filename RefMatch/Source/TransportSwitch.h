#pragma once
// Message-thread state; audio fade remains entirely on the audio thread.
class TransportSwitch
{
public:
    enum class Phase { idle, fadingOut, playing, pausing };
    bool beginReference()
    {
        if (phase != Phase::idle) return false;
        phase = Phase::fadingOut;
        return true;
    }
    bool fadeComplete(bool muted, bool audioInactive = false)
    {
        if (phase != Phase::fadingOut || (!muted && !audioInactive)) return false;
        phase = Phase::playing;
        return true;
    }
    void requestMix()
    {
        if (phase == Phase::playing) cancelled = true;
        else if (phase == Phase::fadingOut) phase = Phase::idle;
        else phase = Phase::pausing;
    }
    bool playComplete(bool ok, bool referenceStillRequested)
    {
        const bool keepReference = ok && referenceStillRequested && !cancelled;
        cancelled = false;
        phase = keepReference ? Phase::idle : Phase::pausing;
        return keepReference;
    }
    void reset() { phase = Phase::idle; cancelled = false; }
    bool pending() const { return phase != Phase::idle; }
    Phase getPhase() const { return phase; }
private:
    Phase phase = Phase::idle;
    bool cancelled = false;
};
