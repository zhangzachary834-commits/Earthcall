# Geometry Execution Substrate Manifesto

**Status:** architectural doctrine  
**Date:** 2026-09-12  
**Human origin:** Zachary Zhang  
**Codification and extension:** GPT-5.6 Sol, from Zach's distinction between geometry-as-opcode and engine-decided kinds  
**Read beside:** `ONTOMATH_FRAMEWORK.md`, `GEOMETRY_ONTOMATH_UNIFICATION_PLAN.md`, `../ontology/AUTHORED_CATEGORIES.md`, `../ontology/NO_BLACK_BOX.md`, `../law/FIRST_MOVER_AUTHORING.md`, and `../Design/Building 2D and 3D Apps with Earthcall Guide.md`

---

## 0. Thesis

Earthcall must distinguish **what a being is** from **how its form is represented and executed**.

A sphere opcode is not a species of being. A field opcode is not a category. A smooth-surface tag is not an authored identity. They are compact statements to the mathematical and execution substrate about how to construct, lower, evaluate, render, raycast, collide with, or otherwise manifest form.

The central rule is:

> **Authored meaning flows downward into mathematics and execution. Execution tags must never flow upward and silently become authored meaning.**

This is the boundary that lets Earthcall be both ontologically open and computationally efficient.

Hardcoded low-level instruction vocabularies are not a betrayal of authorability. They are necessary substrate **when they are kept at the correct semantic altitude**. `ShapeKind`, `SdfPrim`, `SdfOp`, topology discriminants, and backend render hints are admissible only insofar as they behave like compiler IR, VM instructions, kernel primitives, or representation tags. They must not become the engine's answer to "what kind of thing is this in the world?"

The Person authors the world. The engine lowers it.

---

## 1. The four layers

Earthcall's form pipeline has four conceptually distinct layers. Code may optimize across them, but must not collapse their meanings.

```text
AUTHORED ONTOLOGY
Category · Relation · Formation · Law · Concept · Material · authored Properties
        |
        |  gives meaning, provenance, behavior, membership, shared defaults
        v
AUTHORED MATHEMATICAL FORM
OntoMath · scalar/vector fields · implicit equations · control nets · topology
        |
        |  lowers without adding domain meaning
        v
GEOMETRY EXECUTION IR
ShapeKind · SdfPrim · SdfOp · SpatialKind · patch/edge representation tags
        |
        |  compiles / specializes / caches / serializes
        v
EXECUTION BACKENDS
WGSL/WebGPU · CPU evaluator · tessellator · raycast · collision · save migration
```

Only the first layer answers questions such as:

- Is this a chair?
- Is this a planet?
- Is this a button?
- Is this sacred architecture?
- Is this a vehicle?
- Is this an instance of a Person-authored category?

The mathematical and execution layers may answer different questions:

- Is this represented by an implicit scalar field?
- Does this surface have a boundary?
- Is this patch planar, smooth, or mesh-backed?
- Which primitive evaluator computes this leaf?
- Which operation combines these two fields?
- Can this backend evaluate the form analytically, or must it use a tessellation?

Confusing these two families of questions is the architectural error this manifesto refuses.

---

## 2. `ShapeKind` is a constructor/opcode vocabulary, not ontology

`ShapeKind` is retained because it is useful. Its usefulness is precisely **not** that it defines authored kinds.

A value such as:

```cpp
ShapeKind::Sphere
```

should be read approximately as:

> "Use the standard sphere constructor/lowering intrinsic with these parameters."

not:

> "The engine has declared that this being is ontologically of kind Sphere."

The current architecture already points in this direction. Named shapes lower into richer mathematical/topological data:

```text
ShapeKind::Sphere
    -> makeSphere(r)
    -> SmoothSurfaceData / OntoMath-compatible form

ShapeKind::Cylinder
    -> cappedCylinder(r, halfH)
    -> ComplexShapeData

ShapeKind::Torus
    -> makeTorus(majorR, minorR)
    -> SmoothSurfaceData

ShapeKind::Field
    -> SdfNode / OntoMath expression
```

After lowering, the named token is no longer sufficient to describe the form's truth. The topology or mathematics is.

Therefore:

