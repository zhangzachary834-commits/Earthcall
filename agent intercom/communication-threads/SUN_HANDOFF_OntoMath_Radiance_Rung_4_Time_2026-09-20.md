# SUN HANDOFF — OntoMath Radiance Rung 4: rho(p,t)

Date: 2026-09-20
Branch: `sol/ontomath-radiance-rung4-time-20260920`
PR: #273 — `OntoMath radiance Rung 4: canonical world-time input rho(p,t)`
Base: `sync-from-earthcall-main`
Status at handoff: implementation and focused witnesses committed; PR opened as draft; CI verdict pending.

## Telos

Rung 4 adds time as an optional mathematical input to authored radiance without
creating an "animated light" type and without turning time into a mutable
radiance parameter.

The authored model is:

```
rho(p,t) -> scalar
```

Existing `rho(p)` remains the same tree and the same answer because it simply
does not read `t`.

## Canonical time

`OntoMath::kTimeVar == "t"` is the canonical temporal-coordinate name.

OntoMath does not own or invent a clock, nor does the shared symbol decree that
every modality uses the same temporal frame. A channel must explicitly bind
`t`. The Screen/WebGPU radiance context binds it to Earthcall's existing
`Universe::now()` simulation/world clock. `Universe::dt()` is carried beside
it in the same global uniform for future explicitly-admitted consumers.

There is no wall clock, GPU frame counter, or renderer-local animation timer.

## Structural/value invariant

Time is a runtime ambient input, not an authored parameter:

- advancing `Universe::now()` does not mutate the radiance AST;
- it does not recollect the SDF/radiance parameter block;
- it does not regenerate WGSL;
- it does not increment radiance content revision;
- structural edits to the AST still follow the existing compile/cache boundary.

## Important boundary caught during implementation

The first implementation made `t` generally visible to the SDF OntoMath
emitter. That was narrowed before review.

Why: doing so would silently enable time-dependent WebGPU geometry while the CPU
geometry evaluator still binds only `p/x/y/z`. GPU geometry would acquire a
semantic capability its CPU peer did not possess.

The final implementation therefore makes world-time binding an explicit
expression-context capability. Radiance opts in. Geometry does not.

Future Rungs such as chroma `chi(p,t)` can opt into the same canonical binding
deliberately.

## Files changed

- `src/Singularity/OntoMath/ScalarForm.hpp`
- `src/Singularity/Screen/WebGPU/SdfWgsl.cpp`
- `src/Singularity/Screen/WebGPU/SdfWgsl.hpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`
- `tests/singularity/authorable_light_contract_test.cpp`
- `tests/singularity/sdf_wgsl_parameter_refresh_test.cpp`
- `tests/singularity/webgpu_object_test.cpp`
- `docs/plans/ONTOMATH_RADIANCE_NEXT_RUNGS_PLAN_2026-09-20.md`

## Focused witnesses

### CPU / emitter witness

`sdf_wgsl_parameter_refresh_test` now proves:

- `rho(p,t)` compiles;
- `t` consumes zero authored parameter slots;
- WGSL reads `u.time.x`;
- ScalarForm factors can use the same canonical `t` binding.

### Compatibility witness

`authorable_light_contract_test` now proves:

- the saved pre-Rung-4 Sun `rho(p)` evaluates identically when optional `t`
  changes;
- CPU OntoMath resolves an explicitly supplied `ValueLeaf(t)` exactly.

### Live native GPU witness

`webgpu_object_test` uses the real
Object -> drawFieldModel -> drawImplicit path.

It changes radiance structure to `rho=t`, compiles once, then advances
`Universe::now()` from 0.15 to 1.0 and requires:

- visibly brighter center pixel;
- zero SDF program compiles on the time-advance frame;
- at least one memoized SDF program cache hit;
- zero authored SDF parameter bytes uploaded due solely to advancing time.

## Base movement

While the branch was in progress, base advanced by one unrelated commit adding
the Radiance Gallery save/generator artifacts. It did not touch any Rung-4
implementation file. PR #273 therefore targets the new base directly.

## What remains before merge

1. Read the authoritative PR #273 GitHub Actions result.
2. If red, inspect only the failing job/step/log and patch the specific defect.
3. Re-run until green.
4. Update this handoff/plan with verified evidence rather than forecast.
5. Mark PR ready for review only after the focused CI witness is green.
