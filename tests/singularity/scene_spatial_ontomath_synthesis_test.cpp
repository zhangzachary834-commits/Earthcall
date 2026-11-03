// Test-only bridge from Earthcall's real OntoMath::MathNode AST into a
// canonical scene-spatial execution DAG. No production renderer/WGSL consumes
// this compiler.
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

using OntoMath::MathNode;
using OntoMath::ScalarForm;

std::unique_ptr<MathNode> leaf(const std::string& name) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::ValueLeaf;
    n->variableName = name;
    return n;
}

std::unique_ptr<MathNode> constant(double v) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::ScalarLeaf;
    n->scalarForm = ScalarForm::constant(v);
    return n;
}

std::unique_ptr<MathNode> binary(
    MathNode::Op op, std::unique_ptr<MathNode> a,
    std::unique_ptr<MathNode> b) {
    auto n = std::make_unique<MathNode>();
    n->op = op;
    n->children.push_back(std::move(a));
    n->children.push_back(std::move(b));
    return n;
}

std::unique_ptr<MathNode> sharedExpr() {
    return binary(
        MathNode::Op::Scale,
        binary(MathNode::Op::Add, leaf("x"), constant(2.0)),
        constant(3.0));
}

std::unique_ptr<MathNode> authoredScene() {
    auto sdfA =
        binary(MathNode::Op::Sub, sharedExpr(), constant(5.0));
    auto sdfB =
        binary(MathNode::Op::Sub, sharedExpr(), constant(11.0));
    return binary(MathNode::Op::Union, std::move(sdfA), std::move(sdfB));
}

size_t countSourceNodes(const MathNode& n) {
    size_t total = 1;
    for (const auto& child : n.children)
        if (child) total += countSourceNodes(*child);
    return total;
}

bool supported(MathNode::Op op) {
    return op == MathNode::Op::ScalarLeaf ||
           op == MathNode::Op::ValueLeaf ||
           op == MathNode::Op::Add ||
           op == MathNode::Op::Sub ||
           op == MathNode::Op::Scale ||
           op == MathNode::Op::Union;
}

struct CompiledNode {
    uint32_t id = 0;
    MathNode::Op op = MathNode::Op::ScalarLeaf;
    ScalarForm scalarForm;
    std::string variableName;
    std::string stringArg;
    std::vector<uint32_t> children;
    std::string semanticKey;

    // Test-only conservative support theorem attached directly to the compiled
    // execution node. It has no authority when invalid.
    bool supportProofValid = false;
    uint32_t supportWinnerChild = 0;
};

struct CompileCounters {
    uint64_t sourceNodesVisited = 0;
    uint64_t canonicalHits = 0;
    uint64_t nodesCreated = 0;
};

struct EvalCounters {
    uint64_t nodesVisited = 0;
    uint64_t cacheHits = 0;
    uint64_t supportConsultations = 0;
    uint64_t supportBypasses = 0;
    uint64_t supportFallbacks = 0;
};

// Cross-domain Rung 1F separates reusable mathematical execution identity from
// channel-specific theorem authority. The same compiled calculation may serve
// geometry, source radiance, and participating-medium density without allowing
// a theorem from one authored meaning to become authority for another.
enum class SemanticChannel {
    GeometrySdf,
    SourceRadiance,
    MediumDensity
};

enum class ChannelProofKind {
    GeometryDistanceSupport,
    RadianceContributionSupport,
    MediumDensitySupport
};

struct ChannelProof {
    ChannelProofKind kind = ChannelProofKind::GeometryDistanceSupport;
    bool valid = false;
};

struct ChannelProofLedger {
    using Key = std::pair<SemanticChannel, uint32_t>;

    std::map<Key, ChannelProof> proofs;
    std::unordered_map<const MathNode*, std::vector<Key>> reversePremises;

    void attach(
        SemanticChannel channel, uint32_t compiledId, ChannelProofKind kind,
        const std::vector<const MathNode*>& premises) {
        const Key key{channel, compiledId};
        proofs[key] = ChannelProof{kind, true};
        for (const MathNode* premise : premises)
            reversePremises[premise].push_back(key);
    }

    bool valid(SemanticChannel channel, uint32_t compiledId) const {
        auto it = proofs.find(Key{channel, compiledId});
        return it != proofs.end() && it->second.valid;
    }

    ChannelProofKind kind(
        SemanticChannel channel, uint32_t compiledId) const {
        auto it = proofs.find(Key{channel, compiledId});
        assert(it != proofs.end());
        return it->second.kind;
    }