1. `ShapeKind` may remain append-only and serialized for compatibility, bootstrap, tooling, migration, foreign interop, and First-Mover convenience.
2. New application/domain nouns must never be added to it.
3. A Person-authored category must never require a new `ShapeKind` value.
4. Backend code may switch on it to lower or recover representation, but must not infer domain semantics from it.
5. Over time, more code should consume the lowered mathematical representation and less code should depend directly on the original named token.

The frontier is not "delete `ShapeKind`." The frontier is **push it downward until it behaves like a primitive instruction**.

---

## 3. `SdfOp` and `SdfPrim` are geometry IR

The SDF layer makes the intended architecture easiest to see.

`SdfOp` values such as:

```text
Leaf
Morph
Union
Intersect
Subtract
SmoothUnion
```

are operations. They are analogous to bytecode or compiler IR instructions.

`SdfPrim` values such as:

```text
Sphere
Box
RoundBox
Ellipsoid
Cylinder
Cone
Torus
Expr
Convex
```

are kernel primitives or intrinsics. They name compact mathematical evaluators whose semantics are fixed so that CPU and GPU backends can agree.

Neither enum tells Earthcall what a being means.

A cathedral dome may lower to a sphere or quadric. A moon may lower to a sphere. An eye may lower to a sphere. A control handle may lower to a sphere. They may share the same geometry instruction while being utterly different authored beings.

Likewise, two beings may share one authored Category while using different geometric programs.

That independence is a feature, not an inconvenience.

---

## 4. `SpatialKind` is a representation type, not a world kind

`SpatialKind` currently distinguishes:

```text
Polyhedron
SmoothSurface
ComplexShape
Field
Patch
```

This is closer to an IR type tag than to an authored category. It tells the kernel what representation is present and therefore which algorithms are valid.

For example:

- a `Field` can be evaluated as an implicit function;
- a `Patch` carries a control net and is an open parametric surface;
- a `Polyhedron` exposes vertices/faces suitable for exact flat-faced topology operations;
- a `ComplexShape` may contain multiple patches and classified seams;
- a `SmoothSurface` may admit analytic evaluation or specialized ray intersection.

These are legitimate engine distinctions because algorithms genuinely depend on representation.

But they carry a strict restriction:

> **A representation type may choose an algorithm; it may not choose authored meaning.**

This is the same reason a compiler may distinguish `i32`, `f64`, pointer, vector, or aggregate types without thereby deciding whether a value represents money, temperature, guilt, or joy.

A future refactor may replace some `SpatialKind` branches with capabilities, traits, or direct structural inspection. That would be an implementation improvement, not an ontological revolution. The doctrine is independent of the exact C++ shape of the IR.

---

## 5. Topological discriminants are admissible when they state mathematics

The geometry kernel contains distinctions such as:

```text
Quadric vs Parametric
Planar vs Smooth vs Mesh patch
Hard vs Soft vs Fairness edge continuity
LineSegment vs Circle edge model
closed / orientable / hasBoundary
```

These are not domain categories. They state mathematical or topological facts about representation.

A `Hard` edge means a G0 positional join with a normal discontinuity. A `Soft` edge means a G1-continuous normal with a curvature discontinuity. Those facts legitimately affect tessellation, shading, differential geometry, collision approximations, or editing tools.

They do **not** legitimately imply things such as:

```text
Soft edge -> pillow behavior
SmoothSurface -> living thing
Sphere -> planet physics
Patch -> cloth
```

unless a Person-authored Law explicitly creates that connection.

The test is simple:

> If changing a representation while preserving authored meaning would accidentally change the being's domain behavior, the representation tag has leaked upward.

---

## 6. Lowering is one-way with respect to meaning

Compilation may lower semantic structures into smaller execution structures. It must not silently reverse the direction.

Allowed:

```text
category.planet + authored radius property + authored Law
        -> OntoMath sphere equation
        -> SdfPrim::Sphere / quadric form
        -> WGSL instructions
```

Also allowed:

```text
Person authors f(x,y,z)
        -> MathNode
        -> SDF instruction sequence
        -> CPU evaluator / WGSL evaluator
```

Forbidden:

```text
SdfPrim::Sphere
        -> therefore category.planet
```

Forbidden:

```text
SpatialKind::SmoothSurface
        -> therefore apply "organic" gameplay behavior
```

Forbidden:

```text
ShapeKind::Cylinder
        -> therefore this object auto-bonds to another Cylinder because Cylinders "are that kind of thing"
```

