# C5b-2b1 — Routed Span Producer / Consumer Fission Receipt

**Date:** 2026-11-05  
**Branch:** `prism/general-volume-cohort-architecture-reconciled`

## Why this rung exists

C5b-2a established one fused WebGPU transport state for mixed execution families, but First-family work still used the general V5 evaluator.

The existing `KernelRoutedTransport` already derives the richer object C5b needs before it integrates anything:

- per-ray setup and opaque-depth clip;
- tile route classification;
- bounded routed producers;
- ordered `ECSpan` streams in `ecSingles` and `ecOverlaps`.

Historically those producer artifacts were immediately consumed by `ecConsume` and collapsed to a viewport-sized premultiplied RGBA publication. That made “routed transport” appear to be one indivisible full-ray renderer.

C5b-2b1 separates those two lifetimes.

## New API

`KernelRoutedTransport::encodeSpans(..., KernelRoutedSpanConsumer)`

runs the same production path through:

[
setup ightarrow classify ightarrow routeArgs ightarrow S1/S2/fallback producers
]

but does **not** dispatch `ecConsume`.

Instead, after each strip's producers finish, it invokes a caller-supplied consumer with a borrowed `KernelRoutedSpanStrip` view.

The existing `encode(...)` remains the integrated-publication path and still dispatches `ecConsume`.

Both routes share `encodeInternal(...)`, so setup, classifier, producer admission, work ceilings, refusal flags and scratch layouts do not fork into a second implementation.

## Strip-lifetime law

The routed span buffers are reusable scratch, not a durable frame publication.

A `KernelRoutedSpanStrip` contains:

- strip index;
- framebuffer start row;
- valid row count;
- ray scratch capacity;
- the strip's `ECWindow` uniform range;
- borrowed `rays`;
- borrowed `singles`;
- borrowed `overlaps`.

The consumer may record GPU commands that read those ranges **during the callback**. It must not assume their contents survive after the callback returns.

For an external strip consumer, producer dispatches are emitted as closed compute passes before the callback. Therefore the callback may legally open its own compute or render pass. Command-buffer order guarantees that its recorded reads execute before the next strip's producers reuse the scratch.

Ordinary integrated publication retains the existing coalesced compute-pass path.

## Why no full-frame span buffer was added

At native 2560×1440, persisting dozens of possible routed intervals per pixel would turn the proof stream into a very large frame allocation and would destroy the lifetime advantage of the existing strip design.

C5b needs:

[
produce strip_n
ightarrow
consume strip_n
ightarrow
reuse scratch
]

not:

[
produce entire frame
ightarrow
store every interval
ightarrow
consume later.
]

The latter would confuse a local execution program with a frame cache.

## Refusal behavior

- An empty callback is refused by `encodeSpans`.
- Existing routed resource/admission failures remain failures.
- If the external strip consumer returns false, span encoding stops and returns false.
- No callback failure is interpreted as empty support.
- `encodeSpans` does not claim or populate the final routed RGBA publication through `ecConsume`.

## Witness

`webgpu_volume_routed_span_contract_test.cpp` pins the public strip-view type and exact scratch byte geometry:

- rays: `capacity * 48`;
- singles: `capacity * 64 * 16`;
- overlaps: `capacity * 16 * 16`;
- strip uniform view: 48 bytes.

It also pins the borrowed callback type.

Source audit additionally confirms:

- `encode()` calls `encodeInternal(..., nullptr)`;
- `encodeSpans()` calls the same implementation with the span consumer;
- coalesced mode is retained only for ordinary integrated publication;
- the callback occurs after all S1/S2/fallback producers for its strip;
- `ecConsume` remains only on the non-span branch.

## What this rung does not claim

There is no production renderer caller of `encodeSpans()` yet.

Therefore C5b-2b1 does **not** claim:

- mixed-frame routed interval acceleration;
- changed Northern Veil FPS;
- native Metal parity;
- a fused consumer reading routed spans;
- replacement of general First-family evaluation.

The next rung is C5b-2b2: the mixed fused transport path must consume the borrowed routed span stream strip-by-strip while preserving its one continuing (L,T) state.

That consumer must fail open to the existing general First-family path on routed refusal and must not move the accepted V5 quadrature lattice merely because the routed producer found a tighter support program.

GPT-5.6 Sol / Original Prism Sun / 2026-11-05. Human origin and acceptance authority: Zachary Zhang.
