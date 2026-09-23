#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include <complex>

namespace EQDesign {
constexpr int bands = 20;
using Gains = std::array<double, bands>;
struct Coeff { double b0=1,b1=0,b2=0,a1=0,a2=0; };
inline double centre(int i) { return 30.0 * std::pow(16000.0/30.0, double(i)/(bands-1)); }
inline Coeff peak(double sr, double hz, double db, double q=2.0)
{
    hz=std::min(hz,sr*.45);
    const double a=std::pow(10.0,db/40.0), w=2*3.141592653589793*hz/sr;
    const double alpha=std::sin(w)/(2*q), c=std::cos(w), d=1+alpha/a;
    return {(1+alpha*a)/d,-2*c/d,(1-alpha*a)/d,-2*c/d,(1-alpha/a)/d};
}

inline Coeff lowShelf(double sr, double hz, double db)
{
    hz=std::min(hz,sr*.45);
    const double A=std::pow(10.0,db/40.0), w=2*3.141592653589793*hz/sr;
    const double c=std::cos(w), si=std::sin(w), alpha=si/std::sqrt(2.0), beta=2.0*std::sqrt(A)*alpha;
    const double b0=A*((A+1)-(A-1)*c+beta), b1=2*A*((A-1)-(A+1)*c), b2=A*((A+1)-(A-1)*c-beta);
    const double a0=(A+1)+(A-1)*c+beta, a1=-2*((A-1)+(A+1)*c), a2=(A+1)+(A-1)*c-beta;
    return {b0/a0,b1/a0,b2/a0,a1/a0,a2/a0};
}
inline Coeff highShelf(double sr, double hz, double db)
{
    hz=std::min(hz,sr*.45);
    const double A=std::pow(10.0,db/40.0), w=2*3.141592653589793*hz/sr;
    const double c=std::cos(w), si=std::sin(w), alpha=si/std::sqrt(2.0), beta=2.0*std::sqrt(A)*alpha;
    const double b0=A*((A+1)+(A-1)*c+beta), b1=-2*A*((A-1)+(A+1)*c), b2=A*((A+1)+(A-1)*c-beta);
    const double a0=(A+1)-(A-1)*c+beta, a1=2*((A-1)-(A+1)*c), a2=(A+1)-(A-1)*c-beta;
    return {b0/a0,b1/a0,b2/a0,a1/a0,a2/a0};
}
inline double response(const Coeff& c,double hz,double sr)
{
    const auto z=std::polar(1.0,-2*3.141592653589793*hz/sr);
    return 20*std::log10(std::max(1.e-12,std::abs((c.b0+c.b1*z+c.b2*z*z)/(1.0+c.a1*z+c.a2*z*z))));
}
// Fit broad, overlapping peaks to a gain-normalised target. No level matching
// baked into EQ: a reference louder by a constant amount produces a flat curve.
inline Gains fit(const Gains& mix,const Gains& ref,double sr,double smoothing)
{
    Gains target{}, gains{};
    double mean=0;
    for(int i=0;i<bands;++i) { target[i]=ref[i]-mix[i]; mean+=target[i]/bands; }
    for(auto& x:target)x-=mean;
    auto raw=target;
    const double sigma=std::clamp(smoothing,0.,1.)*3.;
    if(sigma>.01)for(int i=0;i<bands;++i) {
        double sum=0,weight=0;
        for(int j=0;j<bands;++j) {
            const double distance=double(i-j),w=std::exp(-.5*distance*distance/(sigma*sigma));
            sum+=raw[j]*w;weight+=w;
        }
        target[i]=sum/weight;
    }
    std::array<Gains,bands> basis{};
    for(int b=0;b<bands;++b)for(int i=0;i<bands;++i)
        basis[b][i]=response(peak(sr,centre(b),1),std::min(centre(i),sr*.45),sr);
    for(int pass=0;pass<30;++pass)for(int b=0;b<bands;++b) {
        double numerator=0,denominator=.03;
        for(int i=0;i<bands;++i) {
            double estimate=0;
            for(int j=0;j<bands;++j)if(j!=b)estimate+=basis[j][i]*gains[j];
            numerator+=basis[b][i]*(target[i]-estimate);denominator+=basis[b][i]*basis[b][i];
        }
        gains[b]=std::clamp(numerator/denominator,-12.,12.);
    }
    return gains;
}
inline Gains scaled(Gains gains,double amount,double limit,double sr)
{
    // First constrain the learned 100% curve to Max Correction, then use Amount
    // as a true wet scaling of that complete correction. 100% is the normal full
    // match; values above 100% deliberately exaggerate the learned correction.
    double maximum=0;
    for(int i=0;i<160;++i) {
        const double hz=20*std::pow(std::min(20000.,sr*.45)/20.,i/159.);
        double db=0;for(int b=0;b<bands;++b)db+=response(peak(sr,centre(b),gains[b]),hz,sr);
        maximum=std::max(maximum,std::abs(db));
    }
    if(maximum>limit && maximum>0.0)for(auto& g:gains)g*=limit/maximum;
    const double wet=std::clamp(amount,0.,2.);
    for(auto& g:gains)g*=wet;
    return gains;
}
}