unless the rule is explicitly a kernel-level mathematical invariant rather than authored world behavior.

The same form may carry many meanings; the same meaning may carry many forms. The lowering relation is therefore many-to-many across the ontology/geometry boundary and must not be treated as identity.

---

## 7. Why this makes WGSL clean

WGSL should receive **math and execution instructions**, not Earthcall's world ontology.

A GPU wants compact values such as:

```text
opcode
primitive id
parameters
child/operand indices
transforms
offsets
blend constants
material/render payload
```

It does not want C++ inheritance, domain classes, private callbacks, or authored taxonomy encoded into shader branches.

That gives Earthcall a natural compilation shape:

```text
Person-authored world
    -> authored mathematical form
    -> normalized geometry IR
    -> packed GPU program/buffers
    -> WGSL evaluation
```

A conceptual instruction stream may look like:

```text
0  PRIM_SPHERE    radius=0.50
1  PRIM_BOX       half=(0.30,0.20,0.40)
2  TRANSLATE      child=1 offset=(0.25,0,0)
3  SMOOTH_UNION   a=0 b=2 k=0.12
4  RETURN         node=3
```

Earthcall does not need to adopt this exact byte layout immediately. The architectural requirement is that the information crossing into WGSL be equivalent in spirit: compact, explicit, inspectable, deterministic execution data whose semantics match the CPU path.

The GPU is an execution backend, not an ontology server.

---

## 8. One source of mathematical truth, many backends

The same authored mathematical form should be consumable by multiple execution paths:

```text
OntoMath / topology
    +-> WebGPU/WGSL analytic evaluator
    +-> CPU evaluator
    +-> raycast evaluator
    +-> collision/support evaluator
    +-> tessellator / marching backend
    +-> save/restore serialization
    +-> debugging / inspection UI
```

This is a multi-backend compiler architecture.

The purpose is not abstraction for abstraction's sake. It gives Earthcall concrete guarantees:

- CPU and GPU can be tested for semantic parity.
- A renderer may change without changing what the authored form is.
- A tessellation can remain a cache/approximation instead of becoming the shape's identity.
- Debugging can inspect the same program the shader executes.
- A future backend can consume the same IR without inventing a new domain model.
- Person-authored geometry remains portable across modalities.

The backend may specialize execution, but it may not reinterpret meaning.

---

## 9. Exactness, fallback, and caches

A representation may have several manifestations.

For example, an implicit field may be:

- evaluated directly by WGSL;
- sphere-marched on CPU;
- tessellated by marching tetrahedra;
- approximated by a cached mesh for a backend that cannot evaluate the analytic form directly.

These are **execution strategies**, not competing identities.

Therefore:

> The most expressive authored mathematical representation is the source of truth; meshes, grids, acceleration structures, and backend buffers are derived artifacts unless explicitly authored as such.

A cached tessellation must not silently become "what the object really is" merely because one backend draws it.

The same principle applies to:

- GPU buffers;
- RPN or bytecode forms;
- BVHs / TriGrids;
- heightfield acceleration grids;
- memoized shader programs;
- smooth-surface tessellation caches.

Derived execution state lives beneath the authored truth.

---

## 10. The semantic-altitude test

Before adding any new enum, tag, opcode, type discriminator, or branch in geometry code, ask:

### Question A — Does this define authored meaning?

If yes, it does **not** belong in the geometry execution substrate. Author a Category, Relation, Formation, Property, Concept, Material, or Law.

### Question B — Does this select a mathematically distinct representation or evaluator?

If yes, it may belong in geometry IR.

### Question C — Could two totally different authored beings legitimately share this value?

If no, the value is suspiciously semantic.

`SdfOp::Union` passes easily. A cathedral and a toy may both use it.

`ShapeKind::Sphere` passes when treated as a constructor intrinsic. An eye and a planet may both use it.

`Category::Planet` would fail as an engine enum because its very purpose is authored meaning.

### Question D — Could the same authored category use several values here without ceasing to be that category?

If yes, the distinction is likely representational rather than ontological.

A `category.tree` may be a field, a mesh, a patch assembly, or a hybrid Formation and remain a tree.

### Question E — Is the distinction needed by a backend to execute correctly?

If yes, hardcoding may be appropriate. The lower layers are allowed to be finite and mechanical.

