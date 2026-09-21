// Warden regression: a process-wide EventBus must not keep speaking through
// an owner after that owner has left scope.
//
// The historical failure was already documented in slow_adapter_parity_test and
// referent_map_invalidation_test: constructing two connected LawManagers in one
// process either doubled the second manager's event intake or crashed because
// the first manager's captured callback remained registered.
//
// This test exercises both layers of the repair:
//   1. one subscription can be released without clearing unrelated listeners;
//   2. LawManager releases the EventBus voices it installed on destruction.

#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <cstdio>
#include <ctime>
#include <string>

namespace {

int g_failures = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        ++g_failures;
        std::printf("  FAILED: %s\n", what);
    } else {
        std::printf("  ok: %s\n", what);
    }
}

struct LifetimePulse {
    int value = 0;
};

std::size_t eventFactCount(const LawManager& manager, const std::string& type) {
    std::size_t count = 0;
    for (const auto& fact : manager.rete().facts()) {
        if (fact && fact->type == type) ++count;
    }
    return count;
}

} // namespace

int main() {
    auto& bus = Core::EventBus::instance();

    int releasedCalls = 0;
    int survivingCalls = 0;

    const auto released = bus.subscribe<LifetimePulse>(
        [&](const LifetimePulse&) { ++releasedCalls; });
    const auto surviving = bus.subscribe<LifetimePulse>(
        [&](const LifetimePulse&) { ++survivingCalls; });

    bus.publish(LifetimePulse{1});
    check(releasedCalls == 1 && survivingCalls == 1,
          "both live EventBus voices hear the first pulse");

    bus.unsubscribe<LifetimePulse>(released);
    bus.publish(LifetimePulse{2});
    check(releasedCalls == 1,
          "a released EventBus voice is silent on later publication");
    check(survivingCalls == 2,
          "releasing one voice does not clear unrelated listeners");
    bus.unsubscribe<LifetimePulse>(surviving);

    {
        LawManager first;
        first.connectToEventBus();

        ECA::Event event{"warden-first-manager", nullptr, nullptr, std::time(nullptr)};
        bus.publish(event);
        check(eventFactCount(first, "warden-first-manager") == 1,
              "the first connected LawManager hears its event exactly once");
    }

    // The process-wide EventBus is still alive, but the first manager is not.
    // A fresh manager at the same lifecycle boundary must be the only manager
    // that hears this event.
    {
        LawManager second;
        second.connectToEventBus();

        ECA::Event event{"warden-second-manager", nullptr, nullptr, std::time(nullptr)};
        bus.publish(event);
        check(eventFactCount(second, "warden-second-manager") == 1,
              "a fresh LawManager hears once after the previous manager left scope");
    }

    std::printf("%s\n", g_failures ? "event_bus_lifetime_test: FAILURES"
                                   : "event_bus_lifetime_test: OK");
    return g_failures ? 1 : 0;
}
