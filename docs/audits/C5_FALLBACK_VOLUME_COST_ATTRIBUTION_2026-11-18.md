# C5 fallback volume cost attribution

Codex / GPT-6 / session `01a1eb67-9c6f-72c0-805f-5caceca627ad` / 2026-11-18T18:36:02-08:00.

Zach and Rendering Warrior Prism Sun asked which work explains the C5 slowdown with zero routed dispatches. This continues the [C5 Exit Tribunal](C5_EXIT_TRIBUNAL_2026-11-18.md). The measured scene is the immediate predecessor of Firmament's latest pass, the eight-field Northern Veil from `31c0bb1a2` (Zone SHA-256 `6c88b1f93f0e3f6b99a7880cabdcf5f5edbf9820632aa285e2d9a39fcc4ff5aa`). C5 is `69ab1586f93be5522c5ce7c260b11f3e784d548a`; default is `d7fb75cc6fa5fc7a0ffe94bdcedbdea572662c09`. Both see the same Zone bytes and materials in separate data roots.

## Native method

The native arm64 Release Engine uses a 2560 × 1440 framebuffer with the six pinned cameras from the exit tribunal, world clock 7.25 and simulation delta 1/60. The scratch launcher replaces only the benchmark entry point, forwarding context and diagnostic renderer object. The ordinary runs leave timestamp features off. Requested OFF–ON–ON–OFF blocks warm and drain the Metal queue before measuring three frames and draining again. The census captures the second warm frame before each timed block and records the submitted fragment WGSL at actual draw sites. The timing summaries use the median of the repeated block times per frame and six measured frames per view and request. The timestamp-only lane drains every frame so prior submitted samples come from the same fixed view. That lane requested a supported feature but produced zero valid volume-pass results and changed frame behavior; only the no-timestamp lanes inform the timings below.

The scratch work and machine-readable data live under `build/c5-attribution`. The diagnostic objects and shader variants stayed outside tracked renderer code. The frozen C5 head, support-certificate witness, Emerald thresholds, and authored and live Zone data were preserved through the attribution run.

## Draw census

Default's literal-zero incident-source branch draws authored indices 4–7 and then 0–3, with a four-medium fused program each. There are two full-viewport draws at 3,686,400 pixels each, for summed scissor area 7,372,800, in all six cameras and both requested modes.

C5 derives three First and five Second members from the current fields. The split gate requires four First members. Therefore whole-screen radial order and tile-region planning are skipped, and ordered First-before-Second, Second-before-First and Joint tile-plan counts are inapplicable. `explicitVolumeSchedule` is true, `scheduledSplit` is false, and the final schedule has one full-viewport Joint packet at 3,686,400 pixels. Its single fragment pipeline consumes all eight media through one draw instance. It has family tags and no First-activity mask. Default and C5 use unrouted physical pipelines in all measured requests.

Dense-inside and reverse views C and D draw the joint trim-off variant, and default switches only its second pipeline to the shorter variant. The other views retain the outside variants. The census records the exact per-view identity, byte size, flags and scissor rectangle. Shader source has a 16-entry event array in an eight-medium program and an eight-entry array in each four-medium program. The family-tagged variant also accumulates per-segment family bits and tests them at every medium's sample predicate. Those are separate variables for the diagnostic controls.

## Controls

- **A** retains the stock C5 path.
- **B** selects precisely the default cohort publication topology in the scratch C5 renderer for these inputs. All other C5 code stays linked. A legacy cohort split can change overlap/chronology semantics, so this is a causal witness with no shipping claim.
- **C** keeps C5's single full-viewport Joint packet and eight-media order, and removes only execution-family tags from that program. The scratch control refuses a split or a true First-activity consumer.
- **Draw suppression** keeps host setup, pipeline/parameter preparation and render pass packaging. It omits the actual volume draw call in the isolated run, so its logical renderer draw counter still includes the skipped draw. This supplies a differential check with timestamps off.
- The proposed forced full-viewport Joint packet is already A's physical draw topology on all six views. Repeating that shape would add no control.


## Results

Completed Metal batch time per frame, in milliseconds, with Routed requested ON. The diagnostic lanes retain the raw per-view runs.

