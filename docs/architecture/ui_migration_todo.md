# UI Migration To-Do

During the `Engine` refactor, hardcoded UI concepts ("gizmos", FaceBrush UI overlays, and `Object` mutation inside the update/render loop) were stripped from `GameUpdate.cpp` and `GameRender.cpp`.

This is required because **no subsystem may define what a thing IS** (Refusal 1 & 6). If a tool needs a preview or a UI handle, it must spawn a real `Object` (or `Relation`) into the world. The renderer draws the world; it should not hold hardcoded `if (mode == Morph) drawHandle()`.

This document preserves the removed C++ logic. The goal is to reimplement these via **Laws**, **First Movers**, or **Formations** that spawn and manage transparent/holographic `Object` entities representing these UI elements.

## Preserved Code

### 1. Morph, Combine, Clay: Field Gizmos & Blend Rails
*From `scratch/GameUpdate.cpp` (Handle Logic) and `scratch/GameRender.cpp` (Rendering)*

**Goal:** Migrate to a Law that observes `selectedObject.isBinaryField` and spawns a "gold cube" object at `operandBOffset`. Interactions should move the spawned object, which writes back to the field properties.

**Original Render Code:**
```cpp
// Translucent ghost of operand B
if (f.children.size() == 2 && f.children[1]) {
    geom::TessMesh ghost = geom::tessellateSdf(*f.children[1], o->getFieldExtent(), 16);
    currentRenderer().setModel(xf);
    currentRenderer().drawOverlay(ghost, glm::vec4(0.35f, 0.85f, 1.0f, 0.16f), 1.0f, true);
    currentRenderer().setModel(glm::mat4(1.0f));
}

// Draggable handle at operand B's offset (gold cube)
glm::vec3 hw = glm::vec3(xf * glm::vec4(o->getFieldOperandBOffset(), 1.0f));
drawHandle(hw, 0.06f, glm::vec4(1.0f, 0.85f, 0.2f, 1.0f), Blend::Alpha);

// Floating blend bead on a screen-aligned rail
if (o->isMorphField()) {
    glm::vec3 rs, rd; float rl = 1.0f;
    blendRail(o, rs, rd, rl);
    glm::vec3 bead = rs + rd * (o->getMorphParam() * rl);
    glm::vec3 rEnd = rs + rd * rl;
    currentRenderer().drawLines({Seg{rs, rEnd}}, glm::vec4(0.55f, 0.55f, 0.6f, 1.0f), 2.0f, Blend::Alpha);
    drawHandle(bead, 0.05f, _blendHandleDragging ? glm::vec4(1.0f, 0.85f, 0.2f, 1.0f) : glm::vec4(0.3f, 0.85f, 1.0f, 1.0f), Blend::Alpha);
}
```

### 2. Polyhedron Vertex Editing (Waterbending)
*From `scratch/GameUpdate.cpp`*

**Goal:** Migrate the screen-space picking and projection of individual vertices out of the update loop. Instead, a tool should emit a targeted modification event on the geometry.

**Original Logic:**
```cpp
// On press: pick the nearest vertex in screen space.
if (mouseLeftNow && !_mouseLeftPressedLast) {
    int best = -1; double bestD = 1e18;
    for (int i = 0; i < obj->getPolyhedronVertexCount(); ++i) {
        glm::vec3 w = glm::vec3(xf * glm::vec4(obj->getPolyhedronVertexLocal(i), 1.0f));
        GLdouble sx, sy, sz;
        if (ecgl::project(w.x, w.y, w.z, mv, pr, vp, &sx, &sy, &sz)) {
            double d = (sx - winX) * (sx - winX) + (sy - winY) * (sy - winY);
            if (d < bestD) { bestD = d; best = i; }
        }
    }
    if (best >= 0 && bestD < 40.0 * 40.0) _morphVertexIndex = best;
}
```

### 3. Face Brush 2D Overlay
*From `scratch/GameRender.cpp`*

**Goal:** Remove `currentRenderer().begin2D(...)` custom brush cursor from the core renderer. If a 2D brush is needed, the `OurVerse` tools should inject UI overlay draws explicitly, not via `Engine::render()`.

**Original Logic:**
```cpp
if (_current3DMode == Mode3D::FaceBrush && _brush.showCursor && _brush.cursorVisible) {
    currentRenderer().begin2D(static_cast<uint32_t>(fbW), static_cast<uint32_t>(fbH));
    float screenX = getCursorX();
    float screenY = getCursorY();
    float cursorSize = _faceBrush.radius * 100.0f * _brush.previewSize;
    // draw circle
    currentRenderer().end2D();
}
```

### 4. Clay Target AABB Outline
*From `scratch/GameRender.cpp`*

**Goal:** Make the targeted object pulse or change material via Law logic rather than hardcoding a yellow wireframe in `GameRender.cpp`.

**Original Logic:**
```cpp
if (_current3DMode == Mode3D::Sculpt && _clayTarget) {
    // computes AABB 
    currentRenderer().drawLines(edges, glm::vec4(1.0f, 0.85f, 0.2f, 1.0f), 2.5f, Blend::Alpha);
}
```

---
*Note: The Preview Object (holographic placeholder during BrushCreate) is also being migrated directly into the Law system during this refactor to ensure it is governed natively.*
