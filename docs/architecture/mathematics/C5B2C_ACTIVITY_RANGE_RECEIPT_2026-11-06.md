# C5b-2c — Camera-Relative First-Activity Quantization Receipt

**Date:** 2026-11-06  
**Branch:** `prism/general-volume-cohort-architecture-reconciled`

## Problem

C5b-2b2 already gives mixed First/Second transport a real producer-to-consumer proof bridge:

1. the routed First-family producer emits conservative ray spans;
2. those spans compact into one durable 32-bit activity word per pixel;
3. the fused V5 mixed-family fragment reads that word;
4. a zero bit may skip First-family sample work while one continuous `(L,T)` state remains authoritative.

The original compactor mapped all 32 bits over the fixed camera-distance domain `[0,2048]`.

That is safe, but coarse: one bit may cover roughly 64 world units even when the entire First cohort occupies a much smaller radial shell.

## C5b-2c law

Keep the same 32-bit certificate and the same transport algebra, but quantize it over the current camera-relative conservative radial support of the First capability cohort:

[
[r_{near}, r_{far}]
]

where the range is derived by `aggregateVolumeCohortSupport(routedCohort, radialSupportFor)`.

This range is query proof state, not authored near/far identity.

## One range, two consumers

The renderer computes one f32 pair `c5bActivityNear/c5bActivityFar`.

The pair is outward-rounded when double-to-float conversion would otherwise narrow it.

The exact same two floats are sent to:

- the routed span compactor, via the existing strip uniform `ECWindow.flags.y/z` as f32 bit patterns;
- the fused V5 fragment, via the previously reserved `VolumeGlobals.volumeOutputError.z/w` lanes.

No new GPU buffer is introduced.

## Producer behavior

`kFirstActivityMaskWgsl` now decodes:

- `quantLo = bitcast<f32>(ecWindow.flags.y)`
- `quantHi = bitcast<f32>(ecWindow.flags.z)`

and maps each routed span into the 32 activity bits relative to that range.

A malformed range or a produced span escaping the claimed conservative range refuses the certificate for that pixel by writing:

`0xffffffffu`

Unknown therefore remains maybe-active. A proof mismatch can increase work, never erase transport.

## Consumer behavior

The fused V5 fragment reads the same runtime range from `u.volumeOutputError.z/w`.

For a sample inside that valid range it computes the corresponding activity bit and may skip First-family work only when the bit is zero.

If the range is invalid or the sample falls outside its tolerance-expanded domain, the fragment sets First activity to true and evaluates normally.

The C5b-2c gate therefore remains fail-open.

## Lifetime and invalidation

The activity range is camera/query derived.

`KernelRoutedTransport` tracks the exact f32 near/far pair beside its existing strip-uniform cache inputs. A changed pair rewrites the strip uniform before the routed producer/compactor runs.

A moving camera therefore cannot reuse a mask under stale quantization coordinates.

## Preserved invariants

C5b-2c does not change:

- authored field order;
- V5 AABB event boundaries;
- V5 sample coordinates or spacing;
- per-medium evaluators;
- First/Second capability membership;
- transport algebra;
- one-state `(L,T)` continuity;
- the 32-bit-per-pixel activity storage size;
- fail-open refusal behavior.

It only increases the spatial resolving power of the existing conservative absence certificate.

## Witness

`volume_cohort_gpu_lowering_test.cpp` now pins that:

- the fused activity lane reads `u.volumeOutputError.z/w`;
- sample binning uses `sampleT - ecC5bQuantLo`;
- fixed `sampleT * (32/2048)` mapping is absent;
- the producer reads the same range from `ECWindow.flags.y/z`;
- producer span quantization uses `lo - quantLo`;
- fixed `lo/2048` mapping is absent;
- a routed span outside the claimed range causes certificate refusal rather than a zero bit.

## Evidence boundary

Source changes and codegen/source witnesses are committed.

No native Metal parity, runtime activity-mask density, GPU timing, or Person-FPS result is claimed by this receipt.

C5b-2c is a proof-resolution improvement. Performance remains an empirical question for M5/Person verification.

GPT-5.6 Sol / Original Prism Sun / 2026-11-06. Human origin and acceptance authority: Zachary Zhang.
