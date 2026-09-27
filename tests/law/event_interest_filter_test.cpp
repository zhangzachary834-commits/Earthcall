// Regression witness: authored state alphas are not arbitrary Event listeners.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <cassert>
#include <cstdio>
#include <memory>

int main() {
    Object author;
    author.setObjectID("event-interest.author");

    LawManager mgr;
    mgr.connectToEventBus();

    auto listenerLaw = std::make_shared<Law>(
        "event-interest-listener", std::vector<Singular*>{&author});
    mgr.add(listenerLaw);

    const std::size_t authored = mgr.rete().internAuthoredAlpha(
        "event-interest-authored-key", "authored state predicate",
        [](const FactPtr& fact) { return fact && fact->isState; });
    mgr.rete().bindLawToAlpha(listenerLaw->getIdentifier(), authored);

    assert(mgr.rete().hasOpaqueBoundAlpha());
    assert(!mgr.rete().hasForeignBoundAlpha());
    assert(!Universe::instance().anyoneHears("law-applied"));
    assert(!Universe::instance().anyoneHears("some-unrelated-event"));

    const std::size_t typed = mgr.rete().internTypeAlpha("law-applied");
    mgr.rete().bindLawToAlpha(listenerLaw->getIdentifier(), typed);
    assert(Universe::instance().anyoneHears("law-applied"));
    mgr.rete().unbindLawFromAlpha(listenerLaw->getIdentifier(), typed);
    assert(!Universe::instance().anyoneHears("law-applied"));

    const std::size_t foreign = mgr.rete().addAlphaNode(
        "foreign / unknowable", [](const FactPtr&) { return true; });
    mgr.rete().bindLawToAlpha(listenerLaw->getIdentifier(), foreign);
    assert(mgr.rete().hasForeignBoundAlpha());
    assert(Universe::instance().anyoneHears("some-unrelated-event"));
    mgr.rete().unbindLawFromAlpha(listenerLaw->getIdentifier(), foreign);
    assert(!mgr.rete().hasForeignBoundAlpha());
    assert(!Universe::instance().anyoneHears("some-unrelated-event"));

    // A published Event is a Singular through the actual EventBus -> Rete ->
    // Law path. The Rete must retain its occurrence, not just its two legacy
    // participant pointers, until the authored condition reads @event.verb.
    Object subject;
    subject.setObjectID("event-interest.subject");
    Universe::instance().setProvider([&](std::vector<Singular*>& out) {
        out.push_back(&subject);
    });
    auto witness = mgr.createLaw("read-the-event-being", {&author});
    witness->setConditionModel(ConditionNode::compare(
        "@event.verb", ConditionNode::Op::Eq,
        PropertyValue(std::string("occurrence-witnessed"))));
    witness->setActionModel(ActionNode::set("position.y", PropertyValue(7.0)));
    mgr.bindTrigger(witness->getIdentifier(), "occurrence-witnessed");

    const ECA::Event occurrence{"occurrence-witnessed", &subject, nullptr,
                                Moment(123.0), "Zach"};
    Core::EventBus::instance().publish(occurrence);
    bool retained = false;
    for (const auto& activation : mgr.rete().agenda()) {
        for (const auto& fact : activation.token.facts) {
            if (fact && fact->occurrence && fact->occurrence->type == occurrence.type) {
                retained = true;
                assert(fact->occurrence->getIdentifier() == occurrence.getIdentifier());
                Universe::EventScope context(&subject, nullptr, fact->occurrence.get());
                PropertyValue value;
                assert(lawGetValue(subject, PropertyPath::parse("@event.verb"), value));
                assert(std::get<std::string>(value) == occurrence.type);
                assert(lawGetValue(subject, PropertyPath::parse("@event.start"), value));
                assert(std::get<double>(value) == 123.0);
                assert(lawGetValue(subject, PropertyPath::parse("@event.occurrenceId"), value));
                assert(std::get<std::string>(value) == occurrence.occurrenceId());
                assert(lawGetValue(subject,
                                   PropertyPath::parse("@event.subject.position.y"), value));
                assert(lawSetValue(subject, PropertyPath::parse("@event.verb"),
                                   PropertyValue(std::string("forged"))) ==
                       PropertyPath::PathResult::ReadOnly);
            }
        }
    }
    assert(retained);
    mgr.tick();
    assert(subject.getPosition().y == 7.0f);
    Universe::instance().setProvider({});

    std::puts("event_interest_filter_test: ALL OK");
    return 0;
}
