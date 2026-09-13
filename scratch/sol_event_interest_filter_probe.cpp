// CI-only semantic witness for the event-interest fast path.
// Copied into tests/law/ by the diagnostic workflow after the source patch.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <cassert>
#include <cstdio>
#include <memory>

int main() {
    Object author;
    author.setObjectID("event-interest.author");
    Object subject;
    subject.setObjectID("event-interest.subject");
    subject.setObjectType("probe");

    Universe::instance().setProvider([&]() {
        return std::vector<Singular*>{&author, &subject};
    });

    LawManager mgr;

    // An ordinary authored continuous condition compiles to an Authored alpha.
    // It consumes STATE facts only; it is not evidence that anybody listens to
    // arbitrary event types such as "law-applied".
    auto stateLaw = std::make_shared<Law>("state-only", std::vector<Singular*>{&author});
    stateLaw->setActivation(Law::Activation::WhileTrue);
    stateLaw->setConditionModel(
        ConditionNode::compare("objectType", ConditionNode::Op::Eq,
                               PropertyValue(std::string("probe"))));
    mgr.add(stateLaw);
    mgr.connectToEventBus();

    assert(mgr.rete().hasOpaqueBoundAlpha());
    assert(!mgr.rete().hasForeignBoundAlpha());
    assert(!Universe::instance().anyoneHears("law-applied"));
    assert(!Universe::instance().anyoneHears("some-unrelated-event"));

    // An explicit event trigger is exact evidence. hearsType(type) must keep
    // this audible even with the narrower opaque-lane fallback.
    auto eventLaw = std::make_shared<Law>("explicit-event-listener",
                                          std::vector<Singular*>{&author});
    mgr.add(eventLaw);
    mgr.bindTrigger(eventLaw->getIdentifier(), "law-applied");
    assert(Universe::instance().anyoneHears("law-applied"));
    mgr.unbindTrigger(eventLaw->getIdentifier(), "law-applied");
    assert(!Universe::instance().anyoneHears("law-applied"));

    // A hand-written alpha is genuinely opaque: it may inspect arbitrary fact
    // fields, including event type, so event-interest MUST fail open.
    const std::size_t foreign = mgr.rete().addAlphaNode(
        "foreign / unknowable", [](const FactPtr&) { return true; });
    mgr.rete().bindLawToAlpha(eventLaw->getIdentifier(), foreign);
    assert(mgr.rete().hasForeignBoundAlpha());
    assert(Universe::instance().anyoneHears("some-unrelated-event"));
    mgr.rete().unbindLawFromAlpha(eventLaw->getIdentifier(), foreign);
    assert(!mgr.rete().hasForeignBoundAlpha());
    assert(!Universe::instance().anyoneHears("some-unrelated-event"));

    Universe::instance().setProvider({});
    std::puts("sol_event_interest_filter_probe: ALL OK");
    return 0;
}
