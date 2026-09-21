#include "Time/timeline.hpp"
#include "Time/Event/Event.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

int main() {
    std::cout << "Testing first-class Timeline ontology...\n";

    const std::size_t registryBefore = Timeline::all().size();

    Timeline world("test-world-timeline");
    Timeline maintenance("test-maintenance-timeline");

    // 1. Timeline is a first-class Singular with arbitrary identity, not an
    // enum-backed engine clock kind. New instances become generically reachable.
    {
        Singular& asBeing = world;
        assert(asBeing.getIdentifier() == "test-world-timeline");
        assert(Timeline::all().size() == registryBefore + 2);

        bool sawWorld = false;
        bool sawMaintenance = false;
        for (Timeline* t : Timeline::all()) {
            if (t == &world) sawWorld = true;
            if (t == &maintenance) sawMaintenance = true;
        }
        assert(sawWorld && sawMaintenance);
    }

    // 2. Temporal domains advance independently. An occurrence in one Timeline
    // does not, merely by occurring, constitute an occurrence in another.
    {
        assert(world.setClock(10.0, 0.5));
        assert(maintenance.setClock(3.0, 0.25));
        assert(world.now() == 10.0);
        assert(maintenance.now() == 3.0);

        assert(world.advanceBy(2.0));
        assert(world.now() == 12.0);
        assert(world.delta() == 2.0);
        assert(maintenance.now() == 3.0);
        assert(maintenance.delta() == 0.25);
    }

    // 3. Timeline contains Moments polymorphically. Event IS a Moment, so it
    // enters the same container without a parallel event-timeline mechanism.
    auto later = std::make_shared<Moment>(30.0);
    auto earlier = std::make_shared<Moment>(5.0);
    auto event = std::make_shared<Event>(
        "maintenance-opportunity", nullptr, nullptr, Moment(20.0), "test");

    {
        assert(world.addMoment(later));
        assert(world.addMoment(earlier));
        assert(world.addMoment(event));
        assert(!world.addMoment(event)); // same being is not inserted twice
        assert(world.moments().size() == 3);

        const auto ordered = world.orderedMoments();
        assert(ordered.size() == 3);
        assert(ordered[0].get() == earlier.get());
        assert(ordered[1].get() == event.get());
        assert(ordered[2].get() == later.get());
        assert(world.latestMoment().get() == later.get());

        const Moment* eventAsMoment = event.get();
        assert(eventAsMoment->asSeconds() == 20.0);
    }

    // 4. Refusal #6: containment and clock head are ordinary discoverable
    // properties, rather than private arrays/fields only C++ can inspect.
    {
        auto* count = world.findProperty("momentCount");
        auto* moments = world.findProperty("moments");
        auto* latest = world.findProperty("latestMoment");
        auto* now = world.findProperty("now");
        auto* delta = world.findProperty("delta");
        auto* hasClock = world.findProperty("hasClock");

        assert(count && std::get<int>(count->value()) == 3);
        assert(moments);
        const auto list = std::get<std::shared_ptr<PropertyList>>(moments->value());
        assert(list && list->elements.size() == 3);
        assert(std::get<std::string>(list->elements[0]) == earlier->getIdentifier());
        assert(std::get<std::string>(list->elements[1]) == event->getIdentifier());
        assert(std::get<std::string>(list->elements[2]) == later->getIdentifier());
        assert(latest && std::get<std::string>(latest->value()) == later->getIdentifier());
        assert(now && std::get<double>(now->value()) == 12.0);
        assert(delta && std::get<double>(delta->value()) == 2.0);
        assert(hasClock && std::get<bool>(hasClock->value()));
    }

    // 5. Universe remains kernel working context, not a competing clock being:
    // it borrows one Timeline and its compatibility clock API projects that head.
    {
        Universe& universe = Universe::instance();
        universe.setTimeline(&world);
        assert(universe.timeline() == &world);
        assert(universe.hasClock());
        assert(universe.now() == 12.0);
        assert(universe.dt() == 2.0);

        universe.setClock(40.0, 0.75);
        assert(world.now() == 40.0);
        assert(world.delta() == 0.75);
        assert(maintenance.now() == 3.0);

        // Unbinding snapshots the last authoritative head into the legacy
        // fallback used by isolated tests/tools with no Timeline.
        universe.setTimeline(nullptr);
        assert(universe.timeline() == nullptr);
        assert(universe.hasClock());
        assert(universe.now() == 40.0);
        assert(universe.dt() == 0.75);
    }

    // 6. Membership is mutable without destroying the Moment being.
    {
        assert(world.removeMoment(event.get()));
        assert(world.moments().size() == 2);
        assert(event->asSeconds() == 20.0);
        world.clearMoments();
        assert(world.moments().empty());
        assert(!world.latestMoment());
    }

    // 7. Identity collisions refuse rather than silently creating two beings
    // whose Law path would spell the same.
    {
        bool refusedDuplicate = false;
        try {
            Timeline duplicate("test-world-timeline");
        } catch (const std::invalid_argument&) {
            refusedDuplicate = true;
        }
        assert(refusedDuplicate);
    }

    std::cout << "Timeline tests passed.\n";
    return 0;
}
