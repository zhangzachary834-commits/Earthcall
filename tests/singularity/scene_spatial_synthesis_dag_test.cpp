// Test-only first rung for post-PR #301 scene-spatial synthesis.
// No production renderer/WGSL path consumes this representation.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

enum class Op { Input, Add, Sub, Mul, Min };

struct Node {
    uint32_t id = 0;
    Op op = Op::Input;
    std::string key;                 // canonical semantic identity
    std::vector<uint32_t> children;  // exact execution dependencies
    std::string authoredDependency;  // non-empty only for authored inputs
    double literal = 0.0;
    bool supportProofValid = false;   // derived annotation, never authority
    uint32_t supportWinnerChild = 0;  // meaningful only for a proved Min node
};

struct Counters {
    uint64_t leafEvaluations = 0;
    uint64_t nodesVisited = 0;
    uint64_t cacheHits = 0;
    uint64_t supportProofConsultations = 0;
    uint64_t supportProofBypasses = 0;
    uint64_t supportProofFallbacks = 0;
};

struct SceneSpatialDag {
    std::vector<Node> nodes;
    std::unordered_map<std::string, uint32_t> canonical;
    std::unordered_map<std::string, std::unordered_set<uint32_t>> reverseAuthored;
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> parents;

    uint32_t intern(Op op, const std::string& key,
                    std::vector<uint32_t> children = {},
                    std::string authoredDependency = {}, double literal = 0.0) {
        auto it = canonical.find(key);
        if (it != canonical.end()) return it->second;
        const uint32_t id = static_cast<uint32_t>(nodes.size());
        nodes.push_back(Node{id, op, key, std::move(children),
                             std::move(authoredDependency), literal, false, 0});
        canonical[key] = id;
        if (!nodes[id].authoredDependency.empty())
            reverseAuthored[nodes[id].authoredDependency].insert(id);
        for (uint32_t child : nodes[id].children) parents[child].insert(id);
        return id;
    }

    std::unordered_set<uint32_t> downstreamFrontier(
        const std::vector<uint32_t>& seeds) const {
        std::unordered_set<uint32_t> dirty;
        std::vector<uint32_t> work = seeds;
        while (!work.empty()) {
            const uint32_t n = work.back();
            work.pop_back();
            if (!dirty.insert(n).second) continue;
            auto p = parents.find(n);
            if (p != parents.end())
                work.insert(work.end(), p->second.begin(), p->second.end());
        }
        return dirty;
    }

    std::unordered_set<uint32_t> invalidationFrontier(
        const std::string& dependency) const {
        auto it = reverseAuthored.find(dependency);
        if (it == reverseAuthored.end()) return {};
        return downstreamFrontier(
            std::vector<uint32_t>(it->second.begin(), it->second.end()));
    }

    size_t setAuthoredLiteral(const std::string& dependency, double value) {
        auto it = reverseAuthored.find(dependency);
        if (it == reverseAuthored.end()) return 0;
        size_t written = 0;
        for (uint32_t id : it->second) {
            nodes[id].literal = value;
            ++written;
        }
        return written;
    }
};

double evalDag(const SceneSpatialDag& dag, uint32_t id,
               std::unordered_map<uint32_t, double>& memo, Counters& c,
               bool allowSupportProof = false) {
    auto m = memo.find(id);
    if (m != memo.end()) {
        ++c.cacheHits;
        return m->second;
    }
    const Node& n = dag.nodes[id];
    ++c.nodesVisited;
    double v = n.literal;
    if (n.op == Op::Input) {
        ++c.leafEvaluations;
    } else {
        assert(n.children.size() == 2);

        // A conservative support proof may select the exact Min winner before
        // either branch is interpreted. Missing/stale proof never has authority:
        // it falls open to ordinary exact evaluation of both children.
        if (allowSupportProof && n.op == Op::Min) {
            ++c.supportProofConsultations;
            if (n.supportProofValid) {
                assert(n.supportWinnerChild == n.children[0] ||
                       n.supportWinnerChild == n.children[1]);
                ++c.supportProofBypasses;
                v = evalDag(dag, n.supportWinnerChild, memo, c, true);
                memo[id] = v;
                return v;
            }
            ++c.supportProofFallbacks;
        }

        const double a =
            evalDag(dag, n.children[0], memo, c, allowSupportProof);
        const double b =
            evalDag(dag, n.children[1], memo, c, allowSupportProof);
        if (n.op == Op::Add) v = a + b;
        else if (n.op == Op::Sub) v = a - b;
        else if (n.op == Op::Mul) v = a * b;
        else if (n.op == Op::Min) v = std::min(a, b);
    }
    memo[id] = v;
    return v;
}

