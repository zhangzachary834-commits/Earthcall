#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleState.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Person/Person.hpp"
#include "Singularity/Screen/HighlightSystem.hpp"
#include <cassert>
#include <iostream>

using namespace Rendering;

int main() {
    std::cout << "=== Running CreatorConsoleState Tests ===" << std::endl;

    // Test singleton access and initial values
    auto& state = getCreatorConsoleState();

    // Check initial defaults based on the header
    assert(state.currentSection == CreatorSection::Create3D);
    assert(state.current3DMode == Mode3D::None);
    assert(state.current3DTarget == ToolTarget3D::WorldObjects);
    assert(state.currentShapeKind == ObjectTypes::ShapeKind::Cube);

    // Test modifying state
    state.currentSection = CreatorSection::Paint;
    assert(getCreatorConsoleState().currentSection == CreatorSection::Paint);

    state.currentSection = CreatorSection::Concepts;
    assert(getCreatorConsoleState().currentSection == CreatorSection::Concepts);

    state.polyhedron.shapeKind = ObjectTypes::ShapeKind::Sphere;
    assert(getCreatorConsoleState().polyhedron.shapeKind == ObjectTypes::ShapeKind::Sphere);

    // Test forgetStaleObjectHandles
    ZoneManager zoneMgr;

    Object* dummyObj = new Object();
    Object* dummyObj2 = new Object();

    state.selectedObject3D = dummyObj;
    state.combineOperandA = dummyObj2;
    state.lastBrushObject = dummyObj;

    // By passing an empty ZoneManager where these objects are NOT registered,
    // they should be cleared.
    forgetStaleObjectHandles(zoneMgr, nullptr);

    assert(state.selectedObject3D == nullptr);
    assert(state.combineOperandA == nullptr);
    assert(state.lastBrushObject == nullptr);

    delete dummyObj;
    delete dummyObj2;

    // Test Undo / Redo for Spawn & Delete
    auto testZone = std::make_shared<Zone>("TestZone", "default");
    zoneMgr.addZone(testZone);
    zoneMgr.switchTo(0);

    auto testObj = std::make_shared<Object>();
    zoneMgr.active().addObject(testObj);
    state.recordSpawn(testObj, "Spawn test object");

    assert(state.canUndo());
    assert(!state.canRedo());
    assert(zoneMgr.active().getOwnedObjects().size() == 1);

    state.performUndo(zoneMgr);
    assert(!state.canUndo());
    assert(state.canRedo());
    assert(zoneMgr.active().getOwnedObjects().empty());

    state.performRedo(zoneMgr);
    assert(state.canUndo());
    assert(!state.canRedo());
    assert(zoneMgr.active().getOwnedObjects().size() == 1);

    // Test Delete Undo/Redo
    state.recordDelete(testObj, "Delete test object");
    zoneMgr.active().removeObject(testObj.get());
    assert(zoneMgr.active().getOwnedObjects().empty());

    state.performUndo(zoneMgr);
    assert(zoneMgr.active().getOwnedObjects().size() == 1);

    state.performRedo(zoneMgr);
    assert(zoneMgr.active().getOwnedObjects().empty());

    std::cout << "All CreatorConsoleState tests PASSED!" << std::endl;
    return 0;
}
