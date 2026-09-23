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
enum class ProofKind { None, DensityZeroSupport, RadianceZeroContribution };

std::unique_ptr<MathNode> scalarU(double v) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::ScalarLeaf;
    n->scalarForm = ScalarForm::constant(v);
    return n;
}
std::shared_ptr<MathNode> scalarS(double v) {
    return std::shared_ptr<MathNode>(scalarU(v).release());
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
struct SupportProof {
    bool valid = false;
    Channel channel = Channel::SourceRho;
    ValueKind kind = ValueKind::Scalar;
    ProofKind theorem = ProofKind::None;
    std::string topologyKey;
    std::vector<uint32_t> premiseMath;
    std::vector<const MathNode*> premiseSources;
    std::vector<size_t> zeroPieces;
    uint64_t generation = 0;
};

struct CompiledPiecewise {
    Channel channel = Channel::SourceRho;
    ValueKind kind = ValueKind::Scalar;
    std::string inputVariable;
    std::vector<CompiledPiece> pieces;
    std::string topologyKey;
    SupportProof proof;
};

struct PiecewiseAdapter {
    MathCompiler math;
    uint64_t topologyBuilds = 0;
    uint64_t refusals = 0;
    uint64_t proofBuilds = 0;
    uint64_t proofInvalidations = 0;
    uint64_t proofConsultations = 0;
    uint64_t proofBypasses = 0;
    uint64_t proofFallbacks = 0;
    uint64_t proofRefusals = 0;
    uint64_t proofPremiseInspections = 0;
    uint64_t exactEvaluationsAvoided = 0;

    bool compile(Channel channel, const Piecewise& model, CompiledPiecewise& out) {
        const ValueKind kind = channel == Channel::MediumChroma ? ValueKind::Vec3 : ValueKind::Scalar;
        const SupportProof priorProof = out.proof;

        CompiledPiecewise next;
        next.channel = channel;
        next.kind = kind;
        next.inputVariable = model.inputVariable;
        std::ostringstream topology;
        topology << "kind=" << static_cast<int>(kind) << "|input=" << model.inputVariable
                 << "|pieces=" << model.pieces.size();

        std::vector<uint32_t> premiseMath;
        std::vector<const MathNode*> premiseSources;
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
            premiseMath.push_back(mathId);
            premiseSources.push_back(p.mathNode.get());
            topology << "|" << p.hasLo << ":" << p.lo << ":" << p.includeLo
                     << ":" << p.hasHi << ":" << p.hi << ":" << p.includeHi
                     << ":math=" << mathId;
        }
        next.topologyKey = topology.str();

        // Recompiling a stable vessel may preserve a proof only when every
        // declared premise remains identical. Canonical math equality alone is
        // insufficient: replacing an authored source node with a new but
        // equivalent node is still an authored-premise change.
        if (priorProof.valid) {
            const bool stillValid =
                priorProof.channel == next.channel &&
                priorProof.kind == next.kind &&
                priorProof.topologyKey == next.topologyKey &&
                priorProof.premiseMath == premiseMath &&
                priorProof.premiseSources == premiseSources;
            if (stillValid) next.proof = priorProof;
            else ++proofInvalidations;
        }

        out = std::move(next);
        ++topologyBuilds;
        return true;
    }

    bool buildDensityZeroSupportProof(
        const Piecewise& model, CompiledPiecewise& vessel) {
        if (vessel.channel != Channel::MediumDensity ||
            vessel.kind != ValueKind::Scalar ||
            vessel.pieces.size() != model.pieces.size()) {
            ++proofRefusals;
            return false;
        }

        SupportProof proof;
        proof.valid = true;
        proof.channel = Channel::MediumDensity;
        proof.kind = ValueKind::Scalar;
        proof.theorem = ProofKind::DensityZeroSupport;
        proof.topologyKey = vessel.topologyKey;

        const std::map<std::string, double> noVars;
        for (size_t i = 0; i < model.pieces.size(); ++i) {
            ++proofPremiseInspections;
            const auto& authored = model.pieces[i];
            assert(authored.mathNode);
            proof.premiseMath.push_back(vessel.pieces[i].math);
            proof.premiseSources.push_back(authored.mathNode.get());

            if (authored.mathNode->op != MathNode::Op::ScalarLeaf)
                continue;
            const auto value = authored.mathNode->scalarForm.evaluate(noVars);
            if (value.has_value() && std::abs(*value) < 1e-12)
                proof.zeroPieces.push_back(i);
        }

        if (proof.zeroPieces.empty()) {
            ++proofRefusals;
            return false;
        }

        proof.generation = ++proofBuilds;
        vessel.proof = std::move(proof);
        return true;
    }

    bool buildRadianceZeroContributionProof(
        const Piecewise& model, CompiledPiecewise& vessel) {
        if (vessel.channel != Channel::SourceRho ||
            vessel.kind != ValueKind::Scalar ||
            vessel.pieces.size() != model.pieces.size()) {
            ++proofRefusals;
            return false;
        }

        SupportProof proof;
        proof.valid = true;
        proof.channel = Channel::SourceRho;
        proof.kind = ValueKind::Scalar;
        proof.theorem = ProofKind::RadianceZeroContribution;
        proof.topologyKey = vessel.topologyKey;

        const std::map<std::string, double> noVars;
        for (size_t i = 0; i < model.pieces.size(); ++i) {
            ++proofPremiseInspections;
            const auto& authored = model.pieces[i];
            assert(authored.mathNode);
            proof.premiseMath.push_back(vessel.pieces[i].math);
            proof.premiseSources.push_back(authored.mathNode.get());

            if (authored.mathNode->op != MathNode::Op::ScalarLeaf)
                continue;
            const auto value = authored.mathNode->scalarForm.evaluate(noVars);
            if (value.has_value() && std::abs(*value) < 1e-12)
                proof.zeroPieces.push_back(i);
        }

        if (proof.zeroPieces.empty()) {
            ++proofRefusals;
            return false;
        }

        proof.generation = ++proofBuilds;
        vessel.proof = std::move(proof);
        return true;
    }

    static bool pieceContains(const CompiledPiece& piece, double x) {
        if (piece.hasLo && (x < piece.lo || (x == piece.lo && !piece.includeLo)))
            return false;
        if (piece.hasHi && (x > piece.hi || (x == piece.hi && !piece.includeHi)))
            return false;
        return true;
    }

    bool queryZeroSupport(
        const CompiledPiecewise& vessel, const Piecewise& model,
        double x, double t, bool allowProof) {
        size_t pieceIndex = vessel.pieces.size();
        for (size_t i = 0; i < vessel.pieces.size(); ++i) {
            if (pieceContains(vessel.pieces[i], x)) {
                pieceIndex = i;
                break;
            }
        }

        if (allowProof) {
            ++proofConsultations;
            const SupportProof& proof = vessel.proof;
            const bool authoritative =
                proof.valid &&
                proof.theorem == ProofKind::DensityZeroSupport &&
                proof.channel == Channel::MediumDensity &&
                vessel.channel == Channel::MediumDensity &&
                proof.kind == vessel.kind &&
                proof.topologyKey == vessel.topologyKey;

            if (authoritative && pieceIndex < vessel.pieces.size()) {
                for (size_t zeroPiece : proof.zeroPieces) {
                    if (zeroPiece == pieceIndex) {
                        ++proofBypasses;
                        ++exactEvaluationsAvoided;
                        return true;
                    }
                }
            }
            ++proofFallbacks;
        }

        const auto value = model.evaluate({{"x", x}, {"t", t}});
        assert(value.has_value());
        const auto* scalar = std::get_if<double>(&*value);
        assert(scalar);
        return std::abs(*scalar) < 1e-12;
    }

    bool queryZeroRadianceContribution(
        const CompiledPiecewise& vessel, const Piecewise& model,
        double x, double t, bool allowProof) {
        size_t pieceIndex = vessel.pieces.size();
        for (size_t i = 0; i < vessel.pieces.size(); ++i) {
            if (pieceContains(vessel.pieces[i], x)) {
                pieceIndex = i;
                break;
            }
        }

        if (allowProof) {
            ++proofConsultations;
            const SupportProof& proof = vessel.proof;
            const bool authoritative =
                proof.valid &&
                proof.theorem == ProofKind::RadianceZeroContribution &&
                proof.channel == Channel::SourceRho &&
                vessel.channel == Channel::SourceRho &&
                proof.kind == vessel.kind &&
                proof.topologyKey == vessel.topologyKey;

            if (authoritative && pieceIndex < vessel.pieces.size()) {
                for (size_t zeroPiece : proof.zeroPieces) {
                    if (zeroPiece == pieceIndex) {
                        ++proofBypasses;
                        ++exactEvaluationsAvoided;
                        return true;
                    }
                }
            }
            ++proofFallbacks;
        }

        const auto value = model.evaluate({{"x", x}, {"t", t}});
        assert(value.has_value());
        const auto* scalar = std::get_if<double>(&*value);
        assert(scalar);
        return std::abs(*scalar) < 1e-12;
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

    // Rung 1I: theorem authority belongs to the rendered-field vessel,
    // not to the canonical math node. Two channels intentionally compile
    // byte-identical zero mathematics to the same calculation IDs.
    auto zeroDensity = twoPiece(scalarS(0.0), scalarS(0.0));
    auto zeroRho = twoPiece(scalarS(0.0), scalarS(0.0));
    CompiledPiecewise cZeroDensity, cZeroRho;
    assert(adapter.compile(Channel::MediumDensity, zeroDensity, cZeroDensity));
    assert(adapter.compile(Channel::SourceRho, zeroRho, cZeroRho));
    assert(cZeroDensity.pieces[0].math == cZeroRho.pieces[0].math);
    assert(cZeroDensity.pieces[1].math == cZeroRho.pieces[1].math);
    assert(cZeroDensity.channel != cZeroRho.channel);
    const uint32_t rhoZeroLeftMath = cZeroRho.pieces[0].math;
    const uint32_t rhoZeroRightMath = cZeroRho.pieces[1].math;
    const std::string rhoZeroTopology = cZeroRho.topologyKey;
    assert(!cZeroRho.proof.valid);

    // Build a density-only exact-zero support theorem. The same theorem builder
    // explicitly refuses SourceRho even though the canonical zero math is shared.
    assert(adapter.buildDensityZeroSupportProof(zeroDensity, cZeroDensity));
    assert(cZeroDensity.proof.valid);
    assert(cZeroDensity.proof.channel == Channel::MediumDensity);
    assert(cZeroDensity.proof.zeroPieces.size() == 2);
    assert(!adapter.buildDensityZeroSupportProof(zeroRho, cZeroRho));
    assert(!cZeroRho.proof.valid);

    const uint64_t bypassesBeforeDensity = adapter.proofBypasses;
    assert(adapter.queryZeroSupport(cZeroDensity, zeroDensity, -5.0, 0.0, true));
    assert(adapter.proofBypasses == bypassesBeforeDensity + 1);

    // Even a deliberately copied density proof cannot acquire radiance
    // authority: the vessel channel check forces exact SourceRho fallback.
    CompiledPiecewise forgedRho = cZeroRho;
    forgedRho.proof = cZeroDensity.proof;
    const uint64_t bypassesBeforeForgedRho = adapter.proofBypasses;
    const uint64_t fallbacksBeforeForgedRho = adapter.proofFallbacks;
    assert(adapter.queryZeroSupport(forgedRho, zeroRho, -5.0, 0.0, true));
    assert(adapter.proofBypasses == bypassesBeforeForgedRho);
    assert(adapter.proofFallbacks == fallbacksBeforeForgedRho + 1);

    // Topology mutation invalidates the density theorem on the stable vessel.
    // While invalid, support-enabled queries fall open to exact Piecewise truth.
    const uint64_t invalidationsBeforeTopology = adapter.proofInvalidations;
    zeroDensity.pieces[0].hi = -2.0;
    zeroDensity.pieces[1].lo = -2.0;
    assert(adapter.compile(Channel::MediumDensity, zeroDensity, cZeroDensity));
    assert(adapter.proofInvalidations == invalidationsBeforeTopology + 1);
    assert(!cZeroDensity.proof.valid);
    const uint64_t fallbacksBeforeTopology = adapter.proofFallbacks;
    assert(adapter.queryZeroSupport(cZeroDensity, zeroDensity, -5.0, 7.0, true));
    assert(adapter.proofFallbacks == fallbacksBeforeTopology + 1);

    // Local re-proof restores the density bypass. Runtime x/t movement then
    // consumes the same theorem with zero topology or theorem rebuilds.
    assert(adapter.buildDensityZeroSupportProof(zeroDensity, cZeroDensity));
    const uint64_t proofBuildsBeforeRuntime = adapter.proofBuilds;
    const uint64_t topologyBuildsBeforeProofRuntime = adapter.topologyBuilds;
    for (double x : {-9.0, -3.0, 0.0, 12.0}) {
        for (double t : {0.0, 2.0, 99.0}) {
            assert(adapter.queryZeroSupport(
                cZeroDensity, zeroDensity, x, t, true));
        }
    }
    assert(adapter.proofBuilds == proofBuildsBeforeRuntime);
    assert(adapter.topologyBuilds == topologyBuildsBeforeProofRuntime);

    // Authored child mutation on density only invalidates density proof state,
    // while the separately authored SourceRho vessel retains its exact topology
    // and canonical zero calculation IDs.
    const uint64_t invalidationsBeforeChild = adapter.proofInvalidations;
    zeroDensity.pieces[0].mathNode->scalarForm = ScalarForm::constant(2.0);
    assert(adapter.compile(Channel::MediumDensity, zeroDensity, cZeroDensity));
    assert(adapter.proofInvalidations == invalidationsBeforeChild + 1);
    assert(!cZeroDensity.proof.valid);
    assert(cZeroRho.topologyKey == rhoZeroTopology);
    assert(cZeroRho.pieces[0].math == rhoZeroLeftMath);
    assert(cZeroRho.pieces[1].math == rhoZeroRightMath);
    assert(!cZeroRho.proof.valid);

    // Invalid density proof falls open: left interval is now non-zero while the
    // untouched right interval remains exactly zero.
    const uint64_t fallbacksBeforeChild = adapter.proofFallbacks;
    assert(!adapter.queryZeroSupport(
        cZeroDensity, zeroDensity, -5.0, 0.0, true));
    assert(adapter.queryZeroSupport(
        cZeroDensity, zeroDensity, 5.0, 0.0, true));
    assert(adapter.proofFallbacks == fallbacksBeforeChild + 2);

    // Re-proof becomes partial: only the still-zero right interval is allowed
    // to bypass. The nonzero left interval continues to use exact fallback.
    assert(adapter.buildDensityZeroSupportProof(zeroDensity, cZeroDensity));
    assert(cZeroDensity.proof.zeroPieces.size() == 1);
    const uint64_t bypassesBeforePartial = adapter.proofBypasses;
    const uint64_t fallbacksBeforePartial = adapter.proofFallbacks;
    assert(!adapter.queryZeroSupport(
        cZeroDensity, zeroDensity, -5.0, 0.0, true));
    assert(adapter.queryZeroSupport(
        cZeroDensity, zeroDensity, 5.0, 0.0, true));
    assert(adapter.proofBypasses == bypassesBeforePartial + 1);
    assert(adapter.proofFallbacks == fallbacksBeforePartial + 1);

    // Rung 1J: source radiance gets its own theorem algebra rather than
    // reusing density semantics. The underlying zero math remains shareable.
    auto theoremRho = twoPiece(scalarS(0.0), scalarS(0.0));
    CompiledPiecewise cTheoremRho;
    assert(adapter.compile(Channel::SourceRho, theoremRho, cTheoremRho));
    assert(cTheoremRho.pieces[0].math == rhoZeroLeftMath);
    assert(cTheoremRho.pieces[1].math == rhoZeroRightMath);

    assert(adapter.buildRadianceZeroContributionProof(
        theoremRho, cTheoremRho));
    assert(cTheoremRho.proof.valid);
    assert(cTheoremRho.proof.channel == Channel::SourceRho);
    assert(cTheoremRho.proof.theorem ==
           ProofKind::RadianceZeroContribution);
    assert(cTheoremRho.proof.zeroPieces.size() == 2);

    // The radiance theorem cannot be consumed by density, symmetric with the
    // Rung 1I hostile density->radiance copy test.
    CompiledPiecewise forgedDensity = cZeroDensity;
    forgedDensity.proof = cTheoremRho.proof;
    const uint64_t bypassesBeforeForgedDensity = adapter.proofBypasses;
    const uint64_t fallbacksBeforeForgedDensity = adapter.proofFallbacks;
    assert(adapter.queryZeroSupport(
        forgedDensity, zeroDensity, 5.0, 0.0, true));
    assert(adapter.proofBypasses == bypassesBeforeForgedDensity);
    assert(adapter.proofFallbacks == fallbacksBeforeForgedDensity + 1);

    const uint64_t rhoBypassesBefore = adapter.proofBypasses;
    assert(adapter.queryZeroRadianceContribution(
        cTheoremRho, theoremRho, -5.0, 0.0, true));
    assert(adapter.queryZeroRadianceContribution(
        cTheoremRho, theoremRho, 5.0, 0.0, true));
    assert(adapter.proofBypasses == rhoBypassesBefore + 2);

    // Runtime x/t motion consumes the same radiance theorem without semantic
    // or proof rebuilds.
    const uint64_t rhoBuildsBeforeRuntime = adapter.proofBuilds;
    const uint64_t rhoTopologyBeforeRuntime = adapter.topologyBuilds;
    for (double x : {-8.0, -1.0, 0.0, 9.0}) {
        for (double t : {0.0, 3.0, 55.0}) {
            assert(adapter.queryZeroRadianceContribution(
                cTheoremRho, theoremRho, x, t, true));
        }
    }
    assert(adapter.proofBuilds == rhoBuildsBeforeRuntime);
    assert(adapter.topologyBuilds == rhoTopologyBeforeRuntime);

    // A radiance-only topology mutation invalidates radiance proof authority
    // while leaving the already-compiled density vessel and its theorem alone.
    const std::string densityTopologyBeforeRhoMutation =
        cZeroDensity.topologyKey;
    const uint64_t densityGenerationBeforeRhoMutation =
        cZeroDensity.proof.generation;
    const uint64_t invalidationsBeforeRhoTopology =
        adapter.proofInvalidations;
    theoremRho.pieces[0].hi = -4.0;
    theoremRho.pieces[1].lo = -4.0;
    assert(adapter.compile(
        Channel::SourceRho, theoremRho, cTheoremRho));
    assert(adapter.proofInvalidations ==
           invalidationsBeforeRhoTopology + 1);
    assert(!cTheoremRho.proof.valid);
    assert(cZeroDensity.topologyKey == densityTopologyBeforeRhoMutation);
    assert(cZeroDensity.proof.generation ==
           densityGenerationBeforeRhoMutation);

    const uint64_t rhoFallbacksBeforeTopology =
        adapter.proofFallbacks;
    assert(adapter.queryZeroRadianceContribution(
        cTheoremRho, theoremRho, -8.0, 0.0, true));
    assert(adapter.proofFallbacks ==
           rhoFallbacksBeforeTopology + 1);

    assert(adapter.buildRadianceZeroContributionProof(
        theoremRho, cTheoremRho));

    // A radiance child edit narrows the radiance theorem only. Density proof
    // state remains untouched even though zero calculations were canonicalized
    // across both semantic channels.
    const uint64_t densityGenerationBeforeRhoChild =
        cZeroDensity.proof.generation;
    theoremRho.pieces[0].mathNode->scalarForm =
        ScalarForm::constant(4.0);
    const uint64_t invalidationsBeforeRhoChild =
        adapter.proofInvalidations;
    assert(adapter.compile(
        Channel::SourceRho, theoremRho, cTheoremRho));
    assert(adapter.proofInvalidations ==
           invalidationsBeforeRhoChild + 1);
    assert(!cTheoremRho.proof.valid);
    assert(cZeroDensity.proof.generation ==
           densityGenerationBeforeRhoChild);

    const uint64_t rhoFallbacksBeforeChild =
        adapter.proofFallbacks;
    assert(!adapter.queryZeroRadianceContribution(
        cTheoremRho, theoremRho, -8.0, 0.0, true));
    assert(adapter.queryZeroRadianceContribution(
        cTheoremRho, theoremRho, 5.0, 0.0, true));
    assert(adapter.proofFallbacks ==
           rhoFallbacksBeforeChild + 2);

    assert(adapter.buildRadianceZeroContributionProof(
        theoremRho, cTheoremRho));
    assert(cTheoremRho.proof.zeroPieces.size() == 1);
    const uint64_t rhoBypassesBeforePartial = adapter.proofBypasses;
    const uint64_t rhoFallbacksBeforePartial = adapter.proofFallbacks;
    assert(!adapter.queryZeroRadianceContribution(
        cTheoremRho, theoremRho, -8.0, 0.0, true));
    assert(adapter.queryZeroRadianceContribution(
        cTheoremRho, theoremRho, 5.0, 0.0, true));
    assert(adapter.proofBypasses ==
           rhoBypassesBeforePartial + 1);
    assert(adapter.proofFallbacks ==
           rhoFallbacksBeforePartial + 1);

    // Economics are expressed as semantic work units rather than wall-clock
    // timing in this tiny deterministic witness.
    assert(adapter.exactEvaluationsAvoided > adapter.proofBuilds);
    assert(adapter.proofPremiseInspections >= adapter.proofBuilds);

    // Rung 1J regression contract: pin the deterministic theorem economics.
    // These counters make accidental proof leakage, unexpected fallback work,
    // or proof-rebuild churn visible instead of letting a weaker inequality
    // silently absorb the regression.
    assert(adapter.proofBuilds == 6);
    assert(adapter.proofInvalidations == 4);
    assert(adapter.proofConsultations == 39);
    assert(adapter.proofBypasses == 29);
    assert(adapter.proofFallbacks == 10);
    assert(adapter.proofRefusals == 1);
    assert(adapter.proofPremiseInspections == 12);
    assert(adapter.exactEvaluationsAvoided == 29);

    std::printf("RENDERED_FIELD_PIECEWISE_SYNTHESIS parity=1 channels=5 "
                "piecewise_topology_identity=1 child_math_shared=1 "
                "runtime_rebuilds=0 density_value_edit_local=1 "
                "density_topology_edit_local=1 typed_vec3_chroma=1 "
                "scalar_to_chroma_refused=1 timeline_consumed=1 "
                "timeline_value_changes_without_rebuild=1 "
                "vessel_scoped_density_zero_proof=1 cross_channel_math_shared=1 "
                "radiance_cannot_borrow_density_proof=1 "
                "topology_change_invalidates_density_proof=1 "
                "invalid_proof_exact_fallback=1 local_reproof=1 "
                "runtime_proof_rebuilds=0 density_child_edit_local=1 "
                "partial_zero_support_reproof=1 "
                "radiance_zero_contribution_proof=1 "
                "radiance_cannot_authorize_density=1 "
                "radiance_runtime_rebuilds=0 radiance_child_edit_local=1 "
                "partial_radiance_zero_reproof=1 "
                "proof_builds=%llu proof_invalidations=%llu "
                "proof_consultations=%llu proof_bypasses=%llu "
                "proof_fallbacks=%llu proof_refusals=%llu "
                "proof_premise_inspections=%llu exact_evaluations_avoided=%llu "
                "pretty_print_identity=0 full_scene_serialization_identity=0\n",
                static_cast<unsigned long long>(adapter.proofBuilds),
                static_cast<unsigned long long>(adapter.proofInvalidations),
                static_cast<unsigned long long>(adapter.proofConsultations),
                static_cast<unsigned long long>(adapter.proofBypasses),
                static_cast<unsigned long long>(adapter.proofFallbacks),
                static_cast<unsigned long long>(adapter.proofRefusals),
                static_cast<unsigned long long>(adapter.proofPremiseInspections),
                static_cast<unsigned long long>(adapter.exactEvaluationsAvoided));
    return 0;
}
