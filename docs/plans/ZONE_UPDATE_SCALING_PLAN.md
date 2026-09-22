# `Zone::update` Scaling — Implementation Plan

**Date**: 2026-08-26
**Author**: Claude Opus 5, from a measured investigation of "60 toruses make the frame jagged"
**Status**: Plan only. Nothing here is implemented. No save file or source was modified.
**Probe**: `scratch/probes/torus_frame_cost_probe.cpp` (new, untracked — the measurements below)
**Reads**: `tests/singularity/frame_lag_test.cpp`, `docs/CPU_GPU_MICRO_MASTERY_REMEDIATION_PLAN.md`

---

## 0. What was measured, and what it ruled out

Zach spawned 50–60 toruses through **Creator Console → Create → Torus**
(`Create3DConsole.cpp:176` → `ShapeKind::Torus` → `setSmoothSurface(geom::makeTorus(...))`)
and the frame went "slower and jagged". Not the implicit-expression route in the lower
panel (`Create3DConsole.cpp:331`) — a different path, not measured here.

The obvious explanation was that toruses raymarch per fragment and the heavy GPU probe
(512x512, camera at z=150, sphere SDFs) was fragment-blind. **That explanation is wrong.**

```
A. 60 objects at the heavy probe's own conditions
   60 cubes     512x512  cam z=150   frame 0.59 ms (1706 fps)  draws=1
   60 toruses   512x512  cam z=150   frame 3.77 ms ( 265 fps)  draws=60

B. The same 60 toruses at a real window
   60 toruses  1920x1080 cam z=8.0   frame 3.81 ms ( 262 fps)  draws=60
   60 toruses  3840x2160 cam z=8.0   frame 3.79 ms ( 264 fps)  draws=60
```

Resolution-independent across an 8x pixel range, so not fragment-bound. Also ruled out:

- `sdfFromSmooth` + `sdfwgsl::compile` regenerate 8,535 bytes of WGSL **per object per
  frame**, but cost 0.0019 ms each — 6.7 ms per *second* at 60 objects. Noise.
- `pickSurface` over all 60 toruses: 0.096 ms/frame (cubes: 0.065 ms). Noise.

**60 toruses render at 255 fps in isolation. The renderer is not the cause.**

What `frame_lag_test` — which already splits the frame by phase — has been reporting:

```
    64 objects -> frame  1.143 ms  (zone  1.051  relations 0.000  law 0.091)
   128 objects -> frame  3.331 ms  (zone  3.141  relations 0.000  law 0.176)
   256 objects -> frame 11.117 ms  (zone 10.745  relations 0.001  law 0.355)
   512 objects -> frame 40.729 ms  (zone 39.982  relations 0.001  law 0.739)

  STANDING  Zone::update grows as n^k, k = 1.752  (aspiration 1.150, baseline 1.760)
  ok        LawManager::tick grows as n^k, k = 1.006
```

`Zone::update` is ~98% of the frame and grows at **n^1.75**. `LawManager::tick` is clean
linear. Superlinear growth is what "jagged" feels like: each object costs more than the last.

**The uncomfortable part**: the micro-mastery work optimized GPU submission, and at a few
hundred objects submission was never the binding term — at 512 objects it is a rounding
error beside 40 ms of `Zone::update`. The heavy probe looked excellent because 4,500 objects
is where submission finally dominates. No Person's world has 4,500 objects. The probe
measured the term that was not binding.

---

## Phase 0 — Split `Zone::update` in the harness

**Do this first.** Everything below is a hypothesis about *which* loop until the harness
says so. `frame_lag_test.cpp` already states the principle: *"'the frame is slow' is not a
finding — 'the frame is slow **in collision**' is."* It then stops one level short: `zone`
is a single undifferentiated number.

`FrameResult` (`frame_lag_test.cpp:183`) carries `zoneMs / relationsMs / lawMs / totalMs`.
Split `zoneMs` to match the four things `Zone::update` (`Zone.cpp:379`) actually does:

| Field | Covers | Where |
|---|---|---|
| `groundScanMs` | the `baseline`-attribute scan for the floor | `Zone.cpp:383-390` |
| `rotationMs` | `updateRotation` over objects with pending rotation | `Zone.cpp:401-403` |
| `automationMs` | `updateAutomations` | `Zone.cpp:404-406` |
| `physicsMs` | `Physics::updateBodies` | `Zone.cpp:408-411` |

Also record `substeps` — see Phase 3; it is not constant and it multiplies everything above.

`Zone::update` has no seams for this today. Add them the cheapest honest way: a
`Zone::UpdateTiming` struct filled only when a pointer is passed
(`void update(float dt, UpdateTiming* out = nullptr)`), so the frame path pays nothing when
nobody is measuring. **Do not** add a global timing singleton — the six refusals aside, a
hidden accumulator is the shape of Refusal #6.

