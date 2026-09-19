# Audit: WritePixel Action & Law Authoring Surface in Basic Pixel Changer

**Author:** Gemini Spark (Autonomous Agent)  
**Date:** 2026-09-15  
**Timestamp:** 2026-09-15T20:12:00-07:00  
**Zone Inspected:** `saves/zones/BasicPixelChanger/zone.json` (`basic_pixel_changer.ecform`)  
**Target Law:** `saves/laws/law-basic-pixel-changer/law.json`  
**Subsystems Audited:**
- `src/Singularity/Screen/LawGraphWindow.cpp` (Law authoring & card inspector UI)
- `src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp` (`ActionNode::Kind::WritePixel` execution & trace)
- `src/Singularity/Input/Interaction/InteractionChannel.cpp` (Sense coordinate publisher)
- `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp` (ECA lifecycle, application records, and trace)
**Reference Documentation:** `docs/Analysis/Authorable_Pixel_Writer.md`, `docs/architecture/interaction/INTERACTION_AS_LAW.md`

---

## 1. Executive Summary

When inspecting the **Basic Pixel Changer** zone (`basic-pixel-canvas`) within Earthcall's **Law Authoring Window** (`LawGraphWindow`), selecting the `WritePixel` action node card displays only two disabled descriptive text strings and four bare `pathPicker` buttons:
1. `Face path`
2. `U path`
3. `V path`
4. `Color path`

While the underlying substrate execution engine (`ActionModel.cpp:691–740`) contains rigorous type checking, copy-on-write material isolation (`Object::ownMaterial()`), trace recording (`ActionNode::Trace`), and event publication (`surface-pixel-written`), the **Law Authoring Window** provides zero visual transparency into:
- What each property semantically represents ($U, V$ as continuous normalized coordinates in $[0, 1]^2$ vs. discrete raster texels).
- Which target canvas/surface texture is being addressed and its discrete pixel resolution (e.g. $512 \times 512$).
- The live evaluation of these property paths at runtime (no live readouts of `@interaction-channel.hoveredU/V` or color).
- Why an execution succeeded or failed (failures silently drop into `emitEffect` / `ActionNode::Trace` without surfacing in the UI).
- How color arrives at the canvas (the separate `color-selection-changed` Law decoupled from the click Law).
- How point-writing relates to the broader mathematical domain of writing (OntoMath defined sets and `ElevatePixels`).

This audit analyzes the code behind this opacity, diagnoses the architectural seams, and outlines concrete remediation steps.

---

## 2. Codebase Audit of Current Implementation

### 2.1 The UI Representation: `LawGraphWindow.cpp`
In `src/Singularity/Screen/LawGraphWindow.cpp` (lines 2129–2138), the inspector card for `ActionNode::Kind::WritePixel` is rendered as follows:

```cpp
case ActionNode::Kind::WritePixel: {
    ImGui::TextDisabled("Replace one surface sample through the Screen channel.");
    ImGui::TextDisabled("Every operand is a PropertyPath; no shape or palette meaning is fixed here.");
    if (pathPicker("Face path", node.pixelFacePath)) changed = true;
    if (pathPicker("U path", node.pixelUPath)) changed = true;
    if (pathPicker("V path", node.pixelVPath)) changed = true;
    if (pathPicker("Color path", node.pixelColorPath)) changed = true;
    break;
}
```

#### Deficiencies in the Inspector:
1. **Complete Absence of Semantic Typing & Bounds:**
   - `Face path` accepts any arbitrary `PropertyPath`. The user is not told it must resolve to an integer $\ge 0$ (where $-1$ indicates a raycast miss or untextured element).
   - `U path` and `V path` give no indication that they require floating-point values normalized to $[0.0, 1.0]$, rather than integer pixel indices $(x, y)$ or world-space offsets.
   - `Color path` gives no indication that it must resolve to a `glm::vec3` (RGB), as opposed to a single scalar float, an integer hex value, or a `vec4` (RGBA).
2. **Default Seeding Mismatch (`LawGraphWindow.cpp:1732–1740`):**
   ```cpp
   case ActionNode::Kind::WritePixel:
       if (node.pixelFacePath.empty())
           node.pixelFacePath = PropertyPath::parse("@interaction-channel.hoveredFace");
       if (node.pixelUPath.empty())
           node.pixelUPath = PropertyPath::parse("@interaction-channel.hoveredU");
       if (node.pixelVPath.empty())
           node.pixelVPath = PropertyPath::parse("@interaction-channel.hoveredV");
       if (node.pixelColorPath.empty())
           node.pixelColorPath = PropertyPath::parse("@creation-channel.activeColor");
   ```
   Notice that default seeding targets `@creation-channel.activeColor`. In the actual `BasicPixelChanger` zone, this caused a known regression (documented in `Authorable_Pixel_Writer.md`) where the canvas remained white because the zone had migrated to using the canvas's own `paintColor` property, populated by a separate color picker law. The UI does not reveal which color pipeline is in use.

