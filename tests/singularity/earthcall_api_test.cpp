#include "Singularity/Foreign/API/EarthcallAPI.hpp"
#include "Singularity/Foreign/API/SecurityManager.hpp"
#include "Singularity/FirstMoverOntology/Legacy/DesignSystem.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <string>

int main() {
    std::cout << "=== Running EarthcallAPI Tests ===" << std::endl;

    Integration::EarthcallAPI api;

    // --- DESIGN SYSTEM TESTS ---
    std::cout << "\n--- Design System Tests ---" << std::endl;
    // 1. Permission check: creation should fail without "design_system" permission
    Integration::EarthcallAPI::DesignElement elem1;
    elem1.name = "box_1";
    elem1.type = "shape";
    elem1.position = glm::vec3(10.0f, 20.0f, 0.0f);
    elem1.scale = glm::vec3(150.0f, 100.0f, 1.0f);
    elem1.properties["shape_type"] = "rectangle";
    elem1.properties["color"] = "#ff0000";

    bool createdNoPerm = api.createDesignElement(elem1);
    assert(!createdNoPerm && "Creation should fail when permission is missing");

    // Grant permission via SecurityManager
    Integration::SecurityManager::instance().grantPermission(
        Integration::PermissionType::DESIGN_SYSTEM, "earthcall_api"
    );
    assert(api.hasPermission("design_system") && "Permission for design_system should now be granted");

    // 2. Test creation without attached DesignSystem (data tracking mode)
    bool createdWithPerm = api.createDesignElement(elem1);
    assert(createdWithPerm && "Creation should succeed when permission is granted");

    auto elements = api.getDesignElements();
    assert(elements.size() == 1 && "Should have 1 design element stored");
    assert(elements[0].name == "box_1");
    assert(elements[0].position.x == 10.0f);

    // 3. Test modification
    Integration::EarthcallAPI::DesignElement elem1Mod = elem1;
    elem1Mod.position = glm::vec3(50.0f, 60.0f, 0.0f);
    bool modified = api.modifyDesignElement("box_1", elem1Mod);
    assert(modified && "Modification should succeed for existing element");

    elements = api.getDesignElements();
    assert(elements[0].position.x == 50.0f);

    // 4. Test creation with attached DesignSystem
    DesignSystem ds;
    api.setDesignSystem(&ds);

    Integration::EarthcallAPI::DesignElement shapeElem;
    shapeElem.name = "star_1";
    shapeElem.type = "star";
    shapeElem.position = glm::vec3(100.0f, 100.0f, 0.0f);
    shapeElem.scale = glm::vec3(80.0f, 80.0f, 1.0f);
    shapeElem.rotation = glm::vec3(0.0f, 0.0f, 45.0f);
    shapeElem.properties["color"] = "#00ff00";

    bool createdShape = api.createDesignElement(shapeElem);
    assert(createdShape && "Shape creation with DesignSystem attached should succeed");
    assert(ds.getShapeSystem()->getShapeElements().size() == 1 && "ShapeSystem should contain 1 shape");

    Integration::EarthcallAPI::DesignElement textElem;
    textElem.name = "label_1";
    textElem.type = "text";
    textElem.position = glm::vec3(200.0f, 200.0f, 0.0f);
    textElem.properties["text"] = "Hello World";
    textElem.properties["color"] = "#ffffff";

    bool createdText = api.createDesignElement(textElem);
    assert(createdText && "Text creation with DesignSystem attached should succeed");
    assert(ds.getTextSystem()->getTextElements().size() == 1 && "TextSystem should contain 1 text element");

    Integration::EarthcallAPI::DesignElement effectElem;
    effectElem.name = "glow_1";
    effectElem.type = "effect";
    effectElem.properties["effect_type"] = "glow";
    effectElem.properties["intensity"] = "0.8";

    bool createdEffect = api.createDesignElement(effectElem);
    assert(createdEffect && "Effect creation with DesignSystem attached should succeed");
    assert(ds.getEffectsSystem()->getEffects().size() == 1 && "EffectsSystem should contain 1 effect");

    // 5. Test template application
    bool templateApplied = api.applyDesignTemplate("card");
    assert(templateApplied && "Template application should succeed");

    auto allElements = api.getDesignElements();
    assert(allElements.size() >= 7 && "Should have accumulated elements from template");

    // 6. Test deletion
    bool deleted = api.deleteDesignElement("box_1");
    assert(deleted && "Deletion of existing element should succeed");

    bool deleteNonExistent = api.deleteDesignElement("non_existent_element");
    assert(!deleteNonExistent && "Deletion of non-existent element should fail");

    // --- WORLD/ENVIRONMENT ACCESS TESTS ---
    std::cout << "\n--- World/Environment Access Tests ---" << std::endl;
    assert(!api.createZone("zone1", 0, 0, 100, 100));
    assert(!api.addZoneObject("zone1", "tree", 10, 10));
    assert(!api.setZoneTheme("zone1", "forest"));
    assert(api.getZones().empty());
    assert(!api.createObject("rock", glm::vec3(0.0f)));
    assert(!api.modifyObject("rock1", glm::vec3(1.0f), glm::vec3(1.0f)));
    assert(!api.deleteObject("rock1"));
    assert(!api.setCameraPosition(glm::vec3(10.0f)));

    // getCameraPosition doesn't check permissions and returns hardcoded vec3(0,0,0) right now
    auto camPos = api.getCameraPosition();
    assert(camPos.x == 0.0f && camPos.y == 0.0f && camPos.z == 0.0f);

    Integration::SecurityManager::instance().grantPermission(
        Integration::PermissionType::WORLD_ACCESS, "earthcall_api"
    );
    assert(api.hasPermission("world_access"));

    // createZone returns false when _zoneManager is null
    assert(!api.createZone("zone1", 0, 0, 100, 100));
    assert(api.addZoneObject("zone1", "tree", 10, 10));
    assert(api.setZoneTheme("zone1", "forest"));
    assert(api.getZones().empty()); // Hardcoded to empty right now
    assert(api.createObject("rock", glm::vec3(0.0f)));
    assert(api.modifyObject("rock1", glm::vec3(1.0f), glm::vec3(1.0f)));
    assert(api.deleteObject("rock1"));
    assert(api.setCameraPosition(glm::vec3(10.0f)));

    // --- DATA/SAVE ACCESS TESTS ---
    std::cout << "\n--- Data/Save Access Tests ---" << std::endl;
    assert(!api.saveData("key1", "value1"));
    assert(api.loadData("key1") == "");
    assert(api.getDataKeys().empty());

    Integration::SecurityManager::instance().grantPermission(
        Integration::PermissionType::DATA_ACCESS, "earthcall_api"
    );
    assert(api.hasPermission("data_access"));

    assert(api.saveData("key1", "value1"));
    assert(api.loadData("key1") == ""); // Hardcoded
    assert(api.getDataKeys().empty()); // Hardcoded


    // --- COMMUNICATION TESTS ---
    std::cout << "\n--- Communication Tests ---" << std::endl;
    bool event_received = false;
    api.registerCallback("test_event", [&event_received](const std::string& data) {
        event_received = true;
        assert(data == "data");
    });
    api.sendEvent("test_event", "data");
    assert(event_received);

    event_received = false;
    api.unregisterCallback("test_event");
    api.sendEvent("test_event", "data");
    assert(!event_received);


    // --- PERMISSIONS TESTS ---
    std::cout << "\n--- Permissions Tests ---" << std::endl;
    assert(!api.hasPermission("ui_control"));
    api.requestPermission("ui_control");
    // Depending on SecurityManager, it might auto-grant or just queue.
    // We explicitly grant to test getGrantedPermissions properly.
    Integration::SecurityManager::instance().grantPermission(
        Integration::PermissionType::UI_CONTROL, "earthcall_api"
    );
    assert(api.hasPermission("ui_control"));

    auto granted = api.getGrantedPermissions();
    // Check if the granted string vector contains the mapped integer for UI_CONTROL
    bool foundUiControl = false;
    std::string uiControlIntStr = std::to_string(static_cast<int>(Integration::PermissionType::UI_CONTROL));
    for (const auto& p : granted) {
        if (p == uiControlIntStr) {
            foundUiControl = true;
            break;
        }
    }
    assert(foundUiControl && "Granted permissions should include ui_control");

    std::cout << "\n=== ALL Tests Passed Successfully! ===" << std::endl;
    return 0;
}
