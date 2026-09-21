#pragma once

// No audio, OS calls or UI ownership. A failed command always restores MIX.
class SwitchState
{
public:
    enum class Phase { idle, fadingMix, startingSpotify, pausingSpotify };
    bool begin(bool currentlyReference, bool controlReady)
    {
        if (phase != Phase::idle || (!currentlyReference && !controlReady)) return false;
        phase = currentlyReference ? Phase::pausingSpotify : Phase::fadingMix;
        return true;
    }
    bool fadeReady(bool muted, bool audioInactive)
    {
        if (phase != Phase::fadingMix || (!muted && !audioInactive)) return false;
        phase = Phase::startingSpotify;
        return true;
    }
    bool complete(bool success)
    {
        const bool reference = phase == Phase::startingSpotify && success;
        phase = Phase::idle;
        return reference;
    }
    bool pending() const { return phase != Phase::idle; }
    Phase getPhase() const { return phase; }
private:
    Phase phase = Phase::idle;
};
