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

struct BlockingAsyncEvent {};
struct CancelledAsyncEvent {};
struct DrainAsyncEvent {};

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

void testUnsubscribe() {
    std::cout << "[Test] Unsubscribe\n";

    int received = 0;
    auto token = EventBus::instance().subscribe<SimpleEvent>(
        [&received](const SimpleEvent&) { ++received; });

    assert(EventBus::instance().unsubscribe(token));
    assert(!EventBus::instance().unsubscribe(token));
    EventBus::instance().publish(SimpleEvent{7});
    assert(received == 0);

    std::cout << "  ✓ Revoked listener is not called\n";
    EventBusTestFriend::clear();
}

void testQueuedAsyncUnsubscribe() {
    std::cout << "[Test] Queued Async Unsubscribe\n";

    std::promise<void> blockerEnteredPromise;
    auto blockerEntered = blockerEnteredPromise.get_future();
    std::promise<void> releaseBlockerPromise;
    auto releaseBlocker = releaseBlockerPromise.get_future().share();
    std::promise<void> drainedPromise;
    auto drained = drainedPromise.get_future();
    std::atomic<bool> cancelledRan{false};

    EventBus::instance().subscribe<BlockingAsyncEvent>(
        [&](const BlockingAsyncEvent&) {
            blockerEnteredPromise.set_value();
            releaseBlocker.wait();
        });
    auto cancelledToken = EventBus::instance().subscribe<CancelledAsyncEvent>(
        [&](const CancelledAsyncEvent&) { cancelledRan.store(true); });
    EventBus::instance().subscribe<DrainAsyncEvent>(
        [&](const DrainAsyncEvent&) { drainedPromise.set_value(); });

    // Occupy the single worker so the cancellable delivery is definitely
    // queued but not entered when unsubscribe() revokes its liveness gate.
    EventBus::instance().publishAsync(BlockingAsyncEvent{});
    assert(blockerEntered.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
    EventBus::instance().publishAsync(CancelledAsyncEvent{});
    EventBus::instance().publishAsync(DrainAsyncEvent{});
    assert(EventBus::instance().unsubscribe(cancelledToken));
    releaseBlockerPromise.set_value();

    assert(drained.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
    assert(!cancelledRan.load());

    std::cout << "  ✓ Revoked listener is skipped by an already-queued async snapshot\n";
    EventBusTestFriend::clear();
}

int main() {
    std::cout << "\n=== Core::EventBus Test Suite ===\n\n";

    testSynchronousPublishSubscribe();
    testPrioritizedSubscribe();
    testReentrantPublish();
    testAsyncPublish();
    testUnsubscribe();
    testQueuedAsyncUnsubscribe();

    // Clean shutdown
    EventBus::instance().shutdown();

    std::cout << "\n✓ All tests passed!\n\n";

    return 0;
}