    size_t invalidate(
        const std::unordered_set<const MathNode*>& changedFrontier) {
        std::set<Key> invalidated;
        for (const MathNode* changed : changedFrontier) {
            auto it = reversePremises.find(changed);
            if (it == reversePremises.end()) continue;
            for (const Key& key : it->second) {
                auto proof = proofs.find(key);
                if (proof != proofs.end() && proof->second.valid) {
                    proof->second.valid = false;
                    invalidated.insert(key);
                }
            }
        }
        return invalidated.size();
    }
};

struct OntoSceneCompiler {
    std::vector<CompiledNode> nodes;
    std::unordered_map<std::string, uint32_t> canonical;
    std::unordered_map<const MathNode*, uint32_t> sourceToCompiled;
    std::unordered_map<const MathNode*, std::vector<const MathNode*>>
        sourceParents;

    // Authored source premise -> compiled execution nodes whose support proof
    // depends on that premise. This lets change-driven invalidation avoid a
    // global proof-table scan.
    std::unordered_map<const MathNode*, std::unordered_set<uint32_t>>
        proofDependents;
    uint64_t supportProofBuilds = 0;

    std::string localPayload(const MathNode& n) const {
        // Do not use MathNode::print() as semantic identity. The key is built
        // from the serialized/local semantic payload plus already-canonical
        // child IDs. ScalarForm::normalized() is exact algebra, so using its
        // JSON only canonicalizes representation, not meaning.
        std::string out =
            "op=" + std::to_string(static_cast<int>(n.op)) +
            "|var=" + n.variableName +
            "|arg=" + n.stringArg;
        if (n.op == MathNode::Op::ScalarLeaf)
            out += "|scalar=" + n.scalarForm.normalized().toJson().dump();
        return out;
    }

    std::string semanticKey(
        const MathNode& n, const std::vector<uint32_t>& childIds) const {
        std::string key = localPayload(n);
        key += "|children=";
        for (uint32_t id : childIds) {
            key += std::to_string(id);
            key.push_back(',');
        }
        return key;
    }

    uint32_t internCurrent(
        const MathNode& n, const std::vector<uint32_t>& childIds,
        CompileCounters& counters) {
        assert(supported(n.op));
        const std::string key = semanticKey(n, childIds);
        auto it = canonical.find(key);
        if (it != canonical.end()) {
            ++counters.canonicalHits;
            return it->second;
        }
        const uint32_t id = static_cast<uint32_t>(nodes.size());
        nodes.push_back(
            CompiledNode{id, n.op, n.scalarForm, n.variableName,
                         n.stringArg, childIds, key, false, 0});
        canonical.emplace(key, id);
        ++counters.nodesCreated;
        return id;
    }

    uint32_t compile(
        const MathNode& n, CompileCounters& counters,
        const MathNode* parent = nullptr) {
        ++counters.sourceNodesVisited;
        assert(supported(n.op));
        if (parent) sourceParents[&n].push_back(parent);

        std::vector<uint32_t> childIds;
        childIds.reserve(n.children.size());
        for (const auto& child : n.children) {
            assert(child);
            childIds.push_back(compile(*child, counters, &n));
        }

        const uint32_t id = internCurrent(n, childIds, counters);
        sourceToCompiled[&n] = id;
        return id;
    }

    uint32_t recompileOne(
        const MathNode& n, CompileCounters& counters) {
        ++counters.sourceNodesVisited;
        std::vector<uint32_t> childIds;
        childIds.reserve(n.children.size());
        for (const auto& child : n.children) {
            assert(child);
            auto it = sourceToCompiled.find(child.get());
            assert(it != sourceToCompiled.end());
            childIds.push_back(it->second);
        }
        const uint32_t id = internCurrent(n, childIds, counters);
        sourceToCompiled[&n] = id;
        return id;
    }

    std::unordered_set<const MathNode*> repairFrom(
        const MathNode& changed, CompileCounters& counters) {
        // The authored change feed names the changed source node. Repair walks
        // only its source-parent dependency frontier; it never scans the
        // canonical table or the whole authored scene to rediscover relevance.
        std::unordered_set<const MathNode*> repaired;
        std::vector<const MathNode*> work{&changed};
        while (!work.empty()) {
            const MathNode* n = work.back();
            work.pop_back();
            if (!repaired.insert(n).second) continue;
            recompileOne(*n, counters);
            auto p = sourceParents.find(n);
            if (p != sourceParents.end())
                work.insert(work.end(), p->second.begin(), p->second.end());
        }
        return repaired;
    }

