# PropertyPath and Memory Micromastery — Implementation Plan

**Author:** Gemini Spark / Firmament (Gemini 3.1 Pro) · session `firmament-property-memory-micromastery-2026-11-04` · 2026-11-04T01:57:47-08:00 (PST)
**Commissioned by:** Zachary Zhang ("BROOOOOOOO YAAASSSSSS WRITE IMPLEMETNATION PLANANNNNNNNNNNN")
**Parent Architecture & Tasks:**
- [`docs/architecture/ontology/PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md`](../architecture/ontology/PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md)
- [`docs/architecture/ontology/PROPERTY_AS_PREDICATION_NOT_BEING.md`](../architecture/ontology/PROPERTY_AS_PREDICATION_NOT_BEING.md)
- [`docs/architecture/ontology/NO_BLACK_BOX.md`](../architecture/ontology/NO_BLACK_BOX.md)
- [`docs/architecture/law/PROPHETIC_RETE.md`](../architecture/law/PROPHETIC_RETE.md)
- [`docs/architecture/law/DERIVED_STATE_LEDGER.md`](../architecture/law/DERIVED_STATE_LEDGER.md)
- [`docs/Agenda/Tasks/Specific Tasks/Rendering and OntoMath/Property_Storage_and_OntoMath_Binding/Property_Storage_and_OntoMath_Binding.md`](../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Property_Storage_and_OntoMath_Binding/Property_Storage_and_OntoMath_Binding.md)
- [`docs/Agenda/Tasks/Specific Tasks/Architecture and Ontology/Purge_Singular_References_From_PropertyValue/Purge_Singular_References_From_PropertyValue.md`](../Agenda/Tasks/Specific%20Tasks/Architecture%20and%20Ontology/Purge_Singular_References_From_PropertyValue/Purge_Singular_References_From_PropertyValue.md)

---

## 0. Architectural Intent vs. Current Tree State

### Zach's Foundational Direction
1. **Property is Predication, Not Being (`PROPERTY_AS_PREDICATION_NOT_BEING.md`):** A `Singular` is the bearer of identity; a `Property` is a legible predication of that bearer; a `PropertyPath` is an address through its articulated state. Relations join beings; Properties disclose them.
2. **Six Authored Memory Relationships (`PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md` §2):** Without exposing raw C++ addresses or segfaults, Persons author the meaning-bearing relationships between PropertyPaths and value cells:
   - **Follow path** (re-resolve path on read; keeps no cell alive)
   - **Share cell** (two or more strong bindings to one mutable value cell; independent rebinding detaches only the rebound path)
   - **Unique cell** (single strong owner; sharing refuses until explicitly converted; weak observers allowed)
   - **Observe weakly** (reads a live cell without extending lifetime; refuses explicitly after final owner releases)
   - **Copy value** (independent cell(s) with explicit deep-copy rules)
   - **Derive** (OntoMath expression over bound source paths with declared dependency invalidation)
3. **Law-Governed Read/Write Micromastery & Bootstrap (`PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md` §§5a–5b):**
   - Permissions for Property reads, writes, shares, and derivations are governed by authored Laws rooted at `Singularity/TransferPolicy`—never a second permission table on `Property` or `DataStructure`.
   - **Migration Default:** Ordinary reads and writes remain open until an explicit reason (structural read-only wrapper, hardcoded Kernel guard, or authored Zone Metalaw) closes them.
   - **Constitutional Exception:** Hardcoded pre-bootstrap Kernel guards refuse unconsented positive Person body/location writes before any Metalaw runs; moving a Person by Law requires a signed, revocable consent sub-Relation (`PRIMARY_AND_SUB_RELATIONS.md`).
   - **Bootstrap:** Kernel parses and verifies First Mover provenance → attested First Mover seeds bootstrap Metalaws → Metalaws configure `TransferPolicy` → downstream Laws branch on policy, actor, and Zone jurisdiction against the committed policy snapshot.

