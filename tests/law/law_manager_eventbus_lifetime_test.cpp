// Warden regression: a connected LawManager may leave the world while the
// process-global EventBus remains. Its two listener roads must leave with it.
//
// Before the repair, Earthcall tests worked around this by insisting on ONE
// connected LawManager per process; referent_map_invalidation_test records that
// constructing a second connected manager after the first had gone away could
// segfault, and slow_adapter_parity_test records duplicate hearing.
//
// This witness checks the ownership relation directly instead of depending on
// allocator reuse: the EventBus registry must contain exactly the live manager's
// roads, and none after its destructor.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Singularity/Core/EventBus.hpp"

#include <cstdio>
#include <typeindex>

namespace Core {
struct EventBusTestFriend {
    static void clear() {
        EventBus::instance().clear();
    }

    template<typename Event>
    static std::size_t listenerCount() {
        auto& bus = EventBus::instance();
        std::lock_guard<std::mutex> lock(bus._mutex);
        auto it = bus._listeners.find(typeid(Event));
        return (it == bus._listeners.end() || !it->second) ? 0 : it->second->size();
    }
};
} // namespace Core

namespace {
int failures = 0;
void check(bool ok, const char* what) {
    if (ok) std::printf("  ok: %s\n", what);
    else { ++failures; std::printf("  FAILED: %s\n", what); }
}
} // namespace

int main() {
    using Core::EventBusTestFriend;

    EventBusTestFriend::clear();
    check(EventBusTestFriend::listenerCount<ECA::Event>() == 0,
          "no ECA road exists before a manager connects");
    check(EventBusTestFriend::listenerCount<Core::Event::Custom>() == 0,
          "no Custom road exists before a manager connects");

    {
        LawManager first;
        first.connectToEventBus();
        check(EventBusTestFriend::listenerCount<ECA::Event>() == 1,
              "first manager owns exactly one ECA road");
        check(EventBusTestFriend::listenerCount<Core::Event::Custom>() == 1,
              "first manager owns exactly one Custom road");
    }

    check(EventBusTestFriend::listenerCount<ECA::Event>() == 0,
          "first manager takes its ECA road with it");
    check(EventBusTestFriend::listenerCount<Core::Event::Custom>() == 0,
          "first manager takes its Custom road with it");

    // This publish used to be able to enter the already-destroyed first
    // manager. With no road left it is intentionally unheard.
    Core::EventBus::instance().publish(
        ECA::Event{"warden-after-first-manager", nullptr, nullptr, std::time(nullptr)});

    {
        LawManager second;
        second.connectToEventBus();
        check(EventBusTestFriend::listenerCount<ECA::Event>() == 1,
              "second manager does not inherit a ghost ECA road");
        check(EventBusTestFriend::listenerCount<Core::Event::Custom>() == 1,
              "second manager does not inherit a ghost Custom road");

        Core::EventBus::instance().publish(
            ECA::Event{"warden-second-manager", nullptr, nullptr, std::time(nullptr)});
    }

    check(EventBusTestFriend::listenerCount<ECA::Event>() == 0,
          "second manager also releases its ECA road");
    check(EventBusTestFriend::listenerCount<Core::Event::Custom>() == 0,
          "second manager also releases its Custom road");

    EventBusTestFriend::clear();
    std::printf("law_manager_eventbus_lifetime_test: %s\n",
                failures == 0 ? "PASS" : "FAIL");
    return failures == 0 ? 0 : 1;
}
