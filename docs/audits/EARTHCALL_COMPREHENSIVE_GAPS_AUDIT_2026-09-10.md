# Comprehensive Audit of Gaps Across Earthcall

**Author:** Antigravity (Gemini 3.8 Flash)  
**Session ID:** `59058917-39c1-4f32-8b8a-70ee25823356`  
**Date:** 2026-09-10  
**Timestamp:** 2026-09-10T23:33:00-07:00  
**Status:** Complete Architectural & Functional Gap Audit  

---

## Executive Summary

This audit performs a full-spectrum inspection of the Earthcall repository across its core ontology, law engine, runtime substrates, persistence layer, modality channels, and authoring surfaces. It addresses Zach's explicit mandates, open critical items in `To-do list.md`, and technical findings from recent passes.

The gaps fall into **seven primary domains**:
1. **Ontological Purity & Refusal Violations** (Person modeled as Object, Categories as Objects, hardcoded physics easter eggs)
2. **First Movers, Model Context Protocol (MCP) & Authorship Governance** (unauthenticated tool execution without First Mover bounds)
3. **Singular & Set-to-Set Creation Polymorphism** (`Create` hardcoded to `Object`, missing Singular set-to-set window)
4. **Law Engine, Prophetic Rete & Formal Logic** (undefined execution order, missing `NOT` operator, unshared invalidation ledger, unbounded creation)
5. **Terminal as Pure Universal Substrate & Lexeme Foundation** (standalone CLI vs. first-class modality, linguistic expression vs. use collapse)
6. **Persistence & Split-Substrate Architecture** (per-Zone independent serialization vs. monolithic world saves, legacy duplicate ID resolution)
7. **Person Interface & Sensory Feedback** (Retina Mac 2D pick/draw coordinate mismatch, Creator Console tool regressions, unintegrated closed-form undo)

---

## 1. Ontological Integrity & The Seven Refusals

### 1.1 Critical Refusal #5 Violation: Person Modeled as an In-World Object
- **Finding:** In multiple live save files—including `saves/worlds/basic_pixel_changer.json`, `far_lands.ecform`, `synthesis_studio_living.json`, `basic_2d_button_zone.json`—an `Object` exists with `objectID: "Zach"` and `displayName: "Zachary Zhang"`.
- **Root Cause:** Generator scripts (`scripts/author_synthesis_studio.py:325` and `scripts/author_far_lands.py:250`) executed `category_being("Zach", "Zachary Zhang")`. Because `CategoryManager::create` constructs `std::make_shared<Object>(name)` with `shapeKind: 12` (extra-spatial), this minted an `Object` entity named `"Zach"`.
- **Ontological Conflict:** Refusal #5 states: *"Person means Human. A Person strictly represents an actual human being interacting with Earthcall. AI agents or generative models are not Persons... Never model an AI as a Person [and never model a Person as an Object]."* Zach raised this with urgency: `"CRITICAL: WHY IS TTHERE AN "Object" CALLED "Zach"?!?!?!? PERSON IS NOT OBJECTTTTTTTTTT - Zach"`.
- **Broader Defect:** `CategoryManager` implements categories as `Object` pointers (`src/ConstructedBeing/CategoryManager.cpp:12`). Under `AUTHORED_CATEGORIES.md`, categories are rooted acyclic Formations of beings, not raw `Object` instances.

### 1.2 Critical Refusal #7 Violation: Hardcoded C++ F7 Easter Egg & Rigid Physics
- **Finding:** Pressing `F7` invokes `_keyboardHandler->bindKey(GLFW_KEY_F7, "toggle_gravity_field", ...)` in `src/Singularity/Core/EngineInit.cpp:476`, toggling or dynamically instantiating a hardcoded C++ `Physics::PhysicsLaw` (LawType `GravityField`) that pulls all objects together.
- **Ontological Conflict:** Refusal #7 states: *"No new methods to define variable behavior: The order of behavior, representation, and resource allocation depend on Person-authored Laws, represented by data."* Zach observed: `"LMAOOOOO WHAT I ACCIDENTALLY PRESSED f7 INSTEAD OF f8 AND SUDDENLY EVYERHITNG STARTED GETTING PULLED TOWARD EACH OTHER... THAT IS SO COOL BUT WE GOTTA MAKE IT A FIRST MOVER LAW NOT A RANDOM EASTER EGG"`.
- **The Gap:** Physics behaviors (gravity fields, collision detection, impulses) remain hardcoded C++ methods rather than decoupled, toggleable First Mover Laws with registered governable properties.

