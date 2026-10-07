#include "Singularity/Core/EventBus.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <future>

namespace Core {
struct EventBusTestFriend {
    static void clear() {
        EventBus::instance().clear();
    }
};
} // namespace Core

using namespace Core;

struct SimpleEvent {
    int value;
};

struct PrioritizedEvent {
    int value;
};

struct AsyncEvent {
    int value;
};

struct PingEvent {
    int value;
};

struct PongEvent {
    int value;
};

void testSynchronousPublishSubscribe() {
    std::cout << "[Test] Synchronous Publish/Subscribe\n";

    int receivedValue = 0;
    EventBus::instance().subscribe<SimpleEvent>([&receivedValue](const SimpleEvent& e) {
        receivedValue = e.value;
    });

    EventBus::instance().publish(SimpleEvent{42});

    assert(receivedValue == 42);
    std::cout << "  ✓ Synchronous delivery successful\n";
    EventBusTestFriend::clear();
}

void testPrioritizedSubscribe() {
    std::cout << "[Test] Prioritized Subscribe\n";

    std::vector<int> executionOrder;

    EventBus::instance().subscribe<PrioritizedEvent>([&executionOrder](const PrioritizedEvent&) {
        executionOrder.push_back(1);
    }, 10); // Priority 10

    EventBus::instance().subscribe<PrioritizedEvent>([&executionOrder](const PrioritizedEvent&) {
        executionOrder.push_back(3);
    }, 30); // Priority 30 (Highest)

    EventBus::instance().subscribe<PrioritizedEvent>([&executionOrder](const PrioritizedEvent&) {
        executionOrder.push_back(2);
    }, 20); // Priority 20

    EventBus::instance().publish(PrioritizedEvent{0});

    assert(executionOrder.size() == 3);
    assert(executionOrder[0] == 3); // Priority 30 should run first
    assert(executionOrder[1] == 2); // Priority 20 second
    assert(executionOrder[2] == 1); // Priority 10 third

    std::cout << "  ✓ Listeners executed in correct priority order\n";
    EventBusTestFriend::clear();
}

void testReentrantPublish() {
    std::cout << "[Test] Re-entrant Publish\n";

    int pingReceived = 0;
    int pongReceived = 0;

    // Setup ping listener that publishes a pong
    EventBus::instance().subscribe<PingEvent>([&pingReceived](const PingEvent& e) {
        pingReceived = e.value;
        EventBus::instance().publish(PongEvent{e.value * 2});
    });

    // Setup pong listener
    EventBus::instance().subscribe<PongEvent>([&pongReceived](const PongEvent& e) {
        pongReceived = e.value;
    });

    // Publish initial ping
    EventBus::instance().publish(PingEvent{5});

    assert(pingReceived == 5);
    assert(pongReceived == 10);

    std::cout << "  ✓ Re-entrant publish did not deadlock and executed successfully\n";
    EventBusTestFriend::clear();
}

void testAsyncPublish() {
    std::cout << "[Test] Async Publish\n";

    std::promise<int> promise;
    auto future = promise.get_future();

    EventBus::instance().subscribe<AsyncEvent>([&promise](const AsyncEvent& e) {
        promise.set_value(e.value);
    });

    EventBus::instance().publishAsync(AsyncEvent{99});

    // Wait deterministically for the background worker thread to process the job
    assert(future.wait_for(std::chrono::seconds(2)) == std::future_status::ready && "Async publish timed out!");
    assert(future.get() == 99);

    std::cout << "  ✓ Async delivery successful\n";
    EventBusTestFriend::clear();
}

int main() {
    std::cout << "\n=== Core::EventBus Test Suite ===\n\n";

    testSynchronousPublishSubscribe();
    testPrioritizedSubscribe();
    testReentrantPublish();
    testAsyncPublish();

    // Clean shutdown
    EventBus::instance().shutdown();

    std::cout << "\n✓ All tests passed!\n\n";

    return 0;
}
