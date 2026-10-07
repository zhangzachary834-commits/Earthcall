#include "Time/Event/Event.hpp"
#include "Time/Moment/Moment.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "json.hpp"

#include <cassert>
#include <iostream>
#include <memory>

using json = nlohmann::json;

int main() {
    std::cout << "Testing Event as distinguished Moment...\n";

    // 1. Direct inheritance and Moment polymorphism
    {
        Event e("jump-started", nullptr, nullptr, Moment(42.5), "Zach");
        assert(e.isInstant());
        assert(e.asSeconds() == 42.5);
        assert(e.propStart() == 42.5);
        assert(e.propEnd() == 42.5);
        assert(e.verb() == "jump-started");
        assert(e.type == "jump-started");
        assert(e.author == "Zach");

        // Event IS a Moment
        const Moment& m = e;
        assert(m.asSeconds() == 42.5);

        // Event IS a Singular
        const Singular& s = e;
        assert(s.getIdentifier().find("event.jump-started.") == 0);
    }

    // 2. An Event distinguishing an Interval
    {
        Moment span = Moment::interval(10.0, 25.0);
        Event window("interaction-window", nullptr, nullptr, span, "Author");
        assert(window.isInterval());
        assert(window.asSeconds() == 10.0);
        assert(window.endSeconds().has_value() && *window.endSeconds() == 25.0);
        assert(window.verb() == "interaction-window");
    }

    // 3. Participants and Identifier stability
    {
        Object subj;
        Object obj;
        Event e("collision", &subj, &obj, Moment(100.0));
        assert(e.subject == &subj);
        assert(e.object == &obj);
        assert(e.verb() == "collision");

        std::string id = e.getIdentifier();
        assert(id.find("event.collision." + subj.getIdentifier()) != std::string::npos);
    }

    // 4. Property reflection (Refusal #6: No black box)
    {
        Object subj;
        Event e("zone-entered", &subj, nullptr, Moment(123.456), "FirstMover");

        auto* pVerb = e.findProperty("verb");
        assert(pVerb != nullptr);
        assert(std::get<std::string>(pVerb->value()) == "zone-entered");

        auto* pType = e.findProperty("type");
        assert(pType != nullptr);
        assert(std::get<std::string>(pType->value()) == "zone-entered");

        auto* pStart = e.findProperty("start");
        assert(pStart != nullptr);
        assert(std::get<double>(pStart->value()) == 123.456);

        auto* pSubject = e.findProperty("subject");
        assert(pSubject != nullptr);
        assert(std::get<std::string>(pSubject->value()) == subj.getIdentifier());

        auto* pAuthor = e.findProperty("author");
        assert(pAuthor != nullptr);
        assert(std::get<std::string>(pAuthor->value()) == "FirstMover");
    }

    // 5. Serialization and JSON round-trip
    {
        Event e("object-clicked", nullptr, nullptr, Moment(15.0), "Zach");
        json j = e.toJson();
        assert(j["verb"] == "object-clicked");
        assert(j["type"] == "object-clicked");
        assert(j["start"] == 15.0);
        assert(j["author"] == "Zach");
    }

    // 6. ECA::Event alias and EventBus transport
    {
        bool received = false;
        Core::EventBus::instance().subscribe<ECA::Event>([&](const ECA::Event& ev) {
            assert(ev.verb() == "bus-test");
            assert(ev.asSeconds() == 99.0);
            assert(ev.timestamp().asSeconds() == 99.0);
            received = true;
        });

        ECA::Event echo("bus-test", nullptr, nullptr, Moment(99.0));
        Core::EventBus::instance().publish(echo);
        assert(received);
    }

    std::cout << "All Event tests passed successfully!\n";
    return 0;
}
