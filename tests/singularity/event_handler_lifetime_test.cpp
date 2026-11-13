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
int failures = 0;
void check(bool ok, const char* description) {
    if (!ok) {
        ++failures;
        std::fprintf(stderr, "FAILED: %s\n", description);
    } else {
        std::printf("ok: %s\n", description);
    }
}
struct DummyEvent { int value; };
struct OtherEvent { int value; };
} // namespace

int main() {
    using Core::EventBus;
    using Core::EventBusTestFriend;
    using Core::EventHandler;

    auto& handler = EventHandler::instance();
    auto& bus = EventBus::instance();
    const auto baseline = EventBusTestFriend::listenerCount<DummyEvent>();
    const auto otherBaseline = EventBusTestFriend::listenerCount<OtherEvent>();

    int originalCalls = 0;
    int replacementCalls = 0;
    handler.registerHandler<DummyEvent>("replace-me", [&](const DummyEvent&) { ++originalCalls; });
    check(EventBusTestFriend::listenerCount<DummyEvent>() == baseline + 1,
          "registration installs exactly one listener");
    bus.publish(DummyEvent{1});
    check(originalCalls == 1, "registered callback receives the event");

    handler.registerHandler<DummyEvent>("replace-me", [&](const DummyEvent&) { ++replacementCalls; });
    check(handler.getHandlerCount() == 1 && handler.hasHandler("replace-me"),
          "re-registering the same name retains exactly one managed handler");
    check(EventBusTestFriend::listenerCount<DummyEvent>() == baseline + 1,
          "re-registering the same name unsubscribes the previous listener");
    bus.publish(DummyEvent{2});
    check(originalCalls == 1 && replacementCalls == 1,
          "only the replacement callback receives subsequent events");

    handler.removeHandler("replace-me");
    check(EventBusTestFriend::listenerCount<DummyEvent>() == baseline &&
              !handler.hasHandler("replace-me"),
          "removeHandler revokes the EventBus subscription");
    bus.publish(DummyEvent{3});
    check(replacementCalls == 1, "removed callback cannot fire again");

    handler.registerHandler<DummyEvent>("one", [](const DummyEvent&) {});
    handler.registerHandler<DummyEvent>("two", [](const DummyEvent&) {});
    check(EventBusTestFriend::listenerCount<DummyEvent>() == baseline + 2,
          "distinct handler names install two listeners");
    handler.clearAllHandlers();
    check(EventBusTestFriend::listenerCount<DummyEvent>() == baseline &&
              handler.getHandlerCount() == 0,
          "clearAllHandlers revokes every managed subscription");

    handler.registerHandler<DummyEvent>("change-type", [](const DummyEvent&) {});
    handler.registerHandler<OtherEvent>("change-type", [](const OtherEvent&) {});
    check(EventBusTestFriend::listenerCount<DummyEvent>() == baseline &&
              EventBusTestFriend::listenerCount<OtherEvent>() == otherBaseline + 1,
          "same-name replacement across event types revokes the old type");
    handler.removeHandler("change-type");
    check(EventBusTestFriend::listenerCount<OtherEvent>() == otherBaseline,
          "cross-type replacement token can be revoked");

    std::printf("event_handler_lifetime_test: %s\n", failures ? "FAILED" : "PASSED");
    return failures ? 1 : 0;
}
