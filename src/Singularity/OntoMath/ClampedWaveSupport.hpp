#pragma once
#include "Singularity/OntoMath/ScalarForm.hpp"
#include <array>
#include <optional>
#include <cmath>
#include <limits>

// Mathematical compiler recipe, not a domain/renderer kind. Recognizes only a
// literal-parameter product of clamped X/Y ramps and abs(Z+three harmonics).
// It grants a conservative support envelope, not an alternate field evaluator.
// Every call rederives from the current source; no borrowed/cached AST state.
// Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
namespace OntoMath {
struct ClampedWaveSupport {
    std::array<float,4> limits{},curvature{},amplitudes{},frequencies{},phases{};
};
namespace clamped_wave_detail {
using N=MathNode;using O=MathNode::Op;
inline const N* child(const N* n,O op,unsigned i,unsigned count){
    return n&&!n->unsupported&&n->op==op&&n->children.size()==count&&n->children[i]?n->children[i].get():nullptr;
}
inline std::optional<float> literal(const N* n){
    if(!n||n->unsupported||n->op!=O::ScalarLeaf||!n->children.empty())return {};
    if(n->scalarForm.terms.empty())return 0.0f;
    if(n->scalarForm.terms.size()!=1)return {};
    const auto& t=n->scalarForm.terms[0];
    if(!t.factors.empty()||!t.trans.empty()||!std::isfinite(t.coefficient))return {};
    const float f=static_cast<float>(t.coefficient);return std::isfinite(f)?std::optional<float>(f):std::nullopt;
}
inline bool number(const N* n,float v){const auto a=literal(n);return a&&*a==v;}
inline bool symbol(const N* n,const char* v){return n&&!n->unsupported&&n->op==O::ValueLeaf&&n->variableName==v&&n->children.empty();}
inline const N* clamp(const N* n){return child(n,O::Clamp,0,3)&&number(n->children[1].get(),0)&&number(n->children[2].get(),1)?n->children[0].get():nullptr;}
inline const N* oneMinus(const N* n){return child(n,O::Sub,1,2)&&number(n->children[0].get(),1)?n->children[1].get():nullptr;}
inline bool plusY(const N* n,float value){return child(n,O::Add,0,2)&&symbol(n->children[0].get(),"y")&&number(n->children[1].get(),value);}
inline std::optional<ClampedWaveSupport> match(const N* n){
    const auto xy=child(n,O::Scale,0,2),z=child(n,O::Scale,1,2);
    const auto x=child(xy,O::Scale,0,2),ys=child(xy,O::Scale,1,2);
    const auto yr=child(ys,O::Scale,0,2),yf=child(ys,O::Scale,1,2);
    auto xd=oneMinus(clamp(x));auto zd=oneMinus(clamp(z));auto yd=oneMinus(clamp(yf));auto rise=clamp(yr);
    if(!child(xd,O::Div,0,2)||!child(zd,O::Div,0,2)||!child(yd,O::Div,0,2)||!child(rise,O::Scale,0,2))return {};
    if(!symbol(child(xd->children[0].get(),O::Abs,0,1),"x"))return {};
    auto wx=literal(xd->children[1].get()),wz=literal(zd->children[1].get()),den=literal(yd->children[1].get()),a=literal(rise->children[0].get());
    auto yadd=rise->children[1].get();if(!child(yadd,O::Add,1,2)||!symbol(yadd->children[0].get(),"y"))return {};
    auto wy=literal(yadd->children[1].get());
    if(!wx||!wy||!wz||!den||!a||!(*wx>0&&*wy>0&&*wz>0&&*a>0)||*den!=2* *wy||!plusY(yd->children[0].get(),*wy))return {};
    auto bent=child(zd->children[0].get(),O::Abs,0,1);std::array<const N*,3> waves{};
    for(int i=2;i>=0;--i){if(!child(bent,O::Add,1,2))return {};waves[i]=bent->children[1].get();bent=bent->children[0].get();}
    if(!symbol(bent,"z"))return {};
    ClampedWaveSupport out;out.limits={*wx,*wy,-*wy+1/ *a,*wz};
    if(out.limits[2]>*wy||!std::isfinite(out.limits[2]))return {};
    long double curvature=0;
    for(unsigned i=0;i<3;++i){
        const auto w=waves[i];if(!w||w->unsupported||w->op!=O::ScalarLeaf||!w->children.empty()||w->scalarForm.terms.size()!=1)return {};
        const auto& term=w->scalarForm.terms[0];if(!term.factors.empty()||term.trans.size()!=1)return {};
        const auto& t=term.trans[0];if(t.variable!="x"||t.kind!=(i==1?TransFactor::Kind::Cos:TransFactor::Kind::Sin))return {};
        out.amplitudes[i]=static_cast<float>(term.coefficient);out.frequencies[i]=static_cast<float>(t.scale);out.phases[i]=static_cast<float>(t.shift);
        if(!std::isfinite(out.amplitudes[i])||!std::isfinite(out.frequencies[i])||!std::isfinite(out.phases[i]))return {};
        curvature+=std::abs(static_cast<long double>(out.amplitudes[i]))*out.frequencies[i]*out.frequencies[i];
    }
    out.curvature={std::nextafter(static_cast<float>(curvature),std::numeric_limits<float>::infinity())*1.00002f,.001f,0,0};
    if(!std::isfinite(out.curvature[0]))return {};return out;
}
inline std::optional<ClampedWaveSupport> search(const N* n,unsigned depth){
    if(!n||n->unsupported||depth>64)return {};if(auto r=match(n))return r;
    // Only multiplicative ancestors: never infer support through addition,
    // cancellation, division or a computed clamp bound.
    if(n->op==O::Scale&&n->children.size()==2){for(const auto& c:n->children)if(auto r=search(c.get(),depth+1))return r;}
    return {};
}
}
inline std::optional<ClampedWaveSupport> clampedWaveSupport(const Piecewise* p){
    if(!p||p->pieces.size()!=1)return {};const auto& x=p->pieces[0];
    if(x.guard||x.call||x.fold||x.hasLo||x.hasHi||x.whereLEZero)return {};
    return clamped_wave_detail::search(x.mathNode.get(),0);
}
inline bool literalZero(const Piecewise* p){
    if(!p||p->pieces.size()!=1)return false;const auto& x=p->pieces[0];
    return !x.guard&&!x.call&&!x.fold&&!x.hasLo&&!x.hasHi&&!x.whereLEZero&&clamped_wave_detail::number(x.mathNode.get(),0);
}
}