### Observed Implementation in the Tree Today
- **Rung 1 Landed (`2026-10-01` / `2026-10-07`):**
  - `PropertyPath::parse` pre-calculates `_joinedIds` (`Earthcall::StringId`), and `Singular::findProperty` scans contiguous 4-byte `_propertyNames` in L1 cache (`src/ConstructedBeing/Singular/Singular.hpp:189-201`).
  - `readMathBindings` (`src/ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp:266`) preserves exact `PropertyValue` alternatives (64-bit `long`, vectors, shared `PropertyList`/`PropertyDict`) instead of widening every variable to `double`.
  - `PropertyPath::resolve` (`src/ConstructedBeing/Singular/Property/PropertyPath.cpp:110-241`) pins getter-returned containers in `ResolvedSlot::containerPins`, validates list indices via `std::from_chars`, supports detached codec-window edits on `OntoMath::ScalarField`/`VectorField`, and checks `isStructurallyWritable()` / `exposesMutableContainer()` before `Unchanged` shortcuts.
  - `registerDerivedReading` (`src/ConstructedBeing/Singular/Property/PropertyPath.hpp:124`) exposes unstored read-only observations (`.evaluation`) and routes consuming laws to the continuous sweep via `pathReadsUntrackedDerivedReading()`.
- **Four Concrete Gaps Blocking Completion:**
  1. **Fossil Being-Pointers in `PropertyValue`:** `PropertyValue` (`src/ConstructedBeing/Singular/Property/PropertyValue.hpp:42-45`) still admits `Singular*`, `Object*`, `Relation*`, and `Formation*`, smuggling a shadow relation graph inside predicates and forcing `propertyValueUnchanged` to return `false` on every tick (`docs/Analysis/CLOCK_POINTER_BODY_SEAMS_ANALYSIS_2026-10-13.md` §2).
  2. **Silent Rete Deafness on Shared Alias Writes:** Writing `alias.layers.0.color.g` in `PropertyPath::setValue` (`src/ConstructedBeing/Singular/Property/PropertyPath.cpp:369-373`) only notifies `(slot.owner, "alias")`. Any other path sharing that container (`"paint"`) receives zero change notifications, leaving Rete alpha nodes listening on `"paint"` silently deaf.
  3. **Scalar vs. Container Sharing Asymmetry & Non-Persistent Topology:** `PropertyDict` and `PropertyList` share storage via C++ `std::shared_ptr`, whereas scalars (`int`, `double`, `glm::vec3`, `std::string`) copy by value, and `Unique`/`Weak` cell lifetimes do not exist. Furthermore, `propertyValueToJson` flattens shared containers into independent trees on save.
  4. **Unwired Read/Write Access Funnel & `DataStructure::writeBounds` Debt:** `lawGetValue` and `lawSetValue` (`src/ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp:233-264`) do not consult `TransferPolicy` or Zone jurisdiction, and `DataStructure::writeBounds` (`src/ConstructedBeing/Singular/Property/DataStructure.hpp:23`) sits as an unused parallel permission field.

---

## 1. Staged Implementation Rungs

Each rung is scoped as a small, focused diff with its own regression witness and zero stale-tree overwrites. Do not skip rungs or combine them into a single mega-commit.

---

### Rung 0 — Purge Being-Pointer Alternatives from `PropertyValue`
**Goal:** Enforce Zach's 2026-10-13 verdict ([`Purge_Singular_References_From_PropertyValue.md`](../Agenda/Tasks/Specific%20Tasks/Architecture%20and%20Ontology/Purge_Singular_References_From_PropertyValue/Purge_Singular_References_From_PropertyValue.md)): Properties hold pure predicates/data; cross-being relationships live in `Relation` / `Formation` or constitutive `Property::asSingular()` bridges (`BodyPart`).

#### Exact Inventory of Live Touchpoints in `src/` and `tests/`
1. `src/ConstructedBeing/Singular/Property/PropertyValue.hpp:42-45`:
   - Remove `Singular*`, `Object*`, `Relation*`, `Formation*` from the `PropertyValue` `std::variant`.
