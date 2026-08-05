#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>

class Object;
class ZoneManager;

namespace Core { class Game; }

// CursorTools: handles 3D selection and UI for object picking
class CursorTools {
public:
    explicit CursorTools(ZoneManager* mgr);
    CursorTools();

    // Update selection/picking each frame
    void update(Core::Game& game);

    // Render the cursor tools window (returns whether open)
    void renderUI(bool& open);

    // Selected objects
    Object* getPrimary() const { return _primary; }
    Object* getSecondary() const { return _secondary; }

    // Utility: clear selections
    void clearSelection();

private:
    ZoneManager* _mgr;
    Object* _primary = nullptr;
    Object* _secondary = nullptr;

    // Settings
    bool _enabled = true;
    bool _selectOnClick = true;
    bool _appendWithShift = true;

    // Internal: perform 3D pick using camera data in Game and objects in active Zone
    Object* pickObjectAtCursor3D(Core::Game& game) const;

    // Apply currently selected law to selected objects (hooked via UI)
    void applyLawToSelection(int lawId);
};