### 1.3 Refusal #6 Edge: Object Rotation Lossy Exemption
- **Finding:** `tests/singularity/no_black_box_test.cpp` maintains `Object::rotation` under `kWriteExemptions` because Euler angle conversion round-trips lossily through transform matrices.
- **The Gap:** A non-lossy representation or dual representation (quaternion/matrix-native property) is required to retire the exemption.

---

## 2. First Movers, MCP Protocol & Governance Boundaries

### 2.1 The Model Context Protocol (MCP) Server Lacks First Mover Authorization
- **Finding:** The newly landed MCP server (`scripts/mcp-server.js` and `src/Singularity/Foreign/mcp/earthcall-mcp-server.js`) allows connected external AI models to call 17 tools (including `earthcall_write_property`, `earthcall_spawn_object`, `earthcall_author_law`, and `earthcall_delete_object`) without passing through Earthcall's `Identity/` First Mover ledger.
- **Zach's Explicit Directive:**
  > *"CRITICAL: Ensure the MCP protocol abides by Earthcalls First Mover and authorship-owner-stakeholder framework. All LLMs musts be registered First Movers and the scope of their permitted actions, answered by the ontology itself (e.g. Person answers on whose behalf, Singulars answers on what they may act, Moment answes when, Zone/Home answers where agents may act, Law answers how, Lexeme and Hierarchy of Joys answers why, Relation/Formation answers with) (the tool should reject if it goes outside of the Person-authored First mover bounds). - Zach"*
- **The Gap:** The MCP bridge currently accepts anonymous commands over WebSocket and injects actions into the live engine without validating the caller's registered First Mover standing, without an explicit Person stakeholder authorization, and without bounding mutations to authorized Zones or Singulars.

### 2.2 First Mover Model Attribution Fragmentation
- `saves/worlds/` still contains anonymous or omnibus signatures (`studio.author.codex`) conflating multiple models (Sol, Terra, Luna, Astra).
- Standing for autonomous agents (e.g. Jules) in `Identity/` remains incomplete.

---

## 3. Singular & Set-to-Set Creation Polymorphism

### 3.1 `ActionNode::Kind::Create` is Mono-Typed to `Object`
- **Finding:** In `src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp:1000-1068`, `ActionNode::Kind::Create` (Kind 11) exclusively instantiates `std::make_unique<Object>()` and registers it via `world->addObject()`.
- **The Gap:** It cannot create any other kind of `Singular` (e.g., `Lexeme`, `Formation`, `Law`, `Relation`, `Zone`). This forced the ad-hoc introduction of separate action kinds like `AuthorZone` (Kind 19) and `AddRelation` (Kind 20).
- **Required Architecture:** Per Zach's directive:
  > *"Singular set to set creation must be able to create every kind of Singular, so we stop having to invent new ActionKinds or op codes for every individual Singular. Creating a new Law via Laws should use ActionNode create Singular (and then select Singular kind based on all the classes that inherit Singular), or use the set to set Creation node, which must be designed to ask the same thing."*
  > *"Retire ObjectConcept into SingularConcept, and SingularConcepts should literally just be Singulars that others branch off of."*

### 3.2 Person-Facing Set-to-Set Creation Window Missing
- While underlying set-to-set replication exists headlessly (`Singular_and_Object_Set_to_Set_Creation`), there is currently no dedicated, intuitive visual or terminal authoring tool for Persons to compose set-to-set mappings directly.

---

## 4. Law Engine, Prophetic Rete & Formal Logic

### 4.1 Undefined Law Execution Order
- Two laws firing in the same tick that write to the same property resolve non-deterministically (whichever order Rete drains its agenda).
- No Person-authored conflict resolution or prioritization framework exists. If no default is authored in a Zone, conflicting coinciding laws should refuse to fire rather than race.

### 4.2 Missing Formal Logic `NOT` Operator
- `ConditionModel` supports `Kind::AllOf` (1) and `Kind::AnyOf` (2), but lacks a first-class `Not` node (`Kind::Not`).
- Zach's directive: *"Add NOT operator for condition nodes and action nodes if we don't have that already. We want everything in formal logic. - Zach"*.

### 4.3 Silent Law Deafness & Shared Invalidation Ledger
- As proven in `docs/Analysis/DERIVED_STATE_AND_THE_SILENCE_OF_LAWS_2026-09-10.md`, eleven derived structures in the law engine lack a unified invalidation ledger.
- A relation change or Zone swap does not broadcast a relation-revision signal, blocking Formation Rete Rung 4 and leaving category-scoped laws computationally expensive or silently deaf.