2. `src/ConstructedBeing/Singular/Property/PropertyValueJson.cpp:15-17, 123-126`:
   - Remove `refJson(const Singular*)` fallback from `propertyValueToJson`. Retain `"ref"` handling in `propertyValueFromJson` as a safe `std::monostate` no-op for legacy save compatibility so old files never crash on load.
3. `src/ConstructedBeing/Singular/Property/PropertyPath.cpp:198-203`:
   - Remove the `std::get_if<Singular*>` / `Object*` / `Relation*` / `Formation*` branch inside `PropertyPath::resolve()`. Constitutive sub-Singular traversal continues to work through `currentRegistered->asSingular()` at line 168 (`PropertyRef::asSingular()` and `ComputedProperty::asSingular()` do not use `PropertyValue` to return `Singular*`!).
4. `src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp`:
   - Line 674 (`Kind::Spawn` parent resolution): `parentPath` already resolves `std::string` identifiers against `world->getOwnedObjects()`; remove the dead `else if (std::holds_alternative<Object*>(pv))` branch.
   - Lines 1213–1223 (`Kind::Create` prototype resolution): `resolveLawRoot(subject, source, start)` already resolves `@identifier` roots directly and `PropertyPath::resolve` traverses `Property::asSingular()`. Replace the `lawGetValue` pointer extraction with `slot = source.resolve(*prototype, start); prototype = (slot.prop ? slot.prop->asSingular() : nullptr);`.
5. `src/ConstructedBeing/Singular/Object/Creation/ObjectConcept.cpp:293-299`:
   - Simplify `isValueLike(const PropertyValue& v)` to `!std::holds_alternative<std::monostate>(v)`.
6. `src/Singularity/Screen/LawGraphWindow.cpp:532-535`:
   - Remove the four `std::holds_alternative<Singular*/Object*/Relation*/Formation*>` checks from `propertyTypeName()`.
7. `src/Singularity/Screen/ImageCodecChannel.cpp` & `tests/law/dynamic_property_reachability_test.cpp:82-84`:
   - Inspect how `ImageCodecChannel` populates `image.regions` (currently `PropertyDict` of region properties or child Formations/Relations) and migrate `dynamic_property_reachability_test.cpp`'s nested-owner witness to traverse a registered `Property::asSingular()` bridge (or constitutive sub-Singular) rather than stuffing `static_cast<Singular*>(&region)` into a `PropertyDict`.

#### Invariants & Verification
- **Invariants:** `Property::asSingular()` still works for C++ constitutive members (`BodyPart`, `ComputedProperty<MemorySubject, Singular*>` in `property_memory_access_test.cpp:53`), while `is_property_value_alternative<Singular*>` becomes `false` at compile time.
- **Verification:** Build and run `property_memory_access_test`, `dynamic_property_reachability_test`, `law_creation_test`, `object_concept_test`, and `no_black_box_test`. Add a static assertion `static_assert(!is_property_value_alternative<Singular*>)` in `property_memory_access_test.cpp`.

---

### Rung 1 — Checked `ValueCell` Table (`Unique`, `Shared`, `Weak`) & Complete Alias Change Feed
**Goal:** Give Earthcall true low-level memory micromastery over **both scalars/vectors and containers** without raw pointers, and fix the Rete alias-deafness bug where mutating one shared path fails to wake laws watching another path bound to the same cell.

#### Proposed Design
1. **Opaque Generation-Checked `ValueCell` Substrate (`src/ConstructedBeing/Singular/Property/PropertyCell.hpp/.cpp`):**
   - Introduce a machine-level `PropertyCellTable` (owned beneath the Kernel, never a `Singular` or domain noun):
     ```cpp
     struct CellId {
         uint32_t slot = 0;
         uint32_t generation = 0;
         bool valid() const noexcept { return generation != 0; }
     };

     enum class CellOwnershipMode : uint8_t {
         InlineValue = 0, // Default fast path: unshared inline value on Singular
         Shared      = 1, // Ref-counted strong shared ownership
         Unique      = 2, // Single strong owner; sharing refuses until converted
         Weak        = 3  // Generation-checked non-owning observation
     };
     ```
   - Each live `ValueCell` in `PropertyCellTable` stores:
     - `PropertyValue value;` (works identically for `int`, `double`, `glm::vec3`, `PropertyDict`, `OntoMath::VectorField`, etc.)
     - `uint32_t generation;`
     - `uint32_t strongCount;` (`1` when `Unique`, `>= 1` when `Shared`, `0` when released/expired)
     - `bool isUniqueLocked;` (when `true`, `shareCell()` refuses with `PathResult::ForbiddenShare` until the owner explicitly converts ownership mode to `Shared`)
     - `uint64_t revision;` (monotonic counter bumped on every semantic value write)
     - `SmallVector<CellObserver, 2> observers;` where `CellObserver = { Singular* bearer, Earthcall::StringId rootPropertyId, CellOwnershipMode mode }`.