The rule is not "no enums." The rule is **no accidental ontology in enums**.

---

## 11. Laws may inspect geometry, but representation must not impersonate category

A Law may legitimately ask geometrical questions.

Examples:

```text
if surface curvature exceeds threshold...
if signed distance < 0...
if this patch has a boundary...
if two surfaces intersect...
if volume < X...
```

These are authored rules over mathematical facts.

A transitional system may also expose `shape.kind` or `spatialKind` for tooling and migration. But world behavior should prefer **meaningful mathematical predicates** or authored Category membership over raw implementation tags whenever possible.

This distinction matters because Earthcall currently carries legacy paths such as physics targets filtered by `ShapeKind`/`SpatialKind`, and automatic bond rules keyed by `ShapeKind` pairs.

Those paths require scrutiny.

A geometry-sensitive kernel invariant may stay in C++. An authored behavioral rule must migrate upward into Law/Category space.

The correct question is not merely "does this code branch on `ShapeKind`?" It is:

> **Why does it branch? To execute geometry, or to decide what the being means and how it should behave?**

The first is substrate. The second is ontology leakage.

---

## 12. Known leakage seams

This doctrine does not pretend the current tree is already pure.

Two important seams are visible today:

### 12.1 Physics `LawTarget` geometry filters

The legacy physics target can filter directly by `ShapeKind`, beside a newer `SpatialKind` filter and object-type/category-like mechanisms.

Filtering for a genuinely geometric algorithm may be legitimate. Filtering because a sphere is presumed to have some authored semantic behavior is not.

The long-term direction is:

- authored semantic selection -> Category / Relation / Law;
- mathematical selection -> explicit mathematical/topological predicates;
- representation selection -> restricted to substrate/tooling paths.

### 12.2 Automatic bond rules keyed by `ShapeKind`

A table keyed by pairs of shape tokens is especially dangerous because it invites form to acquire hidden meaning.

If the bond is a mathematical invariant of the representation, document why.
If it is world behavior, it belongs in authored Law.

These are migration seams, not reasons to abolish the instruction vocabulary.

---

## 13. CPU/WGSL parity is an architectural invariant

When an opcode or IR node exists, its meaning must not depend on backend accident.

For every geometry operation that both CPU and WebGPU implement:

```text
same instruction + same inputs -> same mathematical result within declared numerical tolerance
```

The shader may use a faster evaluation strategy. The CPU may use a closed-form path. The tessellator may sample the same function. But these are implementations of one mathematical operation.

This is why OntoMath unification matters. An instruction vocabulary is safe only if it compiles from a common source of truth rather than becoming a collection of backend-specific approximations that merely share names.

Parity tests should therefore be treated like compiler conformance tests, not merely renderer regressions.

---

## 14. Serialization rule

Serialized execution enums are append-only because save files are sacred and old worlds must remain interpretable.

But preserving an enum integer does not grant that enum ontological dignity.

Serialization answers:

> "How do I decode this historical execution/constructor token?"

It does not answer:

> "What was this being ultimately?"

That meaning must survive through authored beings, Relations, Categories, Formations, Laws, Materials, Properties, and mathematical form.

Migration code may translate old `ShapeKind` values into newer topology/OntoMath structures. That is exactly analogous to a compiler decoding an old bytecode version into a newer internal representation.

---

## 15. First Movers and convenience constructors

First Movers need convenient doors.

A developer, migration script, foreign bridge, test, or AI First Mover may reasonably say:

```text
make me a sphere of radius 0.5
```

The existence of that door does not require "Sphere" to become an authored world category.

The proper interpretation is:

```text
First Mover convenience request
    -> known constructor
    -> authored/inspectable mathematical payload
    -> ordinary Object with ordinary provenance
```

The constructor is scaffolding. The resulting being must remain legible and governable.

This is why `ShapeKind` is allowed to exist while Refusal 3 simultaneously rejects new enum values for Person-authored kinds. The two rules are not contradictory once semantic altitude is explicit.

---

## 16. No black box applies all the way down to the IR boundary

Low-level does not mean secret.

If an instruction carries a radius, blend value, extent, control point, field expression, or other authored-relevant parameter, that value must remain inspectable and governable through Earthcall's property/mathematical interfaces unless it is genuinely derived kernel state.

A GPU buffer handle may stay beneath the Kernel.
A Person-authored radius may not.

