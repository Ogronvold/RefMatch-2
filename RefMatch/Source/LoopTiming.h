#pragma once
#include <algorithm>
namespace LoopTiming {
constexpr double graceMs=3000;
inline bool missingExpired(double now,double since) {return now-since>=graceMs;}
inline bool confirmsSeek(double position,double in,double out,double elapsedMs) {
    return position>=in-.6 && position<=in+std::min(out-in-.1,1.+std::max(0.,elapsedMs)/1000.);
}
}