Then run the existing scaling harness. The 64/128/256/512 table will name the loop.

**Exit test**: the scaling table prints a growth exponent per sub-phase, and exactly one of
them is meaningfully above 1.0. Everything below assumes it is `physicsMs`; **if the harness
says otherwise, stop and re-plan** — this is a measurement, not a prediction.

---

## Phase 1 — The constant factor inside the all-pairs loop

Assuming Phase 0 fingers physics, the loop is `Physics.cpp:327` (outer) and `:343` (inner).
It is genuinely all-pairs, but two things that do not depend on `j` are being recomputed
inside the inner loop:

**1.1 — B's AABB is rebuilt per pair.** `Physics.cpp:347-351` walks
`b->collisionZone.corners` for every `(i, j)`, making the pre-filter O(n² · corners) rather
than O(n²). A's AABB is correctly hoisted to `:338-342`; B's is not.

*Change*: precompute all AABBs once per substep into a `std::vector<std::pair<vec3,vec3>>`
indexed alongside `objects`, before the outer loop. Both loops then read it.

**1.2 — the collision-law scan runs per pair.** `Physics.cpp:359-365` walks every law for
every pair that survives the AABB filter.

*Change*: `anyCollisionLaw` is already hoisted (`:325`). Hoist the per-object answer too —
compute `objectMatchesTarget` once per object per substep into a `std::vector<bool>`, then
the pair test is `allowedA[i] || allowedB[j]`.

Neither change alters behavior; both are pure constant-factor. Expect a large multiple, not
an exponent change — this is the cheap win, and it lands before the structural one.

**Verify**: `frame_lag_test` scaling table before/after; `ground_plane_test`,
`action_spawn_test`, and the collision-touching event tests still pass (`contact-ended`
publication at `Physics.cpp:437-446` must be byte-identical — it is an event edge, and
Events-must-be-edges is a non-negotiable).

---

## Phase 2 — Break the n² with a broad phase

Phase 1 makes the quadratic cheaper; it stays quadratic. To move the exponent, the pair
enumeration itself has to go.

*Change*: a uniform spatial hash keyed on the AABB grid cell, rebuilt once per substep
(O(n)), then pairs enumerated only within and between neighbouring cells. For scattered
world objects this is ~O(n). Sort-and-sweep on one axis is the simpler alternative and is
enough to get the exponent under 1.15.

**⚑ AUTHOR — Zach's call.** A broad phase is the renderer-batching question again in
another subsystem: a mechanism deciding which beings are *considered* together. My reading
is that it is clean — it changes only which pairs are *tested*, never which collide, so the
world's behavior is identical and it is "how the machine senses", not "what a thing is". But
it should be said in the physics doc before it lands, not found in review. The exit test is
strong and should be written first: **the same scene must produce the identical set of
collision pairs with and without the broad phase**, asserted over a randomized scene.

