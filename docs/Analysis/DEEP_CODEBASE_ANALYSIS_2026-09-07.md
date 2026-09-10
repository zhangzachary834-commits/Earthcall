# Earthcall Codebase — Comprehensive Analysis  
*OpenCode o3 session “deep-analysis-2026-09-07”*
*Date: 2026-09-07, 23:55 PDT*

---

## Table of Contents
1. Executive Summary  
2. Architectural Pillars  
3. Sub-system Drill-down  
   3.1 Singularity (kernel & channels)  
   3.2 Property & Reflection Layer  
   3.3 Law System & Prophetic Rete  
   3.4 Zones, Homes, Ourverse  
   3.5 Rendering & OntoMath Geometry  
   3.6 Storage & Persistence  
   3.7 Identity & Security  
4. Governance Documents & Tests  
5. Strengths Observed  
6. Risks / Technical Debt  
7. High-leverage Recommendations (next 3 sprints)  
8. Appendix: Quick Reference to Key Files & Docs

---

## 1. Executive Summary
Earthcall has matured into a coherent **computational ontology with an engine attached**.  All moving parts are now aligned to the Seven Refusals, the _New Kind Framework_, and _No Black Box_.  The code successfully enforces many of the lofty principles described in the manifesto: every meaningful field is discoverable, every rule is serialisable, every new domain is expected to be **authored not coded**.

The foundation is solid but several pressure points could crack as the authored world scales:  
* **Time** is still a global primitive, blocking determinism, undo, and multi-rate zones.  
* **EventBus & global vectors** are single-threaded; async channels already violate that assumption.  
* **Prophetic Rete** is brilliant but has no α-node sharing or runtime profile hooks; memory will balloon.  
* Some engine leftovers (e.g. `Ourverse::ownedObjects`) contradict the newest design docs.  
* CI does not yet run in the cloud; governance rules risk drift as more contributors join.

Addressing these gaps now will preserve the ontology’s integrity while unlocking practical performance headroom.

---

## 2. Architectural Pillars (codified in docs)
| Pillar | Source Doc | Runtime Enforcement |
|---|---|---|
| **Seven Refusals** (esp. “no new domain class”, “no black box”) | AGENTS.md, NO_BLACK_BOX.md, NEW_KIND_FRAMEWORK.md | `no_black_box_test`, type whitelist, router truth probe |
| **Singularity as OS** | EarthcallOurverse.md §§37-75 | `Singularity/` tree, TransferPolicy tiers |
| **PropertyPath reflection** | NO_BLACK_BOX.md §3 | Lazy registry in `Singular`, change feed, tests |
| **Law as data** | LAW_AND_CREATION_SYSTEM.md | `ConditionModel`, `ActionModel`, JSON serialisation |
| **Prophetic Rete** (over-approx before runtime) | PROPHETIC_RETE.md | `Prophetic::Index`, Section F test |
| **Composition/Migration ladders** | NEW_KIND_FRAMEWORK.md & LAW_MIGRATION_FRAMEWORK.md | Audits & task checklists |

These documents are not aspirational—tests and code assert them every build.

---

## 3. Sub-system Drill-down
### 3.1 Singularity
* **EventBus**: Simple, template-based; not thread-safe.  CloudStorage & Web callbacks post from async threads → need queued dispatcher.  
* **Channels**: Audio, Screen, Language, Network, Web, Physical (stub). Pattern: each exposes first-mover bridge objects with registered properties.

### 3.2 Property & Reflection Layer
`Singular` lazily builds a **cache-friendly SoA registry** (`_propertyNames` + `_propertyRegistry`).  Dynamic properties map (`_dynamicProperties`) lets laws add fields at runtime.  Two global callbacks (`notifyPropertyChanged`, `notifyBeingReleased`) keep Rete facts in sync.

Pain points: constructor calls to `buildProperties()` duplicate registration; parent/child formation links copy by value.

### 3.3 Law System & Prophetic Rete
* **Law.cpp**: identity (slug), authors/targets as Formations, Activation/Scope/Drive semantics, authority levels.  
* **Condition/Action models** compile to closures; first-mover laws may carry handwritten closures.  
* **Prophetic Rete**: Pass 1-3 static analysis yields relevance filter; Pass 4 uses classic Rete network.  Safety guaranteed by widening.  Missing: α-node sharing, per-node counters, action→beta back-pointers.

### 3.4 Zones, Homes, Ourverse
`ZoneManager` owns vector<Zone>, enforces home logic, identity dedup.  `Ourverse` now exposes gathering Zone, joy hierarchy, filaments, metalaws—but still carries an `ownedObjects` bag contrary to OURVERSE.md; needs cleanup.

