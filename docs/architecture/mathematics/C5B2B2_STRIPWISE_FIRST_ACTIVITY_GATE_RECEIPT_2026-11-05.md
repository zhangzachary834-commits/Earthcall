# C5b-2b2 — Stripwise First-Activity Gate Receipt

**Date:** 2026-11-05  
**Branch:** `prism/general-volume-cohort-architecture-reconciled`

## Status

C5b-2b2 is the first production bridge where routed mathematics can remove First-family evaluator work **inside a mixed fused V5 ray** while preserving V5's existing sample lattice and one continuing transport state.

It is an activity gate, not yet routed quadrature/value substitution.

## Dataflow

[
	ext{routed First-family producer}
ightarrow
	ext{ECSpan strip stream}
ightarrow
	ext{conservative activity compactor}
ightarrow
	ext{one }u32	ext{ per framebuffer pixel}
ightarrow
	ext{mixed fused V5 sample gate}
]

For each V5 sample:

- Second-family members execute exactly as before.
- First-family members execute only when the conservative routed activity bit says **maybe active**.
- A zero bit means the routed proof stream established no First-family positive-support span in that canonical distance bin.
- A one bit means maybe active, overlap, refusal, or uncertainty and therefore retains the ordinary V5 evaluator.

Thus the optimization direction is one-sided:

[
0 Rightarrow 	ext{proved First-inactive}
]

but

[
1 
otRightarrow 	ext{proved active}.
]

False positives cost performance only. False negatives are forbidden.

## Canonical distance coordinate

The first compactor draft considered normalizing activity bins to the routed cohort's own ray envelope. That is unsound for a mixed V5 set because the routed cohort and the full mixed set can have different AABB-derived ([t_0,t_1]) envelopes.

C5b-2b2 therefore uses a cohort-independent coordinate:

[
s in [0,2048].
]

The routed setup already refuses rays whose accepted endpoint exceeds 2048, so the compactor and mixed V5 consumer can share this absolute camera-distance domain without borrowing either cohort's local envelope.

There are 32 bins. Each bin spans 64 world-distance units.

The compactor marks every bin touched by every routed single or overlap span. Boundary rounding is deliberately conservative and may mark one additional bin.

Malformed/out-of-domain spans refuse to:

[
0xffffffff.
]

## Durable certificate, stripwise production

The routed producer's raw `rays`, `singles`, and `overlaps` remain strip-lifetime scratch.

C5b-2b2 does **not** retain them for the whole frame.

Instead:

[
Produce(strip_n)
ightarrow
Compact(strip_n)
ightarrow
ReuseScratch(strip_{n+1}).
]

The compacted result is a persistent device buffer:

`KernelRoutedTransport::firstActivityMask`

with one `u32` per framebuffer pixel.

At 2560×1440 this is:

[
2560 	imes 1440 	imes 4
=
14{,}745{,}600	ext{ bytes}
approx 14.1	ext{ MiB}.
]

This is materially smaller than retaining dozens of routed span records per pixel.

## One producer, two possible consumers

A frame can need both:

1. routed RGBA publication for screen regions C5a proves independently composable; and
2. the First-family activity certificate for ambiguous mixed regions.

C5b-2b2 does not run routed span production twice.

`KernelRoutedTransport::encodeWithSpans()` performs:

[
producer
ightarrow
activity compactor
ightarrow
existing ecConsume
]

for each strip before scratch reuse.

`encodeSpans()` omits `ecConsume` when the routed pipeline exists only as an execution-proof source.

Ordinary `encode()` remains the established integrated routed publication path.

## Fused V5 consumption

Activity-gated mixed programs expose:

`Program::kernelVolumeUsesFirstActivityMask`.

Their group-0 binding 3 is the conservative activity mask. Routed publication programs also use binding 3, but the two modes are mutually exclusive at pipeline construction.

The fused shader:

1. loads one activity word for the pixel;
2. maps the existing `sampleT` to the canonical 32-bin ([0,2048]) domain;
3. derives `ecC5bFirstActive`;
4. adds that guard only to First-family member evaluation.

The shader does **not** add:

- AABB events;
- support events;
- sample points;
- segment splits;
- a second transmittance state;
- a second radiance state.

The accepted V5 quadrature lattice is unchanged.

## Fail-open / stale-state law

Unknown is never interpreted as absence.

Routed/compactor refusal writes or retains an all-ones certificate so the mixed V5 path evaluates First-family members normally.

If the activity source is required but source preparation/encoding never reaches the compactor, the renderer marks the certificate all-ones before the draw. It therefore cannot reuse a previous frame's zero bits as present truth.

The all-ones host upload is a rare refusal path, not a normal per-frame clear.

## Program/cache identity

The activity-mask lane is shader structure.

The fused program identity distinguishes:

- grouped families with no activity mask;
- grouped families with the activity mask.

A pipeline compiled without binding 3 cannot alias one compiled with it.

## Tests and source witnesses

`volume_cohort_gpu_lowering_test.cpp` pins:

- activity binding emission;
- pixel-word load;
- canonical `sampleT * (32 / 2048)` mapping;
- First-family-only gating;
- Second-family non-gating;
- one continuing V5 transmittance state;
- unchanged authored AABB event lattice;
- refusal when the activity mask is requested without grouped execution.

`webgpu_volume_routed_span_contract_test.cpp` pins:

- all-ones refusal semantics;
- exact-zero valid-empty semantics;
- both singles and overlaps consumed;
- absolute 2048-domain mapping;
- absence of cohort-local `(s-t0)/(t1-t0)` normalization.

The routed compactor pipeline is released through the same owner transaction as the other common routed kernels.

## What C5b-2b2 does not claim

C5b-2b2 does **not** yet use routed quadrature values in place of the First-family V5 evaluator.

It only skips First-family evaluation where routed mathematics proves the sample's canonical bin inactive.

Therefore the next optimization rung is C5b-2b3:

- lower routed span/quadrature quanta into the continuing mixed transport state;
- preserve Second-family and Joint V5 algebra;
- retain the same truth/refusal direction;
- prove parity before changing the accepted numerical lattice;
- demonstrate economics on M5 / native Northern Veil.

## Evidence boundary

Source, cache identity, GPU compactor plumbing, renderer scheduling, and codegen/contract witnesses are committed and source-audited.

No native Metal/Person FPS result is inferred by this receipt. CI/workflow state is checked separately at the branch tribunal.

GPT-5.6 Sol / Original Prism Sun / 2026-11-05. Human origin and acceptance authority: Zachary Zhang.