**Verify**: growth exponent for `physicsMs` under 1.15 (the harness's own aspiration), plus
the pair-set equivalence test above.

---

## Phase 3 — The substep feedback loop, which is the "jagged"

`Zone.cpp:392-396`:

```cpp
const float maxStep = 0.02f;
const float maxFrameTime = 0.1f;
if (dt > maxFrameTime) dt = maxFrameTime;
int steps = std::max(1, (int)std::ceil(dt / maxStep));
```

Everything in Phases 1–2 runs `steps` times (`Zone.cpp:398`). At 60 fps, `dt` = 16.7 ms and
`steps` = 1. **Cross ~20 ms and `steps` becomes 2 — physics cost doubles instantly**, which
pushes `dt` higher, which is already the next frame's input. It is clamped at 5 substeps by
`maxFrameTime`, so the amplification is bounded at 5x, but the cliff at 20 ms is a step
function sitting exactly where a loaded world lands.

That is the mechanism behind "slower **and jagged**", as distinct from merely slower: the
frame does not degrade smoothly, it doubles its own work the moment it misses.

*Change*: decouple simulation rate from frame rate — accumulate elapsed time and run a
fixed number of fixed-size steps per frame, carrying the remainder, with a hard cap on
steps per frame that **drops** simulation time rather than spending more of it. This is the
standard fixed-timestep accumulator and it removes the positive feedback.

**⚑ AUTHOR.** Dropping simulation time is an ontological statement: under load the world's
clock runs slower than the Person's. The alternative — always spending the time — is what
produces the spiral. Earthcall already has a `Universe` clock and a `Moment` ontology, so
this decision belongs with them, not buried in a physics constant. Worth a paragraph in the
Time framework that `b87aeceb` opened.

**Verify**: a harness case that feeds deliberately long `dt` values and asserts the frame
cost does not step-change; `time_flow_test` still passes.

---

## Phase 4 — Make identical shapes batch

Independent of the above, and the one torus-specific item.

Cubes collapse to **1 draw call**; 60 toruses take **60**. `42be002e` already documented
why: the instanced batch groups by `TessMesh` *pointer*, and `Object::rebuildGeometryCaches()`
gives every Object its own `_smoothMesh` (`Object.hpp:174`), so N separately-authored
identical toruses own N distinct addresses and never batch.

Cost is linear at ~0.065 ms/torus/frame (measured: 1 → 0.22 ms, 30 → 2.03, 60 → 3.92,
120 → 7.63). At 60 objects this is 3.9 ms — real, worth removing, and **not** what makes the
frame jagged. Sequence it after Phases 0–3 so it is not mistaken for the fix.

*Change*: a content-addressed tessellation cache. Key a shared `TessMesh` on the
`SmoothSurfaceData` that produced it (form, axes, params, zTrim, pkind) so every torus with
identical parameters resolves to one instance; the Object holds a `shared_ptr` to it. Then
the existing pointer-keyed batch path collapses them for free, with no renderer change.

**Trap**: this makes meshes shared, which puts them under the same rule as Materials —
*paint is on the Material, and materials are shared*. Any writer that mutates a `TessMesh`
in place must diverge first, exactly as `Object::ownMaterial` does. The `revision` counter
added in `a3e7fc66` is the existing hook; the plan's own note there said no current writer
mutates in place. **Re-verify that before sharing meshes**, because sharing turns "no writer
does this" from a tidiness matter into a correctness one.

**Verify**: 60 toruses report `draws=1`; a scene with two different-radius toruses reports
`draws=2`; `webgpu_sdf_parity_test` and `webgpu_object_test` still pass; re-run
`scratch/probes/torus_frame_cost_probe.cpp` and record the new numbers.

---

## Phase 5 — Make the harness able to fail

`frame_lag_test` currently prints:

```
STANDING  Zone::update grows as n^k, k = 1.752  (aspiration 1.150, baseline 1.760)
```

`STANDING` is neither pass nor fail. A green suite is therefore compatible with near-quadratic
world update — which is exactly the state the last three weeks shipped in. The design is
deliberate and mostly right (`frame_lag_test.cpp:27`: *"Nothing here fails for merely being
slow today"*), and the ratchet-against-baseline behavior should stay.

*Change*: keep the ratchet, but let a growth **exponent** be judged differently from an
absolute duration. A duration that is slow today is a fact about the machine; an exponent of
1.75 is a fact about the algorithm and does not improve on faster hardware. Once Phase 2
lands and the exponent is under the aspiration, re-record the baseline and make an exponent
regression a real failure.

Until then, at minimum: print the exponent line **last** and mark it loudly, so it is not
the thing a reader's eye slides past on a 70/70 run.

---

## Ordering

| Phase | Depends on | Done when |
|---|---|---|
| 0 Split `zoneMs` | — | exactly one sub-phase shows k > 1.0; the loop is named, not guessed |
| 1 Hoist AABB + law scan | 0 | large constant-factor drop; collision events byte-identical |
| 2 Broad phase | 0, 1 | `physicsMs` exponent < 1.15; pair-set equivalence test passes |
| 3 Fixed-timestep accumulator | 0 | long-`dt` case shows no step-change in frame cost |
| 4 Content-addressed tessellation | — | 60 toruses report `draws=1`; divergence-on-write verified |
| 5 Harness can fail on exponent | 2 | baseline re-recorded; exponent regression fails the suite |

Phase 4 is independent and can run in parallel. **Phase 0 blocks 1, 2, and 5** — the whole
point is to stop guessing which loop, and this plan's own Phase 1 assumption is a hypothesis
until the harness confirms it.

---

## Not in scope, deliberately

- **Anything in the renderer.** It was measured and it is not the cause. Phase 4 is a
  batching win, not a fix for the reported symptom, and the plan says so.
- **Retiring the legacy physics engine.** `Zone.cpp:408` gates all of this on
  `Physics::getLegacyEngineEnabled()`. Whether that engine should exist is a real question
  and not this plan's; every change here must keep working under the flag as it stands.

---

## The human thread

The symptom, the reproduction, and the clarification that this was the Creator Console's
**Create → Torus** template rather than the implicit-expression panel are Zach's — that
distinction is what made the measurement point at the right path. The observation that the
frame is "slower **and jagged**" rather than just slower is his too, and it is the detail
that led to Phase 3: uniform slowness and a step-function cliff have different causes, and
he described the cliff.

The phase-split principle in Phase 0 is quoted from `frame_lag_test.cpp:180`, which is
already in the tree and already right — it just stops one level above where the cost lives.
The measurements, the ruling-out of the renderer, the substep feedback analysis, and the
phase ordering are mine.
