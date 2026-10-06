#include "json.hpp"
#include "Time/Moment/Moment.hpp"

#include <cassert>
#include <iostream>

int main() {
    std::cout << "Testing Moment class...\n";

    // 1. Default constructor
    {
        Moment m;
        assert(m.isInstant());
        assert(m.asSeconds() == 0.0);
        assert(m.propStart() == 0.0);
        assert(m.propEnd() == 0.0);
    }

    // 2. Double constructor
    {
        Moment m(42.5);
        assert(m.isInstant());
        assert(m.asSeconds() == 42.5);
        assert(m.propStart() == 42.5);
        assert(m.propEnd() == 42.5);
    }

    // 3. Interval constructor
    {
        Moment span = Moment::interval(10.0, 25.0);
        assert(span.isInterval());
        assert(span.asSeconds() == 10.0);
        assert(span.endSeconds().has_value() && *span.endSeconds() == 25.0);
        assert(span.propStart() == 10.0);
        assert(span.propEnd() == 25.0);
    }

    // 4. Instant constructor
    {
        Moment m = Moment::instant(15.0);
        assert(m.isInstant());
        assert(m.asSeconds() == 15.0);
        assert(m.propStart() == 15.0);
        assert(m.propEnd() == 15.0);
    }

    // 5. Named identifier constructor
    {
        Moment m("my-moment", Moment(10.0));
        assert(m.getIdentifier() == "my-moment");
        assert(m.asSeconds() == 10.0);
    }

    // 6. Property reflection (findProperty)
    {
        Moment m(123.456);

        auto* pKind = m.findProperty("kind");
        assert(pKind != nullptr);
        assert(std::get<int>(pKind->value()) == static_cast<int>(Moment::Kind::Instant));

        auto* pStart = m.findProperty("start");
        assert(pStart != nullptr);
        assert(std::get<double>(pStart->value()) == 123.456);

        auto* pEnd = m.findProperty("end");
        assert(pEnd != nullptr);
        assert(std::get<double>(pEnd->value()) == 123.456);

        auto* pCycle = m.findProperty("cpuClockCycle");
        assert(pCycle != nullptr);
        // Ensure property evaluation returns a value without throwing/crashing.
        (void)pCycle->value();

        m.setStart(100.0);
        assert(m.asSeconds() == 100.0);
    }

    // 7. Serialization and JSON round-trip
    {
        Moment span = Moment::interval(10.0, 20.0);
        nlohmann::json j = span.toJson();
        assert(j["kind"] == static_cast<int>(Moment::Kind::Interval));
        assert(j["start"] == 10.0);
        assert(j["end"] == 20.0);

        Moment m2 = Moment::fromJson(j);
        assert(m2.isInterval());
        assert(m2.asSeconds() == 10.0);
        assert(m2.endSeconds().has_value() && *m2.endSeconds() == 20.0);
    }

    std::cout << "All Moment tests passed successfully!\n";
    return 0;
}
