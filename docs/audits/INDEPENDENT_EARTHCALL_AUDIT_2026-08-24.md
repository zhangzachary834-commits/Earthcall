# Independent Architectural and Codebase Audit of Earthcall

**Author:** Antigravity (Gemini 3.7 Flash)  
**Session ID:** `aa7f3fac-a3dd-401f-bbab-463c53ad8e3a`  
**Date:** 2026-08-24  
**Timestamp:** 2026-08-24T13:46:00-07:00  
**Scope:** Full-tree architectural, ontological, computational, and technical audit formed from first-principles inspection of the codebase, build system, test harness, and source implementations without consulting prior audits or reviews.

---

## 1. Executive Summary & Core Verdict

Earthcall is a fundamentally novel software architecture: a **Person-centered ontological substrate** where the engine serves as a modality vessel rather than the order of truth. Unlike conventional virtual environments, games, or simulation engines where types, schemas, and mechanics are hardcoded in C++ domain classes, Earthcall enforces that **domain reality is authored in-world as data** (via `Singular`, `Relation`, `Formation`, `Lexeme`, `ObjectConcept`, and `Law`), while C++ is strictly constrained to irreducible modality channels (`Singularity/`), kernel guards, and invariant ontological anchors (`Person`, `Soul`, `BodyPart`).

