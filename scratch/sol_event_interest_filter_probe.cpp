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

    LawManager mgr;
    mgr.connectToEventBus();

    auto listenerLaw = std::make_shared<Law>(
        "event-interest-listener", std::vector<Singular*>{&author});
    mgr.add(listenerLaw);

    // Directly exercise the classification contract. An Authored alpha is a
    // readable predicate compiled from law text. Continuous ConditionModel
    // alphas reject !isState facts, so their existence is NOT evidence that
    // anyone hears an arbitrary event type.
    const std::size_t authored = mgr.rete().internAuthoredAlpha(
        "event-interest-authored-key", "authored state predicate",
        [](const FactPtr& fact) { return fact && fact->isState; });
    mgr.rete().bindLawToAlpha(listenerLaw->getIdentifier(), authored);

    assert(mgr.rete().hasOpaqueBoundAlpha());
    assert(!mgr.rete().hasForeignBoundAlpha());
    assert(!Universe::instance().anyoneHears("law-applied"));
    assert(!Universe::instance().anyoneHears("some-unrelated-event"));

    // An explicit typed trigger is exact evidence and remains audible through
    // hearsType(type), independent of the opaque fallback.
    const std::size_t typed = mgr.rete().internTypeAlpha("law-applied");
    mgr.rete().bindLawToAlpha(listenerLaw->getIdentifier(), typed);
    assert(Universe::instance().anyoneHears("law-applied"));
    mgr.rete().unbindLawFromAlpha(listenerLaw->getIdentifier(), typed);
    assert(!Universe::instance().anyoneHears("law-applied"));

    // A hand-written alpha is genuinely opaque. It may inspect arbitrary fact
    // fields, including event type, so the event-interest answer MUST fail open.
    const std::size_t foreign = mgr.rete().addAlphaNode(
        "foreign / unknowable", [](const FactPtr&) { return true; });
    mgr.rete().bindLawToAlpha(listenerLaw->getIdentifier(), foreign);
    assert(mgr.rete().hasForeignBoundAlpha());
    assert(Universe::instance().anyoneHears("some-unrelated-event"));
    mgr.rete().unbindLawFromAlpha(listenerLaw->getIdentifier(), foreign);
    assert(!mgr.rete().hasForeignBoundAlpha());
    assert(!Universe::instance().anyoneHears("some-unrelated-event"));

    std::puts("sol_event_interest_filter_probe: ALL OK");
    return 0;
}