2. **Complete Alias Fan-Out on Mutation (Closing Rete Deafness):**
   - When `PropertyPath::setValue` mutates a cell (either replacing its scalar/vector/container value or mutating a nested `PropertyDict`/`PropertyList`/`VectorField` element inside it):
     1. `cell.revision` increments.
     2. The cell iterates its `observers` list and calls `Singular::notifyPropertyChanged(obs.bearer, StringInterner::resolve(obs.rootPropertyId))` for **every** live `Unique`, `Shared`, and `Weak` observer (not just the single `(slot.owner, slot.prop)` through which the write entered).
   - When a `Singular` is destroyed (`~Singular()`) or a path is rebound (`setDynamicProperty` / `rebindPath`) or removed (`removeDynamicProperty`), it detaches its `(this, rootId)` observer from the old cell, decrementing `strongCount` if `Unique`/`Shared`. When `strongCount` hits `0`, the cell's storage is reclaimed and `generation` is marked expired so any remaining `Weak` handles immediately resolve to `PathResult::ExpiredCell`.

3. **Explicit `PathResult` Vocabulary (`PropertyPath.hpp`):**
   - Extend `PropertyPath::PathResult` with explicit, non-silent outcomes:
     - `ExpiredCell` (weak read/write after final strong owner released)
     - `UniqueOwnershipRefused` (attempt to share a cell currently held as `Unique`)
     - `AccessDenied` (reserved for Rung 3 Law/TransferPolicy refusal)

#### Invariants & Verification
- **Derived-State Ledger Entry (`docs/architecture/law/DERIVED_STATE_LEDGER.md`):** Register `ValueCell::observers` (depends on path-to-cell binding and `Singular` lifetime; invalidated by `rebind`, `removeDynamicProperty`, and `~Singular`; guarded by `property_memory_access_test`).
- **Verification:** Extend `tests/singularity/property_memory_access_test.cpp` and `tests/law/dynamic_property_reachability_test.cpp` with:
  1. Two distinct objects (`Object A` and `Object B`) sharing a scalar `double` and a `glm::vec3` color cell: writing `A.color.g` wakes a `WhileTrue` Rete Law conditioned on `B.color.g`.
  2. Rebinding `A.color` to a new cell leaves `B.color` attached to the original cell.
  3. Attempting to share a `Unique` cell refuses with `UniqueOwnershipRefused` until converted to `Shared`.
  4. A `Weak` observer reads live mutations while `strongCount > 0`, and returns `ExpiredCell` (never 0, black, or stale memory) the instant the last strong owner releases or is destroyed.

---

### Rung 2 — Authored Binding Algebra in OntoMath & Law Line (Zero New Serialized Enum Kinds)
**Goal:** Let Persons author the six relationships (**Follow path**, **Share cell**, **Unique cell**, **Observe weakly**, **Copy value**, **Derive**) through `MathBindings` and the Law Line without violating Refusal 3 (no renumbering or adding domain opcodes to `ActionNode::Kind`).

#### Proposed Design
1. **Extend `MathBinding` Metadata (`src/ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp`):**
   - Currently `using MathBindings = std::map<std::string, PropertyPath>;` (serialized as `{"var": "path"}`).
   - Preserve 100% backward-compatible JSON serialization for string entries (`"x": "position.x"` defaults to `BindingMode::FollowPath`), while allowing an extended object form when cell semantics are authored:
     ```json
     "source": { "path": "@held.color", "mode": "share" }
     ```
     where `mode` $\in$ `{"follow", "share", "unique", "weak", "copy", "derive"}`.
