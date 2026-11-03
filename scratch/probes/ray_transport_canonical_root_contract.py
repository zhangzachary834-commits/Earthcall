"""Shared scratch mathematical root contract, not native omission authority.

Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Endpoint error and curvature are explicit caller assumptions. Generic OntoMath
derivation and a justified backend execution enclosure remain separate work.
"""

CPP_ROOT_CONTRACT = r'''
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace canonical_fixture {
struct Band { double lo,hi; };
struct Pending { double lo,hi; std::string reason; };
struct Limits { unsigned maxDepth=24,maxNodes=8192,maxRoots=512; double width=1e-6; };
struct Roots {
    std::vector<Band> brackets;
    std::vector<Pending> unresolved;
    unsigned nodes=0,empty=0,monotone=0;
};
inline double down(double x){return std::nextafter(x,-INFINITY);}
inline double up(double x){return std::nextafter(x,INFINITY);}
inline Band point(double x,double error){return {down(x-error),up(x+error)};}
inline bool finite(Band x){return std::isfinite(x.lo)&&std::isfinite(x.hi)&&x.lo<=x.hi;}
inline bool positive(Band x){return x.lo>0;}
inline bool negative(Band x){return x.hi<0;}

// Completeness is conditional on the supplied mathematical curvature and endpoint
// error contracts. No f32/Metal claim is inferred from those assumptions.
template<class F>
Roots isolate(F phi,double a,double b,double level,double curvature,
              double endpointError,Limits limits={}) {
    if(!(std::isfinite(a)&&std::isfinite(b)&&a<b&&std::isfinite(level)&&
         std::isfinite(curvature)&&curvature>=0&&std::isfinite(endpointError)&&
         endpointError>=0&&std::isfinite(limits.width)&&limits.width>0))
        throw std::invalid_argument("invalid root domain or bound");
    Roots out;
    auto pending=[&](double lo,double hi,const char* reason){out.unresolved.push_back({lo,hi,reason});};
    auto value=[&](double s){auto v=point(phi(s),endpointError);return Band{down(v.lo-level),up(v.hi-level)};};
    auto save=[&](double lo,double hi){
        if(out.brackets.size()>=limits.maxRoots)pending(lo,hi,"root_capacity");
        else out.brackets.push_back({lo,hi});
    };
    std::function<void(double,double,unsigned)> search=[&](double lo,double hi,unsigned depth){
        if(out.nodes>=limits.maxNodes){pending(lo,hi,"node_budget");return;}
        ++out.nodes;
        auto va=value(lo),vb=value(hi);
        if(!finite(va)||!finite(vb)){pending(lo,hi,"undefined_endpoint");return;}
        const double hLow=down(hi-lo),hHigh=up(hi-lo);
        if(!(hLow>0&&std::isfinite(hHigh))){pending(lo,hi,"endpoint_resolution");return;}
        const double bow=up(up(up(curvature*hHigh)*hHigh)*.125);
        if(down(std::min(va.lo,vb.lo)-bow)>0 || up(std::max(va.hi,vb.hi)+bow)<0){++out.empty;return;}
        const Band diff{down(vb.lo-va.hi),up(vb.hi-va.lo)};
        std::array<double,4> q{diff.lo/hLow,diff.lo/hHigh,diff.hi/hLow,diff.hi/hHigh};
        const double radius=up(up(curvature*hHigh)*.5);
        const Band deriv{down(down(*std::min_element(q.begin(),q.end()))-radius),
                         up(up(*std::max_element(q.begin(),q.end()))+radius)};
        const bool mono=positive(deriv)||negative(deriv);
        const bool bracket=(positive(va)&&negative(vb))||(negative(va)&&positive(vb));
        if(mono&&bracket){
            ++out.monotone;
            double l=lo,r=hi;auto left=va;
            // The complete interval retains a unique root; never replace it with a point.
            for(unsigned iter=0;iter<128;++iter){
                if(r-l<=limits.width){save(l,r);return;}
                const double mid=l+(r-l)*.5;
                if(!(mid>l&&mid<r)){save(l,r);return;}
                if(out.nodes>=limits.maxNodes){pending(l,r,"node_budget");return;}
                ++out.nodes;auto vm=value(mid);
                if(!finite(vm)){pending(l,r,"undefined_interior");return;}
                if(!positive(vm)&&!negative(vm)){
                    // Inverse-Lipschitz enclosure inside the already proved bracket.
                    const double minSlope=positive(deriv)?deriv.lo:-deriv.hi;
                    const double errorRadius=up(std::max(std::abs(vm.lo),std::abs(vm.hi))/minSlope);
                    save(std::max(l,down(mid-errorRadius)),std::min(r,up(mid+errorRadius)));
                    return;
                }
                if((positive(left)&&negative(vm))||(negative(left)&&positive(vm)))r=mid;
                else {l=mid;left=vm;}
            }
            pending(l,r,"iteration_budget");return;
        }
        if(depth>=limits.maxDepth){pending(lo,hi,"depth_budget");return;}
        const double mid=lo+(hi-lo)*.5;
        if(!(mid>lo&&mid<hi)){pending(lo,hi,"endpoint_resolution");return;}
        search(lo,mid,depth+1);search(mid,hi,depth+1);
    };
    for(unsigned bin=0;bin<8;++bin){
        const double lo=bin==0?a:a+(b-a)*(double(bin)/8);
        const double hi=bin==7?b:a+(b-a)*(double(bin+1)/8);
        search(lo,hi,0);
    }
    std::sort(out.brackets.begin(),out.brackets.end(),[](Band x,Band y){return x.lo<y.lo;});
    return out;
}

// Slab clipping uses only exact zero for a constant coordinate. Division and
// intersection are outward. Invalid/nonfinite data refuses the whole query.
inline bool clipAxis(double origin,double direction,double half,double& a,double& b){
    if(!std::isfinite(origin)||!std::isfinite(direction)||!std::isfinite(half)||half<0)
        throw std::invalid_argument("invalid slab");
    if(direction==0)return std::abs(origin)<=half;
    const Band n0{down(-half-origin),up(-half-origin)},n1{down(half-origin),up(half-origin)};
    const Band x{down(std::min(n0.lo/direction,n0.hi/direction)),up(std::max(n0.lo/direction,n0.hi/direction))};
    const Band y{down(std::min(n1.lo/direction,n1.hi/direction)),up(std::max(n1.lo/direction,n1.hi/direction))};
    if(!finite(x)||!finite(y))throw std::invalid_argument("nonrepresentable slab interval");
    a=std::max(a,std::min(x.lo,y.lo));b=std::min(b,std::max(x.hi,y.hi));
    return b>a;
}
}
'''

