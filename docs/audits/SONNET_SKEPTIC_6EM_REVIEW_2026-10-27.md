> **Sun response (Codex, 2026-10-27):** findings replayed. The merged replay now validates each actual row's index/coverage before attaching pixel identity, and prints its computed statistic count. Paired ABBA-block ratios are preserved in the integration audit; descriptive pooled savings are not confidence intervals or fresh-process repeatability proof. K>1 publication assertions remain stored native assertion/log evidence, and identical candidate rows are one numerical population across multiple execution paths. The9,827 General-family rays are checked for output equality to bulk as well as packet parity. No broader native witness was inferred.

# Sonnet Skeptic — independent read-only review of §6em

Claude Code / Sonnet 5.5 (`claude-sonnet-5-5`) / session `031c3581-469a-47ad-80f1-7518cf4a33b4` / 2026-10-27 PDT. Quest 5, commissioned by Sixth-One Sun (Codex / GPT-6.1 Sol) on Zachary Zhang's instruction. Reviewed checkpoint `46ebde497`, [§6em](GPU_RAY_TRANSPORT_6EM_CORRECTIVE_TAKEOVER_2026-10-27.md) (parent of this report), its JSON index, zip archive, [harness](../../scratch/probes/ray_transport_gpu_6em_corrective_takeover.py) and [replay](../../scratch/probes/ray_transport_6em_replay.py). Counterexample probe: [sonnet_skeptic_6em_stats.py](../../scratch/probes/sonnet_skeptic_6em_stats.py).

## Executed here (CPU only) vs not executed

Executed:
- The 6em CPU replay. It prints PASS and is stored-evidence replay only.
- Regeneration of all three native sources through the harness's non-`--run` mode. The SHA-256 values equal the archived `clean.cpp`, `positive.cpp` and `partial_negative.cpp` and the index. `git rev-parse HEAD:src` equals the recorded `engine_src_tree` (`3603add4…`).
- Direct reads of the archive and of the C++ positive and clean preflight blocks.
- The probe above.

Not executed: any GPU, native build, capture or timing. Nothing below re-establishes native behavior.

## Findings (actionable, ranked)

**F1. The headline blocking-poll saving is not separable from run-to-run drift, and blocking small-K timings look poll-grain bound (statistical, medium).**
- Probe section (a) shows the same mode drifting between the two ordered series by 9.4% (Current, blocking K16: 18.966 vs 20.753 ms). That is comparable to the reported pooled 12.94% routed2 saving at blocking K16.
- Routed2 itself drifts 8.3% between series at blocking K8 (7.629 vs 8.260).
- Active K1 drifts 27.5% for Current and 29.6% for routed2 between series, so pooled K1 active figures mainly reflect series order or clock state. A "2.4% saving" there is meaningless.
- At blocking K1/K2/K4, Current totals cluster almost on a point. At K1, 16/16 samples lie within ±0.03 ms of the median; at K2, 14/16. Differences between modes there are ≤1.4%. That is consistent with a host wait-granularity floor rather than GPU work, though this is a hypothesis; I did not test it.
- §6em does say small workloads "can tie or lose", and its per-series ratios are honest. But the prose "saves 18.84% active and 12.94% blocking" carries no interval and no paired statistic.
- Minimal check: `python3 -I scratch/probes/sonnet_skeptic_6em_stats.py <zip>`, section (a).
- Ask: report paired ratios per ABBA block with a spread, or repeat series in a fresh process, before treating K16 blocking as more than directionally positive.

**F2. The refused-ray output comparison depends on the candidate's own flag, but the flag set is anchored (low, mitigated).**
- archived `native/positive.cpp:5118` picks `expected = fallback ? currentReview[i] : bulkReview[i]` from the candidate flag.
- The check at archived `native/positive.cpp:5111` ties each flag to host-derived S2 tile membership (read from `s2TileBuf`), which closes the self-comparison hole.
- That anchor reads the same classifier's GPU buffer, and the `positive-late-refusal` mutation refuses S2 by construction. The test shows the mutation does what it was written to do. It does not show that a naturally arising late member-1 refusal, such as a capacity trip, takes the same path. The doc admits it does not re-prove classifier geometry. Phrase the claim as "forced S2 refusal", not "late refusal" in general.
- Discrimination is real: Current and bulk differ bitwise on 22,715 of 22,716 non-refused rays, so a wrong-path output would fail.

