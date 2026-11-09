# Field-VM Dependency Preflight

**Lineage:** Lattice Sun ☀️🌐  
**Date:** 2026-11-08  
**Scope:** derived execution/query preflight only. This document does not widen Field-VM admission.

## Constitutional split

The scene Field VM now sits at a frontier where authored mathematics may depend on facts not contained in a local field point. Those facts must not be invented by the renderer.

The required separation is:

```
authored meaning
    !=
binding / query context
    !=
derived execution program
    !=
consumer knownness
```

A `MathNode::Bind("radius")` says that authored mathematics reads a PropertyPath. It does **not** say which Singular supplies that path, what its current value is, or whether the current visibility query is authorized to read it.

A world `Piecewise::guard` says applicability depends on a subject/world condition. It does **not** grant the renderer a subject.

A finite resolved binding does not, by itself, make the current Field VM capable of executing the expression. Admission remains a separate lowering judgement.

## Landed preflight

`FieldVmDependencies.hpp` derives a structural dependency manifest from authored `SdfNode` / OntoMath trees without changing them.

It records:

- `PropertyPath` — authored `Bind(path)`;
- `NamedValue` — a non-local ValueLeaf or ScalarForm symbol;
- `WorldGuard` — a Piecewise world condition requiring a subject;
- `PureGuard` — local mathematical control, explicitly **not** a subject dependency;
- `FunctionCall` — named-function registry context;
- `WorldFold` — world-enumeration context.

The manifest is Kernel-derived evidence. It is not serialized authored identity and adds no new OntoMath or SDF kind.

## Readiness is not admission

`assessFieldVmDependencies(...)` accepts an explicit `FieldVmDependencyContext` and reports:

- missing values/providers;
- non-finite values;
- unsupported value representations;
- unresolved subject/function/world context.

Only finite arithmetic scalars and finite `vec3` values are currently considered representable binding evidence.

`ready == true` means only:

> every dependency named by this manifest has an explicit provider/value in the supplied preflight context.

It does **not** mean:

> `lowerFieldVm` must accept the expression.

It does **not** mean:

> a scene visibility consumer may return Known.

Those are later, independent offices.

## Structural identity versus runtime evidence

The same authored AST may be preflighted against:

```
radius = 0.5
```

and later:

```
radius = 0.9
```

without changing the dependency manifest.

Therefore a future cache must not fold runtime binding values into the program's structural identity merely because they participate in execution.

Conversely, a retained execution result may not survive a binding-value change unless that consumer's validity key explicitly includes the relevant value/revision authority.

The safe ordering is:

```
authored structure
    -> dependency manifest
    -> admitted execution structure
    -> query-time dependency snapshot
    -> execution proof
    -> KnownVisible / KnownOccluded / Unknown
```

No arrow may be traversed backward as semantic authority.

## Falsifiers

`field_vm_dependency_test` pins the present boundary:

1. local `x` is not an external dependency, but `Bind("radius")` is;
2. changing `radius` while retaining the same AST leaves the structural manifest unchanged;
3. missing binding provider revokes readiness;
4. non-finite binding value revokes readiness;
5. unsupported PropertyValue representation revokes readiness;
6. an explicitly supplied named-value provider can satisfy named-value preflight;
7. world guard, function call, and fold remain unresolved without their corresponding contexts;
8. a pure `whereLEZero` guard is recorded but does not invent a subject dependency;
9. empty PropertyPath is structurally invalid before any runtime provider can bless it.

## Explicit non-goals

This rung does **not**:

- add a Bind opcode to the Field VM;
- resolve a PropertyPath against a Singular;
- choose a subject for a world guard;
- execute a fold or named function;
- lower world-dependent Piecewise;
- alter CompleteScene knownness;
- create cache keys or retain snapshots.

Those require an explicit authority model first.

## Next lawful bridge

A later admission rung may consume this manifest only after it can answer, for each dependency:

1. **who owns the authoritative value/context?**
2. **when is that authority sampled?**
3. **what revisions/events invalidate the sampled evidence?**
4. **how is missing/non-finite/unauthorized evidence represented?**
5. **which query is allowed to call the resulting answer Known?**

Until then, dependence is named and refusal is inspectable.

That is progress: the renderer no longer has to treat “needs the world” as one opaque error string, but it still does not pretend to possess the world.

— **Lattice Sun** ☀️🌐