    bool rebuildUnionSupportProof(
        const MathNode& rootSource,
        const MathNode& leftSource,
        const MathNode& rightSource,
        const MathNode& sharedLeftSource,
        const MathNode& sharedRightSource,
        const MathNode& leftBiasSource,
        const MathNode& rightBiasSource) {
        const uint32_t rootId = sourceToCompiled.at(&rootSource);
        const uint32_t leftId = sourceToCompiled.at(&leftSource);
        const uint32_t rightId = sourceToCompiled.at(&rightSource);
        const uint32_t sharedLeftId = sourceToCompiled.at(&sharedLeftSource);
        const uint32_t sharedRightId = sourceToCompiled.at(&sharedRightSource);
        const uint32_t leftBiasId = sourceToCompiled.at(&leftBiasSource);
        const uint32_t rightBiasId = sourceToCompiled.at(&rightBiasSource);

        CompiledNode& root = nodes[rootId];
        root.supportProofValid = false;

        if (root.op != MathNode::Op::Union ||
            root.children.size() != 2 ||
            root.children[0] != leftId ||
            root.children[1] != rightId)
            return false;

        const CompiledNode& left = nodes[leftId];
        const CompiledNode& right = nodes[rightId];
        if (left.op != MathNode::Op::Sub ||
            right.op != MathNode::Op::Sub ||
            left.children.size() != 2 ||
            right.children.size() != 2)
            return false;

        // The theorem is derived from canonical compiled semantic identity:
        // both subtraction branches must consume the same compiled shared
        // subtree. Source-object pointer equality is neither required nor used.
        if (sharedLeftId != sharedRightId ||
            left.children[0] != sharedLeftId ||
            right.children[0] != sharedRightId ||
            left.children[1] != leftBiasId ||
            right.children[1] != rightBiasId)
            return false;

        const CompiledNode& leftBias = nodes[leftBiasId];
        const CompiledNode& rightBias = nodes[rightBiasId];
        if (leftBias.op != MathNode::Op::ScalarLeaf ||
            rightBias.op != MathNode::Op::ScalarLeaf)
            return false;

        const std::map<std::string, double> noVars;
        const auto leftValue = leftBias.scalarForm.evaluate(noVars);
        const auto rightValue = rightBias.scalarForm.evaluate(noVars);
        if (!leftValue.has_value() || !rightValue.has_value())
            return false;

        // min(shared-a, shared-b): the larger bias always yields the smaller
        // result for every runtime value of shared.
        root.supportWinnerChild =
            (*leftValue >= *rightValue) ? leftId : rightId;
        root.supportProofValid = true;

        const MathNode* premises[] = {
            &rootSource, &leftSource, &rightSource,
            &sharedLeftSource, &sharedRightSource,
            &leftBiasSource, &rightBiasSource
        };
        for (const MathNode* premise : premises)
            proofDependents[premise].insert(rootId);

        ++supportProofBuilds;
        return true;
    }

    size_t invalidateSupportProofs(
        const std::unordered_set<const MathNode*>& changedFrontier) {
        std::unordered_set<uint32_t> invalidatedIds;
        for (const MathNode* changed : changedFrontier) {
            auto it = proofDependents.find(changed);
            if (it == proofDependents.end()) continue;
            for (uint32_t id : it->second) {
                if (nodes[id].supportProofValid) {
                    nodes[id].supportProofValid = false;
                    invalidatedIds.insert(id);
                }
            }
        }
        return invalidatedIds.size();
    }
};

double evalCompiled(
    const OntoSceneCompiler& compiler, uint32_t id,
    const std::map<std::string, double>& vars,
    std::unordered_map<uint32_t, double>& memo,
    EvalCounters& counters, bool allowSupportProof = false) {
    auto m = memo.find(id);
    if (m != memo.end()) {
        ++counters.cacheHits;
        return m->second;
    }
    ++counters.nodesVisited;
    const CompiledNode& n = compiler.nodes[id];
    double v = 0.0;
    if (n.op == MathNode::Op::ScalarLeaf) {
        const auto value = n.scalarForm.evaluate(vars);
        assert(value.has_value());
        v = *value;
    } else if (n.op == MathNode::Op::ValueLeaf) {
        auto it = vars.find(n.variableName);
        assert(it != vars.end());
        v = it->second;
    } else {
        assert(n.children.size() == 2);

        // Support is only optimization authority when a valid theorem is
        // attached to this exact compiled Union node. Invalid/missing proof
        // falls open to exact evaluation of both children.
        if (allowSupportProof && n.op == MathNode::Op::Union) {
            ++counters.supportConsultations;
            if (n.supportProofValid) {
                assert(n.supportWinnerChild == n.children[0] ||
                       n.supportWinnerChild == n.children[1]);
                ++counters.supportBypasses;
                v = evalCompiled(
                    compiler, n.supportWinnerChild, vars, memo,
                    counters, true);
                memo[id] = v;
                return v;
            }
            ++counters.supportFallbacks;
        }

        const double a = evalCompiled(
            compiler, n.children[0], vars, memo,
            counters, allowSupportProof);
        const double b = evalCompiled(
            compiler, n.children[1], vars, memo,
            counters, allowSupportProof);
        if (n.op == MathNode::Op::Add) v = a + b;
        else if (n.op == MathNode::Op::Sub) v = a - b;
        else if (n.op == MathNode::Op::Scale) v = a * b;
        else if (n.op == MathNode::Op::Union) v = std::min(a, b);
        else assert(false);
    }
    memo[id] = v;
    return v;
}

