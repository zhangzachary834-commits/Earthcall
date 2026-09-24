# SUN HANDOFF — Already-Known Execution-Key Consumer After PR #350

Date: 2026-09-24  
Canonical branch: `sync-from-earthcall-main`  
Parent analysis: `docs/analysis/rendering_relevance_economics_after_pr329_2026-09-23.md`  
Parent bounded successor: PR #350 — Rendering relevance economics

## Why this successor exists

The post-PR329 relevance-economics investigation is complete.

Its decisive result was not that prophetic rendering failed. The result was narrower and more useful:

> Prediction is valuable when it can be fused into an execution identity the machine already possesses. Prediction becomes expensive when the machine must repeatedly search the world to discover which prediction is relevant.

The maintained single-authored-Perlin ray-march workload failed economically because theorem applicability had to be rediscovered spatially at each sample. Generic proof traversal was ~34–38% slower, and hidden relevance work overwhelmed the exact evaluations saved.

Do **not** reopen that road unless new evidence changes the premise.

This successor begins at the remaining promising boundary: **already-known execution keys**.

## Mission

Find the smallest real renderer/transport consumer where Earthcall already knows the semantic execution identity independently of theorem lookup, then test whether Scene-DAG / prophetic semantic metadata can be attached directly to that identity with no hidden relevance search.

The candidate consumer must be real enough that its key exists in production control flow for reasons other than proof lookup.

Examples worth auditing:

- a known radiance-source binding;
- a known medium binding;
- an already-selected Object/program;
- an already-known semantic channel;
- an already-selected compiled authored subexpression;
- another stable execution slot that the renderer/transport path already possesses.

Do not assume which one wins. Audit first.

## Hard constraints

Preserve all of these:

1. **Exact authored mathematics is sovereign.** Unknown/stale/unsupported metadata fails open to exact execution.
2. **PR329 remains zero-authority until this successor earns a separately measured promotion.**
3. **No hidden relevance search.** A candidate that needs a spatial lookup, hierarchy walk, record scan, semantic hash search, or variable-length candidate discovery merely to find its theorem slot is not the target.
4. **Stable identity is required before authority.** Raw `Piecewise*` + revision is diagnostic identity only. Address reuse, removal/re-addition, stale slot reuse, and producer-contract errors must be hostile test cases.
5. **Channel sovereignty.** Equal canonical math does not merge authority across `SourceRho`, density, extinction, scattering, chroma, phase/directionality, or emission.
6. **V1–V4 volumetric semantics remain independent.** In particular, zero density must never erase independently authored V4 self-emission.
7. **Incremental repair, never global rebuild by default.** A local authored mutation should invalidate/repair only its dependency frontier. Unaffected compiled semantic state should remain intact.
8. **No synthetic benchmark theater.** Do not invent a magic integer key in a test if production must perform extra work to derive that key.

## First bounded rung

Before changing pixels, perform an audit and test-only witness around exactly one already-known execution key.

The first rung should answer four questions:

### A. What is the key?

State exactly which production identity is already known at the decision point.

Bad answer:
`world/sample position -> search -> key`

Good answer:
`already-selected source/medium/program/channel slot -> metadata`

### B. What is the conservative action?

Keep the first theorem/action deliberately tiny.

A good first action is one where exact behavior is obvious and fail-open semantics are trivial to verify, e.g. conservative source/medium admission or another no-op/skip that cannot cross semantic channels.

Do not broaden theorem vocabulary just to make the demo impressive.

### C. Is dispatch actually direct?

Instrument and report:

- dispatch lookups;
- metadata tests/branches;
- record scans;
- hierarchy walks;
- hash probes/searches;
- exact evaluations avoided;
- artifact build time;
- incremental repair time;
- artifact bytes;
- resident/upload bytes if applicable.

A direct-dispatch claim requires zero hidden relevance search on the hot path.

### D. Does hostile identity fail open?

Test at minimum:

- authored premise mutation;
- revision change;
- source/medium removal and re-addition;
- simulated slot/address reuse;
- deliberately stale artifact;
- unrelated channel with byte-identical math;
- independent V4 emission present while density theorem says zero.

Every stale/ambiguous case must fall back to exact authored execution.

## Graduation gate

The first test-only witness may earn a pixel-authoritative native A/B only if all are true:

- the key is already present in production execution independently of theorem lookup;
- hot-path decision cost is fixed/bounded and independent of theorem/world count;
- no hidden spatial/semantic search exists;
- hostile lifetime/identity cases fail open;
- local edits preserve unaffected artifact state;
- semantic channels remain independent;
- full accounting shows a plausible positive margin.

If the witness passes, **then** run a native exact-vs-authoritative A/B for that one consumer and measure wall/GPU/CPU/residency effects before promotion.

If it fails, write the negative result and try a different already-known boundary only if the failure teaches something genuinely new. Do not mutate into another generic search architecture.

## Explicitly rejected roads from PR #350

Do not spend this successor re-proving any of the following unless the premise materially changes:

- production NO-PROOF shader fork from dormant WGSL size alone;
- generic per-sample Perlin proof consultation;
- range-grid traversal as theorem discovery;
- direct-run candidate scans that hide record tests;
- synthetic direct-dispatch keys that omit production key derivation;
- “more theorem breadth” as a substitute for an economical consumer.

Those roads are documented in:

`docs/analysis/rendering_relevance_economics_after_pr329_2026-09-23.md`

## Suggested audit order

1. Re-read current canonical and the parent analysis.
2. Re-read the latest relevant Agent Intercom updates around PR #329/#350 only; no Big Chungus dumps.
3. Inspect production call sites for `RenderedFieldSemanticObserver`, radiance source binding, volume-medium binding, and whichever transport/program dispatch currently already carries stable-ish identity.
4. Pick the **smallest** candidate whose key truly already exists.
5. Build a test-only direct-dispatch witness beside the relevant semantic/transport tests, not by inserting authority into the hottest shader loop first.
6. Add hostile identity/lifetime tests and full accounting.
7. Decide promotion vs rejection from evidence.
8. Update Agent Intercom with exact evidence, rejected hypotheses, and the next continuation point.

## Branch / PR policy

Create **one** successor branch/PR for this handoff and keep all hourly continuations on that same branch/PR until the bounded successor is genuinely finished or superseded.

Do not spawn a new branch/PR every hour.

Each run must begin by re-reading:

- current `sync-from-earthcall-main`;
- this handoff;
- latest Agent Intercom update for the successor;
- active successor PR/branch;
- exact-head CI.

Other Suns may advance canonical.

## Definition of done

This successor is done when one of these is true:

### Positive completion
A real already-known execution-key consumer has:
- no hidden relevance search;
- hostile fail-open identity semantics;
- local incremental repair;
- full economic accounting;
- native exact-vs-authoritative A/B;
- a clear measured verdict on whether it earns authority.

### Negative completion
The audited real candidate(s) cannot meet the no-hidden-search / identity / economics gate, and the reason is documented precisely enough that another Sun should not repeat the same road.

When definition of done is reached:
- write a final analysis document under `docs/analysis`;
- write a final Agent Intercom verdict;
- stop/disable the successor automation;
- do not invent another rung merely to keep the Sun alive.

## First instruction to the next Sun

Start from this handoff and the parent analysis. Audit production for the smallest already-known semantic execution key. Prefer a source/medium/program/channel boundary over any spatial sample-position road. Build no authority until the direct-key and hostile identity tests are green. Then measure, not assume.