### 4.4 Unbounded Creation & Loop Budgets
- `kMaxBirthsPerTick` does not exist: a `Create` action inside a `WhileTrue` loop can mint infinite beings per tick.
- `maxChainRounds` is settable and serialized, but unclamped upon deserialization (`Law.cpp:2360`).

---

## 5. Pure Terminal Substrate & Lexeme Foundation

### 5.1 Terminal Mode as a True Universal Substrate
- **Zach's Critical Directive:**
  > *"CRITICAL: Ensure Earthcall is fully runnable as a pure terminal program--i.e., Earthcall with Terminal as the substrate. This is one of the simplest and most accessible ways we can try to model the entire ontology at once once the foundational models are complete, and one of the best ways to ensure my prototype is a truly universal substrate, to refine away all the edges that lock it to only one Singularity form. - Zach"*
- **The Gap:** `src/terminal_entry.cpp` currently exists as an auxiliary CLI tool with a custom REPL. It is not yet wired as a universal Singularity Sense-Act substrate channel capable of driving the full engine loop, headless Law execution, and live world state without any graphical dependencies.

### 5.2 Lexeme Depth & Linguistic Binding
- Current `Lexeme` beings carry only `symbol` and `conceptualWeightValue`.
- As proven in the 2026-09-10 language audit (`LANGUAGE_CONTEXT_AND_SEMANTIC_BINDING_2026-09-10.md`), the live language ingress collapses the distinctions between an expression, a token occurrence, and its interpretations. Identical spellings overwrite each other in `_symbolIndex`, and `Utterance` remains detached from the live path.

---

## 6. Persistence, Serialization & Split Substrate

### 6.1 Per-Zone Serialization Autonomy (Phases 2 & 4)
- Although split-substrate serialization (`.ecform` text + `.ecmatter` FlatBuffers) and content-addressed generation commits landed, Earthcall still primarily saves and loads monolithic session worlds from the Creator Console rather than allowing individual Zones to be saved, loaded, and distributed autonomously.
- Basic Pixel Changer demonstrated the first Zone-scoped Law activation, but generalized materials, categories, and isolated Zone export remain open.

### 6.2 Duplicate Bare Object IDs in Legacy Matter Buffers
- While composite `(owner, id)` resolution stopped duplicate bare IDs from silently overwriting semantic paint, legacy save files still contain duplicate IDs that require clean migration.

---

## 7. Person Interface & Sensory Surface Gaps

### 7.1 HiDPI / Retina Coordinate Space Discrepancy
- On Retina displays, 2D screen-space picking coordinates diverge from WebGPU framebuffer drawing coordinates (`SYNTHESIS_STUDIO_AUDIT_2026-09-02.md` §A0).
- This renders 2D HUD buttons and sliders unclickable under native macOS scaling.

### 7.2 Creator Console Tool Regressions
- **Pottery Tool:** Enlarging 3D geometry stretches the existing `FaceTexture` pixels instead of dynamically expanding texture resolution or adjusting UV density.
- **Rotate Tool:** Selecting Rotate in the 3D tool mode does not visibly rotate via the angle sliders on the shape; only the bottom "Target Rotation" sliders work.
- **Fuse Objects Tool:** Lacks clear visual feedback or transparent failure reporting when fusing shapes.

### 7.3 Unbound Closed-Form Undo ($Cmd+Z / Cmd+Y$)
- The mathematics framework specifies closed-form temporal reversibility (`ONTOMATH_FRAMEWORK.md` §6), but $Cmd+Z$ and $Cmd+Y$ remain unbound.

---

## Recommended Action Roadmap

1. **Immediate Purity Remediation:**
   - Erase the `"Zach"` object from save files (`basic_pixel_changer.json`, `far_lands.ecform`, etc.) and fix generator scripts.
   - Refactor `CategoryManager` so categories are rooted `Formation`s rather than `Object` instances.
   - Convert the hardcoded `F7` keybinding into a Person-authored First Mover Law (`physics-attraction`).
2. **First Mover MCP Enforcement:**
   - Introduce First Mover authentication and bounds validation in `earthcall-mcp-server.js` and `WebSocketServer.cpp` (verifying Person authorization, target Singular, Zone, and Law permissions).
3. **Set-to-Set Polymorphism & Formal Logic:**
   - Generalize `ActionNode::Kind::Create` to support any `Singular` kind (`CreateSingular`).
   - Add `ConditionNode::Kind::Not` for formal logic completeness.
4. **Terminal Substrate:**
   - Wire `Singularity/Screen/TerminalChannel` to allow Earthcall to run as a pure terminal program without graphics libraries.
5. **HiDPI 2D Picking Fix:**
   - Unify coordinate spaces between window points and framebuffer pixels for 2D picking on Retina displays.
