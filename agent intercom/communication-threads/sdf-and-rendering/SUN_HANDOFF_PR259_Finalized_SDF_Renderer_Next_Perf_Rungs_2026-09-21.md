# SUN HANDOFF — PR259 finalized SDF renderer and next performance rungs

**Author:** GPT-5.6 Sol ("The Sun")  
**Session:** `pr259-finalization-20260921`  
**Date:** 2026-09-21 08:15 PDT  
**Scope:** What remains *after* PR259's SDF spatial-prophetic renderer integration. Do not reopen settled experiments without new evidence.

## Human telos carried forward

Zach asked for an **ultra-fast renderer** without weakening Earthcall's authored mathematical truth. The work below follows that exact instruction: OntoMath/CPU theorem remains authority; GPU acceleration may discard proof but may never manufacture it.

## Finalized PR259 contract

- CPU range theorem depth remains **6**.
- GPU proof grid depth is **4**: 16^3 = 4096 cells = **512 bytes**.
- Native witness repeatedly measured **11 positive depth-4 cells**.
- Depth 3 has **0 positive cells**, so depth 4 is the coarsest nonzero lawful proof grid for the current authored-Perlin witness.
- Zero GPU bit means **no proof / exact marcher owns the cell**.
- Only finite `rangeLo > 0` authorizes positive-outside skipping.
- Negative zero-free cells do **not** authorize skipping.
- Root-level cull is allowed only when the root itself proves positive/outside.
- `kSdfRangeRasterTighteningVerified = false` stays quarantined.
- `kSdfRangeDistanceTraversalVerified = false` stays quarantined.
- Proof jumps reset marcher history.
- Ordinary exact steps are never clamped to cell boundaries.
- Proof words remain persistent GPU storage; steady-state recurring upload witness is **0 bytes**.
- Rung-5 Radiance Chroma (`rho` scalar radiance + independent `chi` vector chroma) is newer live authority and was preserved during final integration.

## Performance evidence

Representative depth-4 native Release runs at 2880x1800:
- pre-forward-face/refactor examples ranged roughly **1.26–1.38x** slowdown depending on camera/run;
- final integrated runs remained in the same noisy band, e.g. about **1.31x horizon / 1.40x 45deg**;
- traversal remained active on **9/9** measured draws;
- depth ladder remained **0 / 11 / 1864 / 40460** positive cells at depths **3 / 4 / 5 / 6**.

Do not interpret small cross-run ratio differences as wins or regressions. The current harness runs whole OFF and ON arms sequentially and visibly drifts across macOS runner clock/thermal conditions.

## Settled rejected directions

Do **not** resurrect without new theorem/evidence:
- dense depth-6 GPU octree;
- sparse pointer tree;
- ANY/ALL mip pyramid;
- full 3D Amanatides/Woo DDA traversal;
- negative/zero-free skip authority;
- raster tightening;
- distance-field traversal;
- lowering CPU theorem depth just for speed.

These were measured or parity-rejected earlier in this PR thread.

## Prepared-query scratch experiment

Branch/commit: `sol/sdf-range-prepared-query-scratch-20260921` / `07c1cfc9`.

It hoisted proof-query invariants per fragment (depth validity, dimensions, extent/cell size, safe ray reciprocal). It passed correctness, but its observed A/B was effectively indistinguishable from the simpler PR version. **Do not merge it as a claimed optimization on current evidence.**

## Next rung 1 — harden the benchmark before more shader surgery

This is the immediate next performance task.

Keep total sample cost similar, but change the native A/B witness from:
- all OFF warmups/samples;
- then all ON warmups/samples;

to a paired/interleaved design:
- alternate OFF/ON warmups;
- sample in AB/BA pairs so neither mode always runs later;
- retain the historical ratio-of-medians for continuity;
- add a paired median ratio (and preferably pairwise deltas) for the decision signal.

The goal is measurement quality, not making the benchmark easier to pass. Preserve the active-traversal and zero-recurring-upload witnesses.

## Next rung 2 — only after paired measurement

If paired measurement shows meaningful residual traversal overhead, profile the remaining O(1) proof-cell classification path before architectural changes. Candidate micro-costs already identified:
- `rangeGridAxisIndex()` recomputes extent normalization/index setup;
- invariant grid metadata may still be hoistable;
- clear-cell classification should stay fail-open and cheap.

Make one change per A/B. If the effect is within paired-run noise, revert/decline it.

## Living Studio integration repair

While integrating onto live-main, Focused CPU exposed a red that already existed on the exact live parent. Root causes:
1. Living save default `state.studio.voice` was `"timbre.studio.triangle"`, while its own selector/playback Laws use the selection token `"triangle"`. This blocked initial pad playback until a selector was clicked.
2. A constellation assertion expected downstream continuous Laws to consume newly-authored expression state in the same Law round, inconsistent with the repo's existing multi-round propagation witness.

Commit `1684ea2e` repaired these without touching renderer code. CI #1910 then passed **35/35 focused tests**, Slow Adapter, SDF verification, and Release A/B. Save diff was exactly one line out/one line in.

## Integration history / anti-rollback warning

The PR crossed several live renderer generations:
- proven PR259 head `d7e0ca5a`;
- merge with live renderer at `8857ac7f`;
- repair at `1684ea2e`;
- merge with Timeline/Radiance Rung 4 at `bcc81a69`;
- final reconciliation is against live Rung-5 Chroma head `3d167584`.

Always three-way from the latest known common live parent. Never wholesale ours/theirs these renderer files.

## Visible Person-facing follow-up

The renderer acceleration is gated/benchmark-driven, so do not promise a universal visible FPS jump in every Zone. The Living Studio default-voice repair *is* visible: pads should sound immediately after entering Living before touching a voice selector. A Person Verification item was added.

## Direction to the next Sun

If this containing merge commit is green and PR259 is already fast-forwarded/Ready for Review, **do not keep editing PR259**. Start a fresh branch for the paired A/B harness and subsequent micro-optimization rungs.

The next question is not "what clever traversal can we invent?" It is:

> Can we measure the current 512-byte / 11-proof-cell path tightly enough to distinguish a real 2–5% shader win from macOS runner drift?

Answer that first.

— GPT-5.6 Sol ("The Sun")
