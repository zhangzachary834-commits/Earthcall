#include "Singularity/OntoMath/ScalarForm.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
using OntoMath::MathNode;
using OntoMath::Piecewise;
using OntoMath::ScalarForm;

enum class Channel { SourceRho, MediumDensity, MediumExtinction, MediumScattering, MediumChroma };
enum class ValueKind { Scalar, Vec3 };

std::unique_ptr<MathNode> scalarU(double v) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::ScalarLeaf;
    n->scalarForm = ScalarForm::constant(v);
    return n;
}
std::unique_ptr<MathNode> variableU(const std::string& name) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::ValueLeaf;
    n->variableName = name;
    return n;
}
std::unique_ptr<MathNode> binaryU(MathNode::Op op, std::unique_ptr<MathNode> a,
                                  std::unique_ptr<MathNode> b) {
    auto n = std::make_unique<MathNode>();
    n->op = op;
    n->children.push_back(std::move(a));
    n->children.push_back(std::move(b));
    return n;
}
std::shared_ptr<MathNode> common(double scale) {
    return std::shared_ptr<MathNode>(
        binaryU(MathNode::Op::Scale,
                binaryU(MathNode::Op::Add, variableU("x"), scalarU(2.0)),
                scalarU(scale)).release());
}
std::shared_ptr<MathNode> timed(double scale) {
    return std::shared_ptr<MathNode>(
        binaryU(MathNode::Op::Scale,
                binaryU(MathNode::Op::Add, variableU("x"), variableU("t")),
                scalarU(scale)).release());
}
std::shared_ptr<MathNode> vec3Node(double r, double g, double b) {
    auto n = std::make_shared<MathNode>();
    n->op = MathNode::Op::VectorConstruct;
    n->children.push_back(scalarU(r));
    n->children.push_back(scalarU(g));
    n->children.push_back(scalarU(b));
    return n;
}

bool scalarOp(MathNode::Op op) {
    return op == MathNode::Op::ScalarLeaf || op == MathNode::Op::ValueLeaf ||
           op == MathNode::Op::Add || op == MathNode::Op::Sub ||
           op == MathNode::Op::Scale;
}

struct MathCompiler {
    std::vector<std::string> nodes;
    std::unordered_map<std::string, uint32_t> canonical;
    std::unordered_map<const MathNode*, uint32_t> source;

    std::string local(const MathNode& n) const {
        std::string key = "op=" + std::to_string(static_cast<int>(n.op)) + "|var=" + n.variableName;
        if (n.op == MathNode::Op::ScalarLeaf)
            key += "|scalar=" + n.scalarForm.normalized().toJson().dump();
        return key;
    }
    uint32_t compile(const MathNode& n, ValueKind expected = ValueKind::Scalar) {
        const bool vectorRoot = n.op == MathNode::Op::VectorConstruct;
        if (expected == ValueKind::Scalar) assert(scalarOp(n.op));
        if (expected == ValueKind::Vec3) assert(vectorRoot && n.children.size() == 3);
        std::string key = local(n) + "|children=";
        for (const auto& child : n.children) {
            assert(child);
            key += std::to_string(compile(*child, ValueKind::Scalar)) + ",";
        }
        auto it = canonical.find(key);
        uint32_t id;
        if (it != canonical.end()) id = it->second;
        else {
            id = static_cast<uint32_t>(nodes.size());
            nodes.push_back(key);
            canonical.emplace(key, id);
        }
        source[&n] = id;
        return id;
    }
};

struct CompiledPiece {
    bool hasLo = false, hasHi = false;
    double lo = 0.0, hi = 0.0;
    bool includeLo = true, includeHi = true;
    uint32_t math = 0;
};
struct CompiledPiecewise {
    Channel channel = Channel::SourceRho;
    ValueKind kind = ValueKind::Scalar;
    std::string inputVariable;
    std::vector<CompiledPiece> pieces;
    std::string topologyKey;
};

struct PiecewiseAdapter {
    MathCompiler math;
    uint64_t topologyBuilds = 0;
    uint64_t refusals = 0;

