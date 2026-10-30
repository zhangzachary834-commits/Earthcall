# §6fe step 2b — single-active-member tiles to S1 with a member id: bit-identical on 7 of 8 poses, one refusal→accept pixel at pose 40, timing handed off (toggle default OFF)

Origins: Zach's "start 2b" go-ahead (after my [§6fe work-trace census](GPU_RAY_TRANSPORT_6FE_WORK_TRACE_CENSUS_2026-10-29.md) showed Fallback is expensive by work quantity, not tail); the Firmament's [step 2b blueprint](FIRMAMENT_ANSWER_TO_SONNET_6FD_STEP2A_CONTRACT_AND_STEP2B_BLUEPRINT_2026-10-29.md) (member id packed in the list entry; `ecConsume` reads only `Subpanel.info.x`); my §6fd step 2a toggle it builds on. I implemented the blueprint's four substitutions and tested the claim rather than trusting it.

## What changed (all default OFF; `setRoutedSingleMemberS1ForDiagnostics`; implies the 2a empty-tile skip)
- Classifier variant: `activeCount == 1u && !tileCanOverlap` → S1 list entry `(firstTrailingBit(activeMemberMask) << 28) | tileId`.
- S1 producer variant (by `replace()` from the shipped WGSL): unpack tile/member, `member = activeMember`, stack 15→20 (the fallback producer's), `Subpanel.info.x = activeMember`. Producer arithmetic otherwise untouched; consumer untouched. The first build failed shader validation (my `member` out of scope at the subpanel emission) — fixed and rerun; no result below comes from that attempt.

## Native result (Release Engine, 2560×1440, wgpu-native/Metal; harness `ray_transport_6fe_engine.py parity2b`, 18 s, no validation errors)
Poses `{0,8,16,24,32,40,48,56}` (per the Firmament's warning, `8,16,56` are inert controls). Per pose: Current, original routed, 2a, 2b normal, 2b poisoned scratch, 2b zero-budget refusal, 2b poisoned refusal. [JSON](GPU_RAY_TRANSPORT_6FE_STEP2B_PARITY_2026-10-29.json); repeated run also passes.

| Pose | S1 tiles orig → 2b | Fallback tiles orig → skip → 2b | 2b vs original publication |
|---:|---:|---|---|
| 0 | 98,085 → 98,247 | 44,008 → 44,008 → 43,846 | identical |
| 8 / 16 / 56 | unchanged | unchanged (inert) | identical |
| 24 | 37,197 → 149,224 | 193,203 → 120,020 → 7,993 | identical |
| 32 | 40,572 → 151,350 | 189,828 → 122,122 → 11,344 | identical |
| 40 | 35,695 → 140,252 | 194,705 → 124,969 → 20,412 | **1 pixel differs** |
| 48 | 48,441 → 137,182 | 181,959 → 129,490 → 40,749 | identical |

Pose 32 fallback 189,828→11,344 matches the Firmament's prediction exactly. S2 counts are unchanged everywhere. 2b poisoned == clean 2b; 2b zero-budget refusal == original refusal and == Current snapshots; poisoned refusal likewise.

**The deviation.** At pose 40 pixel (797,463) the original routed lane refused (published `w = -1`, so Current paints it) while 2b published transport `(0.0281, 0.0043, 0.0290, 0.0426)`. No accepted ray changed and no accepted ray became refused (0 and 0 on all poses). The §6fe census found exactly one reason-4 (degenerate coarse interval) fallback refusal at pose 40, consistent with a refusal that arises in a member the tile-level proof calls empty — which 2b, like S1/S2 already do for their unprocessed members, no longer evaluates. I did not isolate that ray's refusing member, so that mechanism is inferred, not proven. Consequence: 2b is **not** claimed bit-identical; it is exact except that a proof-skipped member's refusal can no longer force Current. That widens the §6fd budget contract ("work bounds work performed") to "refusals are decided by evaluated members" — a decision for Sixth-One Sun/Zach, not mine. Coverage: 8 poses of 64, 3.7 M rays each; poses 20,28,36,44 and the inside population are unchecked.

## Not done / handoff
- **Timing: see "Timing result (2026-10-30)" below.** Compare against Current is still not measured by this mode.
- Defaults unchanged; no ScreenChannel/Law exposure; wider-pose and inside-population parity still open.

Evidence class: native Metal bitwise publication/snapshot comparison (not timing, not Person evidence).

Signed: Claude Code / Sonnet 5.5 / worktree `authorable-gpu-runs-268d29` / 2026-10-29 (PDT).

## Decision recorded (2026-10-29)
Zach, in chat: **accept the change.** Contract for 2b: refusals are decided by the members the classifier proves wave-active; a proof-skipped member's refusal no longer forces Current (as S1/S2 already behave). This accepts the contract only. The toggle stays default OFF until timing, wider-pose (20,28,36,44) and inside-population parity are recorded; the pose-40 refusal mechanism is still unisolated.

## Timing result (2026-10-30)
Zach, in chat: "Reset and rerun" (after a first attempt timed out at the focus gate and consumed budget; I reset `earthcall-6fe-native-budget.json` to 0 on his say-so). Run: real Release Engine, 2560×1440, `skiptiming`, 4×4 Latin-square blocks per series, 64 frames per sample, fresh work dir, 212.5 s native time. The window was confirmed frontmost by `lsappinfo` on two consecutive checks before I created the gate file (every frame logged `focused: true`; handoff 2.0 s). Summary: [JSON](GPU_RAY_TRANSPORT_6FE_STEP2B_TIMING_2026-10-30.json).

| Lane (ms/frame, mean of 4) | queued | fenced |
|---|---:|---:|
| 0 original routed (uncoalesced) | 96.43 | 87.81 |
| 1 coalesced | 90.54 | 86.50 |
| 2 coalesced + skip-empty (2a) | 87.11 | 84.28 |
| 3 coalesced + 2b | 85.21 | 81.25 |

Paired per-block ratios (mean): 2b/coalesced **0.942 queued, 0.939 fenced**; 2b/2a **0.980 queued, 0.964 fenced**; 2a/coalesced 0.962 / 0.974. 2b beat 2a in 7 of 8 paired blocks; the exception is queued block 3 (1.043), so the queued 2b-vs-2a margin (≈2%) is within that run-to-run spread, while fenced (all four blocks 0.95–0.98) is cleaner. 2b/coalesced was below 1 in all 8 blocks (0.92–0.97).

**What this establishes:** with the pose-40 refusal contract, 2b is ≈6% cheaper than coalesced routed and ≈2–4% cheaper than 2a on this route, in one process, one device, one run (single repetition of the whole series). **What it does not:** this mode has no Current lane, so it does not say whether routed now beats Current (§6fb found coalesced slower than Current); poses 20/28/36/44 and the inside population are unchecked for parity, so 2b stays default OFF.

## Wider-pose and inside parity (2026-10-30)
Zach, in chat: "Run parity". New harness mode `parity2b_more` (same per-pose gates as `parity2b`: original vs skip-empty publication hash, clean vs poisoned 2b, forced zero-budget refusal equals Current, accepted-ray diff classification, S1-up/fallback-down list counters) on **outside poses 20, 28, 36, 44** and **eight inside-population poses** (route poses 0, 8, … 56 with the eye among the curtains; harness index 1000+k). 2560×1440, real Release Engine, 32.5 s native, all 12 poses PASS. [JSON](GPU_RAY_TRANSPORT_6FE_STEP2B_PARITY_MORE_2026-10-30.json).

- **Bit-identical publication to the original routed lane on 11 of 12 poses**, including all 8 inside poses (where 2b moved tiles heavily: e.g. inside pose 16 had all 230,400 tiles in Fallback originally and all in S1 under 2b; inside pose 8 and 24 similar) and outside 20, 28, 36.
- **Pose 44: 2 pixels differ** ((757,552), (1404,794)): original refused, 2b published transport. Same class as pose 40's one pixel. No accepted ray changed and none became refused (0 and 0 on every pose).
- Across all 20 poses now checked (8 earlier + 12 here), 3 pixels of ≈74 M rays change, all refusal→accept. This is exactly the behavior of the contract Zach accepted ("refusals decided by classifier-proved active members").
- **Still not done:** which member refused on the pose-40/44 pixels (inferred from the contract, not proven); 44 of 64 outside poses and the 8 other inside poses were not run; one device.
- Decision: parity gate is now met on 20 poses under the accepted contract; the remaining gate before a default change is the Current-vs-2b same-process comparison (Person list).
