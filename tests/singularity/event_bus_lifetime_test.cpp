#include "Singularity/Core/EventBus.hpp"
#include <cassert>
#include <iostream>

using namespace Core;

struct DummyEvent {
    int value;
};

class TransientSubscriber {
public:
    TransientSubscriber(int& counter) : _counter(counter) {
        _subId = EventBus::instance().subscribe<DummyEvent>([this](const DummyEvent& e) {
            _counter += e.value;
        });
    }

    ~TransientSubscriber() {
        EventBus::instance().unsubscribe(_subId);
    }

private:
    int& _counter;
    uint64_t _subId = 0;
};

void testTransientSubscriber() {
    int counter = 0;

    {
        TransientSubscriber subscriber(counter);
        EventBus::instance().publish(DummyEvent{5});
        assert(counter == 5);
    } // subscriber destroyed here

    // Should NOT trigger the callback
    EventBus::instance().publish(DummyEvent{10});

    assert(counter == 5); // Remained 5!
    std::cout << "  ✓ Transient subscriber successfully unsubscribed on destruction\n";
}

int main() {
    std::cout << "\n=== Core::EventBus Lifetime Test Suite ===\n\n";

    testTransientSubscriber();

    std::cout << "\n✓ All tests passed!\n\n";

    return 0;
}