2. **How `ActionNode::Kind::Map` Executes Each Mode:**
   - When `ActionNode::Kind::Map` evaluates an identity `ValueLeaf` with a binding:
     - `copy`: deep-clones the value (including acyclic/bounded-cycle `PropertyList`/`PropertyDict` via iterative traversal; refuses or preserves internal alias topology per authored rule) into an independent destination cell.
     - `share`: binds the destination property slot to the source's `CellId` as `CellOwnershipMode::Shared` (upgrading an unshared `InlineValue` slot into a shared `ValueCell`, or refusing with `UniqueOwnershipRefused` if the source is locked `Unique`).
     - `unique`: moves unique cell ownership from source to destination (or initializes a new `Unique` cell).
     - `weak`: binds the destination property slot as a `CellOwnershipMode::Weak` observer of the source's `CellId`.
     - `follow` / `derive`: evaluates the OntoMath `Piecewise` expression over the bound paths and registers the dependency edges `(sourceBearer, sourcePath) -> (destBearer, destPath)` so source mutations invalidate the derived property and wake Rete!
3. **Law Line Compiler Metalaws (`src/Singularity/Terminal/LawSentence.cpp`):**
   - Just as `set glow to @lamp.brightness` lowers to `Map` with `copy` semantics today, add authored Metalaw compiler rules for `share <dest> with <src>`, `borrow <dest> from <src>` (weak), and `derive <dest> from <expr>`—requiring zero new `ActionNode::Kind` enums.

#### Invariants & Verification
- **Invariants:** Existing saves with `"bindings": {"x": "position.x"}` deserialize byte-identically and behave identically.
- **Verification:** Add an end-to-end "Red Object in Hand → Building Form" witness in `property_memory_access_test.cpp`:
  - `Person` holds `Object A` (`color = red`); `Building.color` derives from or shares `Object A.color`; changing `A.color` to `blue` updates `Building` and wakes its Law; destroying `A` makes a `weak` observation refuse cleanly instead of reading dangling memory.

---

### Rung 3 — Unified Law-Governed Read/Write Access Funnel (`TransferPolicy` + Zone Jurisdiction)
**Goal:** Unite Property read/write/share/derive permissions with `Singularity/TransferPolicy` and Zone jurisdiction (`PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md` §§5a–5b) while keeping the ungoverned L1 hot path branchless.

#### Proposed Design
1. **Retire Separate `DataStructure::writeBounds` Authority (`src/ConstructedBeing/Singular/Property/DataStructure.hpp`):**
   - Deprecate `DataStructure::writeBounds` as an independent check and route any container bounds through the single `TransferPolicy` + Zone Metalaw office so Earthcall never has two competing permission systems (Refusal 6).
2. **`AccessContext` and Unified Gate Office (`src/Singularity/TransferPolicy.hpp/.cpp`):**
   - Define a lightweight `AccessContext`:
     ```cpp
     enum class AccessOp : uint8_t { Read, Write, Share, BorrowWeak, TransferUnique, Derive };
     struct AccessContext {
         const Singular* actor = nullptr;   // Person, FirstMover, or Channel
         const Law* authorizingLaw = nullptr;
         const Zone* originZone = nullptr;
         const Zone* targetZone = nullptr;
         AccessOp op = AccessOp::Read;
     };
     ```
   - Extend `TransferPolicy` (without changing existing `canTransfer` set-to-set defaults!) with:
     - `PathResult checkAccess(const Singular& bearer, const PropertyPath& path, const ValueCell* effectiveCell, const AccessContext& ctx) const;`
