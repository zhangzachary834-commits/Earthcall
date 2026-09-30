# SUN UPDATE — Rung 9 response ownership + compiler boundary
**Date:** 2026-09-24  
**Repository:** `zhangzachary834-commits/Earthcall`  
**PR:** #375 — `Rung 9: authored material response invariant`  
**Branch:** `sol/rung9-material-response-20260924`  
**Live canonical at implementation pass:** `2c581e751c8d53a273257bd088efadb9f358d4ea`  
**Implementation head before this documentation commit:** `fedcfcdd5c70d1fefea9d2f60400949a864fc612`  
**Focused CI at that implementation head:** run `36080946261` / #3186 queued when inspected.

## What became real in this pass

Rung 9 now exists as authored receiver truth through the compiler boundary without changing rendered pixels yet.

The chain is:

```
Material.responseExpr
    -> Material PropertyPath / Law bridge
    -> Material JSON persistence
    -> RenderMaterial projection
    -> response-specific OntoMath admission
    -> structural identity + numeric parameter recollection
    -> explicit refusal
```

No new Material kind, BRDF class hierarchy, response manager, or renderer-owned material ontology was added.

## Material ownership

The existing `Material : Singular` now owns optional `responseExpr` beside `colorExpr`.

It has a separate `responseRevision` rather than borrowing the historical color-expression revision. This matters because receiver response and albedo/color are independent authored predicates:
- editing `colorExpr` is not evidence that response mathematics changed;
- editing `responseExpr` is not evidence that color structure changed.

The response expression round-trips through Material JSON and is exposed as the string-serialized AST Property `responseExpr`, following the existing `colorExpr` bridge pattern. A successful PropertyPath write bumps response revision; malformed JSON refuses without replacing the previous authored response.

`RenderMaterial` now projects the borrowed response AST plus its response revision to the renderer boundary. Material remains identity/revision authority.

## Minimum new OntoMath vocabulary

The audit found that existing `wi` / `wo` directional coordinates were sufficient for incoming/outgoing direction but there was no receiver-normal value.

The only new generic ambient name added is:

```
n : Vector
```

This is not a BRDF operation or Material-specific class. It is the receiving-surface normal coordinate admitted by the Rung-9 response context.

## Separate admission authority

A dedicated response compiler context now admits:
- `p` / `x,y,z`,
- receiver normal `n`,
- existing directional coordinates `wi.x/y/z`,
- existing directional coordinates `wo.x/y/z`.

Authored response currently must evaluate to `Vector`, so it can express chromatic receiver response rather than only a scalar multiplier.

Crucially, this context does **not** admit `t`. No honest Material-owned Timeline path has been established in this bounded rung, so the compiler refuses temporal response instead of stealing source time, medium time, or renderer-global time.

Volume phase still owns its own admission context. It may reuse `wi/wo`, but it cannot read `n`. Generic scalar fields likewise cannot read `n`.

Same mathematical vocabulary does not mean shared semantic authority.

## Structure / value law

`inspectResponseExpression(...)` returns response structure plus read-set facts.

`collectResponseParams(...)` reuses the exact same lowering/admission path to collect numeric values.

The focused witness constructs an authored response containing receiver-normal and both directions and proves:
- the response context admits `n/wi/wo`;
- a numeric-only leaf edit changes parameter values while preserving structural identity;
- replacing that leaf with directional structure changes structural identity;
- `t` refuses;
- receiver normal refuses in generic scalar and volume-phase contexts;
- unsupported response mathematics refuses rather than fabricating output.

Absence has the explicit structural identity:

```
<material-response:legacy-blinn-phong>
```

This is the compatibility seam for the next renderer-wiring pass.

## Material witnesses

`material_being_test` now proves:
- `responseExpr` is discoverable through Material Properties;
- PropertyPath can author the serialized AST;
- response revision changes independently of color revision;
- JSON save/load preserves the authored response;
- malformed Property writes refuse without mutating prior response truth.

## What did NOT change

No WebGPU shading equation consumes `responseExpr` yet.

Therefore this pass does not claim:
- different native pixels from two authored responses;
- response-driven SDF pipeline cache reuse;
- structural response invalidation in the renderer memo;
- source/visibility changes leaving an already-compiled response program untouched at native runtime.

Those are the next implementation boundary, not silently implied by this compiler rung.

Existing absent-response rendering is unchanged because production shading has not been rewired.

## Exact continuation

1. Inspect the exact-head focused CI spawned by this documentation head; fix only concrete owned failures.
2. Once the ownership/compiler rung is green, wire `RenderMaterial.responseExpr` into the WebGPU receiving-surface SDF path.
3. Extend the existing SDF memo with response pointer/revision/structure identity without aliasing color, source, visibility, or volume revisions.
4. Preserve exact hardcoded Blinn-Phong only when `responseExpr` is absent.
5. For authored response, lower a response evaluator that receives the actual surface normal, source->receiver incoming direction, and receiver->eye outgoing direction.
6. Add native same-source/same-visibility pixel witnesses and cache telemetry for numeric vs structural response edits.
7. Keep PR #375 Draft until those native/cache witnesses and preserved Rung 3–8 / V1–V5 gates are exact-head green.

Do not begin Rung 10 from this update.
