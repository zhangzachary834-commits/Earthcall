# Canonical producer corrective repairs — reliable fixture base, generative producer still open

Zach reassigned the corrective work to Codex: **“I’ll have you do the corrective repairs and then Sparkly Guy does the generative steps.”** This implements the repairs from the [§6cf review](SPARKLY_CANONICAL_PRODUCER_6CF_REVIEW_2026-10-21.md), following Sparkly's fixture experiment and the Firmament's canonical/curvature proposal. The [existing Intercom thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md) carries ownership and the hand-back. Zach's current-or-better-accuracy shipping checkpoint remains the goal.

**Checkout:** `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop`, branch `claude/ray-domain-transport-rung0`, starting HEAD `7e58b5389`. This audit is mirrored on default for discovery; implementation and full artifacts are in the ray checkout. No production C++, app/default/budget/scene/light/save/data-home changes. No push or PR. Ordinary timestamps remain disabled.

**Result:** structural root/uncertainty/near-parallel repairs, safe runner failure behavior, pinned-fixture refusal, complete native fallback and actual matching-cost measurements are implemented and tested. The retained fixture still uses a manually specified mathematical model; it is explicitly refused on fixture edits and **has no native omission authority**. Generic OntoMath recipe extraction, justified authored/backend error bounds and GPU generation remain Sparkly's generative work. This pass does not pretend those missing mechanisms are fixed by a label.

## Corrections made

1. **One shared C++ root contract.** `ray_transport_canonical_root_contract.py` supplies the actual implementation to the scalar tests and native probe. A value/secant bow may reject root-free spans; single-root bisection additionally requires a derivative interval excluding zero plus a definite endpoint sign bracket. Opposite signs alone no longer discard the rest of a bin. Outward interval arithmetic carries supplied endpoint/curvature assumptions.
2. **Typed retention.** Results contain full root brackets and separately typed unresolved intervals/reasons for depth, node, capacity, undefined or representability failures. No depth cap turns a possible double crossing into a root midpoint. Overlapping root slivers remain represented; arbitrary `1e−4` event merging/span removal is gone. Outward f32 packet conversion preserves narrow retained intervals.
3. **Whole-ray diagnostic fallback.** Any unresolved member clears the partial canonical pieces and invokes the previously qualified complete branch-aware host schedule for the affected ray. A missing fallback evaluator refuses loudly. This is the old diagnostic schedule, not a newly certified production fallback or a universal integration theorem.
4. **Exact-zero parallel handling.** Small nonzero slopes use the full clipped ray interval and outward slab divisions. Only exact zero is constant. Nonfinite/unrepresentable slab queries refuse. The fallback box loop uses the same exact-zero distinction.
5. **Runner integrity.** Fresh external work directories, fixture hashes before preparation and again before execution, source-identity replay checks, safe report labels, nonzero child/qualification propagation and strict RGB+T/parity gates replace the old “print failure, return success” path. Changed authored fixtures cannot inherit this literal model. The old historical red reproducer is pinned to `017499162`, so it continues to reproduce the original faults after the live scripts are corrected.
6. **Qualified controls reused.** The new generator composes the existing overlap-repair and production-marcher harnesses instead of copying their old incomplete variants. Five closed-form tests, original-AST comparisons, stored-native baseline reproduction, actual Metal adapter evidence, raw rows and all six reference gates are retained. The misleading hardcoded `15.86` baseline and summed “1.62×” claim are removed.
7. **Actual connected measurement.** At repeat 1, each candidate sample starts its clock before rebuilding the CPU schedule, uploads the ray/single/overlap arrays, dispatches the actual consumer and blocks for completion. ABBA compares the same 2,304 rays with the production-generated current consumer. Stable scene/evaluators/buffers and fixed-clock preparation are resident; compilation, opaque depth/composition, full viewport and dynamic lifecycle are outside this scope. Repeated throughput workloads explicitly omit a per-unique-ray connected claim.

The mathematical root enclosure is conditional on the caller's curvature and endpoint-error contracts. The pinned fixture uses an explicitly declared **double-math diagnostic** endpoint allowance; this is not an established bound on Metal Fast functions or original authored f32 execution. `native_omission_admitted=false`, `generic_OntoMath_extraction=false`, `GPU_producer=false`, and `shipping_performance_qualified=false` remain machine-readable.

## Verification that actually ran

**CPU:** 33,640 assertions pass, including 33,593 independently sampled positive-support points across all four members; three roots inside one coarse bin; the close root pair; forced depth/node/capacity exhaustion; undefined/tangent cases; actual narrow-lobe root isolation followed by GL2; parallel, near-parallel and boundary clipping; and forced whole-ray fallback. All 20 historical missed-crossing windows match actual retained root brackets **without the old extra ±0.02 matching margin**. The frozen default census has 9,332 root brackets and zero unresolved leaves/fallback rays under its declared mathematical assumptions. That census and finite scan are not a universal device proof.

