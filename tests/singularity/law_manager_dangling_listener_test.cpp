#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Singularity/Core/EventBus.hpp"
#include <cassert>
#include <iostream>

int main() {
    {
        LawManager laws;
        laws.connectToEventBus();
    }
    // LawManager is now destroyed.
    // If listeners were not unsubscribed, publishing an ECA::Event or Core::Event::Custom
    // will invoke a lambda capturing a dangling `this` pointer or dangling LawManager state.
    ECA::Event ev("test_event", nullptr, nullptr, 0);
    Core::EventBus::instance().publish(ev);

    Core::Event::Custom customEv;
    Core::EventBus::instance().publish(customEv);

    std::cout << "law_manager_dangling_listener_test: ALL OK" << std::endl;
    return 0;
}