size_t eraseDirtyMemo(
    const std::unordered_set<uint32_t>& dirty,
    std::unordered_map<uint32_t, double>& memo) {
    size_t erased = 0;
    for (uint32_t id : dirty) erased += memo.erase(id);
    return erased;
}

size_t invalidateDirtyProofAnnotations(
    SceneSpatialDag& dag, const std::unordered_set<uint32_t>& dirty) {
    size_t invalidated = 0;
    for (uint32_t id : dirty) {
        Node& n = dag.nodes[id];
        if (n.supportProofValid) {
            n.supportProofValid = false;
            ++invalidated;
        }
    }
    return invalidated;
}

// This deliberately tiny proof builder recognizes one exact algebraic shape:
// min(shared - biasA, shared - biasB). Because both branches have the identical
// shared child, the branch with the larger bias is <= the other branch for every
// runtime value of shared. If the shape is not exactly recognized, it refuses to
// prove anything and execution must fall back to exact evaluation.
bool rebuildMinSupportProof(SceneSpatialDag& dag, uint32_t minId,
                            uint32_t leftId, uint32_t rightId,
                            uint32_t leftBiasId, uint32_t rightBiasId) {
    Node& root = dag.nodes[minId];
    root.supportProofValid = false;
    if (root.op != Op::Min || root.children.size() != 2 ||
        root.children[0] != leftId || root.children[1] != rightId)
        return false;

    const Node& left = dag.nodes[leftId];
    const Node& right = dag.nodes[rightId];
    if (left.op != Op::Sub || right.op != Op::Sub ||
        left.children.size() != 2 || right.children.size() != 2)
        return false;
    if (left.children[0] != right.children[0] ||
        left.children[1] != leftBiasId ||
        right.children[1] != rightBiasId)
        return false;

    const double leftBias = dag.nodes[leftBiasId].literal;
    const double rightBias = dag.nodes[rightBiasId].literal;
    root.supportWinnerChild =
        (leftBias >= rightBias) ? leftId : rightId;
    root.supportProofValid = true;
    return true;
}

// Reference evaluator intentionally repeats the shared authored expression for
// each independent SDF. This is the exact-semantics authority for this witness.
double naive(double p, double shift, double scale, double biasA, double biasB,
             Counters& c) {
    auto shared = [&]() {
        c.leafEvaluations += 3; // p, shift, scale
        const double shifted = p + shift;
        return shifted * scale;
    };
    c.leafEvaluations += 2; // two independent authored biases
    const double sdfA = shared() - biasA;
    const double sdfB = shared() - biasB;
    return std::min(sdfA, sdfB);
}

} // namespace

