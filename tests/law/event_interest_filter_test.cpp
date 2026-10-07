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

    std::puts("event_interest_filter_test: ALL OK");
    return 0;
}