---

### 2.2 The Execution Substrate: `ActionModel.cpp`
In `src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp` (lines 691–740), `WritePixel` compiles into a runtime closure:

```cpp
case Kind::WritePixel: {
    const PropertyPath facePath = pixelFacePath;
    const PropertyPath uPath = pixelUPath;
    const PropertyPath vPath = pixelVPath;
    const PropertyPath colorPath = pixelColorPath;
    return [facePath, uPath, vPath, colorPath](const ECA::Event&, Singular& subject) {
        const PixelWriteSink& sink = pixelWriteSink();
        if (!sink) {
            emitEffect("WritePixel", false, "no screen pixel channel bound");
            return;
        }

        PropertyValue faceValue, uValue, vValue, colorValue;
        double face = 0.0, u = 0.0, v = 0.0;
        if (!lawGetValue(subject, facePath, faceValue) ||
            !propertyValueToNumber(faceValue, face)) {
            emitEffect("WritePixel", false, "face '" + facePath.toString() + "' does not read");
            return;
        }
        if (!lawGetValue(subject, uPath, uValue) ||
            !propertyValueToNumber(uValue, u)) {
            emitEffect("WritePixel", false, "u '" + uPath.toString() + "' does not read");
            return;
        }
        if (!lawGetValue(subject, vPath, vValue) ||
            !propertyValueToNumber(vValue, v)) {
            emitEffect("WritePixel", false, "v '" + vPath.toString() + "' does not read");
            return;
        }
        if (!lawGetValue(subject, colorPath, colorValue) ||
            !std::holds_alternative<glm::vec3>(colorValue)) {
            emitEffect("WritePixel", false, "color '" + colorPath.toString() + "' does not read a vec3");
            return;
        }

        std::string reason;
        if (!sink(subject, static_cast<int>(face), u, v,
                  std::get<glm::vec3>(colorValue), reason)) {
            emitEffect("WritePixel", false, reason.empty() ? "screen pixel write refused" : reason);
            return;
        }
        Core::EventBus::instance().publish(
            ECA::Event{"surface-pixel-written", &subject, nullptr, std::time(nullptr)});
        emitEffect("WritePixel", true);
    };
}
```

#### Diagnostic Breakdown:
1. **Silent Failure Sink:**
   If any of the four paths fail to evaluate, or if `colorValue` is not a `glm::vec3`, `emitEffect` is called with `written = false` and a detailed string description. However:
   - `LawGraphWindow` never inspects or visualizes `ActionNode::Trace`.
   - The user clicking in the viewport sees nothing happen on the canvas and receives zero error or warning badges in the Law Authoring Window.
2. **Decoupled Sink Registration:**
   The action relies on a global `pixelWriteSink()`. If the Screen channel or renderer has not bound this sink (or if off-screen/headless), it fails silently with `"no screen pixel channel bound"`.
3. **Irreversibility / Temporal Obstacle (`ActionModel.cpp:1552–1554`):**
   ```cpp
   case ActionNode::Kind::WritePixel:
       obstacles.push_back("WritePixel: the surface sample it overwrote is not in the law text");
       return;
   ```
   The engine explicitly recognizes that `WritePixel` mutates dense unmodeled substrate memory. It cannot be inverted or rewound in closed form because the overwritten pixel value is not preserved in the law text. This fundamental limitation is completely hidden from the law author.

---

### 2.3 The Zone Architecture in `BasicPixelChanger`
The `BasicPixelChanger` zone (`saves/zones/BasicPixelChanger/zone.json`) distributes pixel changing across two independent laws:

1. **The Palette Law (`law-color-picker-apply`):**
   - **Trigger:** `color-selection-changed` on the UI color picker.
   - **Action:** `Set` property `paintColor` on the `basic-pixel-canvas` object.
2. **The Canvas Click Law (`law-basic-pixel-changer`):**
   - **Trigger:** `object-clicked`.
   - **Condition:** `IsKind` / Identity check matching `basic-pixel-canvas`.
   - **Action:** `WritePixel` reading:
     - `@interaction-channel.hoveredFace`
     - `@interaction-channel.hoveredU`
     - `@interaction-channel.hoveredV`
     - `paintColor` (read from the subject canvas).