3. **Evaluation Order (Strict Constitutional Hierarchy):**
   - **Step 1 — Pre-Bootstrap Kernel Floor:** Hardcoded guards run first and cannot be opened by any Metalaw or `TransferPolicy` gate:
     - Structural read-only properties (`!isStructurallyWritable()`, derived readings, `time.*`, `@world.*`, Kernel `gate.*` properties).
     - Positive writes to a `Person`'s body/location (`position`, `velocity`, `transform`, `body.*`) without a verified, signed, unrevoked consent sub-Relation (`kernel-person-body`).
     - Identity reassignment on an already-keyed `Person` (`Person::setPersonId`).
   - **Step 2 — Open-by-Default Fast Path (`O(1)` Epoch Mask):**
     - Each `Zone` and `Singular` carries a `uint64_t accessPolicyRevision` (default `0` when no access Metalaw has closed a read/write/share gate on that Zone or bearer).
     - When `originZone == targetZone` and `accessPolicyRevision == 0` (and `effectiveCell` has no restricted custodian), `checkAccess` returns `PathResult::Ok` in a single integer check—preserving zero-allocation L1 speed!
   - **Step 3 — Committed-Snapshot Metalaw & Inter-Zone Jurisdiction Check:**
     - When an access Metalaw is active or `originZone != targetZone`:
       - Evaluate against the **already-committed policy snapshot** (`committedPolicyRevision`) so a newly written Law or alias edit cannot authorize the transaction that created it.
       - Enforce **effective-cell authority**: if `Path A` and `Path B` share a `ValueCell`, an access refusal on the effective cell cannot be bypassed by reading/writing through the alternate alias spelling.
       - For inter-Zone accesses (`originZone != targetZone`), check the sender Zone's transmission policy and the receiver Zone's reception policy independently.
       - Stratify dependency evaluation with a bounded depth guard (`kMaxAuthDepth = 4`); any authorization cycle refuses explicitly with `PathResult::AccessDenied` and records the refusal in the Law application trace without leaking protected values in error strings.

#### Invariants & Verification
- **Invariants:** Existing `TransferPolicy::canTransfer` `Gated` defaults (`enabled`, `name`, `type`) remain scoped to set-to-set creation so ordinary ungoverned worlds continue reading/writing `enabled` until an explicit Zone access Metalaw closes it (`property_path_precalc_test`).
- **Verification:** Add `tests/law/property_access_governance_test.cpp` witnessing:
  1. First Mover bootstrap seeding an access Metalaw that makes `secret.key` read-only for lower-authority Laws while readable by authorized Laws.
  2. Attempting to bypass a closed write gate on `Object A.vault` via a shared alias `Object B.alias` refuses with `AccessDenied`.
  3. Attempting a self-granting Law edit (a Law whose action tries to raise its own access permission in the same transaction) fails against the committed policy snapshot.
  4. Policy change invalidates any cached access decision and wakes Prophetic/Rete listeners.

---

### Rung 4 — Durable Alias Topology & Cross-Zone Custody in Save/Load
**Goal:** Preserve shared/unique/weak cell relationships across save and reload (`FIRST_MOVER_AUTHORING.md` §7 patch-never-regenerate discipline) without inferring false sharing from equal values.

#### Proposed Design
1. **Additive Cell Table Section in `.ecform` / JSON Codec (`src/ConstructedBeing/Singular/Creation/SingularSetToSetCreation.cpp` & `Serialization.cpp`):**
   - Unshared inline properties (`CellOwnershipMode::InlineValue`) serialize **identically to today**—zero format churn on existing properties.
   - Only properties participating in a non-inline cell relationship (`Shared`, `Unique`, `Weak`) emit an additive `propertyBindings` record on their bearer:
     ```json
     "propertyBindings": {
       "alias": { "cell": "cell-uuid-or-zone-scoped-slug", "mode": "shared", "custodianZone": "ZoneA" }
     }
     ```
   - Shared cells themselves serialize once in the custodian Zone's `sharedPropertyCells` table (or bearer root when intra-Singular).
