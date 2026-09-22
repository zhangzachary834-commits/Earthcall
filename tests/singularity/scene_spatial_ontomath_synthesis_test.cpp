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
};

struct CompileCounters {
    uint64_t sourceNodesVisited = 0;
    uint64_t canonicalHits = 0;
    uint64_t nodesCreated = 0;
};

struct OntoSceneCompiler {
    std::vector<CompiledNode> nodes;
    std::unordered_map<std::string, uint32_t> canonical;
    std::unordered_map<const MathNode*, uint32_t> sourceToCompiled;
    std::unordered_map<const MathNode*, std::vector<const MathNode*>>
        sourceParents;

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
                         n.stringArg, childIds, key});
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
};

double evalCompiled(
    const OntoSceneCompiler& compiler, uint32_t id,
    const std::map<std::string, double>& vars,
    std::unordered_map<uint32_t, double>& memo,
    uint64_t& nodesVisited, uint64_t& cacheHits) {
    auto m = memo.find(id);
    if (m != memo.end()) {
        ++cacheHits;
        return m->second;
    }
    ++nodesVisited;
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
        const double a = evalCompiled(
            compiler, n.children[0], vars, memo, nodesVisited, cacheHits);
        const double b = evalCompiled(
            compiler, n.children[1], vars, memo, nodesVisited, cacheHits);
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
    for (double x : ambientSamples) {
        if (!ambientMemo.empty()) {
            ambientCacheEntriesInvalidated += ambientMemo.size();
            ambientMemo.clear();
        }
        uint64_t visited = 0, cacheHits = 0;
        const double exact = exactMathNode(*scene, x);
        const double compiled =
            evalCompiled(compiler, root, {{"x", x}},
                         ambientMemo, visited, cacheHits);
        assert(std::abs(exact - compiled) < 1e-12);
        assert(cacheHits > 0);
        ambientNodesVisited += visited;
        ambientCacheHits += cacheHits;

        assert(compiler.nodes.size() == semanticNodesBeforeAmbient);
        assert(compiler.canonical.size() == canonicalNodesBeforeAmbient);
        assert(compiler.sourceToCompiled.at(scene.get()) == root);
        assert(compiler.sourceToCompiled.at(sharedA) ==
               sharedCompiledBefore);
        assert(compiler.sourceToCompiled.at(sharedB) ==
               sharedCompiledBefore);
        assert(compiler.sourceToCompiled.at(sdfB) ==
               sdfBCompiledBefore);
    }
    assert(ambientCacheEntriesInvalidated > 0);

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

    const uint32_t repairedRoot =
        compiler.sourceToCompiled.at(scene.get());
    const double exactAfter = exactMathNode(*scene, 7.0);
    std::unordered_map<uint32_t, double> repairedMemo;
    uint64_t repairedVisited = 0, repairedCacheHits = 0;
    const double compiledAfter =
        evalCompiled(compiler, repairedRoot, {{"x", 7.0}},
                     repairedMemo, repairedVisited, repairedCacheHits);
    assert(std::abs(exactAfter - compiledAfter) < 1e-12);

    const size_t newCompiledNodes =
        compiler.nodes.size() - compiledNodesBeforeRepair;
    assert(newCompiledNodes == 3);

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

    uint64_t revertParitySamples = 0;
    for (double x : ambientSamples) {
        std::unordered_map<uint32_t, double> memo;
        uint64_t nodesVisited = 0, cacheHits = 0;
        const double exact = exactMathNode(*scene, x);
        const double compiled =
            evalCompiled(compiler, root, {{"x", x}},
                         memo, nodesVisited, cacheHits);
        assert(std::abs(exact - compiled) < 1e-12);
        ++revertParitySamples;
    }
    assert(revertParitySamples == ambientSamples.size());

    std::printf(
        "SCENE_SPATIAL_ONTOMATH_SYNTHESIS parity=1 "
        "source_nodes=%zu compiled_nodes_initial=%zu canonical_hits=%llu "
        "source_nodes_visited_initial=%llu shared_subtree_identity=1 "
        "ambient_samples=%zu ambient_cache_entries_invalidated=%zu "
        "ambient_nodes_visited=%llu ambient_cache_hits=%llu "
        "semantic_nodes_rebuilt_for_ambient=0 "
        "mutation_biasA_to17=1 repaired_source_nodes=%zu repair_source_visits=%llu "
        "new_compiled_nodes=%zu shared_compiled_id_preserved=1 "
        "sdfB_compiled_id_preserved=1 whole_scene_rescan_for_repair=0 "
        "mutation_biasA_revert=1 revert_source_visits=%llu "
        "revert_nodes_created=%llu revert_canonical_hits=%llu "
        "prior_artifact_reused_on_revert=1 revert_parity_samples=%llu "
        "pretty_print_identity=0 production_wgsl_changed=0\n",
        sourceNodes, compiledNodesBeforeRepair,
        static_cast<unsigned long long>(initialCompile.canonicalHits),
        static_cast<unsigned long long>(initialCompile.sourceNodesVisited),
        ambientSamples.size(), ambientCacheEntriesInvalidated,
        static_cast<unsigned long long>(ambientNodesVisited),
        static_cast<unsigned long long>(ambientCacheHits),
        repairedSources.size(),
        static_cast<unsigned long long>(repairCounters.sourceNodesVisited),
        newCompiledNodes,
        static_cast<unsigned long long>(revertCounters.sourceNodesVisited),
        static_cast<unsigned long long>(revertCounters.nodesCreated),
        static_cast<unsigned long long>(revertCounters.canonicalHits),
        static_cast<unsigned long long>(revertParitySamples));
    return 0;
}
