# Formation robustness report + work plan

> **Question:** is `Formation` robust enough today to be the serialization / authoring
> backbone for the envisioned functions — chiefly **shape‑template Objects shared by
> many** (ruling #1), and more broadly a **serializable hypergraph of Singulars +
> Relations**?
>
> **Verdict: No — not yet.** It's a sound *structural skeleton* but is missing the
> serialization backbone, a global identifier resolver, and a safe member‑lifetime
> model. The good news: the **Relation layer is already serialization‑ready**, so the
> gap is bounded. This doc is a handoff plan for a dedicated session.

---

## 1. What `Formation` is today (grounded in code)

`class Formation : public Form, public Singular` (`../../src/Relation/Formation/Formation.{hpp,cpp}`):

- **Members:** `std::vector<Singular*> members` — **raw, non‑owning** pointers.
- **Relations:** `RelationManager relationMgr` — owns `shared_ptr<Relation>`.
- **Subformations:** `std::vector<std::shared_ptr<Formation>> subformations` — owned;
  the recursion that makes it a *hyper*graph (Relations clustered into Formations).
- **Methods:** `addMember/removeMember/hasMember`, `findMemberByIdentifier` (recurses
  subformations), `addRelation/removeRelation`, `rebuildCompleteGraph` (all‑pairs
  "member" relations), `findOrCreateRelationFormation` (groups relations of one `type`
  into a subformation — real hypergraph clustering), `integrateRelationTopology`,
  `applyAttachmentRelations` (resolves parent/child transform rigging by identifier),
  `draw`.

Supporting layer (already strong):
- `Relation` (`src/Relation/Relation.hpp`): endpoints are **string identifiers**
  (`entityA`/`entityB`), a `type` tag, `weight`, `directed`, `events` timeline,
  `attachment` matrices. **Has `toJson`/`fromJson`.**
- `RelationManager` (`src/Relation/RelationManager.hpp`): add/remove/query by entity or
  type. **Has `toJson`/`loadFromJson`.**
- Every `Object` already serializes a stable id: `j["objectID"] = obj.getIdentifier()`
  (`Serialization.cpp`).
- `Zone` holds a live `_formation` it **rebuilds every frame** from the world
  (`Zone::syncFormationMembers` adds `world()` + all owned objects by pointer;
  `applyFormationRelations` runs attachment). So Formation already works as a *runtime*
  view — just never a *persisted* one.

---

## 2. The envisioned functions it must support

From the Manifesto + the agreed two‑layer model (`SHAPE_FORMATION_DAG_PLAN.md`):

- **(V1) Shape‑template Objects shared by many** — a sub‑shape is an extra‑spatial
  Object (the pattern itself), referenced by several instances; opt‑in, edit‑once‑update‑all.
- **(V2) Serializable hypergraph** — Singulars + Relations + Relations‑of‑Relations +
  subformations, persisted and reloaded with identity intact.
- **(V3) Authored layer that compiles to `SdfNode`** — Formation is the authored
  source; `SdfNode` is the native compiled form (Stage 5 compiler).
- **(V4) BodyPart‑style recursion** — "embryonic cells dividing"; `BodyPart` is
  `Formation, Object`, recursing until tissue.
- **(V5) Law/Rete "derived from formations"** — conditions as a DAG over Formations.
- **(V6) Recursive object creation** — new objects from sets of existing objects.

---

## 3. Gap analysis (function → current state)

| Capability needed | State | Notes |
|---|---|---|
| **A. Members with stable identity** | ✅ | every `Singular` has `getIdentifier()`; Objects save `objectID`. |
| **B. Members with safe lifetime** | ❌ | raw non‑owning `Singular*`; no `weak_ptr`/notify. A deleted member = dangling pointer. Unsafe for templates referenced by many. |
| **C. Formation self‑serialization** | ❌ | **no `toJson`/`fromJson` on `Formation`.** (RelationManager + Relation have it; the container doesn't.) |
| **D. Global identifier → Singular resolver** | ❌ | only `Formation::findMemberByIdentifier` (local). No world/registry lookup, so a load can't turn saved ids back into members. |
| **E. "instance‑of / references‑template" relation** | 🟡 | *expressible* as `Relation("instance-of", instanceId, templateId)` (string type + ids, serializable) — but not modeled or used anywhere. |
| **F. Subformation / hypergraph recursion** | ✅ | `subformations` + `findOrCreateRelationFormation` cluster relations by type — genuine hypergraph machinery. |
| **G. Save‑system integration** | ❌ | Formation/relations are **not in the save path** at all; Zone uses Formation only at runtime, rebuilt each frame. |
| **H. Acyclicity / ceilings** | ❌ | no cycle check on relations or subformations; no permission hook (deferred per ruling #2, but the structural cycle check is absent). |
| **I. Compile‑to‑`SdfNode` bridge** | ⛔ N/A | Stage 5; depends on A–G first. |

**Bottom line:** the *relational* substrate (Relation/RelationManager) is
serialization‑ready and identity‑based — exactly right. The gaps are all at the
**Formation container** (B, C), the **resolver** (D), and **save integration** (G),
plus the unused **instance‑of model** (E) and missing **guardrails** (H). So: a sound
skeleton, not yet a backbone.

---

## 4. Work plan (stages; each builds green and is testable)

> Manifesto framing: this hardens **Formation** into the *authored relational layer* —
> where shared identity (template Objects) and the hypergraph (Relations of Relations)
> live natively — so it can later **compile** to `SdfNode` (Stage 5 of the DAG plan).
> The resolver is "beings finding each other by identifier"; lifetime‑safety + acyclicity
> are the First‑Mover / anti‑Babel guardrails (no dangling, no self‑grounding).

### F1 — Singular registry / resolver (foundation)
A global (or per‑Zone/World) directory: `identifier → Singular*`. Objects register on
add to the world / unregister on destroy. Model it on `Physics::registry()`. Gives load
a way to resolve saved identifiers, and runtime cross‑references a lookup. *Verify:* look
up every world object by `objectID`.

### F2 — Safe member lifetime
Decide ownership: **World owns Objects (incl. templates); Formation references them.**
Make membership non‑dangling — store identifiers (+ resolve via F1) or `weak_ptr`, and
drop/skip dead members. *Verify:* delete a member Object → Formation neither crashes nor
keeps a stale pointer.

### F3 — Formation serialization
`Formation::toJson`/`fromJson`: member **identifiers**, `relationMgr` (reuse its
`toJson`/`loadFromJson`), `subformations` (recursive), `relationTypeTag`. `fromJson`
resolves member ids via F1. *Verify:* round‑trip a Formation with members + relations +
a subformation → identical after reload.

### F4 — Save‑system integration
Persist the Zone/World Formation graph. Load order: objects first (existing path) → F1
registry populated → Formations resolve members. *Verify:* save a scene with a Formation,
reload, members + relations intact.

### F5 — Shape‑template model (the V1 payoff)
- A `template` flag/kind on `Object` (extra‑spatial; the pattern, not a manifestation).
- An `"instance-of"` `Relation` (instance → template), opt‑in (deep‑copy stays default).
- A **dirty/recompile** hook: editing a template marks instances for recompile — the
  bridge to DAG Stage 5 (authored Formation → compiled `SdfNode`).
*Verify:* two instances of one template; edit the template → both update on recompile.

### F6 — Guardrails
Cycle check on `instance-of` / subformation links (a template can't transitively instance
itself) — the First‑Mover / acyclicity rule. Permission ceiling (ruling #2) left as a
hook for the future Law/permissions system. *Verify:* attempt a cyclic instance → rejected.

**Sequence:** F1 → F2 → F3 → F4 are the serialization backbone (do in order). F5 is the
shape‑template feature on top. F6 is the guardrail, foldable into F5.

---

## 5. Reuse (don't reinvent)
- `RelationManager::toJson/loadFromJson` and `Relation::toJson/fromJson` — the relation
  graph already (de)serializes; Formation serialization wraps these.
- `Relation`'s string `entityA/entityB/type` — the `instance-of` relation is just a new
  `type`; no new relation machinery needed.
- `Formation::subformations` + `findOrCreateRelationFormation` — hypergraph clustering
  exists; reuse for V2/V4/V5.
- `Formation::applyAttachmentRelations` — precedent for resolve‑by‑identifier + parent/
  child propagation (a model for the template recompile pass).
- `Zone::syncFormationMembers` — the runtime build that F1/F4 make persistable.
- `Object` `objectID` (already saved) — the stable identifier F1/F3 key on.

## 6. Decisions to confirm before F‑work
1. **Registry scope:** global (one Ourverse directory) vs per‑Zone/Home (matches the
   "no one owns the global Ourverse" boundary)? (Affects F1.)
2. **Membership storage:** identifier‑strings (resolve each use) vs `weak_ptr` handles?
   (Affects F2/F3 — strings are save‑native; weak_ptrs are runtime‑safe; maybe both.)
3. **Template identity:** is a template a normal Object flagged `template`, or its own
   `Singular` subtype? (Affects F5 and the ontology.)