# Deliberately pinned fixture model retained from Sparkly's experiment. This is
# neither authored-expression extraction nor a production admission mechanism.
CPP_FIXTURE = r'''
namespace canonical_fixture {
struct Spec {
    int id;std::array<double,3> origin,half;
    double Wx,Wy,yKnee,wz,M2;
    std::array<double,3> amps,freqs,phases;
};
const Spec members[4]={
    {0,{0,48,75},{160,36,46},75,16,-12,3.2,.086772,{-15,-5.5,-2},{.026,.068,.16},{0,.5,1.2}},
    {1,{15,56,140},{150,38,44},70,17,-13,2.8,.103901,{14,-5,2},{.028,.075,.18},{0,.5,1.2}},
    {2,{-5,78,82},{160,36,46},75,16,-12,3.8,.054888,{-13,-5,-1.5},{.024,.060,.14},{0,.5,1.2}},
    {3,{22,72,68},{70,64,36},32,30,-26,3,.093184,{-9,-3.5,-1.5},{.038,.095,.18},{0,.5,1.2}}
};
inline double wave(const Spec& m,double x){
    return m.amps[0]*std::sin(m.freqs[0]*x+m.phases[0])+
           m.amps[1]*std::cos(m.freqs[1]*x+m.phases[1])+
           m.amps[2]*std::sin(m.freqs[2]*x+m.phases[2]);
}
struct Piece {int medium;double lo,hi;};
struct Preparation {
    std::vector<Piece> pieces;
    unsigned roots=0,unresolved=0,nodes=0;
    bool fallback=false;
};
inline Preparation prepare(std::array<double,3> ro,std::array<double,3> d,double clipA,double clipB,Limits limits={}){
    Preparation out;
    // This is an explicit DOUBLE-math fixture assumption, never a Metal error bound.
    constexpr double endpointError=1e-10;
    for(const auto& m:members){
        auto o=ro;for(int k=0;k<3;++k)o[k]-=m.origin[k];
        double a=clipA,b=clipB;bool hit=true;
        for(int k=0;k<3;++k)if(!clipAxis(o[k],d[k],m.half[k],a,b)){hit=false;break;}
        if(!hit||!clipAxis(o[0],d[0],m.Wx,a,b)||!clipAxis(o[1],d[1],m.Wy,a,b)||!(b>a))continue;
        auto phi=[&](double s){return o[2]+s*d[2]+wave(m,o[0]+s*d[0]);};
        const double dx=std::abs(d[0]);
        double Mx=0;
        for(int k=0;k<3;++k)Mx=up(Mx+up(up(std::abs(m.amps[k])*std::abs(m.freqs[k]))*std::abs(m.freqs[k])));
        const double M=up(up(Mx*dx)*dx);
        std::vector<Band> bands;std::vector<double> events{a,b};
        for(double level:{-m.wz,0.,m.wz}){
            auto r=isolate(phi,a,b,level,std::abs(M),endpointError,limits);
            out.roots+=r.brackets.size();out.unresolved+=r.unresolved.size();out.nodes+=r.nodes;
            if(!r.unresolved.empty())out.fallback=true;
            for(auto band:r.brackets){bands.push_back(band);events.push_back(band.lo);events.push_back(band.hi);}
        }
        if(out.fallback)continue;
        // Affine kinks also remain intervals, not silently merged nearby points.
        for(auto plane:std::array<std::pair<int,double>,3>{{{0,0},{1,0},{1,m.yKnee}}}){
            const int k=plane.first;if(d[k]==0)continue;
            const double r=(plane.second-o[k])/d[k];
            if(r>a&&r<b){events.push_back(std::max(a,down(r)));events.push_back(std::min(b,up(r)));}
        }
        std::sort(events.begin(),events.end());events.erase(std::unique(events.begin(),events.end()),events.end());
        for(size_t k=1;k<events.size();++k){
            const double l=events[k-1],h=events[k];if(!(h>l))continue;
            const double mid=l+(h-l)*.5;
            const bool uncertain=std::any_of(bands.begin(),bands.end(),[&](Band band){return l<band.hi&&h>band.lo;});
            if(uncertain||std::abs(phi(mid))<=m.wz+endpointError)out.pieces.push_back({m.id,l,h});
        }
    }
    if(out.fallback)out.pieces.clear();
    return out;
}
}
'''
