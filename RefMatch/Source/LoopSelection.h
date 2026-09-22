#pragma once
#include <algorithm>
#include <cmath>
namespace LoopSelection {
struct Range { double start=0,end=0; };
inline Range drag(double anchor,double end,double duration) {
    if(!std::isfinite(duration)||duration<.5)return {};
    Range r{std::clamp(std::min(anchor,end),0.,duration),std::clamp(std::max(anchor,end),0.,duration)};
    if(r.end-r.start<.5) {r.end=std::min(duration,r.start+.5);r.start=std::max(0.,r.end-.5);}
    return r;
}
inline double seconds(float x,float width,double duration) {
    return width>0?std::clamp(double(x/width),0.,1.)*duration:0.;
}
}
