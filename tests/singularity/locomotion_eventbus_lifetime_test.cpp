// LocomotionChannel owns an EventBus router that captures this. Both explicit
// LawManager removal and whole-manager teardown must revoke that registration
// before the channel's storage disappears.

#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Input/Locomotion/LocomotionChannel.hpp"
#include "Person/PersonEvents.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <cstdio>
#include <mutex>
#include <string>

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
}

int main() {
    using Core::EventBusTestFriend;
    using Singularity::Input::LocomotionChannel;

    const std::size_t baseline =
        EventBusTestFriend::listenerCount<LocomotionChanged>();

    {
        LawManager laws;
        LocomotionChannel::syncRegister(laws);
        auto* channel = LocomotionChannel::find(laws);
        check(channel != nullptr, "LocomotionChannel registered");
        if (channel) channel->installRouting();

        check(EventBusTestFriend::listenerCount<LocomotionChanged>() == baseline + 1,
              "routing installs exactly one LocomotionChanged listener");
        check(laws.remove("locomotion-channel"),
              "explicit LawManager removal destroys the routed channel");
        check(EventBusTestFriend::listenerCount<LocomotionChanged>() == baseline,
              "explicit removal returns routing listeners to baseline");
    }

    {
        LawManager laws;
        LocomotionChannel::syncRegister(laws);
        auto* channel = LocomotionChannel::find(laws);
        if (channel) channel->installRouting();

        check(EventBusTestFriend::listenerCount<LocomotionChanged>() == baseline + 1,
              "fresh manager installs one routing listener");
    }

    check(EventBusTestFriend::listenerCount<LocomotionChanged>() == baseline,
          "LawManager teardown returns routing listeners to baseline");

    std::printf("%s\n",
                g_failures ? "locomotion_eventbus_lifetime_test: FAILURES"
                           : "locomotion_eventbus_lifetime_test: OK");
    return g_failures ? 1 : 0;
}
