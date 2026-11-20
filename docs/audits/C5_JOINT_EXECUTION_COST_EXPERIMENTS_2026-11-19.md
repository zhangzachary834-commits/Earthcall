# C5 Joint execution cost experiments

Sixth-Two Sun / Codex GPT-6 / session `01a1eb67-9c6f-72c0-805f-5caceca627ad` / 2026-11-19T00:17:59-08:00.

Zach and Rendering Warrior Prism Sun commissioned three controls for Joint8 and an equal-width membership sweep. This continues the [chronology experiment](C5_CHRONOLOGY_CARDINALITY_EXPERIMENT_2026-11-18.md). That experiment held the rendering obligation at one joint ray chronology under the existing proofs. None of the forced framebuffer partitions in this record carries chronology authority.

## Pinned run

C5 is `69ab1586f93be5522c5ce7c260b11f3e784d548a`; default is `d7fb75cc6fa5fc7a0ffe94bdcedbdea572662c09`. Both arm64 Release engines loaded the same eight-field predecessor of Firmament's latest pass, Zone `31c0bb1a2` and SHA-256 `6c88b1f93f0e3f6b99a7880cabdcf5f5edbf9820632aa285e2d9a39fcc4ff5aa`. The same six poses, materials, fixed world clock and desktop Mac are used throughout. Ordinary timings are 2560 × 1440. The G2 fixed-sample diagnostic uses 1280 × 720. Each timing cell is the median of two native batches with three completed frames per batch. Routed was requested ON. Native timestamps are disabled after the earlier invalid-query result. No register, spill or occupancy counter is inferred from these timings.

The code and raw outputs live under `build/c5-joint`. Runtime patches key on exact input WGSL fingerprints. The verifier checks the actual drawn shader bytes against the expected patch, with no silently unmodified measured shader. Engine and compiler production source, proof budgets, certificate policy, and both saves stayed as they were.

## G1 · Front-end and support walk

The ray-box events, chord spacing, event sort, support/zero-proof walk, predicates and single transport update are retained. Procedural density, extinction, scattering, phase, chroma and emission are replaced after the proof gate with a tiny synthetic extinction and varying synthetic source. The source keeps the walk observable and the tiny extinction keeps the alpha cutoff from erasing much of the synthetic work. Consequently the stock and synthetic lanes do not necessarily have the same number of active samples or the same termination. Compare the synthetic Joint and synthetic split against each other.

## G2 · Fixed optical work

These diagnostic shaders replace the front-end with a full-viewport triangle, a fixed ray-distance range of 0–128 and a fixed number of positions. They bypass the opaque intersection, event and support computation, and early termination. At each position, every member is visited using a deterministic remap of the ray sample into that member's local bounds. Normal authored optical evaluators and their internal density and source gates are retained. The diagnostic visits the same members and uses the same synthetic coordinates for each member in the single-draw and split lanes. Color has no correctness meaning in either lane. The initial 12-sample pass was near presentation pacing and serves only as a low-load observation. A 192-sample pass raised the cost beyond the pacing floor.

## G3 · Sample-local shared evaluator values

Stock already computes one medium's optical contribution at a time and folds directly into shared `totalExtinction` and `totalSource` in authored order. There is a single transport update per sample. The two stock shaders for the six views each declare eight fragment-private f32 common-expression slots, one per medium. Each scoped medium block writes its slot before the density, extinction, scattering and emission consumers read it.

The scratch variant replaces those eight slots with one scoped SSA value at a time and passes that value to the original consumer functions. The same expression executes and the ray/event/sample positions, support state, accumulation order, transport arithmetic and exit decisions are preserved. The preparation shaders were kept byte-identical. Exact frame comparison uses a native color copy encoded before the UI overlay and mapped after frame completion in a separate untimed frame. A volume-suppressed render of each view witnesses that the volume changed native color output. This test covers six matched native images of this scene.

| View set | Stock fragment FNV-1a-64 / bytes | Sample-local fragment FNV-1a-64 / bytes |
| --- | --- | --- |
| Spawn, Main Veil, Far north, Far south | `eba60b98aace5a5a` / 158,565 | `5a7c42b59a505680` / 158,213 |
| Dense inside, Reverse camera | `2907418542ec52c6` / 153,527 | `6e4cf12c591fc0d0` / 153,175 |

## H · Same-width membership controls