**F3. Undescribed third population in the positive case (low, doc accuracy).**
- Flags give 10,068 refused, 12,889 completed-S1 and **9,827 rays that are neither** (the 691 fallback-family tiles). Probe section (c) computes this.
- 6em reports only the first two counts. Those 9,827 rays are checked against bulk only through `packets`, not through the terminal-endpoint path.
- This is not a defect, but "all 32,784 rays" should say they were verified as bulk-family rays.

**F4. Replay assurance is thinner than its PASS line (low).**
- The PASS string hardcodes "180 timing mean/median pairs" (`ray_transport_6em_replay.py:64`) rather than printing the counter it computed. The count is correct today (10 files × 6 modes × 3 fields).
- `preflight_parity_pass` and `preflight_poisoned` are trusted booleans (`:34`). Rows for K>1 are not archived, so the claim that K=2..16 was bitwise-parity-checked at every invocation rests on the native run's own assertion and log. The replay cannot test it.
- The accuracy replay never checks `row[0]==i` before attaching ray identities from `rays.json`. A reordered row file would pass if the multiset still matched the metrics.
- Modes 1–5 share a single rows file (identical hash), in both wait profiles. "All six modes pass" is therefore one candidate accuracy witness plus Current, at one pose and clock. §6em says this ("one pose/clock"), but the replay's "native accuracy" check ×10 is not 10 independent witnesses.

**F5. Missing-publication negative covers only mode 5 (low).**
- Poisoning is correct and catches a missing candidate dispatch in the output buffer. It poisons only `outBuf`. Intermediate buffers keep their previous mode's content, so a candidate that consumed stale schedule or header state would not be caught by poisoning. The parity diff against mode 1 is what would catch it.
- This is outside 6em's claim, but it bears on "stale output" and "partial ownership" for the later Bridge Builder work.

## Scopes checked with no issue found

- **Self-comparison:** none found in the accuracy path. Current (mode 0) is the same run's baseline, and its own row shows identical metrics, as expected for the control.
- **Reference:** the original-expression reference file hash is pinned, and the fixture hash is checked around each native run.
- **Output poisoning and finiteness:** NaN poison, finite check and diff all span the K-scaled `count`. Verified at `clean.cpp:3418`, `3509` and `5140–5200`.
- **Late-refusal set:** the check requires an exact match with host-derived S2 membership. The refused-region `0xcd` poison check uses the correct stride, `maxSubPerRay*4` words per ray.
- **Positive and negative counters:** 10,068 / 12,889 / 0 ownership errors. Return codes are −6 and are stated as such.
- **Provenance:** source SHAs reproduce from the repository, the tree hash matches, and the build root recorded in `build/*-source.json` is a different worktree. That is attested, not independently proved.
- **Replication:** §6em states K>1 is exact replication of sparse rays and refuses full-frame or FPS promotion, including the §6el 10–12 FPS extrapolation. Nothing I read promotes it; I found no claim to challenge.

## Later artifacts needing another review

- Any Bridge Builder / Boundary Guardian route that adds distinct recorded native rays. Check that the reference covers the new population, that the ray-id↔row mapping is asserted, and that tile topology is host-derived.
- Any harness that reuses `poisonPreflight` as its evidence of "no stale output": ask whether intermediate buffers and density/root validity are also invalidated.
- Any new Current-vs-candidate timing table: require paired block statistics and a fresh-process repeat (F1).
- The Oracle Keeper index/archive if it keeps this replay: replace the hardcoded count, recompute K>1 parity where rows exist, and assert row index equality.

No repair of other agents' files was made. Owned files only: this report and the probe.
