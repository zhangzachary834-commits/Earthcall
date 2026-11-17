# C5b Ray Interval Scheduler — Architecture Receipt

**Date:** 2026-11-05  
**Branch:** `prism/general-volume-cohort-architecture-reconciled`

## Status

C5b-1 is implemented as ray-lifetime proof/schedule IR. It does **not** yet claim arbitrary cross-family WebGPU execution.

## New substrate

`src/Singularity/Screen/VolumeCohortIntervalSchedule.hpp` accepts conservative positive-support intervals for two execution families and partitions one camera ray into maximal near-to-far runs:

- `First`
- `Second`
- `Joint`

The motivating chronology is representable directly:

[
A ightarrow B ightarrow A
]

without widening the whole ray to joint transport.

Overlap is localized. For example:

[
A=[0,2],quad B=[1,3]
]

becomes:

[
A_{[0,1]} ightarrow Joint_{[1,2]} ightarrow B_{[2,3]}.
]

Nested support similarly becomes outer-family / local-joint / outer-family.

## Prism distinctions

[
	ext{screen-region chronology}

eq
	ext{same-ray interval chronology}
]

and

[
	ext{C5a framebuffer publication order}

eq
	ext{C5b stateful ray-consumer order}.
]

C5a composes independent premultiplied RGBA publications far-to-near.

C5b schedules one continuing ray transport state near-to-far, because transmittance state advances along the ray.

`executeVolumeCohortIntervalSchedule()` therefore requires one consumer to carry the same radiance/transmittance state across all runs. Restarting transport independently at every run is not silently treated as equivalent.

## Refusal law

Invalid proof input is uncertainty, never absence.

C5b refuses the schedule when:

- an interval is non-finite;
- `hi <= lo`;
- `lo < 0`;
- an interval midpoint cannot be represented;
- the bounded run budget is exceeded.

On refusal, the caller must retain joint transport over its authoritative ray segment.

## Witnesses

`tests/singularity/volume_cohort_interval_schedule_test.cpp` covers:

1. exact `A -> B -> A`;
2. local overlap becoming `A -> Joint -> B`;
3. nesting becoming `A -> Joint -> A`;
4. serialization-order invariance;
5. adjacent same-family coalescing;
6. valid empty support;
7. malformed-proof refusal;
8. bounded-complexity refusal;
9. stateful near-to-far execution order.

The repository's existing `file(GLOB_RECURSE TEST_FILES "tests/*.cpp")` configuration auto-discovers this test.

## Existing routed precedent

The routed backend already carries the conceptual shape required by C5b inside one program:

`ECSpan { span, member }`

and `ecConsume` advances one transport state through chronologically ordered subpanels.

That does **not** yet solve cross-family scheduling. Routed and general OntoMath execution remain separate physical programs, and the renderer has no per-ray subinterval bridge between them.

## C5b-2 boundary

The next implementation must lower interval runs into a GPU consumer representation that can:

1. restrict each execution family to the scheduled subinterval;
2. preserve transport composition across run boundaries;
3. execute local `Joint` runs without widening the whole ray;
4. refuse locally if a required segment program cannot be formed;
5. avoid repeating full-ray work for every interval.

More scissors are not the answer: a scissor selects pixels, not subsegments of one ray.

## Evidence boundary

Source and hostile witnesses are committed.

A direct local checkout/compile attempt was blocked because the execution container could not resolve `github.com`. No native WebGPU parity, M5 performance, or Person-FPS claim is attached to C5b-1.

GPT-5.6 Sol / Original Prism Sun / 2026-11-05. Human origin and acceptance authority: Zachary Zhang.