When opening the Law Authoring Window for `law-basic-pixel-changer`, the author only sees the click law. Because `paintColor` is set by an entirely separate law responding to a different event, the authoring window makes it appear as though `paintColor` is an opaque, static field.

---

## 3. The Five Core Reasons the Surface Feels Opaque

| # | Opacity Factor | Root Cause in Code | Substrate Reality |
|---|---|---|---|
| 1 | **Unit & Range Ambiguity** | `LawGraphWindow.cpp:2133–2135` treats $U$ and $V$ as generic strings. | $U, V$ are normalized floats in $[0, 1]$ generated by raycast UV intersection in `InteractionChannel`. They are NOT integer texels $(x, y)$. |
| 2 | **Invisible Target & Resolution** | `ActionNode` carries no target field; target is inherited from `Law::applyTo(subject)`. | Writing requires `subject` to be a `Shape2D` or textured 3D mesh with an owned `Material` texture. Canvas dimensions ($W \times H$) are not displayed. |
| 3 | **Zero Live Readout / Probing** | `pathPicker()` only displays static text or a selection modal; does not evaluate paths live against `g.testSubject`. | The author cannot tell if clicking or hovering is actively producing non-zero $U, V$ or valid face IDs. |
| 4 | **Swallowed Failure Reasons** | `ActionModel.cpp:700–733` calls `emitEffect(..., false, reason)`, but `LawGraphWindow` never renders trace logs. | Misses (face == -1), out-of-bounds UVs, or color type mismatches result in dead clicks with no feedback. |
| 5 | **Missing Domain-of-Writing Seam** | `WritePixel` is a solitary point-write act; it does not visually connect to `ElevatePixels` (Kind 22) or OntoMath defined sets. | Zach's stated design intent is that the *domain* of writing should be authored (OntoMath region $D = \{(u,v) \mid \varphi(u,v) \text{ defined}\}$), which elevates pixels into enumerable properties. |

---

## 4. Remediation & Ergonomics Roadmap

To make `WritePixel` clear, expressive, and transparent in the Law Authoring Window without violating Refusal #7 (No new C++ classes for domain nouns):

### 4.1 Enhanced Inspector UI in `LawGraphWindow.cpp`
Replace the 8-line bare block in `LawGraphWindow.cpp` with a structured property inspector:
1. **Explicit Field Captions with Expected Types & Ranges:**
   - **Face Index:** `int` (Face struck; `0` for 2D quad, $\ge 0$ for 3D meshes, `-1` = miss/refused).
   - **Normalized U Coordinate:** `float` in $[0.0, 1.0]$ (Local horizontal texture ratio).
   - **Normalized V Coordinate:** `float` in $[0.0, 1.0]$ (Local vertical texture ratio).
   - **Source Color:** `vec3` (RGB color vector, e.g. from canvas property or palette).
2. **Live Runtime Probe Badges:**
   When inspecting an active session where `g.testSubject` or an active selection exists:
   - Evaluate `lawGetValue(*g.testSubject, node.pixelUPath, ...)` and display the live value beside the button (e.g. `[U: 0.452] [V: 0.811]`).
   - Show a small color swatch next to `Color path` evaluating the current color.
3. **Target Texture Context Header:**
   Query the target's `Material`:
   - Display target texture resolution: `Target Texture: basic-pixel-canvas (512 x 512 texels, RGBA8)`.
   - Display effective discrete target texel: `Texel: (x: 231, y: 415)`.
4. **Trace & Execution Status Indicator:**
   - Read `Law::applicationLog()` or `g.actionFeed`. If `WritePixel` fired in the last 2 seconds, display a status pill:
     - `🟢 Last Write: Success (Texel modified, surface-pixel-written published)`
     - `🔴 Last Write: Failed ("color 'paintColor' does not read a vec3")`
5. **Bridge to `ElevatePixels`:**
   Add an informational callout: *"For multi-texel selections, mathematical brush regions, or persisted pixel properties, see Action: ElevatePixels."*

---

## 5. Conclusion

The opacity experienced in the Law Authoring Window for `WritePixel` is a presentation gap, not a substrate failure. The engine underneath is executing exact, verified math, copy-on-write material isolation, and event publication. By upgrading `LawGraphWindow.cpp` to surface semantic bounds, live evaluated path probes, canvas texture resolution, and execution trace feedback, the Law Authoring Window will become fully legible to the author.
