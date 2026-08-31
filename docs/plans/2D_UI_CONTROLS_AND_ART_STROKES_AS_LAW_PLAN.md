# Implementation Plan: 2D UI Controls and Art Strokes as Law

**Date:** 2026-08-31
**Author:** Gemini Spark
**Session:** 2026-08-31-Earthcall-2D-UI-and-Art-Strokes
**Requested by:** Zach — *"What’s left to do in Earthcall before Laws can create new Singulars (2D visual blocks) and then make it so that if that block is clicked then some action happens? This is how Earthcall will create graphic interfaces and widgets like buttons. And also Laws that give art tool functionality like drawing Singulars/Objects on the screen as like strokes or something. And then different laws imbue those strokes/visual stuff with behavior. Is all the Sense/Act machinery already available? If so then it’s just a matter of actually creating those laws... Awesome, write your plan inside the docs/ folder and proceed"*

---

## 1. Executive Summary & Ontological Foundation

In Earthcall, graphical user interfaces (buttons, toggles, panels, sliders) and digital art strokes are **not** separate C++ engine subsystems, classes, or retained widgets (Refusal #1, #2, #7). 

- **A UI Widget** is an `Object` with geometry and materials in an authored category (`category.control.button`), governed by decoupled Laws (`object-clicked` → `control-activated` → domain action).
- **A Drawn Stroke** is a `Formation` of `Singular`s (or a parametric patch `Object`) in an authored category (`category.art.stroke`), imbued with physics, acoustics, and interactive behaviors via independent Laws.

This plan details the implementation to bridge the remaining engine gaps in `ActionModel` and `InteractionChannel`, author the required Law archetypes, and verify the full Sense/Act chain through automated regression tests.

---

## 2. Component Breakdown & Seams

### Seam 1: Dynamic Relation Wiring — `ActionNode::Kind::AddRelation` (Kind 20)
**Files:** `src/ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp`, `src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp`
- **Objective:** Enable runtime Law actions (including child actions inside `ActionNode::Create`) to dynamically wire a first-class `Relation` (e.g. `(newborn) --[instance-of]--> category.control.button` or `(newborn) --[instance-of]--> category.art.stroke`) into the active `Zone`'s relation graph.
- **Append-only Enum:** `ActionNode::Kind::AddRelation = 20`.
- **Fields:** `containerToken` (source participant), `elementToken` (target participant), `propertyName` (relation type tag, e.g. `"instance-of"`).
- **Behavior:** Resolves endpoints via `resolveBeingToken`, verifies active `Zone`, ensures idempotent insertion, attaches author provenance from `Universe::currentAuthor()`, records outcome in `ActionNode::Trace`, and publishes `relation-created` on `EventBus`.

### Seam 2: Interaction Channel Threshold Calibration & 2D Picking
**Files:** `src/Singularity/Input/Interaction/InteractionChannel.hpp`, `src/Singularity/Input/Interaction/InteractionChannel.cpp`
- **Objective:** Prevent minor pointer travel during trackpad clicks from erroneously triggering drag classification (`object-drag-started`/`object-drag-ended`) instead of clicks (`object-clicked`), and expose the threshold as a registered property (Refusal #6).
- **Changes:**
  - Replace `static constexpr float kClickSlopPixels = 6.0f` with member `float clickSlopPixels = 12.0f;`.
  - Register `clickSlopPixels` under `InteractionChannel::buildProperties()` with read/write access.
  - Verify raycasting and picking for planar/2D object shapes (`ShapeKind::Plane`, `ShapeKind::Patch`).

### Seam 3: Archetype & Consequence Control Laws
**Files:** `src/Singularity/Input/Interaction/ControlPatterns.hpp`, `src/Singularity/Input/Interaction/ControlPatterns.cpp`, `saves/`
- **Objective:** Seed the fundamental archetype control patterns:
  - `law-control-button-archetype`: `When object-clicked` on `Related(instance-of, category.control.button)` → `Publish control-activated`.
  - Provide helper factories for authoring domain-specific buttons that bind to `control-activated`.

### Seam 4: Art Tool Stroke Generation & Behavior Imbuement
**Files:** `src/Singularity/Input/Interaction/ControlPatterns.hpp`, `src/Singularity/Input/Interaction/ControlPatterns.cpp`
- **Objective:** Author First-Mover / Person laws for interactive drawing:
  - **Stroke Generation Law:** Senses drag state while Draw mode is active (`@interaction-channel.leftDown && @creation-channel.activeMode == "Draw"`), mints stroke `Singular`s along `@interaction-channel.pointerWorld`, and adds an `instance-of` relation to `category.art.stroke`.
  - **Behavior Laws:**
    - Stroke Acoustic Reaction: When `object-hover-entered` on `Related(instance-of, category.art.stroke)` → `PlayAudio`.
    - Stroke Illumination: `WhileTrue` with `@world.pointerOver` → `Map color`.

---

## 3. Implementation Steps & Verification Checklist

- [ ] **Step 1: Implement `ActionNode::Kind::AddRelation` (Kind 20)**
  - Update `ActionModel.hpp` enum and factory.
  - Update `ActionModel.cpp` serialization (`toJson`/`fromJson`), compilation (`compile()`), trace reporting, and event publishing.
- [ ] **Step 2: Register `clickSlopPixels` on `InteractionChannel`**
  - Update `InteractionChannel.hpp` and `InteractionChannel.cpp`.
- [ ] **Step 3: Add `tests/law/add_relation_action_test.cpp`**
  - Verify `ActionNode::addRelation` creates active `Relation`s in the `Zone` and asserts Rete condition facts.
- [ ] **Step 4: Add `tests/singularity/ui_button_law_test.cpp`**
  - Verify end-to-end: Button Spawn Law → `object-clicked` → `control-activated` → domain mutation.
- [ ] **Step 5: Add `tests/tools/stroke_drawing_law_test.cpp`**
  - Verify stroke Singular generation along pointer trajectory and behavior imbuement via category Laws.
- [ ] **Step 6: Build & Test Suite Verification**
  - Run full test suite (`ctest`) and ensure 100% pass rate with 0 regressions.