2. **Load-Time Hydration & Expiry Honesty:**
   - On load, rebuild `PropertyCellTable` handles from `sharedPropertyCells` and wire `propertyBindings`.
   - Two properties bound to `"cell-1"` before save resolve to the exact same `CellId` after load.
   - If a `Weak` binding references a cell whose strong owners were removed, it hydrates as an expired handle (`PathResult::ExpiredCell`), never silently fabricating a default value.
   - Existing saves without `propertyBindings` load every property as `InlineValue` with zero accidental sharing inferred from equal values.

#### Invariants & Verification
- **Invariants:** All save writes obey `FIRST_MOVER_AUTHORING.md` §7 (stage, verify nothing erased, atomic rename) and run inside `TempSaveRoot` / `RealSaveTreeGuard` in tests.
- **Verification:** Round-trip test in `tests/zones/save_roundtrip_test.cpp` and `tests/singularity/property_memory_access_test.cpp`: construct shared scalar/container cells, a unique cell, and a weak observer across two Singulars; save to a sandboxed Zone; reload into a fresh `ZoneManager`; assert alias writes still propagate to both Singulars and weak expiry still holds.

---

### Rung 5 — Cache & Layout Benchmarking (Interned Parallel Arrays vs. Selective Dense SoA)
**Goal:** Follow Zach's measurement rule (`PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md` §7): measure the existing `StringId` + parallel array lookup + `PropertyCellTable` against selective dense type-specific SoA storage on lived-in worlds before changing layout.

#### Proposed Design
1. **Benchmark Harness (`tests/singularity/property_memory_benchmark_test.cpp` & `frame_lag_test`):**
   - Measure across growing Property and alias counts ($N \in \{64, 512, 4096\}$):
     - Path resolution (`PropertyPath::resolve`)
     - Direct and shared cell reads/writes (`getValue` / `setValue`)
     - Multi-alias Rete change fan-out (`notifyPropertyChanged` → `markFactDirty`)
     - OntoMath parameter buffer refresh (`readMathBindings`)
     - Save/load alias hydration time.
2. **Selective Promotion Rule:**
   - Only promote uniform hot populations (e.g., dense `double` / `glm::vec3` cell slabs in `PropertyCellTable`) if they beat the current L1 `StringId` scan + handle table on real workloads (`chess_app`, `Synthesis Studio`, `Northern Veil`), and register any new cache in `DERIVED_STATE_LEDGER.md`.

---

## 2. Decisions Reserved for Zach (`⚑ AUTHOR`)

Before Rung 3 and Rung 4 lock in serialized policy or cross-Zone conflict semantics, these three authorial choices remain strictly yours:

1. **⚑ AUTHOR — Shared-Cell Write Precedence Across Authority Levels (gates Rung 3):**
   When `Path A` and `Path B` share one `ValueCell` and are written by Laws of different authority standings (or governed by different paths), does a write by a higher-authority Law raise an authority floor on the **cell itself** (preventing a lower-authority Law from overwriting that cell through *either* path), or is authority evaluated per-path/per-gate on each write?
2. **⚑ AUTHOR — Self-Editing Access Metalaws After Bootstrap (gates Rung 3):**
   Under the committed-snapshot rule (a newly created Law cannot authorize its own creation), may an already-committed access Metalaw authorize a subsequent edit to its *own* condition/action text, or must edits to an access Metalaw require an independent higher-standing Metalaw (`authority-over` DAG) or First Mover signature?
3. **⚑ AUTHOR — Custody Relation Kind vs. Qualified Ownership/Governance (gates Rung 4):**
   For cross-Zone shared cells (`Zone ↔ bearer, qualified by PropertyPath`), should custody be a **sixth distinct Relation kind** alongside authorship, ownership, governance, dependency, and Constitution, or a path-qualified form of the existing **ownership / governance** Relation kinds?

---

## 3. Recommended Immediate Next Step

**Rung 0 (Purging Being-Pointers from `PropertyValue`)** and **Rung 1 (Checked `ValueCell` Table + Complete Alias Change Fan-Out)** have **zero** blocking `⚑ AUTHOR` ambiguities—you already ruled on both (`2026-10-01` and `2026-10-13`).

Say the word and we can execute **Rung 0** and **Rung 1** right now with focused diffs and full `ctest` verification!
