// A connected LawManager owns two EventBus listeners. Its destruction must
// revoke those listeners before the manager's storage disappears.
//
// This is deliberately a listener-registry witness instead of a crash probe:
// the pre-fix failure is deterministic and does not depend on allocator reuse.
// It guards the exact sequential-manager lifecycle that slow_adapter_parity_test
// and referent_map_invalidation_test historically had to avoid.
//
// Origin: Zach's lifecycle-correctness audit handoff, continued by GPT-5.6 Sol,
// 2026-09-20.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Singularity/Core/EventBus.hpp"

#include <cstdio>
#include <mutex>
#include <string>
#include <typeindex>

namespace Core {
struct EventBusTestFriend {
    template <typename Event>
    static std::size_t listenerCount() {
        EventBus& bus = EventBus::instance();
        std::lock_guard<std::mutex> lock(bus._mutex);
        auto it = bus._listeners.find(typeid(Event));
        return (it == bus._listeners.end() || !it->second) ? 0 : it->second->size();
    }
};
} // namespace Core

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++g_failures;
        std::printf("  FAILED: %s\n", what.c_str());
    } else {
        std::printf("  ok: %s\n", what.c_str());
    }
}

} // namespace

int main() {
    using Core::EventBusTestFriend;

    const std::size_t baseEca = EventBusTestFriend::listenerCount<ECA::Event>();
    const std::size_t baseCustom =
        EventBusTestFriend::listenerCount<Core::Event::Custom>();

    {
        LawManager first;
        first.connectToEventBus();
        check(EventBusTestFriend::listenerCount<ECA::Event>() == baseEca + 1,
              "first connected LawManager owns exactly one ECA listener");
        check(EventBusTestFriend::listenerCount<Core::Event::Custom>() == baseCustom + 1,
              "first connected LawManager owns exactly one Custom listener");
    }

    check(EventBusTestFriend::listenerCount<ECA::Event>() == baseEca,
          "destroying first LawManager returns ECA listeners to baseline");
    check(EventBusTestFriend::listenerCount<Core::Event::Custom>() == baseCustom,
          "destroying first LawManager returns Custom listeners to baseline");

    {
        LawManager second;
        second.connectToEventBus();
        check(EventBusTestFriend::listenerCount<ECA::Event>() == baseEca + 1,
              "second connected LawManager does not inherit a stale ECA listener");
        check(EventBusTestFriend::listenerCount<Core::Event::Custom>() == baseCustom + 1,
              "second connected LawManager does not inherit a stale Custom listener");
    }

    check(EventBusTestFriend::listenerCount<ECA::Event>() == baseEca,
          "destroying second LawManager returns ECA listeners to baseline again");
    check(EventBusTestFriend::listenerCount<Core::Event::Custom>() == baseCustom,
          "destroying second LawManager returns Custom listeners to baseline again");

    std::printf("%s\n",
                g_failures ? "law_manager_eventbus_lifetime_test: FAILURES"
                           : "law_manager_eventbus_lifetime_test: OK");
    return g_failures ? 1 : 0;
}