### High-Level Status:
- **Build Status:** Clean build (`cmake --build build -j8`).
- **Test Suite:** **62 of 63 tests pass** (100% of implemented tests passing). Test #50 (`webgpu_particle_test`) is explicitly marked as `PENDING_FEATURE_TESTS` and correctly skipped as an unbuilt feature specification. `chess_app_test` passed cleanly in 2.89s.
- **Ontological Coherence:** Extremely high adherence to the Six Refusals. The property-path reflection system (`No Black Box`), append-only enum discipline, cryptographic identity root (`Ed25519` / `SingularId`), and symbolic algebra (`OntoMath`) are genuinely operational in code.
- **Architectural Debt & Tensions:** A small set of localized structural tensions remain:
  1. `src/Time/` sits as an unratified top-level directory containing an empty `Duration/` folder and an empty `Time.h` placeholder class.
  2. `class Body : public Object` in `src/Person/Body/Body.hpp` still inherits `Object` (violating Refusal #4, where Body is reserved exclusively as a Person vessel).
  3. `class EventEntity : public Singular` (`src/Singularity/Core/EventEntity.hpp`) represents ontological class-proliferation debt for custom events.
  4. `Ourverse` retains residual engine-bag members (`cameraPos`, `ownedObjects`).
  5. `Singularity/FirstMoverOntology/Legacy/DesignSystem` holds ~83 KB of legacy direct-manipulation / text editing code that should be retired or migrated to law-based interaction.

---

## 2. Ontological Architecture & The Six Refusals Compliance Analysis

| Refusal | Rule | Audit Findings & Code Evidence | Verdict |
|---|---|---|---|
| **Refusal 1: No new C++ class for domain nouns** | Domain nouns (`Robot`, `Vehicle`, `Tree`, `Category`) must be authored in-world, never carved into C++. *(Exception: Human form / `BodyPart`)* | `CategoryManager` manages categories as authored extra-spatial `Object` instances. Concepts are captured via `ObjectConcept`. No domain noun classes exist. Limbs (`Arm`, `Finger`, `Torso`) fall within the human form exception. Minor debt: `EventEntity` (`src/Singularity/Core/EventEntity.hpp`) acts as a dedicated C++ class for events. | **PASS (with minor debt)** |
| **Refusal 2: No new top-level directory for subsystems** | Top level must strictly reflect the ontology (`ConstructedBeing`, `Person`, `Relation`, `ZonesOfEarth`, `Identity`, `Singularity`). | Modality channels are cleanly nested under `Singularity/` (Core, Audio, Language, Network, Physical, OntoMath, Foreign, Input, Screen, Storage, FirstMoverOntology). **Defect:** `src/Time/` sits at the top level of `src/` without formal ratification in `DIRECTORY_ORDERING.md`. | **WARNING** |
| **Refusal 3: No new enum value for kinds of things** | Enums (`ConditionNode::Kind`, `ActionNode::Kind`, `BeingKind`, `Moment::Kind`, `TransFactor::Kind`) are strictly append-only and serialized as integers. Burned values must never be reused. | Verified in `ConditionModel.hpp` (kinds 12/13 burned for pair quantifiers; `BeingKind::World = 6` burned), `ActionModel.hpp` (kinds 0–19 sequentially assigned and serialized), and `SingularId.hpp`. | **PASS** |
| **Refusal 4: `Body` is reserved for Persons** | A `Body` represents an embodied someone; objects have visual components, never a `Body`. | Only `Person` instances instantiate `Body`. However, in `src/Person/Body/Body.hpp:13`, `class Body : public Object` inherits `Object` instead of directly inheriting `Singular`. | **FAIL (Inheritance Debt)** |
| **Refusal 5: `Person` means Human** | `Person` strictly models human beings. AI models and bots are First Movers or Objects. | `Person.hpp` models the human vessel, linking `Soul`, `Body`, `Formation _joys`, and cryptographic `SingularId` (Ed25519 public key `did:earthcall:...`). AI agents operate as First Movers or in-world Objects. | **PASS** |
| **Refusal 6: No Black Box** | Every field is registered via a `PropertyPath` governed by `TransferPolicy` (Kernel, Governable, Gated). Ungoverned state is forbidden; beneath-the-Kernel state must be explicitly documented. | Verified by `no_black_box_test.cpp` and widespread use of `PropertyRef` / `ComputedProperty`. Sealed registers and beneath-the-kernel hardware handles are documented with explicit rationale comments. | **PASS** |

---

## 3. Substrate & Modality Layer (`Singularity/`)

The modality layer (`src/Singularity/`) cleanly separates machine mechanics from world ontology:

1. **`Singularity/Core` (`Engine`, `EventBus`, `CreationChannel`):**
   - Central tick orchestration (`tick()`, `update()`, `render()`) is properly decoupled.
   - `EventBus` provides low-latency dispatch and echo filtering (e.g. `law-applied` echo only publishes if `Universe::anyoneHears("law-applied")`).
   - `CreationChannel` implements first-mover developer spawning cleanly behind governable property paths (`@creation-channel.enabled`, `active3DMode`).

2. **`Singularity/OntoMath` (`ScalarForm`, `Term`, `TransFactor`, `Piecewise`, `CurveModel`):**
   - Outstanding implementation of exact symbolic mathematics.
   - Terms are stored in canonical form (`std::map<std::string, double>` factors and `std::vector<TransFactor>` sorted transcendental functions).
   - Symbolic calculus is closed: differentiation and integration for polynomial, signomial, exponential, logarithmic, and trigonometric functions are exact.
   - Bounded recursion (max depth 32) prevents computational blowups.

3. **`Singularity/Screen` (Renderer, WebGPU, GL, BrushSystem, Window Tools):**
   - Unified geometry representation (`geom::SdfNode` backed by `OntoMath::MathNode`).
   - Shader generation (`SdfWgsl.cpp`) translates exact symbolic math nodes directly into WGSL compute/fragment shaders.
   - Full decoupling: `Material` is an ontological being; `RenderMaterial` is a lightweight GPU bridge.

4. **`Singularity/Audio` (`AudioSystem`):**
   - Implements procedural sound synthesis driven by OntoMath fields and Law properties (`ActionNode::PlayAudio`).
   - Strict adherence to kernel safety: `infrasound_floor_test` verifies that sub-audible frequencies (<20 Hz) directed at a Person's body trigger a loud refusal rather than silent filtering or unauthorized physical excitation.

5. **`Singularity/Storage` (`SaveSystem`, `CloudStorage`, `BinaryPack`, `Frontier`):**
   - High modularity with per-Zone (`saves/zones/<id>/zone.json`) and per-Home (`saves/homes/<id>/home.json`) storage separation.
   - Fallback protection: `saves/backups/before-load.json` preserves unsaved state.

---

## 4. Law, ECA Engine, & Authorship System

The Law system in `src/ZonesOfEarth/AuthorsOfLaw/` represents the central operational engine of Earthcall:

1. **AST Representation vs. Closures:**
   - Laws are stored as data trees (`ConditionNode`, `ActionNode`), allowing full serialization, authoring in-world, Rete network compilation, and change introspection.
   - Compiled ECA closures are derived on demand.

2. **Continuous Monitoring vs. Event Edges:**
   - `Activation::OnEvent` handles discrete transitions.
   - `Activation::WhileTrue` handles continuous per-tick monitoring.
   - `Activation::OnBecomeTrue` handles edge transitions (false $\to$ true).

3. **Rete Compilation:**
   - `ReteNetwork` (`Law.cpp`) compiles condition trees into Alpha (single-predicate) and Beta (relational join) memories.
   - Prevents redundant $O(N \times M)$ evaluations across ticks.

4. **Change Recording & Auditability:**
   - `ChangeRecorder` and `LawAuditLogger` log every mutation with precise authorship attribution (`StakeholderRecord`).
   - `ActionNode::Trace` provides fine-grained reporting of whether each action node actually landed a write (`wrote == true`), preventing zombie drive sessions.

---

## 5. Being Model: Singular, Object, Lexeme, Formation, & Relation

1. **`Singular` (`ConstructedBeing/Singular/`):**
   - Universal base class for everything participating in Relations and Laws.
   - Property registration via lazy `buildProperties()` prevents double registration.
   - Dynamic authored properties (`DynamicPropertyBridge`) preserve type integrity and prevent silent type coercion bugs.

2. **`Lexeme` (`Singular/Lexeme/`):**
   - Linguistic and symbolic units are first-class Singulars with an exact `conceptualWeight` (stored as `PropertyValue` supporting OntoMath fields).
   - Allows linguistic reasoning and telos to be governed by mathematical laws.

3. **`Relation` & `Formation` (`src/Relation/`):**
   - Relations are first-class beings holding non-owning pointers (`_a`, `_b`) and serialized by stable identifiers.
   - Formations provide the dual role: an unrooted Formation is a mathematical set; a rooted Formation is an authored category.
   - Deep-copy semantics on copy construction prevent subformation aliasing.

---

## 6. Person, Soul, Body, Identity, and Governance

1. **Person Identity (`Identity/SingularId`):**
   - Cryptographic identity rooted in 32-byte Ed25519 public keys (`did:earthcall:<base32>`).
   - Clear distinction between **who a Person is** (unforgeable cryptographic key) and **what a Person is called** (mutable `Lexeme` display name).

2. **Soul & Body:**
   - `Soul` is correctly bound to `Person` (not a separate independent entity; shares the Person's identifier).
   - `Body` contains `BodyPart` limbs (`Arm`, `Finger`, `Torso`, etc.), correctly representing the invariant human form.

---

## 7. Spatial & Social Architecture: Zone, Home, Gathering, and Ourverse

1. **`Zone` & `Home` (`src/ZonesOfEarth/`):**
   - `Zone` acts as the spatial womb for Objects and Formations.
   - `Home` (`class Home : public Zone`) models dwellings with kernel-enforced governance bits (`entryRequiresWill`, `cannotForceStay`, primary home transfer locks).

2. **`Ourverse` (`src/ZonesOfEarth/Ourverse/`):**
   - Modeled as the vessel of unity: maintains the unowned gathering Zone, mutual filaments between Zones, shared Joys, and first-mover metalaws.

---

## 8. Specific Architectural Tensions, Debts, & Inconsistencies

During our first-principles code audit, the following concrete issues were identified:

### Issue A: `src/Time/` Top-Level Directory Violation & Incomplete Metaphysics
- **Location:** `src/Time/Time.h`, `src/Time/Duration/`, `src/Time/Moment/`
- **Finding:** `src/Time/` was introduced as a top-level directory in `src/`. However, `DIRECTORY_ORDERING.md` specifies that the top level consists exclusively of `ConstructedBeing`, `Person`, `Relation`, `ZonesOfEarth`, `Identity`, and `Singularity`.
- **Detail:** `Time.h` contains only a stub class with a placeholder comment (`"// Placeholder for a first-order vessel of Time in Earthcall"`), `Duration/` is completely empty, and `Moment.hpp` declares `class Moment : public Singular`.
- **Resolution:** Either formally amend `DIRECTORY_ORDERING.md` to establish `Time` as a top-level ontological pillar, or relocate `Moment` into `ConstructedBeing/Singular/Moment/` or `ZonesOfEarth/` / `Singularity/OntoMath/`.

### Issue B: `Body` Inherits `Object` (Refusal #4 Violation)
- **Location:** `src/Person/Body/Body.hpp:13`
- **Finding:** `class Body : public Object`
- **Detail:** `AGENTS.md` Refusal #4 and `EarthcallOurverse.md` explicitly mandate that `Body` is reserved for Persons, while `Object` is for visual/extra-visual domain entities. Having `Body` inherit `Object` forces `Body` to inherit Object's spatial/geometric fields, creating a category confusion between an embodied vessel and an object.
- **Resolution:** Refactor `Body` to inherit `Singular` directly, holding visual components/geometry as associated members or formations.

### Issue C: `EventEntity` as a Domain Noun Class (Refusal #1 Violation)
- **Location:** `src/Singularity/Core/EventEntity.hpp` and `.cpp`
- **Finding:** `class EventEntity : public Singular`
- **Detail:** Creating a dedicated C++ class for custom events violates Refusal #1 ("No new C++ class for a domain noun"). Events should be represented either as runtime `ECA::Event` payloads or as authored `Relation` instances connecting source, target, and a `Moment`.
- **Resolution:** Deprecate and remove `EventEntity`, routing custom events through `Relation` graphs.

### Issue D: Residual Engine-Bag State in `Ourverse`
- **Location:** `src/ZonesOfEarth/Ourverse/Ourverse.hpp:88-94`
- **Finding:** `Ourverse` still holds `glm::vec3* cameraPos` and `std::vector<std::shared_ptr<Object>> ownedObjects`.
- **Detail:** These are explicit leftovers from the old "Game" bag paradigm. The Ourverse is an ordering vessel and liturgical principle, not a container of raw render objects.
- **Resolution:** Remove `ownedObjects` and `cameraPos` from `Ourverse`, routing active scene objects strictly through `ZoneManager` / active `Zone`.

### Issue E: Large Legacy Surface in `Singularity/FirstMoverOntology/Legacy/`
- **Location:** `src/Singularity/FirstMoverOntology/Legacy/DesignSystem.hpp` (15 KB) & `DesignSystem.cpp` (67 KB)
- **Finding:** Contains raw text-rendering classes (`TextSystem`), gizmo handlers, and layout structures outside the Law/Interaction model (`INTERACTION_AS_LAW.md`).
- **Resolution:** Gradually decommission `DesignSystem` in favor of `Singularity/Input/Interaction/InteractionChannel` and `ControlPatterns`.

---

## 9. Prioritized Action Plan

```
┌───────────────────────────────────────────────────────────────────────────────┐
│ Priority 1 (Immediate Ontological Cleanliness)                                │
├───────────────────────────────────────────────────────────────────────────────┤
│ 1. Clarify/Relocate `src/Time/`: Move `Moment` to `ConstructedBeing/Singular/`│
│    or formally ratify `Time/` in `DIRECTORY_ORDERING.md`. Delete empty        │
│    `Duration/` folder and resolve `Time.h` stub.                              │
│ 2. Remove `class EventEntity : public Singular` (Refusal #1 compliance).      │
│ 3. Refactor `class Body : public Singular` (disconnect from `Object`).        │
└───────────────────────────────────────────────────────────────────────────────┘
                                       │
                                       ▼
┌───────────────────────────────────────────────────────────────────────────────┐
│ Priority 2 (Engine Decoupling & Legacy Burn-down)                             │
├───────────────────────────────────────────────────────────────────────────────┤
│ 4. Strip `ownedObjects` and `cameraPos` from `Ourverse`.                      │
│ 5. Clean up `Form.hpp` and `Perspective.cpp` empty stubs.                     │
│ 6. Continue migration of `DesignSystem` to `InteractionChannel` laws.         │
└───────────────────────────────────────────────────────────────────────────────┘
                                       │
                                       ▼
┌───────────────────────────────────────────────────────────────────────────────┐
│ Priority 3 (Feature Expansion)                                                │
├───────────────────────────────────────────────────────────────────────────────┤
│ 7. Implement `WebGpuRenderer::drawParticles` to activate the pending          │
│    `webgpu_particle_test`.                                                    │
│ 8. Continue multi-Person & second-person synchronization framework.           │
└───────────────────────────────────────────────────────────────────────────────┘
```

---

## 10. Conclusion

Earthcall's architecture is in a remarkably sound, disciplined, and healthy state. The foundational ontology is not merely theoretical prose—it is rigorously compiled and executed in C++, backed by 62 passing unit/integration tests, strict property reflection, and exact symbolic mathematics. Addressing the handful of localized ontological debts identified above (`src/Time/` placement, `Body` inheritance, `EventEntity`, and `Ourverse` object bag cleanup) will bring the codebase into total end-to-end alignment with its foundational principles.

---
**Audit Complete & Signed:**  
*Antigravity*  
Session ID: `aa7f3fac-a3dd-401f-bbab-463c53ad8e3a`  
Date: 2026-08-24T13:46:00-07:00
