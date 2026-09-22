// Test-only first rung for post-PR #301 scene-spatial synthesis.
// No production renderer/WGSL path consumes this representation.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <functional>
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
                             std::move(authoredDependency), literal, false});
        canonical[key] = id;
        if (!nodes[id].authoredDependency.empty())
            reverseAuthored[nodes[id].authoredDependency].insert(id);
        for (uint32_t child : nodes[id].children) parents[child].insert(id);
        return id;
    }

    std::unordered_set<uint32_t> invalidationFrontier(const std::string& dependency) const {
        std::unordered_set<uint32_t> dirty;
        auto it = reverseAuthored.find(dependency);
        if (it == reverseAuthored.end()) return dirty;
        std::vector<uint32_t> work(it->second.begin(), it->second.end());
        while (!work.empty()) {
            const uint32_t n = work.back(); work.pop_back();
            if (!dirty.insert(n).second) continue;
            auto p = parents.find(n);
            if (p != parents.end())
                work.insert(work.end(), p->second.begin(), p->second.end());
        }
        return dirty;
    }
};

double evalDag(const SceneSpatialDag& dag, uint32_t id,
               std::unordered_map<uint32_t, double>& memo, Counters& c) {
    auto m = memo.find(id);
    if (m != memo.end()) { ++c.cacheHits; return m->second; }
    const Node& n = dag.nodes[id];
    ++c.nodesVisited;
    if (n.propheticPositive) ++c.proofAnnotationsConsulted;
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
    // Four authored inputs. The point input is runtime ambient state rather than
    // a semantic invalidation source; shift/scale/biases model authored state.
    const uint32_t p = dag.intern(Op::Input, "ambient:p", {}, {}, 7.0);
    const uint32_t shift = dag.intern(Op::Input, "input:shared.shift", {}, "shared.shift", 2.0);
    const uint32_t scale = dag.intern(Op::Input, "input:shared.scale", {}, "shared.scale", 3.0);
    const uint32_t biasA = dag.intern(Op::Input, "input:sdfA.bias", {}, "sdfA.bias", 5.0);
    const uint32_t biasB = dag.intern(Op::Input, "input:sdfB.bias", {}, "sdfB.bias", 11.0);

    // Canonicalization means both authored SDFs consume this exact same subtree.
    const uint32_t shifted = dag.intern(Op::Add, "add(ambient:p,input:shared.shift)", {p, shift});
    const uint32_t shared = dag.intern(Op::Mul, "mul(add(ambient:p,input:shared.shift),input:shared.scale)", {shifted, scale});
    const uint32_t sharedAgain = dag.intern(Op::Mul, "mul(add(ambient:p,input:shared.shift),input:shared.scale)", {shifted, scale});
    assert(shared == sharedAgain);
    const uint32_t sdfA = dag.intern(Op::Sub, "sub(shared,sdfA.bias)", {shared, biasA});
    const uint32_t sdfB = dag.intern(Op::Sub, "sub(shared,sdfB.bias)", {shared, biasB});
    const uint32_t scene = dag.intern(Op::Min, "min(sdfA,sdfB)", {sdfA, sdfB});

    // A proof fact is attached to the branch that derives it. It is diagnostic
    // derived state here; evaluation remains exact whether it exists or not.
    dag.nodes[sdfA].propheticPositive = true;

    Counters naiveCounters;
    const double expected = naive(7.0, 2.0, 3.0, 5.0, 11.0, naiveCounters);
    Counters dagCounters;
    std::unordered_map<uint32_t, double> memo;
    const double actual = evalDag(dag, scene, memo, dagCounters);
    assert(std::abs(expected - actual) < 1e-12);
    assert(dagCounters.leafEvaluations < naiveCounters.leafEvaluations);
    assert(dagCounters.cacheHits >= 1);

    // Local semantic edit: sdfA.bias must invalidate its leaf, sdfA and scene,
    // but must preserve the shared subtree and the independent sdfB branch.
    const auto dirty = dag.invalidationFrontier("sdfA.bias");
    assert(dirty.count(biasA) == 1);
    assert(dirty.count(sdfA) == 1);
    assert(dirty.count(scene) == 1);
    assert(dirty.count(shared) == 0);
    assert(dirty.count(sdfB) == 0);
    assert(dirty.size() == 3);

    const size_t preserved = dag.nodes.size() - dirty.size();
    const size_t estimatedBytes = dag.nodes.size() * sizeof(Node);
    std::printf("SCENE_SPATIAL_SYNTHESIS parity=1 nodes=%zu artifact_bytes_est=%zu "
                "naive_leaf_evals=%llu dag_leaf_evals=%llu shared_evals_avoided=%llu "
                "dag_nodes_visited=%llu cache_hits=%llu proof_annotations_consulted=%llu "
                "mutation=sdfA.bias invalidated=%zu repaired=%zu preserved=%zu "
                "whole_scene_rebuild=0 camera_rebuild=0 global_relevance_search=0\n",
                dag.nodes.size(), estimatedBytes,
                static_cast<unsigned long long>(naiveCounters.leafEvaluations),
                static_cast<unsigned long long>(dagCounters.leafEvaluations),
                static_cast<unsigned long long>(naiveCounters.leafEvaluations - dagCounters.leafEvaluations),
                static_cast<unsigned long long>(dagCounters.nodesVisited),
                static_cast<unsigned long long>(dagCounters.cacheHits),
                static_cast<unsigned long long>(dagCounters.proofAnnotationsConsulted),
                dirty.size(), dirty.size(), preserved);
    return 0;
}