**Negative harness controls:** seven pass: existing-directory preservation, replay source mismatch, invalid repeat, unsafe label, a child exiting 7, failed AST qualification despite a zero child exit, and an edited authored fixture. No GPU or app invocation is involved in those controls. The historical exact red cases also still reproduce against their pinned old source revision.

**Final native source on observed Apple M5 / Metal, timestamps off:**

| Check | Normal corrected fixture | Forced node budget 0 |
|---|---:|---:|
| Rays | 2,304 | 2,304 |
| Whole-ray fallbacks | 0 | **2,304** |
| Unresolved leaves reported | 0 | 175,056 |
| Single panels / overlap intervals | 28,764 /245 | 23,776 /253 |
| Schedule bytes | 575,552 | 495,872 |
| RGB pooled mean /p99 /max error | 6.72976e−6 /4.96912e−5 /1.13653e−4 | 6.92274e−6 /5.05613e−5 /9.15713e−5 |
| T mean /p99 /max error | 1.96301e−5 /1.10855e−4 /1.65180e−4 | 2.02057e−5 /1.09961e−4 /1.39098e−4 |
| All six reference gates | pass | pass |
| Original-AST, five closed-form and fused/unfused checks | pass | pass |

Normal original-AST GPU disagreement is RGB `2.68221e−7`, T `7.15256e−7`; fallback RGB `2.90573e−7`, T `6.55651e−7`. Both baseline runs reproduce stored T exactly and RGB within `4.47035e−8`. The forced control proves the real complete fallback executes on device, including all 253 overlaps; it was not accepted on a CPU-only flag. Its producer-inclusive timing was intentionally skipped. Per-ray accuracy regressions remain recorded in the machine artifact; aggregate improvement is not universal improvement.

The earlier 65,536-step authored-AST Simpson reference and convergence witness remain a finite independent reference, not an exact-real oracle. The installed app and inhabited data home were never opened.

## Matching economics: keep the corrections, reject this host producer for shipping

The final source's actual same-workload ABBA measurement gives current resident consumer **1.27242 ms** versus CPU-producer + schedule-upload + A* consumer + fence **17.26407 ms**, means of eight samples each. Candidate samples range `15.8–21.21825 ms`; these are **host elapsed times**, not GPU timestamps. Both sides execute 2,304 frozen rays, no 128× mismatch. The current/candidate ratio is **0.07370**, so this host-per-ray preparation strategy loses decisively in this measured sparse scope. The initial standalone producer observation is 14.9493 ms; the connected timing charges actual recomputation and uploads rather than adding that observation to another process's clock.

The separate resident 64-dispatch control measures current `0.507246 ms` versus A* `0.138459 ms` after normalization; this is a sparse repeated consumer diagnostic, not a new full-viewport result. The earlier §6cc 3.34–3.37× matched 294,912-invocation **consumer-only** evidence remains historically valid. This correction earns no shipped performance claim, and we did not keep retiming an unprofitable host candidate.

## Reproduce and continue

From the **ray checkout**, use fresh external directories:

```sh
python3 scratch/probes/ray_transport_canonical_producer_corrective_test.py --root . --work /private/tmp/new-canonical-cpu
python3 scratch/probes/ray_transport_canonical_harness_test.py --root . --output /private/tmp/new-harness-result.json
python3 scratch/probes/ray_transport_canonical_producer_metal.py --root . --work /private/tmp/new-canonical-metal --label normal
python3 scratch/probes/ray_transport_canonical_producer_metal.py --root . --work /private/tmp/new-canonical-metal --run-only --node-budget 0 --skip-connected --label forced-fallback
```

The old preflight and Python producer-test entrypoints now forward to the shared C++ corrective witness and require `--work`; they no longer run independent unsafe handwritten isolators or assign the narrow-lobe roots as an implementation test. Read `docs/BUILD_AND_ENVIRONMENT.md` before native compilation; the native wrapper links the existing configured app objects. The original marcher body, frozen fixtures, generated C++ and script hashes are captured in the [machine record](CANONICAL_PRODUCER_CORRECTIVE_REPAIRS_2026-10-22.json). Normal/fallback raw GPU+AST rows are retained as deterministic gzip archives beside it. Their hashes and pixel order were verified, and all six reference metrics were independently recomputed from both archived row sets.

**Sparkly's next generative office:** keep this tested typed bracket/unresolved contract and its negative witnesses; derive canonical groups/support from current authored expressions through OntoMath, preserve original execution/error units and validity, then instantiate each ray's actual boundaries on the GPU with bounded storage and real fallback. Replace the literal pinned fixture model rather than relaxing its hash guard. Prove independent coverage for the admitted forms and qualify same-answer/reference accuracy and actual **producer + upload/lookup/repair + consumer + fallback** native costs. Keep one implementation owner and reserve one native timing lane.

Then qualify unique 2560×1440 output, opaque depth/composition, camera/clock/source/overlap changes, edit/device/revision lifetime and inhabited movement. Those gates remain open. No new Person-visible change this corrective pass; existing Person appearance/movement acceptance stays open. Codex releases the native lane here; no jobs remain running.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-22T00:30:14.245935-07:00`.