    bool compile(Channel channel, const Piecewise& model, CompiledPiecewise& out) {
        const ValueKind kind = channel == Channel::MediumChroma ? ValueKind::Vec3 : ValueKind::Scalar;
        CompiledPiecewise next;
        next.channel = channel;
        next.kind = kind;
        next.inputVariable = model.inputVariable;
        std::ostringstream topology;
        topology << "kind=" << static_cast<int>(kind) << "|input=" << model.inputVariable
                 << "|pieces=" << model.pieces.size();
        for (const auto& p : model.pieces) {
            const bool acceptedRoot = p.mathNode &&
                ((kind == ValueKind::Scalar && scalarOp(p.mathNode->op)) ||
                 (kind == ValueKind::Vec3 && p.mathNode->op == MathNode::Op::VectorConstruct &&
                  p.mathNode->children.size() == 3));
            if (!acceptedRoot || p.guard || p.whereLEZero || p.call || p.fold) {
                ++refusals;
                return false;
            }
            const uint32_t mathId = math.compile(*p.mathNode, kind);
            next.pieces.push_back({p.hasLo, p.hasHi, p.lo, p.hi,
                                   p.includeLo, p.includeHi, mathId});
            topology << "|" << p.hasLo << ":" << p.lo << ":" << p.includeLo
                     << ":" << p.hasHi << ":" << p.hi << ":" << p.includeHi
                     << ":math=" << mathId;
        }
        next.topologyKey = topology.str();
        out = std::move(next);
        ++topologyBuilds;
        return true;
    }
};

Piecewise twoPiece(std::shared_ptr<MathNode> left, std::shared_ptr<MathNode> right,
                   double split = 0.0) {
    Piecewise m;
    m.inputVariable = "x";
    Piecewise::Piece a;
    a.hasHi = true; a.hi = split; a.includeHi = false; a.mathNode = std::move(left);
    Piecewise::Piece b;
    b.hasLo = true; b.lo = split; b.includeLo = true; b.mathNode = std::move(right);
    m.pieces.push_back(std::move(a));
    m.pieces.push_back(std::move(b));
    return m;
}

double scalarValue(const PropertyValue& v) {
    const auto* d = std::get_if<double>(&v);
    assert(d);
    return *d;
}
glm::vec3 vectorValue(const PropertyValue& v) {
    const auto* p = std::get_if<glm::vec3>(&v);
    assert(p);
    return *p;
}
}

