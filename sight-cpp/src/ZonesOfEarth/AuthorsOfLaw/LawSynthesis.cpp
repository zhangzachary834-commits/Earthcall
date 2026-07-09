#include "LawSynthesis.hpp"

#include <utility>

namespace LawSynthesis {

namespace {

// Join the two constitutent condition trees (either may be absent).
void joinConditions(Law& out, const Law& a, const Law& b, bool all) {
    const ConditionModel* ca = a.conditionModel();
    const ConditionModel* cb = b.conditionModel();
    if (ca && cb) {
        std::vector<ConditionNode> children{*ca, *cb};
        out.setConditionModel(all ? ConditionNode::all(std::move(children))
                                  : ConditionNode::any(std::move(children)));
    } else if (ca) {
        out.setConditionModel(*ca);
    } else if (cb) {
        out.setConditionModel(*cb);
    }
}

void recordSynthesisProvenance(Law& out, const Law& a, const Law& b) {
    out.recordProvenance("synthesized-from", out, a, true, 1.0f);
    out.recordProvenance("synthesized-from", out, b, true, 1.0f);
}

} // namespace

std::shared_ptr<Law> compose(const std::string& name,
                             const Law& a, const Law& b,
                             const std::vector<Singular*>& authors,
                             bool allConditions,
                             bool sequentialActions) {
    auto law = std::make_shared<Law>(name, authors);

    joinConditions(*law, a, b, allConditions);

    const ActionModel* aa = a.actionModel();
    const ActionModel* ab = b.actionModel();
    if (aa && ab) {
        ActionNode joined = ActionNode::sequence({*aa, *ab});
        if (!sequentialActions) joined.kind = ActionNode::Kind::Parallel;
        law->setActionModel(std::move(joined));
    } else if (aa) {
        law->setActionModel(*aa);
    } else if (ab) {
        law->setActionModel(*ab);
    }

    recordSynthesisProvenance(*law, a, b);
    return law;
}

std::shared_ptr<Law> synthesizeByDemonstration(
    const std::string& name,
    Law& a, Law& b,
    Singular& referent,
    const std::vector<std::string>& watchPaths,
    int steps, float dt,
    const std::vector<Singular*>& authors) {
    // Watch, then let the constituents jointly work the referent. The
    // recorder captures the ENTIRE cumulative process — which angle, what
    // order, every intermediate value — not just the final outputs.
    ChangeRecorder recorder;
    for (const auto& path : watchPaths) recorder.watch(path);
    recorder.begin(referent);
    for (int step = 0; step < steps; ++step) {
        a.applyTo(referent);
        b.applyTo(referent);
        recorder.sample(dt);
    }
    recorder.end();

    auto law = std::make_shared<Law>(name, authors);
    law->setActionModel(recorder.fit());   // ONE fused model of the joint process
    joinConditions(*law, a, b, /*all=*/true);
    recordSynthesisProvenance(*law, a, b);
    return law;
}

} // namespace LawSynthesis
