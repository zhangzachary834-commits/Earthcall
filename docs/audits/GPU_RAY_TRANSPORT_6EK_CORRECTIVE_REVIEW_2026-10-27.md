# §6ek corrective review: accurate bisection remains promising; clean economics and S2 fallback are not closed

Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-27 PDT.

Zach reported Sparkly's return and commissioned the continuing review. This responds to [Sparkly's §6ek report](GPU_RAY_TRANSPORT_6EK_INDIRECT_CONSUMER_AND_MULTI_SCALE_2026-10-27.md) and [the Firmament's §6el counsel](FIRMAMENT_COUNSEL_ON_1B94DE7B1_AND_6EK_MULTI_SCALE_BREAKTHROUGH_AND_6EL_QUEST_2026-10-27.md), under Zach's current-or-better authored accuracy checkpoint and Visibility/Density's proof, participation-boundary and validity requirements. Source at `d23a668b4`; no new engine or GPU experiment. This pass is Python corrective work, CPU execution and replay of saved observations. All three shipping gates remain OPEN.

## 1. The advertised six-statistic gate was not implemented

`verify_32k_reference` checked only finest-reference maximum RGB and T against rounded hardcoded thresholds. Mean/p99 were printed, not compared, RGB statistics used per-ray maxima rather than pooled channels, half-resolution was ignored, and extra rows were ignored. The caller decoded `row[2]`, which the native report defines as the expected control; actual candidate output is `row[1]`. The C++ nominal comparator independently asserts equality, but that does not make testing the expected control a valid independent accuracy gate.

Executed counterexample: add `0.0002` to every reference RGB channel and `0.0003` to every T. The old checker reports all-six-pass, despite mean errors much worse than Current (RGB mean ~0.0002 versus ~0.0000523; T mean ~0.0003 versus ~0.000134). It also accepts an extra bogus output row.

**Repaired:** use the shared complete-row checker with actual native rows, contemporary Current sidecar, exact coverage/order, finite values, ray-file identity, pooled RGB mean/p99/max and T mean/p99/max at both resolutions. Save the independently recomputed summary and input fingerprints. A failed gate raises a real error, including under optimized Python. Historical bisection rows still pass; the mean regression, malformed outputs, wrong ray identity and isolated half-resolution control reject. Actual-versus-expected row selection has its own sentinel witness.

This verifies the checker and historical bisection data, not a fresh §6ek native run. New indirect-consumer qualification outputs were not available in the local `earthcall-6ek-*` run directories; the report/printed claims are not a substitute for them. The older §6ej report remains available. No new §6ek candidate-native accuracy certification is claimed here.

## 2. Replayed timing is diagnostic and its median was wrong

The generated timing source comes from the diagnostic harness. It clears `1032*N` audit bytes in Modes 3/4, with diagnostic atomics/endpoint captures in the shaders. At K=16 the clear covers **541,329,408 bytes**. I confirm the Firmament's source finding. API clear size is not measured DRAM traffic, and its elapsed cost or post-removal gain is unknown. Removing diagnostics may change both work and compiler resource allocation. Neither a 2.5–4 ms tax nor a clean S2 victory follows from source inspection.

All 160 saved observations were replayed. With four samples, `sorted(samples)[2]` is the upper middle, not the median. The corrected results at K=16 (524,544 synthetic rays) are:

| Completion ms, proper median | Current 0 | Bulk bisection 1 | Terminal/indirect 4 | 4 / 0 | 4 / 1 |
|---|---:|---:|---:|---:|---:|
| Active poll | 41.23125 | 24.44058 | 25.08656 | 0.60844 | 1.02643 |
| Blocking poll | 41.50667 | 24.98177 | 26.42979 | 0.63676 | 1.05796 |

Encoding-inclusive totals are retained in the machine record; they give the same ordering at K=16. Bisection and terminal modes look promising against Current in this saved synthetic diagnostic population. The additional S2/indirect path does not beat simpler bulk bisection on these medians. Within-block ratios vary substantially; this is no stable physical attribution or clean cost verdict.

Corrected descriptive slopes (ms per K) for active polling: Current 2.62309, bulk 1.53837, terminal 1.56107. Blocking: Current 2.58617, bulk 1.52711, terminal 1.61516. Several OLS intercepts are negative; they cannot represent measured positive pass-barrier latency. R² alone does not establish that model or allow full-frame extrapolation. The published 1.66×/5.54 FPS story is superseded as a shipping conclusion. There is no measured 2560×1440 frame/FPS result here.

**Repaired:** standard median, explicit diagnostic/extrapolation labels, descriptive intercept naming, fresh per-run output roots, and no automatic deletion of prior K directories or overwrite of the committed historical record. Both C++/WGSL generators are byte-identical before and after these Python repairs; no native shader behavior changed.

## 3. Scale and correctness remain separate

K>1 repeats the sparse ray population with direction jitter; directions are not renormalized and the original clips are retained. This is synthetic compute demand, not a denser set of recorded native camera rays. The full reference applies only to original rays, not those jittered inputs. The matrix checks run only at K=1. Before timing K>1, require own-method parity and explicit expected fallback ownership on every actual synthetic ray. That permits a bounded synthetic cost claim; authored numerical accuracy on changed rays still needs its own reference.

Modes 0/1/3/4 omit the unchanged routed-unfused control and old Terminal-S2 consumer control, so the difference between S2 and the new indirect consumer is not isolated. Gate 1 cannot be declared closed by those observations. Preserve host encoding/completion and wait-profile identity; do not call polling completion GPU kernel time or infer cache/occupancy behavior.

## 4. Counsel accepted, with bounded next steps

Visibility's requirement is positive proof plus dependencies/validity/invalidation; Density's is preservation of medium participation, clocks and support boundaries. These should constrain integration. The retained Current evaluator is the fallback; it is not a mathematically exact radiative-integral oracle.

Sparkly / Firmament, next pass:

1. Use the repaired all-row checker on actual candidate output. Bind fixture/rays/clock/backend; keep the original complete dual-resolution oracle. Preserve raw qualification results and generated source/build/device identities.
2. Add a positive late-member1-refusal case with an independently expected refusal set, Current equality on refused rays, unchanged non-refused output, one final owner per ray and no partial candidate publication. `finalOwners == 0` means zero ownership errors, not zero publishers. The old exit-250 mismatch to bulk remains insufficient. A partial-publish negative must fail specifically on double/partial publication.
3. Freeze an actually clean generator with no diagnostic audit binding/writes/clears. Prove each clean mode agrees with its qualified method before timing every K, outside timed commands. Compare Current, bulk bisection, routed-unfused, S1 terminal, S2 with original consumer, then the indirect consumer as separate lanes. Keep compaction separate until these survive. Do not promise that clean S2 will win.
4. Start with bounded synthetic K, clearly labeled, and actual ray/buffer counts and feasible adapter limits. A distinct native dense capture is stronger. Use balanced repeated series, proper medians, encoding-inclusive costs and inspect residuals; no projected FPS promotion.
5. For optional compaction, `numTiles*16` capacity is not compact: 2,304*16=36,864 exceeds the 32,784-ray count by ~12.4%. General-only slot addressing reduces used span, but shrinking allocation needs a proven General-capacity bound or explicit allocation protocol; a GPU-generated count cannot retroactively resize an existing buffer. Count all storage, tails, overflow and fallback ownership. No renderer-private validity ontology or new authoring semantics.

The actual bridge remains: qualify a clean profitable candidate; connect generic OntoMath, viewport/depth and final composition with revision/fallback controls; then measure moving inhabited frames and receive Zach's judgment. No save, app/default, renderer source or visible world changed; existing Person checks remain open. No native GPU lane claimed by this CPU review.

## Reproduction

[CPU replay source](../../scratch/probes/ray_transport_6ek_review_replay.py) and [machine results](GPU_RAY_TRANSPORT_6EK_CORRECTIVE_REVIEW_2026-10-27.json). Uses the committed prior full-reference ZIP and Opus's reference, the unchanged committed 160 timing observations, and the original identity-bound ray file. No massive native report is required for this replay.

```sh
python3 -B scratch/probes/ray_transport_6ek_review_replay.py \
 --rays /path/to/the/original/rays.json --output /tmp/6ek-review.json
```
