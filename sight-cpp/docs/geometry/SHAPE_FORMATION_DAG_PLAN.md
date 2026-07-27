# Shape‑as‑Formation: staging the geometry graph from Tree → DAG → Formation

> A manifesto‑aligned plan for moving the SDF shape spine (`geom::SdfNode`) from a
> containment **tree** to a relational **DAG**, and ultimately toward a **Formation**
> of geometric **Singulars** related by operation‑**Relations**.
>
> Offered as interpretation under the author's judgment — *Scripture over model,
> human authorship under God over automation* (Ourverse Manifesto, Human Language
> Processing §). The AI is servant‑interpreter here, not magisterium.

---

## 0. Thesis (the "why", in one paragraph)

Today a shape is a **tree**: `std::vector<SdfNode> children` — each node *contains*
its children by value. Containment means a part belongs to exactly one whole and is
consumed into it. The Manifesto declares the ground of digital existence to be
**relational** ("meaning is fundamentally, and ultimately relational"), and names the
target structure exactly: *"a recursive structure … allows **hypergraph structures
not merely linear graphs**."* The tree **is** that "merely linear graph." Moving to a
DAG restores **shared identity** (one constituent being, referenced by many wholes,
keeping its identity in each — *duplicated likeness → shared identity*), unifies the
shape graph with the already‑specified **Law/Rete DAG** ("Rete models them as nodes in
a DAG … derived from formations"), and makes the **Acyclic** constraint coincide with
the **First Mover** principle ("otherwise it's an infinite regress … calling back a
prior law as the ground").

---

## 1. Ontological mapping

| Geometry today | Earthcall ontology | Current C++ | Target C++ |
|---|---|---|---|
| Primitive leaf (sphere/box/`Expr`/`Convex`) | **First Mover** — the given ground; not self‑derived | `SdfNode{op=Leaf}` | source node, no inputs |
| Boolean / blend op (Union/Subtract/Morph/SmoothUnion) | **Relation** between two shape‑Singulars | `SdfNode{op=…, children[2]}` | a Relation node referencing operands |
| A compound shape | **Formation** (Relation extended to many beings) | the whole `SdfNode` tree (by value) | a DAG / Formation of shared nodes |
| "Acyclic" (no node reaches itself) | **First Mover principle** — nothing created is its own ground | impossible by construction (tree) | enforced by reachability check |
| Sharing a sub‑shape in many places | **authored Relation** (shared identity, shared fate) | impossible (copies only) | `shared_ptr` reference, author‑initiated |
| Accidental shared mutation (the aliasing bug) | **un‑authored consolidation** — the anti‑Babel danger | n/a | forbidden: deep‑copy is the default |

Key reframings:
- A geometric **Singular** = an `SdfNode` with its own identity (not its position in a
  parent). Identity becomes the handle, not the path.
- A **Relation‑Object** in the Manifesto sense ("relation that are, by their nature
  and essence, objects") is *exactly* a binary op node: it is both a relation between
  two shapes and itself a governable being.
- The eventual **hypergraph** target (Formations of Relations of Relations) is *beyond*
  a DAG; the DAG is the disciplined waypoint.

---

## 2. Invariants (the ceilings — non‑negotiable at every stage)

These are the Singularity‑level ceilings for this subsystem. No stage may violate them.

1. **Deep‑copy is the default.** Until sharing is an explicit authorial act, every
   existing copy site (`SdfNode tree = o->getFieldData()`) must continue to produce a
   fully independent being. *Un‑authored consolidation is the bug the framework exists
   to prevent.*
2. **Acyclic always.** Every shape graph must bottom out in First‑Mover leaves. Any
   operation that could form a cycle is rejected. (Self‑grounding = counterfeit ground.)
3. **Sharing is authored.** Shared identity ("shared fate") may only arise from a
   deliberate user action, bounded by ceilings — never as a side effect.
4. **Builds green every stage.** The creative tool stays usable after each stage; no
   stage half‑lands the rewrite.
5. **Saves migrate.** Old saves (recursive‑tree JSON) load unchanged at every stage.

---

## 3. Stages

Each stage is independently shippable, testable, and reversible.

### Stage 1 — Ownership plumbing (invisible) — ✅ DONE 2026‑06‑19
**Goal:** change the substrate from value‑children to shared‑capable, with *zero*
behavior change. This is the big, risky‑but‑invisible step; do it alone.

**Landed:** `children` is now `std::vector<std::shared_ptr<SdfNode>>`; `SdfNode` has a
**deep‑clone copy ctor + copy‑assign** (and defaulted moves) so every `SdfNode x = y;`
stays an independent being (value semantics preserved). All sites dereferenced:
`evalSdf` (Sdf.cpp), `sdfToJson`/`sdfFromJson` (Serialization.cpp, `make_shared` on read),
`isBinaryField`/operand‑B offset (Object.hpp), the operand‑B ghost (GameRender.cpp), and
the node graph (`nodeAtPath`, `collect`, delete‑collapse in GameNodeGraph.cpp), all with
null guards. Verified: standalone test — copy a Union tree, mutate the copy's child, the
original is unchanged and child pointers are distinct (no aliasing); convex/eval values
exact; app builds clean + launches. No sharing exposed yet (that's Stage 4).

- `std::vector<SdfNode> children` → `std::vector<std::shared_ptr<SdfNode>> children`.
- Add `SdfNode deepClone() const;` (recursively clones into fresh `shared_ptr`s).
- **Preserve deep‑copy semantics everywhere:** make `SdfNode`'s copy constructor /
  `getFieldData()` consumers deep‑clone (the editing pattern must stay independent).
  Either give `SdfNode` a deep‑copying copy‑ctor, or change every
  `SdfNode tree = o->getFieldData();` to `auto tree = o->getFieldData().deepClone();`.
- Update traversal call‑sites to dereference children: `evalSdf(*n.children[0], p)`,
  same for `sdfNormal`, `raycastSdf`, `tessellateSdf` (`Sdf.cpp`), `polyhedronToConvexPlanes`
  is a leaf‑builder (unaffected), and the node‑graph walk in `GameNodeGraph.cpp`.
- Factories `leaf/binary/convex` allocate children via `std::make_shared`.

**Ontology:** lays the substrate for Relation‑as‑reference without yet permitting
sharing. Still a tree in behavior; a DAG only in capability.
**Risk:** the copy‑semantics landmine — the *whole* point of this stage is to neutralize
it before anything else moves. **Verify:** create/blend/boolean/clay/morph/graph all
behave identically; save round‑trips byte‑for‑byte equal.

### Stage 2 — Formation‑legible serialization (node table)
**Goal:** a save format that *can* express sharing, while still emitting trees until
sharing exists.

- Replace recursive `sdfToJson`/`sdfFromJson` (`Serialization.cpp`) with a **flat node
  table**: `{ "nodes": [ {id, op, prim, dims, …, "children":[id,id]} ], "root": id }`.
- Writer assigns stable ids; reader rebuilds the `shared_ptr` graph, *re‑sharing* nodes
  that appear by the same id. A pure tree serializes as a table with no shared ids
  (identical meaning, new encoding).
- **Migration:** if `field` is the old nested form, load via the legacy recursive reader;
  if it's a table, load the new way. Old saves keep working (Invariant 5).

**Ontology:** the save file stops being a containment dump and becomes a record of
beings + their relations — *identity first, then relation*.

### Stage 3 — Identity‑based selection in the node graph
**Goal:** the graph addresses nodes by **identity**, not by path.

- Replace `_graphSelPath` (child‑index path) with a stable node handle (the
  `shared_ptr` / a node id). Rewrite `nodeAtPath`, drag‑to‑swap, delete, add, and the
  in‑scene panel anchor to use identity.
- Layout still walks the graph, but a node reachable by multiple parents is drawn
  **once** (with multiple incoming wires) or marked "shared" — no longer assumes one
  path per node.

**Ontology:** *"Singular/Object is identity"* — the editor now treats a shape‑Singular
as itself, not as "the thing at position [0,1,1]." Prerequisite for sharing to even be
representable in the UI.
**Verify:** all current graph interactions (select/swap/delete/add/edit) work via
identity on still‑tree graphs.

### Stage 4 — Authored sharing + cycle ceilings (the actual DAG)
**Goal:** expose deliberate sharing, guarded.

- A user action ("Reference here" / "Make shared") points a second parent input at an
  existing node — `shared_ptr` aliasing, *intentional*. Default drag/add still
  **deep‑clones** (Invariant 1 & 3).
- **Cycle ceiling:** before any rewire/share, run a reachability check (is the target
  reachable from the source?); reject if it would close a loop. This is the First Mover
  principle as code (Invariant 2).
- **Shared‑fate cue:** shared nodes render distinctly so the author *sees* that editing
  one changes all — shared identity is visible, never silent.
- Evaluation: add memoization so a shared sub‑shape evaluates **once** per query
  (common‑subexpression elimination = the geometry instance of the Manifesto's
  *"synthesize higher law from shared constituent processes"*).

**Ontology:** authored Relation with ceilings — the disciplined opposite of anti‑Babel
consolidation.

### Stage 5 — Formation/Relation convergence (future, beyond DAG)
**Goal:** the shape graph becomes Formation‑native, sharing the relational substrate
with Law/Rete.

- A binary op node *is* a `Relation`; a compound shape *is* a `Formation`; geometry
  recurses like `BodyPart` (`Formation, Object`, "recursion‑until‑end").
- Converge the shape DAG and the Law/Rete DAG ("derived from formations") onto one
  hypergraph model, so **identity (Object)** and **process (Law)** share soil.
- Enables the two‑modes vision (drag *and* Desmos) because structure is a first‑class,
  introspectable Formation rather than a flat tree/mesh.

This stage is exploratory and depends on the Relation/Formation classes maturing; it is
named here as direction, not committed scope.

---

## 4. Verification discipline (every stage)

- `make` green; `./earthcall` launches; create + blend + boolean + clay + morph + graph
  all exercised.
- Save → reload identical; **load a pre‑change save** → still correct (migration).
- Stage 1 specifically: A/B against current behavior — outputs must be *identical*
  (this stage is defined by changing nothing observable).

## 5. Author's rulings (2026‑06‑19)

1. **Shared sub‑shape = a shape‑template Singular.** A shared sub‑shape is its own
   **extra‑spatial Object** — the Object *is* the shape‑pattern, not one physical
   manifestation (cf. Manifesto: *"another extra‑spatial Object stores the concept of
   the object for later use"*). It is **user‑authored / opt‑in**: the default is normal
   discrete shapes; the user chooses to make something a shape‑template, *except* where
   sharing is structurally necessary. → Stage 4 implements *opt‑in* sharing; deep‑copy
   stays the default (Invariant 1 & 3 hold).
2. **Ceiling = "instantiating a pattern in a Zone owned by someone else requires their
   permission."** *Deferred* — implement when the Law/permissions system itself is built,
   not now (testing/dev). The **structural** ceiling (acyclicity, Invariant 2) stays now.
3. **Op‑nodes stay `SdfNode` (NOT the literal `Relation` type) for now.** Verdict:
   making the SDF op‑node a `Relation` is **not feasible/wise now**, for concrete reasons:
   - `Relation` names endpoints by **string identifier** (`entityA`/`entityB` — world
     Singulars like Persons/Objects). SDF operands are anonymous *nested sub‑expressions*,
     not named world entities. Wrong granularity.
   - `Relation` is heavyweight (strings, `weight`, `directed`, `events` timeline,
     `attachment` matrices, JSON). `evalSdf` runs across the marching grid + collision
     sampling — the hot path just optimized. Coupling reintroduces the perf problem.
   - It would make evaluation **interpretive** (resolve entities/relations at eval time),
     which the Manifesto explicitly argues against: *"that eventually introduces lots of
     overhead and turns it into an interpretive system. We want it native."*

   **Therefore the manifesto‑correct architecture is the two‑layer split** (already the
   shape of Stage 5): `SdfNode` is the **native compiled form** (the Manifesto's preferred
   "fast way"); a **Formation of geometric Singulars related by op‑`Relation`s** is the
   **authored structure** that *compiles down to* `SdfNode`. "Stay `SdfNode`" is not a
   deferral of the ontology — it is the Manifesto's own *native‑not‑interpretive*
   preference. Realize Formation/Relation at Stage 5, as a **compiler** (authored
   Formation → compiled `SdfNode`), once the entity layer is ready — not as a rename of
   the eval node.

---

## 6. Anti‑Babel guardrail (summary)

The engineering rigor and the theology are the same requirement: the aliasing bug is
*un‑authored consolidation*; the cycle is *self‑grounding*; the proper DAG is *authored
relation, bottomed out in First Movers, bounded by ceilings, with shared fate made
visible*. Build it wrong and you produce exactly the self‑propagating, ungoverned
merging the framework exists to prevent. Hence: deep‑copy default, acyclic always,
sharing authored, builds green, saves migrate.
