# Volume II — reverse-camera rectangle cost diagnostic

Sixth-One Sun (this original session, previously signed Sixth-Two before Zach split the chats) / Codex GPT-6 / session `01a1eb67-9c6f-72c0-805f-5caceca627ad` / 2026-11-20. Completion timestamp is in the signed Volume II reply.

This continues Zach's next-pass commission, the [optical/subset receipt](C5_OPTICAL_LOWERING_AND_CAMERA_SUBSET_2026-11-20.md), and [Sixth-Two Sun's independent review](../../agent%20intercom/communication-threads/sdf-and-rendering/VOLUME_II_C5_SUN_PRISM_HORIZON_2026-11-19.md#sixth-two-sun--sixth-one-sun-prism-sun-and-ef5-horizon--independent-review-of-joint-subset-lowering). The reviewer identified a diagnostic 11-rectangle cover from the saved reverse-camera ray-box masks and proposed a three-way cost control. That proposal and its zero/one-candidate integration warning originate with Sixth-Two, Codex GPT-6.2 Sol, session `01a1f80a-9495-7660-9790-c9d8400b5b06`.

**Verdict:** the recorded-camera Joint4/Joint7 cover costs **170.58 ms**, versus **230.73 ms** for full-screen Joint7 and **227.50 ms** for Joint7 drawn through the identical rectangles. All six repeated native captures match stock C5 exactly. This identifies a useful physical-program membership opportunity in reverse. It does not implement or qualify a runtime tile classifier; PR #882 is not cleared for shipping or Person acceptance.

## Frozen inputs and implementation boundary

- Tested detached C5 HEAD `69ab1586f93be5522c5ce7c260b11f3e784d548a` in `/Users/zacharyzhang/.codex/worktrees/c5-summit-tribunal/Earthcall`.
- Main checkout `/Users/zacharyzhang/Documents/GitHub/Earthcall`, branch `prism/c5-current-default-reconciliation-ii`, was `641b204ccaa5bc42a3988aad44508990a33f0ac5` during this control. Its renderer/test sources match the frozen tested source; its newer report commits are not a newly built renderer.
- Restored eight-field Northern Veil from `31c0bb1a2`, SHA256 `6c88b1f93f0e3f6b99a7880cabdcf5f5edbf9820632aa285e2d9a39fcc4ff5aa`; isolated fixture, native arm64 Release, Metal, 2560 × 1440, fixed clock 7.25.
- D eye `(24.2272, 50, 75.83955)`, target `(48.4544, 38, 51.67910)`; original preflight verifies eye/direction/resolution/focus each frame.
- All controls use camera subset + retained proof memo lifetime + exact-order optical helpers from the preceding pass. Requested Routed ON, zero routed sets/pixels. No timestamp timing, new support policy, threshold or sample-budget change.
- Experimental implementation is entirely in ignored `build/c5-reverse-rectangles/` and the two new preparation/runner scripts in ignored `build/c5-optical-frustum/`. No production renderer/test/save edit, installation, commit or push.

## Independent reconstruction of the diagnostic cover

I independently ORed the saved native raw and enlarged ray-box masks over a 20 × 15 tile grid, 128 × 96 pixels per tile. Source is `build/c5-optical-frustum/rayhit-axis/D.bgra.gz`; uncompressed SHA256 `4b251de28efec1f26f4fca8d62e29f93e4aaf686d63fc9152a3440e7463afd62`. Every viewport pixel carries the diagnostic marker, and raw/enlarged tile unions agree.

| Candidates in tile | Tiles |
| --- | ---: |
| 4 | 240 |
| 5 | 17 |
| 6 | 23 |
| 7 | 20 |

The 240 four-only tiles always contain M0–M3. M4 has no hits anywhere. For every other tile this experiment retains all seven global candidates M0/M1/M2/M3/M5/M6/M7, avoiding additional shader variants. Merging identical horizontal runs down consecutive rows produces six four-member and five seven-member rectangles. An independent coverage check visits every pixel, rejects overlaps/gaps, and requires raw/enlarged mask `0x0f` at every pixel assigned to Joint4.

Joint4 area is **2,949,120 pixels (80%)**; Joint7 area **737,280 pixels (20%)**. This is a measured-camera diagnostic cover, not a runtime or continuous-camera theorem. Its CPU mask scan occurs offline and is excluded from timing; no classifier cost is measured or claimed.

## Preserve one Joint answer at every pixel

Each pixel belongs to exactly one scissor rectangle and receives one complete Joint transport answer. There is no independent family compositing, transport-state handoff or invented chronology.

The ordinary cohort enqueue creates a Joint4 program from M0–M3 with their original order and execution-family tags. Both pipelines are unrouted and have no First-activity mask. After enqueue, the Joint4 header is copied from Joint7: union origin `(-2,61.5,32)`, half extents `(174,70.5,130)`. Thus the proxy geometry, clipped set-level ray envelope, viewport reconstruction and depth clipping remain those of the combined Joint7 baseline. Each retained medium keeps its own origin, extent, temporal coordinate, parameters and proof metadata.

Omitted boxes have no raw or enlarged native hit at any pixel assigned to Joint4. They therefore contribute no accepted per-ray entry/exit events or chord spacing at these measured pixels. This differs from deleting box-hit media on the strength of sampled zero optical evaluations. The ordinary shader keeps its event walk, proof gates, sample rule, early termination and one continuing `(L,T)` recurrence. Exact native output, including all rectangle boundaries, is required; no tolerance is introduced.

## Three-way native cost control

All three modes use the same scratch executable, scene, camera, helper maps and memo lifetime. Both pipelines are enqueued in every mode; only the publication schedule changes. The full mode draws Joint7 once; rect7 draws the same program over all 11 rectangles; mixed uses the same rectangles and substitutes Joint4 in the six four-only rectangles.

Run order is full-1 → rect7-1 → mixed-1 → mixed-2 → rect7-2 → full-2. Each run records two three-frame completed batches after drawn-frame warm-up, with captures untimed. The table is the median of the four per-frame batch means for each mode, totaling 12 timed frames/mode.

| Reverse diagnostic | Volume packets | Unique shaders drawn | Median completed ms |
| --- | ---: | ---: | ---: |
| Full-viewport Joint7 | 1 | 1 | 230.73 |
| Same Joint7, 11 rectangles | 11 | 1 | 227.50 |
| Joint4/Joint7, same 11 rectangles | 11 | 2 | 170.58 |

| Mode | Four completed-batch means, ms/frame |
| --- | --- |
| full | 233.508, 234.223, 225.749, 227.942 |
| rect7 | 225.043, 227.714, 227.284, 230.119 |
| mixed | 172.641, 169.564, 167.376, 171.600 |

Rect7 is about 1.4% faster than full in this short series; this small difference does not establish a submission/scissor gain. Mixed is **25.0% faster than rect7**, saving about **56.92 ms** at the same packet cover, and **26.1% faster than full**. Volume CPU preparation medians per run span 0.0163–0.0229 ms. Warm compiles/refusals, proof builds, parameter/proof uploads and routed sets/pixels are all zero. Total application draw calls/triangles are 5/300 for full and 15/420 for either rectangle mode.

This supports profitable physical shader specialization under the recorded absence mask. It does not identify registers, spill traffic, occupancy or a universal media-width threshold. No new Metal spill trace is taken. Desktop/compositor variation and the short series limit precision.

The mixed result is approximately 0.85% above the **separately measured** 169.15 ms frozen pre-C5 D reference. That reference is the retained `d7fb75cc6fa5fc7a0ffe94bdcedbdea572662c09` executable from the earlier pass, not a new simultaneous baseline run. This demonstrates plausible headroom to recover the reverse regression; it does not establish production parity, statistical no-regression, or Zach's requested Volume I sampler result.

## Actual draw and exactness receipts

| Program | Members | Fingerprint | WGSL bytes |
| --- | ---: | --- | ---: |
| Inside Joint4 helpers | 4 | `48b7a6a4547bc289` | 79387 |
| Reverse Joint7 helpers | 7 | `4aaab0bc34247525` | 135811 |

Every measured draw's complete source bytes match the corresponding preceding combined-control source. The checker verifies ordered cohort membership, exact packet sequence/coordinates, total scissor area, header equality, program sizes, resolution/clock/HEAD and warm telemetry. Each measured mixed schedule has six Joint4 and five Joint7 draws. All six captures equal both the stock C5 D image and the preceding combined D image byte-for-byte, SHA256 `e79add73e52dde3fbb11c92cde9f3afdfb1ad55fc4bbde3ad5d569fd7125c597`.

Artifacts: `build/c5-reverse-rectangles/layout.json`, `Rectangles.hpp`, `results.json`, `results.md`, six run folders with metrics/source/logs/compressed frames, `suite.py`, `check.py`, and the original application link log. `build/c5-optical-frustum/prepare_rectangles.py` reconstructs the mask/layout; `run_rectangles.py` generates the scratch insertion. The first launch aborted before a measured draw because my diagnostic guard used the union-bound center instead of the camera eye. Correcting that guard changed no camera or scene input; rejected logs remain in `rejected-camera-guard/`.

## Promotion boundaries and next pass

The next implementation question is conservative tile admission over actual shader rays. A runtime classifier must account for inverse-projection arithmetic, the slab function's `safeRd` substitution, grazing/near-parallel directions, inside cameras and resizing; uncertainty retains the wider Joint program. It must also preserve the stock set envelope and per-ray event/chord/sample semantics. The offline observed mask is not that authority. Shader variants/cache lifetime and classifier/submission economics need independent measurement.

Sixth-Two's zero/one warning is valid: the earlier camera filter can reduce an originally fused set to zero/one active candidates, then the V0–V4 branch iterates original `volumeDensitySources()`. The measured four/seven cases never exercise this. Before promotion, unsupported zero/one outcomes must retain the original path choice until explicit native witnesses close the integration boundary. This diagnostic requires seven global candidates and does not repair or normalize that general case.

The proof-memo lifetime repair remains independent, with its previous hidden-edit/extent/removal witnesses. Emerald thresholds and the settled support-certificate dossier remain unchanged. A separately identified Volume I legacy sampler remains a required historical comparison alongside the frozen pre-C5 branch reference at the final performance gate; a requested Routed toggle does not identify the executed sampler.

**Merge / Person gate: CLOSED.** The recorded rectangle control closes the cost-attribution question and supplies headroom for a bounded tile-admission prototype. It does not land a renderer repair. Complete numerical admission, zero/one integration and cache review, then qualify the production candidate against current default and the identified Volume I sampler, exact-head retained/CI guards and Zach's six-view visual/performance tribunal. The installed app is unchanged.
