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
    bool propheticPositive = false;  // derived annotation, never authority
    bool propheticValid = false;
};

struct Counters {
    uint64_t leafEvaluations = 0;
    uint64_t nodesVisited = 0;
    uint64_t cacheHits = 0;
    uint64_t proofAnnotationsConsulted = 0;
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
                             std::move(authoredDependency), literal, false, false});
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
               std::unordered_map<uint32_t, double>& memo, Counters& c) {
    auto m = memo.find(id);
    if (m != memo.end()) {
        ++c.cacheHits;
        return m->second;
    }
    const Node& n = dag.nodes[id];
    ++c.nodesVisited;
    if (n.propheticPositive && n.propheticValid)
        ++c.proofAnnotationsConsulted;
    double v = n.literal;
    if (n.op == Op::Input) {
        ++c.leafEvaluations;
    } else {
        assert(n.children.size() == 2);
        const double a = evalDag(dag, n.children[0], memo, c);
        const double b = evalDag(dag, n.children[1], memo, c);
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
        if (n.propheticPositive && n.propheticValid) {
            n.propheticValid = false;
            ++invalidated;
        }
    }
    return invalidated;
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

    // A proof fact lives on the execution branch that derives it. It remains
    // derived state: exact evaluation is authoritative even if this is invalid.
    dag.nodes[sdfA].propheticPositive = true;
    dag.nodes[sdfA].propheticValid = true;

    Counters naiveCounters;
    const double expected =
        naive(7.0, 2.0, 3.0, 5.0, 11.0, naiveCounters);
    Counters dagCounters;
    std::unordered_map<uint32_t, double> memo;
    const double actual = evalDag(dag, scene, memo, dagCounters);
    assert(std::abs(expected - actual) < 1e-12);
    assert(dagCounters.leafEvaluations < naiveCounters.leafEvaluations);
    assert(dagCounters.cacheHits >= 1);
    assert(dagCounters.proofAnnotationsConsulted == 1);
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
    assert(dag.nodes[sdfA].propheticValid);
    assert(ambientRepairCounters.leafEvaluations == 1);

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
    const size_t proofAnnotationsInvalidated =
        invalidateDirtyProofAnnotations(dag, dirty);
    assert(authoredPayloadsWritten == 1);
    assert(proofAnnotationsInvalidated == 1);
    assert(!dag.nodes[sdfA].propheticValid);

    Counters repairCounters;
    const double repaired = evalDag(dag, scene, memo, repairCounters);

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
    assert(memo.at(shared) == sharedBefore);
    assert(memo.at(sdfB) == sdfBBefore);
    assert(dag.nodes.size() == semanticNodeCount);
    assert(dag.canonical.size() == canonicalCount);

    const size_t preserved = dag.nodes.size() - dirty.size();
    const size_t estimatedBytes = dag.nodes.size() * sizeof(Node);
    const size_t authoredPayloadBytesWritten =
        authoredPayloadsWritten * sizeof(double);
    const size_t repairedCacheBytes = erased * sizeof(double);

    std::printf(
        "SCENE_SPATIAL_SYNTHESIS parity=1 nodes=%zu artifact_bytes_est=%zu "
        "naive_leaf_evals=%llu dag_leaf_evals=%llu shared_evals_avoided=%llu "
        "dag_nodes_visited=%llu cache_hits=%llu proof_annotations_consulted=%llu "
        "ambient_sample_change=1 ambient_cache_invalidated=%zu "
        "ambient_repair_nodes=%llu ambient_leaf_evals=%llu "
        "semantic_nodes_rebuilt_for_ambient=0 proof_artifacts_rebuilt_for_ambient=0 "
        "mutation=sdfA.bias invalidated=%zu cache_entries_repaired=%zu "
        "repair_nodes_visited=%llu repair_leaf_evals=%llu preserved=%zu "
        "authored_payload_bytes_written=%zu repaired_cache_bytes=%zu "
        "proof_annotations_invalidated=%zu structural_nodes_rebuilt=0 "
        "whole_scene_rebuild=0 camera_rebuild=0 global_relevance_search=0\n",
        dag.nodes.size(), estimatedBytes,
        static_cast<unsigned long long>(naiveCounters.leafEvaluations),
        static_cast<unsigned long long>(dagCounters.leafEvaluations),
        static_cast<unsigned long long>(
            naiveCounters.leafEvaluations - dagCounters.leafEvaluations),
        static_cast<unsigned long long>(dagCounters.nodesVisited),
        static_cast<unsigned long long>(dagCounters.cacheHits),
        static_cast<unsigned long long>(
            dagCounters.proofAnnotationsConsulted),
        ambientErased,
        static_cast<unsigned long long>(ambientRepairCounters.nodesVisited),
        static_cast<unsigned long long>(
            ambientRepairCounters.leafEvaluations),
        dirty.size(), erased,
        static_cast<unsigned long long>(repairCounters.nodesVisited),
        static_cast<unsigned long long>(repairCounters.leafEvaluations),
        preserved, authoredPayloadBytesWritten, repairedCacheBytes,
        proofAnnotationsInvalidated);
    return 0;
}
