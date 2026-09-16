// Zone-native regression witness for Basic2DButtonZone: the authored orange
// button must remain a normal Object + Law world that works immediately after
// boot, without loading basic_2d_button_zone.json as a legacy session.

#include "support/test_harness.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"

#include <cassert>
#include <cmath>
#include <ctime>
#include <iostream>
#include <string>

namespace {

Object* findObj(Zone& zone, const std::string& id) {
    for (const auto& o : zone.getOwnedObjects()) {
        if (o && o->getIdentifier() == id) return o.get();
    }
    return nullptr;
}

double asNumber(Singular& being, const char* name, double fallback = -999.0) {
    PropertyValue v;
    if (!being.getDynamicProperty(name, v)) return fallback;
    double n = 0.0;
    return propertyValueToNumber(v, n) ? n : fallback;
}

} // namespace

int main() {
    TestSupport::RealSaveTreeGuard guard(TestSupport::GuardCurrentRoot);
    TestSupport::BootedEngineHarness harness("Zach");

    std::size_t buttonIndex = harness.zones.zones().size();
    for (std::size_t i = 0; i < harness.zones.zones().size(); ++i) {
        const auto& zone = harness.zones.zones()[i];
        if (zone && zone->getIdentifier() == "Basic2DButtonZone") {
            buttonIndex = i;
            break;
        }
    }
    assert(buttonIndex < harness.zones.zones().size());
    assert(harness.zones.switchTo(buttonIndex));

    auto zone = harness.zones.zones()[buttonIndex];
    assert(zone && zone->getIdentifier() == "Basic2DButtonZone");
    Object* button = findObj(*zone, "my-2d-button");
    Object* author = findObj(*zone, "Antigravity");
    assert(button && author);
    assert(!findObj(*zone, "Zach") && "never forge a Person as a compatibility Object");

    assert(harness.lawManager.find("law-move-right"));
    assert(harness.lawManager.find("law-move-left"));
    assert(harness.lawManager.find("law-bounce-right"));
    assert(harness.lawManager.find("law-bounce-left"));
    assert(std::fabs(asNumber(*button, "speedDir") - 1.0) < 1e-6);

    const float before = button->getX2D();
    assert(std::fabs(before - 100.0f) < 1e-4f);
    Core::EventBus::instance().publish(
        ECA::Event{"object-clicked", button, nullptr, std::time(nullptr)});
    harness.lawManager.tick();
    const float after = button->getX2D();
    assert(std::fabs(after - (before + 20.0f)) < 1e-4f);

    std::cout << "Zone-native fresh boot entered Basic2DButtonZone and its Object + Law "
                 "button moved from x2D=100 to x2D=120 without a legacy World load.\n";
    return 0;
}