Numbers are zero-based positions in the active medium list. The list is Emerald, Cyan, Violet, Crimson, Far-horizon helical, Far-zenith coronal, Far-southern twin braid, Far-southern gossamer, in that order. Each configuration draws two full-viewport ordinary fused programs, each carrying four media. The two groups publish in the listed order.

- Legacy uses `[4,5,6,7]` then `[0,1,2,3]`.
- Alternating uses `[1,3,5,7]` then `[0,2,4,6]`.
- Pairs uses `[2,3,6,7]` then `[0,1,4,5]`.

The forced controls measure the combined consequences of membership on execution. They cannot isolate compiler register allocation from differently grouped per-ray sample spacing, supports, and optical evaluation.


## Results

Native completed frame times in ms at 2560 × 1440, with default and forced 3+5 reference lanes.

| View | Default | Stock Joint8 | G3 sample-local Joint8 | Forced 3+5 |
| --- | ---: | ---: | ---: | ---: |
| Spawn | 111.9 | 176.7 | 174.3 | 68.7 |
| Main Veil | 138.1 | 208.6 | 208.9 | 89.6 |
| Dense inside | 187.4 | 334.2 | 334.1 | 133.2 |
| Reverse camera | 169.6 | 298.7 | 296.4 | 84.9 |
| Far north | 134.0 | 201.2 | 202.4 | 89.9 |
| Far south | 507.8 | 731.2 | 727.9 | 592.8 |

G1 at 2560 × 1440 and G2 with 192 fixed optical samples at 1280 × 720. Each pair is a separate experiment.

| View | G1 Joint8 | G1 3+5 | G2 Joint8 | G2 3+5 |
| --- | ---: | ---: | ---: | ---: |
| Spawn | 73.4 | 35.9 | 344.8 | 289.2 |
| Main Veil | 90.1 | 47.1 | 387.2 | 309.4 |
| Dense inside | 102.4 | 50.0 | 391.4 | 319.4 |
| Reverse camera | 95.4 | 27.8 | 310.1 | 261.4 |
| Far north | 88.0 | 46.0 | 391.7 | 318.8 |
| Far south | 151.5 | 111.7 | 374.5 | 304.3 |

H at 2560 × 1440. All three pairs use four media per program.

| View | Legacy 4+4 | Alternating 4+4 | Pairs 4+4 |
| --- | ---: | ---: | ---: |
| Spawn | 114.4 | 65.5 | 66.3 |
| Main Veil | 136.9 | 86.8 | 92.7 |
| Dense inside | 186.1 | 113.7 | 127.6 |
| Reverse camera | 174.5 | 67.8 | 152.1 |
| Far north | 136.9 | 85.8 | 87.5 |
| Far south | 512.4 | 511.1 | 513.3 |

- **G1.** Joint remains 37.5–67.6 ms slower than split with authored optics replaced. The box-event and support-walk form retains a substantial cost. The stock and synthetic differences are not additive because the two variants can visit different numbers of samples.
- **G2.** The high-load fixed optical Joint program is 18.6–25.2% slower than two optical programs visiting the same cases, with no event/support walk. The per-view cost is well past the low-load pacing floor. This establishes an optical-only effect for the tested layout and says nothing about actual register allocation.
- **G3.** The local-sharing rewrite matches the stock image byte-for-byte in all six native views. Removing volume changed 239,695–3,346,344 pixels per view, so these are nontrivial volume witnesses. Frame times differ by at most 1.4%. Making the eight named fragment-private slots sample-local leaves the performance blocker in place.
- **H.** At fixed width four, reverse-camera cost spans 67.8–174.5 ms, about 2.57× between the fastest and slowest partitions, while far-south costs cluster near 512 ms. The original field grouping therefore matters for the observed physical cost. These controls change the grouping of event/support and optical work and cannot assign the swing uniquely to compiler pressure.

**Disposition.** Both the event/support structure and the optical-only program sustain a Joint penalty when the other component is removed in a controlled diagnostic. The existing shader already folds one medium at a time. Shortening the source-level lifetime of its shared evaluator values preserves the result but leaves the economics essentially unchanged. PR #882 remains blocked on performance. There is no promoted implementation, new chronology claim, or Person acceptance from these scratch controls.

See `build/c5-joint/results.json` for all per-view medians, warmed batch values, full shader SHA-256 values, byte counts and observed scissor draws. See `build/c5-joint/pixel-comparison.json` for the native scene hashes and positive controls.
