#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Core/EventHandler.hpp"
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
struct DummyEvent { int val; };
}

int main() {
    using Core::EventHandler;
    using Core::EventBusTestFriend;

    const std::size_t baseline = EventBusTestFriend::listenerCount<DummyEvent>();

    int callCount = 0;
    EventHandler::instance().registerHandler<DummyEvent>("test1", [&](const DummyEvent&){
        callCount++;
    });

    check(EventBusTestFriend::listenerCount<DummyEvent>() == baseline + 1,
          "registerHandler installs one listener");

    EventHandler::instance().removeHandler("test1");

    check(EventBusTestFriend::listenerCount<DummyEvent>() == baseline,
          "removeHandler removes the listener");

    EventHandler::instance().registerHandler<DummyEvent>("test2", [&](const DummyEvent&){});
    EventHandler::instance().registerHandler<DummyEvent>("test3", [&](const DummyEvent&){});

    check(EventBusTestFriend::listenerCount<DummyEvent>() == baseline + 2,
          "registerHandler installs more listeners");

    EventHandler::instance().clearAllHandlers();

    check(EventBusTestFriend::listenerCount<DummyEvent>() == baseline,
          "clearAllHandlers removes all listeners");

    std::printf("%s\n",
                g_failures ? "event_handler_lifetime_test: FAILURES"
                           : "event_handler_lifetime_test: OK");
    return g_failures ? 1 : 0;
}