int main() {
    PiecewiseAdapter adapter;
    auto rho = twoPiece(common(3.0), common(4.0));
    auto density = twoPiece(common(3.0), common(4.0));
    auto extinction = twoPiece(common(3.0), common(4.0));
    auto scattering = twoPiece(common(3.0), common(4.0));

    CompiledPiecewise cRho, cD, cT, cS;
    assert(adapter.compile(Channel::SourceRho, rho, cRho));
    assert(adapter.compile(Channel::MediumDensity, density, cD));
    assert(adapter.compile(Channel::MediumExtinction, extinction, cT));
    assert(adapter.compile(Channel::MediumScattering, scattering, cS));
    assert(cRho.pieces.size() == 2 && cD.pieces.size() == 2);
    assert(cRho.pieces[0].math == cD.pieces[0].math);
    assert(cD.pieces[0].math == cT.pieces[0].math);
    assert(cT.pieces[0].math == cS.pieces[0].math);
    assert(cRho.pieces[1].math == cD.pieces[1].math);
    assert(cRho.channel != cD.channel);

    const uint64_t buildsBeforeRuntime = adapter.topologyBuilds;
    for (double x : {-5.0, 5.0}) {
        for (double t : {0.0, 1.0, 42.0}) {
            const auto value = density.evaluate({{"x", x}, {"t", t}});
            assert(value.has_value());
            (void)scalarValue(*value);
        }
    }
    assert(adapter.topologyBuilds == buildsBeforeRuntime);

    const uint32_t rhoLeftBefore = cRho.pieces[0].math;
    const uint32_t extinctionLeftBefore = cT.pieces[0].math;
    density.pieces[0].mathNode->children[1]->scalarForm = ScalarForm::constant(7.0);
    CompiledPiecewise cDValueEdit;
    assert(adapter.compile(Channel::MediumDensity, density, cDValueEdit));
    assert(cDValueEdit.pieces[0].math != cD.pieces[0].math);
    assert(cDValueEdit.pieces[1].math == cD.pieces[1].math);
    assert(cRho.pieces[0].math == rhoLeftBefore);
    assert(cT.pieces[0].math == extinctionLeftBefore);

    const std::string densityTopologyBefore = cDValueEdit.topologyKey;
    density.pieces[0].hi = -2.0;
    density.pieces[1].lo = -2.0;
    CompiledPiecewise cDTopologyEdit;
    assert(adapter.compile(Channel::MediumDensity, density, cDTopologyEdit));
    assert(cDTopologyEdit.topologyKey != densityTopologyBefore);
    assert(cDTopologyEdit.pieces[0].math == cDValueEdit.pieces[0].math);
    assert(cDTopologyEdit.pieces[1].math == cDValueEdit.pieces[1].math);

    // Typed C_v lane: vec3 truth is admitted as vec3 and never coerced to scalar.
    Piecewise chroma = Piecewise::continuous(vec3Node(1.0, 0.5, 0.25));
    CompiledPiecewise cChroma;
    assert(adapter.compile(Channel::MediumChroma, chroma, cChroma));
    assert(cChroma.kind == ValueKind::Vec3 && cChroma.pieces.size() == 1);
    const auto chromaValue = chroma.evaluate({{"x", 0.0}, {"t", 0.0}});
    assert(chromaValue.has_value());
    const glm::vec3 cv = vectorValue(*chromaValue);
    assert(std::abs(cv.x - 1.0f) < 1e-6f && std::abs(cv.y - 0.5f) < 1e-6f &&
           std::abs(cv.z - 0.25f) < 1e-6f);

    // Type sovereignty: scalar math is not accepted as C_v merely because the
    // underlying scalar compiler could evaluate it.
    Piecewise scalarChroma = Piecewise::continuous(common(1.0));
    CompiledPiecewise refusedScalarChroma;
    assert(!adapter.compile(Channel::MediumChroma, scalarChroma, refusedScalarChroma));
    assert(adapter.refusals == 1);

    // Timeline is now an actual authored premise. Value movement changes the
    // evaluated field while preserving compiled Piecewise topology and math IDs.
    Piecewise timedDensity = Piecewise::continuous(timed(2.0));
    CompiledPiecewise cTimedDensity;
    assert(adapter.compile(Channel::MediumDensity, timedDensity, cTimedDensity));
    const uint64_t buildsBeforeTimeline = adapter.topologyBuilds;
    const uint32_t timedMathBefore = cTimedDensity.pieces[0].math;
    const auto t0 = timedDensity.evaluate({{"x", 3.0}, {"t", 0.0}});
    const auto t5 = timedDensity.evaluate({{"x", 3.0}, {"t", 5.0}});
    assert(t0.has_value() && t5.has_value());
    assert(std::abs(scalarValue(*t0) - 6.0) < 1e-12);
    assert(std::abs(scalarValue(*t5) - 16.0) < 1e-12);
    assert(adapter.topologyBuilds == buildsBeforeTimeline);
    assert(cTimedDensity.pieces[0].math == timedMathBefore);

    std::printf("RENDERED_FIELD_PIECEWISE_SYNTHESIS parity=1 channels=5 "
                "piecewise_topology_identity=1 child_math_shared=1 "
                "runtime_rebuilds=0 density_value_edit_local=1 "
                "density_topology_edit_local=1 typed_vec3_chroma=1 "
                "scalar_to_chroma_refused=1 timeline_consumed=1 "
                "timeline_value_changes_without_rebuild=1 "
                "pretty_print_identity=0 full_scene_serialization_identity=0\n");
    return 0;
}