A compiled shader module may stay beneath the Kernel.
The OntoMath expression it executes may not.

A TriGrid may stay derived.
The topology it indexes may not.

The execution substrate is allowed to be mechanical. It is not allowed to be opaque about the world it is executing.

---

## 17. The compiler analogy is architectural, not decorative

Earthcall should increasingly resemble a compiler pipeline in the following sense:

```text
AUTHORED SOURCE
  beings + Relations + Laws + OntoMath

SEMANTIC / MATHEMATICAL IR
  normalized authored structure and mathematical form

LOW-LEVEL GEOMETRY IR
  finite operations, primitives, operands, parameters

BACKEND LOWERING
  WGSL / CPU / tessellation / collision specialization

EXECUTION
  pixels, contacts, intersections, fields, motion
```

This does not mean Earthcall becomes "just a compiler." The Person-centered ontology remains prior to the engine.

It means the engine should behave like a good compiler: preserve source meaning while lowering it into forms machines can execute efficiently.

A compiler is not permitted to decide that every `f32` is a temperature. Likewise, Earthcall's geometry kernel is not permitted to decide that every sphere is a planet.

---

## 18. Consequences for future architecture

This doctrine implies several long-term directions.

### 18.1 Prefer normalized IR over duplicated backend logic

CPU and WGSL should consume equivalent normalized operations where practical rather than each growing independent shape logic.

### 18.2 Prefer structural capability checks over semantic guesses

Ask whether a representation supports analytic raycast, exact implicit evaluation, convex support, or tessellation. Do not infer those capabilities from domain nouns.

### 18.3 Keep `RenderMode` beneath ontology

`Auto`, `Analytic`, and `Mesh` describe manifestation strategy. They are execution hints, never authored categories.

### 18.4 Make lowering inspectable

Debug tooling should eventually be able to show:

```text
Authored form
-> normalized math
-> geometry IR
-> backend program
```

so a Person can understand what the engine is executing without reading shader assembly.

### 18.5 Geometry bytecode/JIT is a natural optimization frontier

If profiling justifies it, SDF/OntoMath programs may be flattened into compact instruction buffers, specialized, cached, or JIT-compiled for CPU/GPU execution.

That optimization is especially compatible with Earthcall because the architecture already insists that rich meaning live above the finite execution vocabulary.

The bytecode must remain a lowering target, never the authored source of truth.

### 18.6 Do not make every mathematical distinction a C++ enum

This manifesto is not permission to proliferate engine kinds. A finite opcode is justified when it materially improves canonical representation or execution. If a mathematical structure can be represented compositionally in OntoMath without a new intrinsic, prefer composition unless a primitive earns its place through correctness, performance, interoperability, or exactness.

---

## 19. Review checklist

When reviewing geometry/render/physics work, ask:

1. What is the authored source of truth?
2. What mathematical form represents it?
3. Which values here are semantic, and which are execution IR?
4. Does any engine enum accidentally answer a domain "what is this?" question?
5. Is a backend choosing only execution strategy, or inventing meaning?
6. Could CPU and WGSL disagree about the same opcode?
7. Is a tessellation/cache being mistaken for identity?
8. Could the same authored Category use another representation without behavior breaking?
9. Could two unrelated Categories share this representation safely?
10. Is any `ShapeKind`/`SpatialKind` branch actually authored behavior that belongs in Law?
11. Is every authored-relevant parameter legible to Law/OntoMath?
12. Does save migration preserve both historical execution encoding and authored meaning?

If these questions have clean answers, the substrate is probably at the correct semantic altitude.

---

## 20. The compact doctrine

Keep this formulation close to the code:

> **Meaning is authored. Mathematics states form. Geometry IR lowers form. Backends execute IR.**
>
> `ShapeKind` is a constructor token, not a species.  
> `SdfPrim` is an intrinsic, not a category.  
> `SdfOp` is an operation, not a Law.  
> `SpatialKind` is a representation type, not authored identity.  
> WGSL receives mathematics, not ontology.  
> A cache is not the thing it caches.  
> Lowering may erase convenience names; it must never invent meaning.

And the governing asymmetry:

> **Ontology may choose geometry. Geometry must never secretly choose ontology.**

That is how Earthcall can remain open-ended for Persons while becoming increasingly efficient, portable, and cleanly compilable all the way down to WGSL.
