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

// ---------------------------------------------------------------------------
// What makes a law A LAW, carried across.
//
// Composition used to move the condition tree and the action tree and nothing
// else, which produced a well-formed law that could never fire: no targets, so
// nothing to apply to; the default activation, whatever the constituents'
// were; no drive flag, so a fused Drive action was never ticked. The trees are
// what the higher law SAYS; these are the terms under which it is said, and a
// law separated from them is a sentence nobody is listening to.
//
// Where the constituents disagree, the composition takes the NARROWER reading.
// A synthesis is not an opportunity to acquire authority neither parent had.
// ---------------------------------------------------------------------------
void carryBindings(Law& out, const Law& a, const Law& b) {
    // Activation and retrigger: inherit on agreement, else the least
    // self-perpetuating option.
    out.setActivation(a.activation() == b.activation() ? a.activation()
                                                       : Law::Activation::OnEvent);
    out.setRetrigger(a.retrigger() == b.retrigger() ? a.retrigger()
                                                    : Law::Retrigger::Absorb);
    // Scope: Everyone only when BOTH parents already ran that wide.
    out.setScope(a.scope() == Law::Scope::Everyone && b.scope() == Law::Scope::Everyone
                     ? Law::Scope::Everyone
                     : Law::Scope::Subject);
    // A fused action containing either parent's Drive is itself a drive.
    out.setDrives(a.drives() || b.drives());

    // The referents. A higher law governs what its constituents governed —
    // the union, deduplicated, and by live reference the way targets are
    // always held.
    for (const Law* parent : {&a, &b}) {
        for (Singular* target : parent->targets().getMembers()) {
            if (!target) continue;
            bool already = false;
            for (Singular* held : out.targets().getMembers()) {
                if (held == target) {
                    already = true;
                    break;
                }
            }
            if (!already) out.addTarget(*target);
        }
    }

    // AUTHORITY IS NOT INHERITED. It is Singularity-granted, clamped on every
    // path that reads a file, and a synthesis is exactly the kind of path that
    // would otherwise become a ladder: compose two ordinary laws, receive a
    // higher one, compose again. The new law starts at the floor like anything
    // else born in the world.
}

// A synthesized law that nobody registered is a law nobody will ever apply,
// and one nobody bound is a law nothing will ever wake. WHAT WAKES a law lives
// in the manager's trigger table, not on the law — so the manager is the only
// place that can answer "what did the constituents listen for", and the higher
// law inherits the UNION: it must hear everything either parent heard, or the
// composition is deaf to half of what it is made of.
void enrol(const std::shared_ptr<Law>& law, const Law& a, const Law& b,
           LawManager* into) {
    if (!into || !law) return;
    into->add(law);
    for (const Law* parent : {&a, &b}) {
        for (const std::string& eventType : into->triggersOf(parent->getIdentifier())) {
            into->bindTrigger(law->getIdentifier(), eventType);   // idempotent per (law, type)
        }
    }
}

} // namespace

std::shared_ptr<Law> compose(const std::string& name,
                             const Law& a, const Law& b,
                             const std::vector<Singular*>& authors,
                             bool allConditions,
                             bool sequentialActions,
                             LawManager* into) {
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

    carryBindings(*law, a, b);
    recordSynthesisProvenance(*law, a, b);
    enrol(law, a, b, into);
    return law;
}

std::shared_ptr<Law> synthesizeByDemonstration(
    const std::string& name,
    Law& a, Law& b,
    Singular& referent,
    const std::vector<std::string>& watchPaths,
    int steps, float dt,
    const std::vector<Singular*>& authors,
    LawManager* into) {
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
    carryBindings(*law, a, b);
    // A fitted model is a curve over time. Whatever the constituents were,
    // what came out of the recorder DRIVES — that is what a fitted process is.
    law->setDrives(true);
    recordSynthesisProvenance(*law, a, b);
    enrol(law, a, b, into);
    return law;
}

} // namespace LawSynthesis

// Authors are REQUIRED, not optional. This built a Law with an empty author
// Formation, and `Law::applyTo` returns Unauthored and refuses to fire for
// exactly that — so every clip it ever migrated arrived in the world inert.
// Nothing enters the world without an author; a migration is not an exception,
// it is a first mover taking responsibility for what they carried over.
std::shared_ptr<Law> fromAutomationClip(const std::string& name,
                                        const Automation::Clip& clip,
                                        const std::vector<Singular*>& authors) {
    auto law = std::make_shared<Law>(name, authors);
    law->setActivation(Law::Activation::WhileTrue);
    law->setScope(Law::Scope::Everyone);

    std::vector<ActionNode> drives;
    for (const auto& track : clip.tracks) {
        std::string propertyName;
        switch (track.channel) {
            case Automation::Channel::PosX: propertyName = "position.x"; break;
            case Automation::Channel::PosY: propertyName = "position.y"; break;
            case Automation::Channel::PosZ: propertyName = "position.z"; break;
            case Automation::Channel::RotX: propertyName = "rotation.x"; break;
            case Automation::Channel::RotY: propertyName = "rotation.y"; break;
            case Automation::Channel::RotZ: propertyName = "rotation.z"; break;
            case Automation::Channel::SclX: propertyName = "scale.x"; break;
            case Automation::Channel::SclY: propertyName = "scale.y"; break;
            case Automation::Channel::SclZ: propertyName = "scale.z"; break;
        }

        // CurveModel::sinusoid phase is in radians. Track phase is in turns [0, 1).
        double phaseRadians = track.phase * 6.28318530717958647692;
        CurveModel curve = CurveModel::sinusoid(track.amplitude, track.frequency, phaseRadians, track.bias);

        drives.push_back(ActionNode::drive(propertyName, curve, "time.sinceApplied"));
    }

    law->setDrives(!drives.empty());
    if (drives.empty()) {
        law->setActionModel(ActionNode::sequence({}));
    } else if (drives.size() == 1) {
        law->setActionModel(drives.front());
    } else {
        law->setActionModel(ActionNode::parallel(std::move(drives)));
    }

    return law;
}


