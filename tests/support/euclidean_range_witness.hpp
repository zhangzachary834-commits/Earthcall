#pragma once

// Host mathematical witnesses, separate from native skip eligibility.
// Extends volume_zero_proof_test; no save writes or research proof upload.
// Codex / GPT-6.1 Sol / session 01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca / 2026-10-15.
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/VolumeZeroProof.hpp"
#include <array>
#include <functional>
#include <random>

namespace EuclideanRangeWitness {
using Node = OntoMath::MathNode;
using RV = Node::RangeValue;
using I = OntoMath::Interval;
using Box = std::array<I, 3>;
using Vars = std::map<std::string, RV>;
inline std::unique_ptr<Node> variable(const std::string& name) {
    auto n = std::make_unique<Node>(); n->op = Node::Op::ValueLeaf; n->variableName = name; return n;
}
inline std::unique_ptr<Node> length() {
    auto n = std::make_unique<Node>(); n->op = Node::Op::Length; n->children.push_back(variable("p")); return n;
}
inline std::unique_ptr<Node> distance() {
    auto n = std::make_unique<Node>(); n->op = Node::Op::Distance;
    n->children.push_back(variable("p")); n->children.push_back(variable("q")); return n;
}
inline RV vector(const Box& b) { return RV::makeVector(b[0], b[1], b[2]); }
inline bool contains(const std::optional<RV>& r, double v) {
    return r && r->kind == OntoMath::ValueKind::Scalar && r->scalar.lo <= v && v <= r->scalar.hi;
}
inline bool unknown(const std::optional<RV>& r) {
    return !r || r->kind != OntoMath::ValueKind::Scalar || !r->scalar.bounded();
}
inline void run(const std::function<void(bool, const std::string&)>& check) {
    const auto len = length(); const auto dist = distance();
    const Box origin{{I(0), I(0), I(0)}};
    const std::array<Box, 5> boxes{{
        Box{{I(3,4), I(4,5), I(0)}}, Box{{I(-4,-3), I(-5,-4), I(0)}},
        Box{{I(-2,3), I(-4,5), I(-6,7)}}, origin, Box{{I(1e-30f),I(0),I(0)}}
    }};
    bool extrema = true, host = true, legacy = true;
    for (const auto& box : boxes) {
        const Vars vars{{"p", vector(box)}};
        const auto r = len->evalMathematicalRange(vars);
        double lo2=0,hi2=0;
        for (auto i : box) {
            const double a = i.lo <= 0 && i.hi >= 0 ? 0 : std::min(std::abs(double(i.lo)),std::abs(double(i.hi)));
            const double b = std::max(std::abs(double(i.lo)),std::abs(double(i.hi)));
            lo2+=a*a;hi2+=b*b;
        }
        extrema &= contains(r,std::sqrt(lo2)) && contains(r,std::sqrt(hi2));
        legacy &= unknown(len->evalRange(vars));
        for (int x=0;x<3;++x) for (int y=0;y<3;++y) for (int z=0;z<3;++z) {
            glm::vec3 p(box[0].lo+(box[0].hi-box[0].lo)*float(x)/2,
                        box[1].lo+(box[1].hi-box[1].lo)*float(y)/2,
                        box[2].lo+(box[2].hi-box[2].lo)*float(z)/2);
            const auto value=len->evaluate({{"p",p}});
            host &= value && contains(r,std::get<double>(*value));
        }
    }
    check(extrema,"Euclidean analysis encloses analytic box extrema, origin crossings and tiny ideal norms");
    check(host,"Euclidean analysis encloses actual host Length evaluation, including square underflow");
    check(legacy,"existing evalRange keeps Length unknown; existing consumers gain no new proof");
    const auto zero = len->evalMathematicalRange({{"p",vector(origin)}});
    check(zero && zero->scalar.exactZero(),"exact zero vector retains a singleton zero norm");

    std::mt19937 rng(20261015u);
    std::uniform_real_distribution<float> pos(-1000,1000), span(0,20), unit(0,1);
    bool sampled=true, ideal=true;
    for(int box=0;box<128;++box) {
        Box a,b;
        for(int i=0;i<3;++i) { float x=pos(rng),y=pos(rng);a[i]=I(x,x+span(rng));b[i]=I(y,y+span(rng)); }
        const auto r=dist->evalMathematicalRange({{"p",vector(a)},{"q",vector(b)}});
        double min2=0,max2=0;
        for(int i=0;i<3;++i) {
            const double lo=double(a[i].lo)-b[i].hi, hi=double(a[i].hi)-b[i].lo;
            const double mn=lo<=0 && hi>=0 ? 0 : std::min(std::abs(lo),std::abs(hi));
            const double mx=std::max(std::abs(lo),std::abs(hi));min2+=mn*mn;max2+=mx*mx;
        }
        ideal &= contains(r,std::sqrt(min2)) && contains(r,std::sqrt(max2));
        for(int j=0;j<64;++j) {
            glm::vec3 p,q;
            for(int i=0;i<3;++i) { p[i]=a[i].lo+(a[i].hi-a[i].lo)*unit(rng);q[i]=b[i].lo+(b[i].hi-b[i].lo)*unit(rng); }
            const auto value=dist->evaluate({{"p",p},{"q",q}});
            sampled &= value && contains(r,std::get<double>(*value));
        }
    }
    check(ideal,"Distance analysis encloses independently derived extrema for 128 independent boxes");
    check(sampled,"Distance analysis encloses 8192 actual host values with independently varying endpoints");
    const auto separated=dist->evalMathematicalRange({{"p",vector(boxes[0])},{"q",vector(origin)}});
    const auto shifted=dist->evalMathematicalRange({{"p",vector(boxes[0])},{"q",RV::makeVector(I(30),I(40),I(0))}});
    check(separated && shifted && separated->scalar.hi<shifted->scalar.lo,
          "Distance respects independent coordinate bindings rather than reusing a stale endpoint");
    check(!len->evalMathematicalRange({{"p",RV::makeScalar(I(3))}}) &&
          !dist->evalMathematicalRange({{"p",vector(origin)},{"q",RV::makeScalar(I(3))}}),
          "scalar Length/Distance overloads are refused, matching the actual evaluator");
    check(unknown(dist->evalMathematicalRange({{"p",vector(origin)}})),"unbound second endpoint remains unknown");
    bool invalid=true;
    for(auto bad:{I(2,1),I(std::numeric_limits<float>::quiet_NaN()),I::infinite()})
        invalid &= !len->evalMathematicalRange({{"p",RV::makeVector(bad,I(0),I(0))}});
    check(invalid,"empty, NaN and infinite component intervals cannot yield a Euclidean bound");
    check(!len->evalMathematicalRange({{"p",RV::makeVector(I(1e30f),I(0),I(0))}}) &&
          !len->evalMathematicalRange({{"p",RV::makeVector(I(1.2e19f),I(1.2e19f),I(1.2e19f))}}),
          "finite ideal norms do not conceal host square or sum overflow");
    check(!dist->evalMathematicalRange({{"p",RV::makeVector(I(std::numeric_limits<float>::max()),I(0),I(0))},
                                       {"q",RV::makeVector(I(-std::numeric_limits<float>::max()),I(0),I(0))}}),
          "Distance subtraction overflow refuses a finite bound");
    auto malformed=length();malformed->children.clear();
    auto nullChild=length();nullChild->children[0].reset();
    check(!malformed->evalMathematicalRange({}) && !nullChild->evalMathematicalRange({}),
          "malformed Euclidean analysis refuses missing/null operands");

    auto radial=std::make_shared<Node>();radial->op=Node::Op::Clamp;
    auto envelope=std::make_unique<Node>();envelope->op=Node::Op::Sub;
    auto scalar=[](double v){auto n=std::make_unique<Node>();n->scalarForm.terms.emplace_back(v);return n;};
    envelope->children.push_back(scalar(1));envelope->children.push_back(length());
    radial->children.push_back(std::move(envelope));radial->children.push_back(scalar(0));radial->children.push_back(scalar(1));
    const Vars far{{"p",RV::makeVector(I(10,11),I(0,1),I(0,1))}};
    const auto positive=radial->evalMathematicalRange({{"p",vector(origin)}});
    const auto empty=radial->evalMathematicalRange(far);
    check(contains(positive,1) && empty && empty->scalar.hi==0,
          "analysis propagates Euclidean bounds through authored subtraction and clamp");
    OntoMath::Piecewise pw;OntoMath::Piecewise::Piece piece;piece.mathNode=radial;pw.pieces.push_back(piece);
    check(!Rendering::densityProvenNonPositive(pw,glm::vec3(10,0,0),glm::vec3(11,1,1)),
          "production volume proof still declines the new radial absence theorem");
    auto timed=distance();timed->children[1]->variableName="sourceClockPoint";
    check(unknown(timed->evalMathematicalRange({{"p",vector(origin)}})),
          "unbound clock/coordinate input is never guessed during analysis");
}
} // namespace EuclideanRangeWitness
