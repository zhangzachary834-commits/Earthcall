// Zach's PropertyPath micromastery ordering, narrow live witness:
// an authored Law configures TransferPolicy; another authored Law reads that
// state and branches together with subject context. This does not yet
// authorize an actual Property read/write or define Zone/actor access policy.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/PropheticRete.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

double height(Object& object) {
    PropertyValue value;
    assert(PropertyPath::parse("position.z").getValue(object, value) ==
           PropertyPath::PathResult::Ok);
    double number = 0.0;
    assert(propertyValueToNumber(value, number));
    return number;
}

void writeGate(Object& author, Object& subject, bool open) {
    Law metalaw(open ? "open-transfer-gate" : "close-transfer-gate");
    metalaw.addAuthor(author);
    metalaw.setActionModel(ActionNode::set(
        "@transfer-policy.gate.shape", PropertyValue(open)));
    assert(metalaw.applyTo(subject) == Law::ApplicationResult::Applied);
    assert(TransferPolicy::instance().isOpen("shape") == open);
}

} // namespace

int main() {
    Object author, first, second;
    author.setObjectID("author.policy-branch-test");
    first.setObjectID("first.policy-branch-test");
    second.setObjectID("second.policy-branch-test");
    first.setDynamicProperty("context.ready", PropertyValue(false));
    second.setDynamicProperty("context.ready", PropertyValue(false));
    TransferPolicy& policy = TransferPolicy::instance();
    policy.setOpen("shape", true);

    Universe::instance().setProvider([&](std::vector<Singular*>& out) {
        out.push_back(&author);
        out.push_back(&first);
        out.push_back(&second);
        out.push_back(&policy);
    });
    Universe::instance().setClock(100.0, 0.1);

    {
        LawManager manager;
        manager.connectToEventBus();

        auto branch = manager.createLaw("policy-and-context-branch", {&author});
        branch->setActivation(Law::Activation::OnBecomeTrue);
        branch->setConditionModel(ConditionNode::all({
            ConditionNode::compare("@transfer-policy.gate.shape",
                                   ConditionNode::Op::Eq, PropertyValue(true)),
            ConditionNode::compare("context.ready",
                                   ConditionNode::Op::Eq, PropertyValue(true))}));
        branch->setActionModel(ActionNode::add("position.z", 1.0));

        // Prophetic analysis must name both potential dependencies. It may
        // over-approximate; excluding either one could make a Law go deaf.
        const Prophetic::LawFacts facts = Prophetic::analyzeLaw(*branch);
        assert(facts.readNames.count("gate.shape") != 0);
        assert(facts.readNames.count("context.ready") != 0);

        writeGate(author, first, false);
        manager.tick();
        assert(std::fabs(height(first)) < 1e-4);
        assert(std::fabs(height(second)) < 1e-4);

        writeGate(author, first, true);
        manager.tick();                 // gate alone cannot satisfy context
        assert(std::fabs(height(first)) < 1e-4);

        assert(PropertyPath::parse("context.ready").setValue(
            first, PropertyValue(true)) == PropertyPath::PathResult::Ok);
        manager.tick();                 // first actor's context changed
        assert(std::fabs(height(first) - 1.0) < 1e-4);
        assert(std::fabs(height(second)) < 1e-4);

        assert(PropertyPath::parse("context.ready").setValue(
            second, PropertyValue(true)) == PropertyPath::PathResult::Ok);
        manager.tick();                 // second actor's context changed
        assert(std::fabs(height(first) - 1.0) < 1e-4);
        assert(std::fabs(height(second) - 1.0) < 1e-4);

        writeGate(author, first, false);
        manager.tick();                 // both held conditions must release
        writeGate(author, first, true);
        manager.tick();                 // policy changed; both may fire again
        assert(std::fabs(height(first) - 2.0) < 1e-4);
        assert(std::fabs(height(second) - 2.0) < 1e-4);
    }

    policy.setOpen("shape", true);
    Universe::instance().setProvider(nullptr);
    std::puts("transfer_policy_branch_reactivity_test: OK");
    return 0;
}
