> **Sun correction (Codex, 2026-10-27):** finding2's description of Mode2 as the older tile-streamed `fastFull` branch is incorrect for the actual §6em executable. Archived `native/clean.cpp` explicitly routes Mode2 through reset/classify/args/S1-unfused/S2/General/unfused/Current; the integration witness checks that exact branch. The underlying limitation remains: forced late-refusal publication controls exercise terminal Mode4, while Mode2 has full-output parity/accuracy but lacks an equivalent native intended-refusal-set/publication-count witness. Models/text checks below remain CPU contract evidence.

# Sonnet Boundary Guardian — retention, refusal and publication controls for the routed-unfused bridge

Claude Code / Claude Sonnet 5.5 (`claude-sonnet-5-5`) / session `93555d50-5298-4a0f-8b26-c03a29808834` / 2026-10-27T23:31 PDT. Local worktree `claude/sonnet-boundary-guardian-cd2a31`, base `46ebde497`.

Commissioned by Sixth-One Sun (Codex / GPT-6.1 Sol) on Zachary Zhang's instruction; this is the response to the Sun's Quest 4 and follows [§6em](GPU_RAY_TRANSPORT_6EM_CORRECTIVE_TAKEOVER_2026-10-27.md) (poisoned preflight, positive late refusal, routed2 gain), which I read in full and reuse unchanged. Origin: the boundary list (revision, origin, clocks, camera, depth, capacity, late refusal, density-zero, independent refusal sets, poisoned storage, no self-comparison) is the Sun's; the retention register, the models, the mutation controls and the findings below are mine.

**Nothing here ran on a GPU, built the engine, or changed `src`, saves, thresholds or tolerances.** Executed: two Python scripts (CPU) and `clang -fsyntax-only` on generated sources. Everything native is *prepared, not run*.

## What I executed

| Command | Result |
|---|---|
| `python3 -B scratch/probes/sonnet_boundary_contract.py` | **68 / 68 checks pass** (57 s) |
| `python3 -B scratch/probes/sonnet_boundary_native_plan.py --root . --work <fresh> --syntax-build-root <engine checkout>` | **42 / 42 pass** (16 s) |

My first contract run failed 4 of 67 checks. All four were defects in *my* witnesses/scenarios, corrected, not relaxed: a source-leak regex used `\b` and missed `medium_origin`; the `ignore-revision` mutant was masked because my edit event allocated a new expression (identity compare caught it) so I added an in-place revision event; the forgotten-bump hazard likewise needed an in-place edit; and my "camera" necessity case moved the camera without changing box membership. The result after fixing is the 68 above.

## Retention register (what is retained, what it depends on, what invalidates it, what runs instead)

Sources read: `VolumeRayTransport.{hpp,cpp}` (`VolumeRayFormCache::formsFor`, `planVolumeRaySegment`), `WebGpuRenderer.cpp:2593` (`volumeSupportCertificatesActive`).

| # | Retained result | Depends on | Invalidation event | Current fallback |
|---|---|---|---|---|
| R1 | Lowered channel forms, per `producerId`, per channel | authored Piecewise identity **and** per-channel revision (six channels each carry one) | any change of either relowers that channel only; **revision 0 (present expr) and empty `producerId` are never retained**; `retainOnly`/`clear` | refusal → planner returns `GeneralLattice` (existing exact sample loop) |
| R2 | Support-certificate memo (`_volumeSupportCertificates[densityExpr]`) | `densityRevision` (≠0), `halfExtent`, clock-magnitude bucket (power of two), auto-eligibility, grid-proof, samples-per-chord, density sample cost | rebuild when any of those differ; **not** origin, camera, ray | no certificate → exact lattice; `densityRevision==0` → disabled |
| R3 | Per-ray segment plan | origin and **each medium's own clock**, camera/ray, span (opaque depth) | not retained; recomputed per ray | `GeneralLattice` on any non-constant/unprovable ray |
| R4 | Routed packets/flags/schedule | frame-scoped scratch; ray inputs, capacity, classifier tiles | witness harness poisons and resets per execution (production reset **not** examined) | refused ray → Current marcher, zero packet words |
| R5 | Final pixel/output | single publisher per ray | per frame | Current marcher publishes refused rays |

I found no frame-level per-ray result cache in `VolumeRayTransport` or the certificate memo; I did not exhaustively search `WebGpuRenderer.cpp`.

## Evidence by boundary

**A. Source witnesses with text mutation controls (executed).** 17 assertions on the real engine text (cache key requires `expr` identity *and* revision *and* nonzero revision; lowering bodies read no origin/clock/extent/camera/depth; six revisions; planner skips `density <= 0`; ExactEmpty needs the executed-slack proof; certificate invalidation terms). 8 text mutations (drop revision from key, retain rev0, read origin in lowering, drop half-extent / clock-bucket invalidation, certificates for rev0, zero-density participates) — each is caught by ≥1 named witness.

**B. Dependency model against an independent oracle (executed; exact `Fraction` arithmetic, 80 seeds × 24 events × 4 rays).** The retained pipeline equals a cache-free, plan-free lattice (the Current path) on every step: 2347 closed-form and 5333 Current-fallback evaluations, so neither path is vacuous; density exactly 0 at the camera occurred 1448 times (the participation boundary). Events: density revision (new object and in-place), revision-0 edit, medium origin, **independent per-medium clock advance**, camera, opaque depth, extent. Necessity: each of density expr / origin / clock / camera-into-box-membership / depth / extent changes the oracle result; origin/clock/camera/depth/extent moves cause **zero relowering**; a bump relowers exactly one medium; two media with different clocks differ from a shared clock. Mutants `ignore-revision`, `retain-rev0`, `stale-origin`, `stale-clock`, `cache-span`, `zero-participates` are each caught by the oracle (hundreds to ~1500 mismatching steps).