| View | Default | A: stock C5 | B: legacy 4 + 4 inside C5 | C: joint 8, tags omitted | Volume draw suppressed, default / C5 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Spawn | 111.0 | 175.0 | 111.0 | 170.3 | 7.8 / 8.1 |
| Main Veil | 136.6 | 207.7 | 139.1 | 205.4 | 16.6 / 16.1 |
| Dense inside | 184.2 | 335.8 | 187.0 | 315.7 | 16.0 / 16.4 |
| Reverse camera | 173.2 | 293.4 | 171.7 | 290.8 | 14.6 / 16.8 |
| Far north | 135.4 | 200.9 | 134.3 | 199.3 | 16.2 / 16.9 |
| Far south | 509.3 | 718.1 | 508.4 | 709.2 | 16.3 / 16.4 |

At all six views and both requests the ordinary runs report warm median routed sets, routed pixels, parameter bytes, proof bytes and volume compiles of zero, with eight cache hits. The default frame totals are six draws and 312 triangles, and stock C5 reports five draws and 300 triangles. Default CPU volume-prepare medians span 6.7–13.0 μs. C5 spans 12.6–19.1 μs. The ON gap is 64–209 ms per frame. Both timestamp-enabled builds reported feature support and zero valid volume-pass samples across 18 timing observations each. Treat those timing fields as unavailable. The independent draw-suppression control reduces the ON inter-build gap to at most 2.2 ms, at 7.8–16.9 ms completed time. That intervention isolates the expensive work to execution of the volume draws. Those residual times include the rest of the frame and presentation pacing.

### Exact shader census

Names below are stable FNV-1a-64 fingerprints of the WGSL submitted to module creation. The census JSON includes full SHA-256, the per-frame packet rectangles and multiplicities, and each saved WGSL module. All shown physical pipelines are unrouted, have one draw instance per packet and carry no First-activity mask.

| Build and view | Program IDs in draw order | WGSL bytes in draw order |
| --- | --- | --- |
| Default A/B/E/F | `edd1873a1f7b280d`, `0bab28f6103a63a2` | 97,332 + 82,204 |
| Default C/D | `edd1873a1f7b280d`, `f7ab3404c4329b72` | 97,332 + 78,374 |
| C5 A/B/E/F | `eba60b98aace5a5a` | 158,565 |
| C5 C/D | `2907418542ec52c6` | 153,527 |
| C5 bypass B, A/B/E/F | `201422b58a876a5b`, `4275415b955d248c` | 97,334 + 82,206 |
| C5 bypass B, C/D | `201422b58a876a5b`, `0739aca3d24b3bcc` | 97,334 + 78,376 |
| C5 untagged C, A/B/E/F | `921f9333b7dd1982` | 157,855 |
| C5 untagged C, C/D | `ce65ae20b204da22` | 152,817 |

All three bypass programs differ from their corresponding default shader by exactly two added blank lines. The corresponding preparation WGSL is byte-identical. The two untagged joint programs differ from stock C5 only by the family mask declaration, its per-segment accumulation and the corresponding per-medium predicates. Their preparation WGSL is byte-identical to stock C5.

## Causal verdict

The 3/5 capability result selects the full-screen joint eight-medium program. That is the costly path on this scene. Selecting the previous two-draw topology inside C5 brings all six views and both mode requests within 3.2% of default frame time. The stock C5 path uses fewer packets and half the summed scissor area, eliminating a packet-count explanation. Removing family tags reduces Routed-ON time by at most 6.0% and leaves the eight-medium program slower than default at every camera. The slow joint draw remains after removing the tags.

The two smaller programs solve different physical subproblems from one unified eight-medium integration. The experiment identifies the draw selection and execution cost; the share due to event/sample work, Metal register pressure, spilling or occupancy is still unmeasured. B also bypasses C5's chronology proof and cannot stand in for a correct general policy. These experimental lanes were exercised for attribution. The production C5 and renderer contract remain as they were. PR #882 stays blocked on performance, with live Person review pending. The [inherited Emerald proof refusal](CURRENT_EMERALD_QUADRATURE_CERTIFICATE_EXPLOSION_2026-11-18.md) retains its existing disposition.
