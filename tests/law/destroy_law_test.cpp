// Destroy may unmake a Law (2026-09-25, for the Law Line's confirmed deletion
// — Zach: "the metalaw that does the deletions should say 'are you sure…'").
// No new opcode: the existing Destroy accepts a Law victim, and the
// LawManager retires it at the end of the tick, after every fact about it is
// retracted. A First Mover is engine substrate and is refused.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <cassert>
#include <ctime>
#include <iostream>
#include <memory>

int main() {
    LawManager laws;
    laws.connectToEventBus();

    Object author;
    author.setObjectID("author");
    author.setDynamicProperty("glow", PropertyValue(0.0));

    Universe::instance().setProvider([&](std::vector<Singular*>& out) {
        out.push_back(&author);
        for (const auto& law : laws.getAll()) {
            if (law) out.push_back(law.get());
        }
    });

    // The Law to be retired: bound to "tick", so facts about it and its
    // trigger binding both exist when it goes.
    auto victim = std::make_shared<Law>("Blue", std::vector<Singular*>{&author});
    victim->setLawIdentifier("victim-law");
    victim->setActionModel(ActionNode::add("glow", 1.0));
    laws.add(victim);
    laws.bindTrigger("victim-law", "tick");
    Law* victimRaw = victim.get();
    victim.reset();

    // The Metalaw that does deletions: Destroy whatever the event names.
    auto deleter = std::make_shared<Law>("delete when confirmed", std::vector<Singular*>{&author});
    deleter->setLawIdentifier("deleter");
    deleter->setActionModel(ActionNode::destroy("@event.object"));
    laws.add(deleter);
    laws.bindTrigger("deleter", "deletion-confirmed");

    // A First Mover cannot be unmade.
    auto firstMover = std::make_shared<FirstMoverLaw>("fm-law");
    laws.add(firstMover);

    Core::EventBus::instance().publish(ECA::Event{"tick", &author, nullptr, std::time(nullptr), ""});
    laws.tick();
    assert(laws.find("victim-law") != nullptr);

    Core::EventBus::instance().publish(
        ECA::Event{"deletion-confirmed", &author, victimRaw, std::time(nullptr), ""});
    laws.tick();
    assert(laws.find("victim-law") == nullptr);
    assert(laws.triggersOf("victim-law").empty());

    // The event it listened for fires again: nothing reaches the freed Law.
    Core::EventBus::instance().publish(ECA::Event{"tick", &author, nullptr, std::time(nullptr), ""});
    laws.tick();

    Core::EventBus::instance().publish(
        ECA::Event{"deletion-confirmed", &author, laws.find("fm-law"), std::time(nullptr), ""});
    laws.tick();
    assert(laws.find("fm-law") != nullptr);

    std::cout << "destroy_law_test: OK\n";
    return 0;
}