double exactMathNode(const MathNode& n, double x) {
    std::map<std::string, PropertyValue> vars{
        {"x", PropertyValue(x)}
    };
    const auto value = n.evaluate(vars);
    assert(value.has_value());
    double numeric = 0.0;
    assert(propertyValueToNumber(*value, numeric));
    return numeric;
}

} // namespace

int main() {
    auto scene = authoredScene();
    const size_t sourceNodes = countSourceNodes(*scene);
    assert(sourceNodes == 15);

    // Keep stable source pointers for the mutation/provenance witness.
    MathNode* sdfA = scene->children[0].get();
    MathNode* sdfB = scene->children[1].get();
    MathNode* sharedA = sdfA->children[0].get();
    MathNode* sharedB = sdfB->children[0].get();
    MathNode* biasA = sdfA->children[1].get();
    MathNode* biasB = sdfB->children[1].get();

    OntoSceneCompiler compiler;
    CompileCounters initialCompile;
    const uint32_t root =
        compiler.compile(*scene, initialCompile);

    // Two separately-authored but structurally identical MathNode subtrees
    // synthesize to one compiled execution identity.
    assert(sharedA != sharedB);
    assert(compiler.sourceToCompiled.at(sharedA) ==
           compiler.sourceToCompiled.at(sharedB));
    assert(compiler.nodes.size() < sourceNodes);

    const uint32_t sharedCompiledBefore =
        compiler.sourceToCompiled.at(sharedA);
    const uint32_t sdfBCompiledBefore =
        compiler.sourceToCompiled.at(sdfB);
    const size_t semanticNodesBeforeAmbient = compiler.nodes.size();
    const size_t canonicalNodesBeforeAmbient = compiler.canonical.size();

    // Real-OntoMath Rung 1E: derive a conservative Union support theorem from
    // canonical compiled identity, not source pointer identity or print text.
    assert(compiler.rebuildUnionSupportProof(
        *scene, *sdfA, *sdfB, *sharedA, *sharedB, *biasA, *biasB));
    assert(compiler.supportProofBuilds == 1);
    assert(compiler.nodes[root].supportProofValid);
    assert(compiler.nodes[root].supportWinnerChild ==
           compiler.sourceToCompiled.at(sdfB));
    assert(compiler.proofDependents.at(biasA).count(root) == 1);
    assert(compiler.proofDependents.at(biasB).count(root) == 1);

    std::unordered_map<uint32_t, double> initialExactMemo;
    EvalCounters initialExactEval;
    const double initialExactCompiled =
        evalCompiled(compiler, root, {{"x", 7.0}},
                     initialExactMemo, initialExactEval, false);

    std::unordered_map<uint32_t, double> initialSupportMemo;
    EvalCounters initialSupportEval;
    const double initialSupportCompiled =
        evalCompiled(compiler, root, {{"x", 7.0}},
                     initialSupportMemo, initialSupportEval, true);
    assert(std::abs(initialExactCompiled - initialSupportCompiled) < 1e-12);
    assert(initialSupportEval.supportConsultations == 1);
    assert(initialSupportEval.supportBypasses == 1);
    assert(initialSupportEval.supportFallbacks == 0);
    assert(initialSupportEval.nodesVisited < initialExactEval.nodesVisited);
    const uint64_t initialSupportNodesAvoided =
        initialExactEval.nodesVisited - initialSupportEval.nodesVisited;

    // Runtime/camera sample changes are evaluation state, not authored semantic
    // changes. Exercise several samples while explicitly invalidating the value
    // memo between samples. The semantic DAG and canonical identities must stay
    // byte-for-byte structurally untouched.
    const std::vector<double> ambientSamples{
        -13.0, -1.25, 0.0, 7.0, 42.5
    };
    std::unordered_map<uint32_t, double> ambientMemo;
    size_t ambientCacheEntriesInvalidated = 0;
    uint64_t ambientNodesVisited = 0;
    uint64_t ambientCacheHits = 0;
    uint64_t ambientSupportBypasses = 0;
    uint64_t ambientSupportNodesAvoided = 0;
    for (double x : ambientSamples) {
        if (!ambientMemo.empty()) {
            ambientCacheEntriesInvalidated += ambientMemo.size();
            ambientMemo.clear();
        }
        const double exact = exactMathNode(*scene, x);

        EvalCounters exactEval;
        const double compiled =
            evalCompiled(compiler, root, {{"x", x}},
                         ambientMemo, exactEval, false);
        assert(std::abs(exact - compiled) < 1e-12);
        assert(exactEval.cacheHits > 0);
        ambientNodesVisited += exactEval.nodesVisited;
        ambientCacheHits += exactEval.cacheHits;

        std::unordered_map<uint32_t, double> supportMemo;
        EvalCounters supportEval;
        const double supported =
            evalCompiled(compiler, root, {{"x", x}},
                         supportMemo, supportEval, true);
        assert(std::abs(exact - supported) < 1e-12);
        assert(supportEval.supportConsultations == 1);
        assert(supportEval.supportBypasses == 1);
        assert(supportEval.supportFallbacks == 0);
        assert(supportEval.nodesVisited < exactEval.nodesVisited);
        ++ambientSupportBypasses;
        ambientSupportNodesAvoided +=
            exactEval.nodesVisited - supportEval.nodesVisited;

        assert(compiler.nodes.size() == semanticNodesBeforeAmbient);
        assert(compiler.canonical.size() == canonicalNodesBeforeAmbient);
        assert(compiler.sourceToCompiled.at(scene.get()) == root);
        assert(compiler.sourceToCompiled.at(sharedA) ==
               sharedCompiledBefore);
        assert(compiler.sourceToCompiled.at(sharedB) ==
               sharedCompiledBefore);
        assert(compiler.sourceToCompiled.at(sdfB) ==
               sdfBCompiledBefore);
        assert(compiler.nodes[root].supportProofValid);
        assert(compiler.supportProofBuilds == 1);
    }
    assert(ambientCacheEntriesInvalidated > 0);
    assert(ambientSupportBypasses == ambientSamples.size());

    const size_t compiledNodesBeforeRepair = compiler.nodes.size();

    // Local authored edit on the real MathNode tree.
    biasA->scalarForm = ScalarForm::constant(17.0);

    CompileCounters repairCounters;
    const auto repairedSources =
        compiler.repairFrom(*biasA, repairCounters);

    // Exactly biasA -> sdfA -> scene is structurally repaired. The separately
    // authored shared subtree and independent sdfB branch retain compiled IDs.
    assert(repairedSources.size() == 3);
    assert(repairedSources.count(biasA) == 1);
    assert(repairedSources.count(sdfA) == 1);
    assert(repairedSources.count(scene.get()) == 1);
    assert(repairedSources.count(sharedA) == 0);
    assert(repairedSources.count(sdfB) == 0);
    assert(compiler.sourceToCompiled.at(sharedA) ==
           sharedCompiledBefore);
    assert(compiler.sourceToCompiled.at(sharedB) ==
           sharedCompiledBefore);
    assert(compiler.sourceToCompiled.at(sdfB) ==
           sdfBCompiledBefore);
    assert(repairCounters.sourceNodesVisited == 3);
    assert(repairCounters.nodesCreated == 3);

    const size_t supportProofsInvalidated =
        compiler.invalidateSupportProofs(repairedSources);
    assert(supportProofsInvalidated == 1);
    assert(!compiler.nodes[root].supportProofValid);

    const uint32_t repairedRoot =
        compiler.sourceToCompiled.at(scene.get());
    const double exactAfter = exactMathNode(*scene, 7.0);

    // The newly repaired root has no support theorem yet. Enabling the support
    // road must therefore fall open to full exact compiled evaluation.
    std::unordered_map<uint32_t, double> fallbackMemo;
    EvalCounters fallbackEval;
    const double fallbackCompiled =
        evalCompiled(compiler, repairedRoot, {{"x", 7.0}},
                     fallbackMemo, fallbackEval, true);
    assert(std::abs(exactAfter - fallbackCompiled) < 1e-12);
    assert(fallbackEval.supportConsultations == 1);
    assert(fallbackEval.supportBypasses == 0);
    assert(fallbackEval.supportFallbacks == 1);

    std::unordered_map<uint32_t, double> repairedMemo;
    EvalCounters repairedEval;
    const double compiledAfter =
        evalCompiled(compiler, repairedRoot, {{"x", 7.0}},
                     repairedMemo, repairedEval, false);
    assert(std::abs(exactAfter - compiledAfter) < 1e-12);

    const size_t newCompiledNodes =
        compiler.nodes.size() - compiledNodesBeforeRepair;
    assert(newCompiledNodes == 3);

    // Re-prove only after the authored premise mutation. The exact same theorem
    // now selects sdfA because biasA=17 exceeds biasB=11.
    assert(compiler.rebuildUnionSupportProof(
        *scene, *sdfA, *sdfB, *sharedA, *sharedB, *biasA, *biasB));
    assert(compiler.supportProofBuilds == 2);
    assert(compiler.nodes[repairedRoot].supportWinnerChild ==
           compiler.sourceToCompiled.at(sdfA));

    std::unordered_map<uint32_t, double> postRepairSupportMemo;
    EvalCounters postRepairSupportEval;
    const double postRepairSupported =
        evalCompiled(compiler, repairedRoot, {{"x", 7.0}},
                     postRepairSupportMemo, postRepairSupportEval, true);
    assert(std::abs(exactAfter - postRepairSupported) < 1e-12);
    assert(postRepairSupportEval.supportConsultations == 1);
    assert(postRepairSupportEval.supportBypasses == 1);
    assert(postRepairSupportEval.supportFallbacks == 0);
    assert(postRepairSupportEval.nodesVisited < repairedEval.nodesVisited);
    const uint64_t postRepairSupportNodesAvoided =
        repairedEval.nodesVisited - postRepairSupportEval.nodesVisited;

    // A second authored mutation returns the same source leaf to its original
    // semantics. This must repair the same three-source frontier but reuse the
    // already-canonicalized biasA/sdfA/scene artifact rather than append another
    // structural copy.
    const size_t compiledNodesBeforeRevert = compiler.nodes.size();
    biasA->scalarForm = ScalarForm::constant(5.0);

    CompileCounters revertCounters;
    const auto revertedSources =
        compiler.repairFrom(*biasA, revertCounters);
    assert(revertedSources.size() == 3);
    assert(revertedSources.count(biasA) == 1);
    assert(revertedSources.count(sdfA) == 1);
    assert(revertedSources.count(scene.get()) == 1);
    assert(revertCounters.sourceNodesVisited == 3);
    assert(revertCounters.nodesCreated == 0);
    assert(revertCounters.canonicalHits == 3);
    assert(compiler.nodes.size() == compiledNodesBeforeRevert);
    assert(compiler.sourceToCompiled.at(scene.get()) == root);
    assert(compiler.sourceToCompiled.at(sharedA) ==
           sharedCompiledBefore);
    assert(compiler.sourceToCompiled.at(sharedB) ==
           sharedCompiledBefore);
    assert(compiler.sourceToCompiled.at(sdfB) ==
           sdfBCompiledBefore);

    const size_t revertSupportProofsInvalidated =
        compiler.invalidateSupportProofs(revertedSources);
    assert(revertSupportProofsInvalidated == 1);

    // Returning to previously-seen semantics reuses the original canonical root,
    // but its stale theorem was invalidated. Rebuild that proof rather than
    // silently reviving old derived state.
    assert(compiler.rebuildUnionSupportProof(
        *scene, *sdfA, *sdfB, *sharedA, *sharedB, *biasA, *biasB));
    assert(compiler.supportProofBuilds == 3);
    assert(compiler.nodes[root].supportWinnerChild ==
           compiler.sourceToCompiled.at(sdfB));

    uint64_t revertParitySamples = 0;
    uint64_t revertSupportBypasses = 0;
    for (double x : ambientSamples) {
        std::unordered_map<uint32_t, double> memo;
        EvalCounters exactEval;
        const double exact = exactMathNode(*scene, x);
        const double compiled =
            evalCompiled(compiler, root, {{"x", x}},
                         memo, exactEval, false);
        assert(std::abs(exact - compiled) < 1e-12);

        std::unordered_map<uint32_t, double> supportMemo;
        EvalCounters supportEval;
        const double supported =
            evalCompiled(compiler, root, {{"x", x}},
                         supportMemo, supportEval, true);
        assert(std::abs(exact - supported) < 1e-12);
        assert(supportEval.supportConsultations == 1);
        assert(supportEval.supportBypasses == 1);
        assert(supportEval.supportFallbacks == 0);
        ++revertSupportBypasses;
        ++revertParitySamples;
    }
    assert(revertParitySamples == ambientSamples.size());
    assert(revertSupportBypasses == ambientSamples.size());

    // ---------------------------------------------------------------------
    // Rung 1F: cross-domain semantic-synthesis boundary.
    //
    // Three independently authored channels intentionally contain identical
    // mathematics. The execution compiler is allowed to share the canonical
    // calculation node. Proof authority is NOT allowed to collapse with it.
    // ---------------------------------------------------------------------
    auto geometryExpr = sharedExpr();
    auto radianceExpr = sharedExpr();
    auto densityExpr = sharedExpr();

    MathNode* geometryScale = geometryExpr->children[1].get();
    MathNode* radianceScale = radianceExpr->children[1].get();
    MathNode* densityScale = densityExpr->children[1].get();

    CompileCounters geometryCompile, radianceCompile, densityCompile;
    const uint32_t geometryCompiled =
        compiler.compile(*geometryExpr, geometryCompile);
    const uint32_t radianceCompiled =
        compiler.compile(*radianceExpr, radianceCompile);
    const uint32_t densityCompiled =
        compiler.compile(*densityExpr, densityCompile);

    // Identical mathematics may share one execution artifact even though the
    // authored truths remain different.
    assert(geometryExpr.get() != radianceExpr.get());
    assert(radianceExpr.get() != densityExpr.get());
    assert(geometryCompiled == radianceCompiled);
    assert(radianceCompiled == densityCompiled);

    ChannelProofLedger channelProofs;
    channelProofs.attach(
        SemanticChannel::GeometrySdf, geometryCompiled,
        ChannelProofKind::GeometryDistanceSupport,
        {geometryExpr.get(), geometryScale});
    channelProofs.attach(
        SemanticChannel::SourceRadiance, radianceCompiled,
        ChannelProofKind::RadianceContributionSupport,
        {radianceExpr.get(), radianceScale});
    channelProofs.attach(
        SemanticChannel::MediumDensity, densityCompiled,
        ChannelProofKind::MediumDensitySupport,
        {densityExpr.get(), densityScale});

    // Same compiled ID, three independent theorem meanings.
    assert(channelProofs.valid(
        SemanticChannel::GeometrySdf, geometryCompiled));
    assert(channelProofs.valid(
        SemanticChannel::SourceRadiance, radianceCompiled));
    assert(channelProofs.valid(
        SemanticChannel::MediumDensity, densityCompiled));
    assert(channelProofs.kind(
        SemanticChannel::GeometrySdf, geometryCompiled) ==
           ChannelProofKind::GeometryDistanceSupport);
    assert(channelProofs.kind(
        SemanticChannel::SourceRadiance, radianceCompiled) ==
           ChannelProofKind::RadianceContributionSupport);
    assert(channelProofs.kind(
        SemanticChannel::MediumDensity, densityCompiled) ==
           ChannelProofKind::MediumDensitySupport);

    // Mutate ONLY the authored medium-density expression. Its source-parent
    // frontier is disjoint from the separately authored geometry/radiance
    // source trees even though all three initially mapped to one compiled node.
    densityScale->scalarForm = ScalarForm::constant(4.0);
    CompileCounters densityRepairCounters;
    const auto densityRepairedSources =
        compiler.repairFrom(*densityScale, densityRepairCounters);
    assert(densityRepairedSources.size() == 2);
    assert(densityRepairedSources.count(densityScale) == 1);
    assert(densityRepairedSources.count(densityExpr.get()) == 1);
    assert(densityRepairedSources.count(geometryScale) == 0);
    assert(densityRepairedSources.count(radianceScale) == 0);

    const size_t crossDomainProofsInvalidated =
        channelProofs.invalidate(densityRepairedSources);
    assert(crossDomainProofsInvalidated == 1);

    // Density's theorem is invalid; geometry and radiance theorem authority on
    // the old shared compiled node remains intact because their authored
    // premises did not change.
    assert(channelProofs.valid(
        SemanticChannel::GeometrySdf, geometryCompiled));
    assert(channelProofs.valid(
        SemanticChannel::SourceRadiance, radianceCompiled));
    assert(!channelProofs.valid(
        SemanticChannel::MediumDensity, densityCompiled));

    const uint32_t densityCompiledAfter =
        compiler.sourceToCompiled.at(densityExpr.get());
    assert(densityCompiledAfter != densityCompiled);
    assert(compiler.sourceToCompiled.at(geometryExpr.get()) ==
           geometryCompiled);
    assert(compiler.sourceToCompiled.at(radianceExpr.get()) ==
           radianceCompiled);

    // Execution truth also separates after the authored density mutation.
    const double crossX = 7.0;
    const double geometryExact = exactMathNode(*geometryExpr, crossX);
    const double radianceExact = exactMathNode(*radianceExpr, crossX);
    const double densityExact = exactMathNode(*densityExpr, crossX);
    assert(std::abs(geometryExact - radianceExact) < 1e-12);
    assert(std::abs(densityExact - geometryExact) > 1e-12);

    std::unordered_map<uint32_t, double> crossMemo;
    EvalCounters crossEval;
    const double densityCompiledValue =
        evalCompiled(compiler, densityCompiledAfter, {{"x", crossX}},
                     crossMemo, crossEval, false);
    assert(std::abs(densityCompiledValue - densityExact) < 1e-12);

    // Attach the new density-channel theorem to the new compiled calculation.
    // Geometry/radiance proof records remain scoped to their own channels.
    channelProofs.attach(
        SemanticChannel::MediumDensity, densityCompiledAfter,
        ChannelProofKind::MediumDensitySupport,
        {densityExpr.get(), densityScale});
    assert(channelProofs.valid(
        SemanticChannel::MediumDensity, densityCompiledAfter));
    assert(!channelProofs.valid(
        SemanticChannel::MediumDensity, densityCompiled));

    // Revert the density math to the byte/semantic-identical original. The
    // compiler reuses the common execution node, but the old density theorem
    // does not regain validity merely because the shared node ID returns.
    densityScale->scalarForm = ScalarForm::constant(3.0);
    CompileCounters densityRevertCounters;
    const auto densityRevertedSources =
        compiler.repairFrom(*densityScale, densityRevertCounters);
    assert(compiler.sourceToCompiled.at(densityExpr.get()) ==
           geometryCompiled);
    const size_t densityRevertProofsInvalidated =
        channelProofs.invalidate(densityRevertedSources);
    assert(densityRevertProofsInvalidated == 1);
    assert(!channelProofs.valid(
        SemanticChannel::MediumDensity, geometryCompiled));
    assert(channelProofs.valid(
        SemanticChannel::GeometrySdf, geometryCompiled));
    assert(channelProofs.valid(
        SemanticChannel::SourceRadiance, radianceCompiled));

    // Explicit re-proof restores only density-channel authority.
    channelProofs.attach(
        SemanticChannel::MediumDensity, geometryCompiled,
        ChannelProofKind::MediumDensitySupport,
        {densityExpr.get(), densityScale});
    assert(channelProofs.valid(
        SemanticChannel::MediumDensity, geometryCompiled));

    std::printf(
        "SCENE_SPATIAL_ONTOMATH_SYNTHESIS parity=1 "
        "source_nodes=%zu compiled_nodes_initial=%zu canonical_hits=%llu "
        "source_nodes_visited_initial=%llu shared_subtree_identity=1 "
        "support_proof_from_canonical_identity=1 support_builds=%llu "
        "initial_support_bypasses=%llu initial_support_nodes_avoided=%llu "
        "ambient_samples=%zu ambient_cache_entries_invalidated=%zu "
        "ambient_nodes_visited=%llu ambient_cache_hits=%llu "
        "ambient_support_bypasses=%llu ambient_support_nodes_avoided=%llu "
        "semantic_nodes_rebuilt_for_ambient=0 support_proofs_rebuilt_for_ambient=0 "
        "mutation_biasA_to17=1 repaired_source_nodes=%zu repair_source_visits=%llu "
        "new_compiled_nodes=%zu shared_compiled_id_preserved=1 "
        "sdfB_compiled_id_preserved=1 whole_scene_rescan_for_repair=0 "
        "support_proofs_invalidated=%zu invalid_support_fallbacks=%llu "
        "post_repair_support_bypasses=%llu post_repair_support_nodes_avoided=%llu "
        "mutation_biasA_revert=1 revert_source_visits=%llu "
        "revert_nodes_created=%llu revert_canonical_hits=%llu "
        "revert_support_proofs_invalidated=%zu revert_support_bypasses=%llu "
        "prior_artifact_reused_on_revert=1 revert_parity_samples=%llu "
        "proof_invalidation_global_scan=0 pretty_print_identity=0 "
        "cross_domain_math_shared=1 channel_scoped_proofs=1 "
        "density_only_proof_invalidations=%zu "
        "density_revert_proof_invalidations=%zu "
        "geometry_proof_preserved=1 radiance_proof_preserved=1 "
        "production_wgsl_changed=0\n",
        sourceNodes, compiledNodesBeforeRepair,
        static_cast<unsigned long long>(initialCompile.canonicalHits),
        static_cast<unsigned long long>(initialCompile.sourceNodesVisited),
        static_cast<unsigned long long>(compiler.supportProofBuilds),
        static_cast<unsigned long long>(initialSupportEval.supportBypasses),
        static_cast<unsigned long long>(initialSupportNodesAvoided),
        ambientSamples.size(), ambientCacheEntriesInvalidated,
        static_cast<unsigned long long>(ambientNodesVisited),
        static_cast<unsigned long long>(ambientCacheHits),
        static_cast<unsigned long long>(ambientSupportBypasses),
        static_cast<unsigned long long>(ambientSupportNodesAvoided),
        repairedSources.size(),
        static_cast<unsigned long long>(repairCounters.sourceNodesVisited),
        newCompiledNodes,
        supportProofsInvalidated,
        static_cast<unsigned long long>(fallbackEval.supportFallbacks),
        static_cast<unsigned long long>(postRepairSupportEval.supportBypasses),
        static_cast<unsigned long long>(postRepairSupportNodesAvoided),
        static_cast<unsigned long long>(revertCounters.sourceNodesVisited),
        static_cast<unsigned long long>(revertCounters.nodesCreated),
        static_cast<unsigned long long>(revertCounters.canonicalHits),
        revertSupportProofsInvalidated,
        static_cast<unsigned long long>(revertSupportBypasses),
        static_cast<unsigned long long>(revertParitySamples),
        crossDomainProofsInvalidated,
        densityRevertProofsInvalidated);
    return 0;
}