int main() {
    SceneSpatialDag dag;
    // Runtime sample input is ambient state, not authored semantic state.
    const uint32_t p = dag.intern(Op::Input, "ambient:p", {}, {}, 7.0);
    const uint32_t shift = dag.intern(
        Op::Input, "input:shared.shift", {}, "shared.shift", 2.0);
    const uint32_t scale = dag.intern(
        Op::Input, "input:shared.scale", {}, "shared.scale", 3.0);
    const uint32_t biasA = dag.intern(
        Op::Input, "input:sdfA.bias", {}, "sdfA.bias", 5.0);
    const uint32_t biasB = dag.intern(
        Op::Input, "input:sdfB.bias", {}, "sdfB.bias", 11.0);

    // Canonicalization means both authored SDFs consume this exact same subtree.
    const uint32_t shifted = dag.intern(
        Op::Add, "add(ambient:p,input:shared.shift)", {p, shift});
    const uint32_t shared = dag.intern(
        Op::Mul,
        "mul(add(ambient:p,input:shared.shift),input:shared.scale)",
        {shifted, scale});
    const uint32_t sharedAgain = dag.intern(
        Op::Mul,
        "mul(add(ambient:p,input:shared.shift),input:shared.scale)",
        {shifted, scale});
    assert(shared == sharedAgain);
    const uint32_t sdfA = dag.intern(
        Op::Sub, "sub(shared,sdfA.bias)", {shared, biasA});
    const uint32_t sdfB = dag.intern(
        Op::Sub, "sub(shared,sdfB.bias)", {shared, biasB});
    const uint32_t scene = dag.intern(
        Op::Min, "min(sdfA,sdfB)", {sdfA, sdfB});

    // A conservative support proof lives directly on the Min execution road.
    // Here both branches are (the same shared expression - bias), so the larger
    // bias is the exact Min winner for every runtime value of the shared term.
    assert(rebuildMinSupportProof(
        dag, scene, sdfA, sdfB, biasA, biasB));
    assert(dag.nodes[scene].supportWinnerChild == sdfB);

    Counters naiveCounters;
    const double expected =
        naive(7.0, 2.0, 3.0, 5.0, 11.0, naiveCounters);
    Counters dagCounters;
    std::unordered_map<uint32_t, double> memo;
    const double actual = evalDag(dag, scene, memo, dagCounters);
    assert(std::abs(expected - actual) < 1e-12);
    assert(dagCounters.leafEvaluations < naiveCounters.leafEvaluations);
    assert(dagCounters.cacheHits >= 1);

    // Compare the same exact DAG with its conservative support road enabled.
    // This is the first witness where a proof changes execution: sdfA is not
    // interpreted at all because sdfB is proved to be the Min winner.
    Counters supportCounters;
    std::unordered_map<uint32_t, double> supportMemo;
    const double supported =
        evalDag(dag, scene, supportMemo, supportCounters, true);
    assert(std::abs(expected - supported) < 1e-12);
    assert(supportCounters.supportProofConsultations == 1);
    assert(supportCounters.supportProofBypasses == 1);
    assert(supportCounters.supportProofFallbacks == 0);
    assert(supportCounters.nodesVisited < dagCounters.nodesVisited);
    assert(supportCounters.leafEvaluations < dagCounters.leafEvaluations);
    const uint64_t initialNodesAvoided =
        dagCounters.nodesVisited - supportCounters.nodesVisited;

    const size_t semanticNodeCount = dag.nodes.size();
    const size_t canonicalCount = dag.canonical.size();

    // Runtime/camera movement changes the ambient sample value. It may dirty
    // evaluation values, but it must not rebuild semantic DAG/proof artifacts.
    const auto ambientDirty = dag.downstreamFrontier({p});
    assert(ambientDirty.count(p) == 1);
    assert(ambientDirty.count(shared) == 1);
    assert(ambientDirty.count(sdfA) == 1);
    assert(ambientDirty.count(sdfB) == 1);
    assert(ambientDirty.count(scene) == 1);
    const size_t ambientErased = eraseDirtyMemo(ambientDirty, memo);
    dag.nodes[p].literal = 8.0;
    Counters ambientRepairCounters;
    const double ambientRepaired =
        evalDag(dag, scene, memo, ambientRepairCounters);
    Counters ambientExactCounters;
    const double ambientExpected =
        naive(8.0, 2.0, 3.0, 5.0, 11.0, ambientExactCounters);
    assert(std::abs(ambientRepaired - ambientExpected) < 1e-12);
    assert(dag.nodes.size() == semanticNodeCount);
    assert(dag.canonical.size() == canonicalCount);
    assert(dag.nodes[scene].supportProofValid);
    assert(ambientRepairCounters.leafEvaluations == 1);

    // Runtime movement preserves the proof artifact. Re-run through the support
    // road at the new sample and prove exact parity without rebuilding proof.
    std::unordered_map<uint32_t, double> ambientSupportMemo;
    Counters ambientSupportCounters;
    const double ambientSupported =
        evalDag(dag, scene, ambientSupportMemo, ambientSupportCounters, true);
    assert(std::abs(ambientSupported - ambientExpected) < 1e-12);
    assert(ambientSupportCounters.supportProofConsultations == 1);
    assert(ambientSupportCounters.supportProofBypasses == 1);
    assert(ambientSupportCounters.supportProofFallbacks == 0);

    // Local authored semantic edit. Unlike ambient movement, this invalidates
    // the affected proof annotation in addition to dependent cached values.
    const auto dirty = dag.invalidationFrontier("sdfA.bias");
    assert(dirty.count(biasA) == 1);
    assert(dirty.count(sdfA) == 1);
    assert(dirty.count(scene) == 1);
    assert(dirty.count(shared) == 0);
    assert(dirty.count(sdfB) == 0);
    assert(dirty.size() == 3);

    const double sharedBefore = memo.at(shared);
    const double sdfBBefore = memo.at(sdfB);
    const size_t erased = eraseDirtyMemo(dirty, memo);
    const size_t authoredPayloadsWritten =
        dag.setAuthoredLiteral("sdfA.bias", 17.0);
    const size_t supportProofsInvalidated =
        invalidateDirtyProofAnnotations(dag, dirty);
    assert(authoredPayloadsWritten == 1);
    assert(supportProofsInvalidated == 1);
    assert(!dag.nodes[scene].supportProofValid);

    // Stale proof does not guess. It falls open to exact evaluation while the
    // semantic repair reuses unaffected cached state.
    Counters repairCounters;
    const double repaired =
        evalDag(dag, scene, memo, repairCounters, true);

    // Fresh exact authority after the mutation: no reuse from the synthesized
    // cache is allowed in this comparison.
    Counters mutatedExactCounters;
    const double mutatedExpected =
        naive(8.0, 2.0, 3.0, 17.0, 11.0, mutatedExactCounters);
    assert(std::abs(repaired - mutatedExpected) < 1e-12);

    // Also compare against a completely fresh DAG evaluation after clearing all
    // evaluation state; structural compilation is still preserved.
    std::unordered_map<uint32_t, double> freshMemo;
    Counters freshDagCounters;
    const double freshDag =
        evalDag(dag, scene, freshMemo, freshDagCounters);
    assert(std::abs(repaired - freshDag) < 1e-12);

    // The repaired path must visit exactly the dirty execution frontier and
    // reuse the untouched shared subtree + independent sdfB branch.
    assert(repairCounters.nodesVisited == dirty.size());
    assert(repairCounters.leafEvaluations == 1);
    assert(repairCounters.supportProofConsultations == 1);
    assert(repairCounters.supportProofBypasses == 0);
    assert(repairCounters.supportProofFallbacks == 1);
    assert(memo.at(shared) == sharedBefore);
    assert(memo.at(sdfB) == sdfBBefore);
    assert(dag.nodes.size() == semanticNodeCount);
    assert(dag.canonical.size() == canonicalCount);

    // Re-prove only after the authored semantic change. The winner reverses:
    // with biasA=17 and biasB=11, sdfA is now <= sdfB for every shared value.
    assert(rebuildMinSupportProof(
        dag, scene, sdfA, sdfB, biasA, biasB));
    assert(dag.nodes[scene].supportWinnerChild == sdfA);
    std::unordered_map<uint32_t, double> postProofMemo;
    Counters postProofCounters;
    const double postProofValue =
        evalDag(dag, scene, postProofMemo, postProofCounters, true);
    assert(std::abs(postProofValue - mutatedExpected) < 1e-12);
    assert(postProofCounters.supportProofConsultations == 1);
    assert(postProofCounters.supportProofBypasses == 1);
    assert(postProofCounters.supportProofFallbacks == 0);
    assert(postProofCounters.nodesVisited < freshDagCounters.nodesVisited);
    const uint64_t postProofNodesAvoided =
        freshDagCounters.nodesVisited - postProofCounters.nodesVisited;

    const size_t preserved = dag.nodes.size() - dirty.size();
    const size_t estimatedBytes = dag.nodes.size() * sizeof(Node);
    const size_t authoredPayloadBytesWritten =
        authoredPayloadsWritten * sizeof(double);
    const size_t repairedCacheBytes = erased * sizeof(double);

    std::printf(
        "SCENE_SPATIAL_SYNTHESIS parity=1 nodes=%zu artifact_bytes_est=%zu "
        "naive_leaf_evals=%llu dag_leaf_evals=%llu shared_evals_avoided=%llu "
        "dag_nodes_visited=%llu cache_hits=%llu "
        "support_consultations=%llu support_bypasses=%llu "
        "support_nodes_visited=%llu support_nodes_avoided=%llu "
        "ambient_sample_change=1 ambient_cache_invalidated=%zu "
        "ambient_repair_nodes=%llu ambient_leaf_evals=%llu "
        "ambient_support_bypasses=%llu semantic_nodes_rebuilt_for_ambient=0 "
        "proof_artifacts_rebuilt_for_ambient=0 "
        "mutation=sdfA.bias invalidated=%zu cache_entries_repaired=%zu "
        "repair_nodes_visited=%llu repair_leaf_evals=%llu "
        "invalid_support_fallbacks=%llu preserved=%zu "
        "authored_payload_bytes_written=%zu repaired_cache_bytes=%zu "
        "support_proofs_invalidated=%zu proof_artifacts_rebuilt_after_authored=1 "
        "post_repair_support_bypasses=%llu post_repair_nodes_avoided=%llu "
        "structural_nodes_rebuilt=0 whole_scene_rebuild=0 camera_rebuild=0 "
        "global_relevance_search=0\n",
        dag.nodes.size(), estimatedBytes,
        static_cast<unsigned long long>(naiveCounters.leafEvaluations),
        static_cast<unsigned long long>(dagCounters.leafEvaluations),
        static_cast<unsigned long long>(
            naiveCounters.leafEvaluations - dagCounters.leafEvaluations),
        static_cast<unsigned long long>(dagCounters.nodesVisited),
        static_cast<unsigned long long>(dagCounters.cacheHits),
        static_cast<unsigned long long>(
            supportCounters.supportProofConsultations),
        static_cast<unsigned long long>(
            supportCounters.supportProofBypasses),
        static_cast<unsigned long long>(supportCounters.nodesVisited),
        static_cast<unsigned long long>(initialNodesAvoided),
        ambientErased,
        static_cast<unsigned long long>(ambientRepairCounters.nodesVisited),
        static_cast<unsigned long long>(
            ambientRepairCounters.leafEvaluations),
        static_cast<unsigned long long>(
            ambientSupportCounters.supportProofBypasses),
        dirty.size(), erased,
        static_cast<unsigned long long>(repairCounters.nodesVisited),
        static_cast<unsigned long long>(repairCounters.leafEvaluations),
        static_cast<unsigned long long>(
            repairCounters.supportProofFallbacks),
        preserved, authoredPayloadBytesWritten, repairedCacheBytes,
        supportProofsInvalidated,
        static_cast<unsigned long long>(
            postProofCounters.supportProofBypasses),
        static_cast<unsigned long long>(postProofNodesAvoided));
    return 0;
}