### 3.5 Rendering & OntoMath Geometry
* WebGPU SDF renderer; shaders generated from OntoMath distance expressions.  
* Tessellation cache deliberate failure guarded by `smooth_tessellation_cache_test`.  
* Spatial acceleration Phase-3 plan exists but unimplemented; current marcher is O(N shapes · pixels).

### 3.6 Storage & Persistence
* FlatBuffers (`earthcall_heavy.fbs`) for bulk world; JSON helper for monolithic saves.  
* before-load snapshot (3 MB) currently committed—flagged for .gitignore.  
* Zone identities now stored once under `saves/zones/<id>`; worlds reference them.

### 3.7 Identity & Security
ed25519 keys, IdentityLedger, FirstMoverRegister.  SecurityManager stub; phase-4 plan suggests macaroons per Person.  Authority ceilings enforced in Law.cpp.

---

## 4. Governance Documents & Tests
* **ENGINEERING_DISCIPLINE.md** supplies cultural rules (End-to-End Coherence, Transparent Failure).  
* **BUILD_AND_ENVIRONMENT.md** explains four-verdict frame-lag test, test rationales.  
* Tests guard each previous failure class (paint setter, registry mismatch, law deafness, frame cost, etc.).

Gap: CI not automated (no GitHub Action); all reliance on local `ctest`.

---

## 5. Strengths Observed
1. **Ontology-code parity** – docs are backed by compiler errors & tests.  
2. **Legibility** – every stateful field discoverable via PropertyPath.  
3. **Serialisable behaviour** – laws round-trip JSON; easy inspection.  
4. **Provenance trails** – stakeholder logs on Singulars, audit logs on saves.  
5. **Rich test stories** – each test has a narrative anchored to past bug.

---

## 6. Risks / Technical Debt
| Area | Risk | Consequence |
|---|---|---|
| **Time primitive missing** | Scripts rely on global double; undo/rewind impossible | feature blockage, desync bugs |
| **EventBus single-threaded** | Async callbacks race | random crashes when cloud sync grows |
| **Prophetic Rete unshared α nodes** | O(N rules) memory, filter rebuild latency | performance wall at scale |
| **Ourverse legacy bag** | Violates ontology spec, duplication with Engine scene graph | conceptual drift |
| **Kernel vs property duplication** | Some `ComputedProperty` setters silent | laws appear to write but effect lost |
| **before-load.json in repo** | Huge git history bloat | slow clone, friction for contributors |
| **CI absent** | Refusal/tests only run locally | rule drift, inconsistent environments |

---

## 7. High-leverage Recommendations (3-sprint roadmap)
### Sprint 1 – Infrastructure
1. **Introduce Time::Moment Singular**; wrap global clock; expose `time.now`, `time.delta`, `time.sinceApplied`.  
2. **GitHub Action**: build + `ctest`, enforce no-black-box and frame-lag STANDING threshold.  
3. `.gitignore` for `saves/backups/before-load.json`; move file to local cache.

### Sprint 2 – Performance & Safety
4. **Thread-safe EventBus**: add `postAsync()` with lock-free MPSC queue; main thread drains.  CloudStorage/Web callbacks migrate.  
5. **Prophetic Rete α-node sharing**: hash `(path, op, const)`; reuse nodes; add per-node hit counters exposed via ImGui debug.  
6. **ZoneManager object index**: `unordered_map<slug,shared_ptr<Zone>>`; remove duplicate push.

### Sprint 3 – Ontology Alignment
7. **Remove `Ourverse::ownedObjects`**; engine scene graph owns bag; Ourverse retains only fields in OURVERSE.md.  
8. **Make Singular parent/child formation links weak_ptr; fix copy-ctor dup.**  
9. **Kernel guard audit**: Enumerate each channel guard; ensure refusal logged not silent filter.

Each item ≤500 LOC and yields test-visible gain or doctrine compliance.

---

## 8. Appendix — Quick Reference
| Concern | File | Doc |
|---|---|---|
| Property registry & change feed | `Singular.hpp`, `PropertyPath.hpp` | NO_BLACK_BOX.md §2-3 |
| Law identity & drive logic | `Law.hpp` | LAW_AND_CREATION_SYSTEM.md §§58-176 |
| Prophetic Rete analysis | `PropheticRete.hpp` | PROPHETIC_RETE.md |
| New domain integration rules | – | NEW_KIND_FRAMEWORK.md |
| Engineering culture | – | ENGINEERING_DISCIPLINE.md |
| Build & test semantics | – | BUILD_AND_ENVIRONMENT.md |

*End of analysis.*