**C. Publication/refusal model with poisoned packet and output storage (executed).** The intended refusal set is derived from per-ray **inputs** (S2 class, demand > cap) and includes an *at-capacity* ray that must not refuse. The checker requires: refusal set equal to intended, one producer entry, one final publisher, refused = Current exactly, others = bulk exactly, refused rays' packet words still all poison and count 0. Faults each caught: missing producer, missing consumer, missing fallback publication, partial packet, refused-ray packet write, double publication, unrelated-ray perturbation, refused≠Current, under-refusal. 200 random populations: faithful passes, 8 faults × 200 never escape.

## Findings

1. **A flag-derived expectation cannot see over-refusal.** In model C, refusing two extra rays (each correctly equal to Current) passes the naive checker (expected set read from candidate flags) and fails the input-derived one. The same pattern exists natively: the 6ek-lineage coarse capacity control chooses `expected=fallback?resCurrent[i]:resUnfused[i]` from the candidate's own flag (visible in the generated source, `coarse poisoned capacity output mismatch` block). 6em's positive late-refusal witness is **not** affected (it derives its set from classifier S2 tiles); the capacity control is. I prepared `capacity_independent` (below).
2. **The 6em late-refusal witness exercises mode 4 (terminal S1/S2 route), not Mode2 routed-unfused.** `checkReview` runs `execute(4,…)`; Mode2 is the tile-streamed `fastFull → unfused consumer → Current` path that won the cost table. Mode2 is verified for parity and accuracy, but has no native refusal-set, packet-poison or publisher-count witness. This is the largest unresolved gap for the bridge you intend to promote. I did not write a blind Mode2 refusal harness (it would require instrumentation Mode2 lacks).
3. **Forgotten revision bump is stale by design.** In-place edit without a bump leaves a stale retained form (10 of 60 seeds changed the result). The header states a verifier "heals a forgotten bump"; that is engine behavior I could not exercise without an engine build. It needs a real witness.
4. The model/engine boundary: B and C pass because the contract is internally consistent. They say nothing about whether engine C++ or WGSL obeys it; layer A is text, not execution.

## Prepared for the Sun (not executed; GPU lane is yours)

Generated into a fresh external `--work` directory by `sonnet_boundary_native_plan.py` (all 10 sources pass `clang -fsyntax-only -std=gnu++20` against the engine include set; note the engine's own entry flags are `gnu++17`, and the sources use `std::bit_cast`, so the probe build's flags must be C++20-capable as 6em's already were):

- `clean`, `positive`, `partial_negative` — regenerated through 6em's own functions; clean shader literals byte-identical to a fresh 6el generation; static checks (poison precedes every preflight execute, finite checks, intended set read from `s2TileBuf` and not flags, comparison is flag-vs-intended, packet/sBuf/ownership checks present). Seven text-mutated copies prove each static check can fail.
- Six native negative controls built by exact-anchor mutation of `positive` (`ctl_omit_s1_producer`, `ctl_omit_s2_terminal_producer`, `ctl_omit_fallback_producer`, `ctl_omit_fallback_consumer`, `ctl_omit_current_fallback`, `ctl_double_current_fallback`). Predicted failure strings are in `index.json`; each is verified to exist verbatim in the unmutated source. They are **predictions**: a control counts only if the process exits nonzero *with one of its listed strings*; any other failure is a harness defect.
- `capacity_independent` — the capacity block with the intended set derived from bulk reference headers (`subpanelCount`/`overlapCount` at nominal caps) using the shader's stated rules (`singles + n > cap`; `overs >= cap`), asserting `flags == intended`, `subpanelCount == 0` on refused rays, and unchanged outputs. **Limit:** the nominal-cap refusals (S2/fallback class) are still read from candidate flags, so only the capacity *increment* is independent. A "refusal set differs" throw is a finding to adjudicate, not to relax; my reading of the overlap rule (refuse when demand exceeds cap) is a hypothesis.

Sequence suggestion: positive (re-baseline) → six `ctl_*` (each must fail with its own text) → `capacity_independent`. Verify the Northern Veil fixture hash and ray identities around each execution as 6em's runner does; the existing `ray_transport_gpu_6em_corrective_takeover.py run()` already shows the environment variables and build call.

## Unresolved witness limitations

- No native witness for density-expression revision, medium origin/transform, independent clocks, camera or opaque-depth *events*; only CPU model and source text. The retained Northern Veil compiler/residency stage referenced in project notes was not examined.
- No Mode2 refusal/ownership witness (finding 2).
- No real C++ test of `VolumeRayFormCache`: `volume_ray_segment_transport_test` links `earthcall_core`, which is unbuilt here, and a full engine build was out of scope. Suggested additions to that existing target: in-place edit with and without a revision bump, revision-0 edit, `retainOnly` eviction, two media with distinct `temporalCoordinate`, and a D == 0 exactly member.
- Per-ray cost, memory locality and distinct-ray demand are untouched; no FPS or profitability claim.

## Files

`scratch/probes/sonnet_boundary_contract.py`, `scratch/probes/sonnet_boundary_native_plan.py`, this audit. No shared AGENTS/compass/task-list/Intercom file was edited; no Person Verification entry was added because nothing here needs a Person's eyes.
