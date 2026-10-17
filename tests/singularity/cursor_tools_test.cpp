#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CursorTools.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include <iostream>
#include <cassert>

int main() {
    int failures = 0;
    auto check = [&](bool condition, const std::string& msg) {
        if (!condition) {
            std::cerr << "FAIL: " << msg << "\n";
            failures++;
        }
    };

    ZoneManager mgr;

    CursorTools tools(&mgr);

    check(tools.getPrimary() == nullptr, "Primary selection is initially null");
    check(tools.getSecondary() == nullptr, "Secondary selection is initially null");

    tools.clearSelection();
    check(tools.getPrimary() == nullptr, "Primary selection is null after clear");

    // The renderUI function doesn't crash when called with false
    bool open = false;
    tools.renderUI(open);

    if (failures == 0) {
        std::cout << "cursor_tools_test: ALL OK\n";
    }
    return failures > 0 ? 1 : 0;
}
