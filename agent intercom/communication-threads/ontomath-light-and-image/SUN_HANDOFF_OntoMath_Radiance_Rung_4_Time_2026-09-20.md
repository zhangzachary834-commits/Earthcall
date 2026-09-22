# SUN HANDOFF — OntoMath Radiance Rung 4: rho(p,t)

Date: 2026-09-20
Branch: `sol/ontomath-radiance-rung4-time-20260920`
PR: #273 — `OntoMath radiance Rung 4: relative Timeline input rho(p,t)`
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
`t`. Screen/WebGPU receives an admitted temporal coordinate through the Renderer
boundary and does not know which Timeline or Singular supplied it. Production
currently defaults that coordinate from the Universe-selected broad Timeline;
the native witness uses a Timeline owned by the radiant Object.

There is no wall clock, GPU frame counter, renderer-local animation timer, or
special radiance Timeline kind.

## Structural/value invariant

Time is a runtime ambient input, not an authored parameter:

- advancing the admitted Timeline coordinate does not mutate the radiance AST;
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

The final implementation therefore makes temporal-coordinate binding an explicit
expression-context capability. Radiance opts in. Geometry does not.

Future Rungs such as chroma `chi(p,t)` can opt into the same canonical binding
deliberately.

## Principal files changed

Radiance / OntoMath:
- `src/Singularity/OntoMath/ScalarForm.hpp`
- `src/Singularity/Screen/Renderer.hpp`
- `src/Singularity/Screen/WebGPU/SdfWgsl.cpp/.hpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`

Time ontology:
- removed empty `src/Time/Time.h/.cpp`;
- added `src/Time/timeline.hpp/.cpp`;
- `src/ZonesOfEarth/AuthorsOfLaw/Universe.hpp/.cpp` now projects a borrowed Timeline;
- Engine compatibility storage is backed by one broad Timeline instance whose ontological owner is explicitly the Ourverse through the first-class Relation `world-timeline --owned-by--> Ourverse`; Engine storage is not ownership.

Witnesses:
- `tests/time/timeline_test.cpp`
- `tests/singularity/authorable_light_contract_test.cpp`
- `tests/singularity/sdf_wgsl_parameter_refresh_test.cpp`
- `tests/singularity/webgpu_object_test.cpp`

Doctrine:
- `AGENTS.md`
- `docs/architecture/ontology/TIME_AND_MOMENT.md`
- `docs/architecture/mathematics/ONTOMATH_FRAMEWORK.md`
- `docs/plans/ONTOMATH_RADIANCE_NEXT_RUNGS_PLAN_2026-09-20.md`
- Constitutionalist correction intercom note.

## Focused witnesses

### CPU / emitter witness

`sdf_wgsl_parameter_refresh_test` now proves:

- `rho(p,t)` compiles;
- `t` consumes zero authored parameter slots;
- WGSL reads `u.time.x`;
- ScalarForm factors can use the same canonical `t` binding;
- Piecewise interval bounds whose `inputVariable == "t"` read the same admitted temporal coordinate and refuse when that coordinate is not admitted. This closes a bug where non-x/y/z Piecewise inputs previously fell through to literal `0.0`.

### Compatibility witness

`authorable_light_contract_test` now proves:

- the saved pre-Rung-4 Sun `rho(p)` evaluates identically when optional `t`
  changes;
- CPU OntoMath resolves an explicitly supplied `ValueLeaf(t)` exactly.

### Live native GPU witness

`webgpu_object_test` uses the real
Object -> drawFieldModel -> drawImplicit path.

It changes radiance structure to `rho=t`, compiles once, then advances a Timeline owned by the radiant Object from 0.15 to 1.0 and requires:

- visibly brighter center pixel;
- zero SDF program compiles on the time-advance frame;
- at least one memoized SDF program cache hit;
- zero authored SDF parameter bytes uploaded due solely to advancing time.

## Timeline relativity correction

An earlier draft accidentally treated Timeline as though it were fundamentally
a selectable global clock. Zach corrected that. Timeline is a relative temporal
domain: any Singular may own one through ordinary Relation truth. The broad
world/Ourverse clock is merely one Timeline at broad scope.

The current Drive/Flow/`time.sinceApplied` machinery predates first-class
Timeline and Moment ontology and remains compatibility machinery pending the
future Law/Timeline/Moment architecture by Zach and Clawd Opus 5.

See:
`agent intercom/communication-threads/ontology-and-authorship/TO_CONSTITUTIONALIST_Timeline_Relativity_Correction_2026-09-20.md`.

## Base movement

While the branch was in progress, base advanced by two commits containing the
Radiance Gallery / Cathedral work plus guidance updates. The branch was brought
forward with a real two-parent merge commit, not a fake ref move: the merged tree
started from the latest base tree and then overlaid only this branch's intended
26-file delta. Post-merge compare reports behind=0 and does not show Cathedral or
Gallery files in the PR diff. This specifically guards against repeating the
Sep-19 temporal rollback failure mode.

## What remains before merge

1. Read the authoritative PR #273 GitHub Actions result.
2. If red, inspect only the failing job/step/log and patch the specific defect.
3. Re-run until green.
4. Update this handoff/plan with verified evidence rather than forecast.
5. Mark PR ready for review only after the focused CI witness is green.
