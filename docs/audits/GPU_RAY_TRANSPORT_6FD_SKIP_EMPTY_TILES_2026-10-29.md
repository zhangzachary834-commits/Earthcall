# §6fd step 2a — skip wave-empty tiles: bit-identical output, −3.1% / −2.95% vs coalesced, first measured gain over Current (toggle still default OFF)

Continues the [§6fd census](GPU_RAY_TRANSPORT_6FD_ROUTE_CENSUS_2026-10-29.md). Origins: Sixth-One Sun's §6fc quest (census first; ONE guarded change; keep zero-budget refusal; define the budget contract); the Firmament's post-§6fd counsel (the idea that `ecSetup` can pre-write the exact packet for wave-empty rays, and that S1/S2 already trust the classifier's proof for their other members). I verified the premises in source before building (see the Intercom entry) and changed the mechanism: the counsel proposed editing `ecSetup` unconditionally; I put it behind a new uniform word `flags.x` plus a classifier variant so one binary gives a true A/B. Steps 2b/2c (member-ID S1, two-member S2) are NOT started.

## What changed (all default OFF; `setRoutedSkipEmptyTilesForDiagnostics`)
- Classifier variant: `activeMemberMask == 0` tiles join no list (no producer work). Derived from `kClassifyWgsl` by one substitution; the original classifier and observer are untouched.
- `ecSetup` (flag on): valid rays carry `(id*maxSub, 0, id*maxOver, workLimit==0 ? 0x80000001 : 0)` — exactly what S1 writes for zero-budget refusal and what the producer would leave for a ray with no pieces. Config grows 8→12 words (the producers' 16-byte `Config` still reads the first four).
- **Declared contract:** the work budget bounds work actually performed. Production budget 1024 and budget 0 are bit-identical to the original; positive budgets below 96 (diagnostic-only; §6fc measured 95) no longer refuse rays whose only cost was charging for proven-empty members. This is proof admission relying on the classifier's wave test, the same trust S1/S2 already place in it for members they do not process; it is **not** a claim of identical raw packets at every budget.

## Correctness (native Release Engine, 2560×1440, wgpu-native/Metal; parity run 7.6 s, no validation/device errors)
Pose 32: skip-empty publication `7c14eadd2a2e79c9` and snapshot `9bd2d6aca329496c` are bit-identical to the original routed lane for normal, poisoned-scratch, forced-refusal (zero budget) and poisoned forced-refusal captures; refusal captures equal Current (`4e8588f0cd7f2fb5`). Effectiveness: production fallback tiles fall 189,828 → 122,122 = exactly the 67,706 observer-counted wave-empty tiles. **Pose 0 has no wave-empty tiles (observed 0), so it only shows the toggle is inert there; the skip evidence rests on pose 32 alone.** Compute passes stay 1, dispatches 240. Coverage is two poses; the 64-pose route and other views are not covered.
*An earlier parity attempt in this session failed on my own check (it required wave-empty tiles at pose 0). All its captures had already passed; I fixed the check and re-ran rather than counting the partial run.*

## Timing (separate run, 113.8 s; 3-lane rotated screen, 4 blocks per series, 64 warmup + 2 activation + 64 measured frames per sample; wall-clock only, app GPU timestamps off)
| ms/frame | Current | Coalesced (default) | Skip-empty | Skip vs coalesced | Skip vs Current |
|---|---:|---:|---:|---:|---:|
| Queued complete | 61.616 | 62.291 | **60.357** | **−3.10%** | **−2.04%** |
| Fenced complete | 65.317 | 65.246 | **63.319** | **−2.95%** | **−3.06%** |

Skip-empty is below coalesced in 4/4 blocks in both series; per-block queued values are tight (60.29–60.40 vs 62.28–62.30). In this protocol coalesced vs Current is +1.09% / −0.11% (consistent with §6fb: no gain). Caveats: one process, one route, 12 samples per lane; mean differences of 2–3% are small in absolute terms (≈1.9 ms/frame); this is not an inhabited 30 FPS result, not Person acceptance, and not the cause-level GPU attribution (no fresh trace after coalescing).

## Status and next
Toggle stays **default OFF** because (1) the budget contract above needs Sixth-One Sun's/Zach's acceptance and (2) parity covers only two poses. Recommended before flipping: wider-pose parity (e.g. poses 8,16,24,32,40,48,56 — all contain empties) and the ordinary §6ey-protocol comparison. Then step 2b (single non-zero member → S1 with member ID: 45% of fallback tiles; needs the `ecConsume`-carries-IDs claim verified), then 2c. No tolerance, sampling, lighting or resolution change.

Evidence: [JSON](GPU_RAY_TRANSPORT_6FD_SKIP_EMPTY_TILES_2026-10-29.json); witness `scratch/probes/ray_transport_6fb_engine.{cpp,py}` modes `parity`, `skiptiming`. No push/PR; no live save, default checkout or installed app touched. GPU lane RELEASED.

Signed: Claude Code / Sonnet 5.5 / session `78cc5135-a805-4233-84ab-54fb16681c11` / 2026-10-29 (PDT).
