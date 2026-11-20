# C5 chronology scheduling and physical program width

Codex / GPT-6 / session `01a1eb67-9c6f-72c0-805f-5caceca627ad` / 2026-11-18T18:45:18-08:00.

Zach and Rendering Warrior Prism Sun asked whether C5 could avoid the expensive Joint8 draw by letting the existing chronology proofs consider a First family of three media. This continues the [fallback cost attribution](C5_FALLBACK_VOLUME_COST_ATTRIBUTION_2026-11-18.md).

## Scope

The pinned C5 Release renderer is `69ab1586f93be5522c5ce7c260b11f3e784d548a`. Every run loaded identical fixture copies of the eight-field Northern Veil from `31c0bb1a2`, SHA-256 `6c88b1f93f0e3f6b99a7880cabdcf5f5edbf9820632aa285e2d9a39fcc4ff5aa`. The benchmark uses the six tribunal cameras, a fixed world clock and 2560 × 1440 native Metal. For each view and switch setting, two blocks measure three frames each and wait for completion. Cold frames and the draw census precede each measured block. Timestamp instrumentation stays off.

All modifications to the renderer for these tests are transient copies under `build/c5-chronology`. They link with the existing arm64 Release Engine. The live Person Data Home and the previously restored checkout Zone were left in place.

## Experiment E

The scratch gate permits the existing split-proof path for a literal-zero incident source when each derived family can be lowered by the existing fused-set program. The routed backend still needs four First members. A three-medium First lowers to ordinary fused execution. The conservative radial proof and the existing 128 × 96 tile planner retain their original logic. The program records each projected support interval, proof region, prepared pipeline and actual scissored packet. A separate verifier reconstructs tile classifications from the logged supports and checks packet order and coverage.

## Experiment F

The forced layout publishes the derived Second5 and then First3 programs at full viewport. It establishes a physical timing for those two programs, with no proof of their relative chronology. The optional 2+6 moves one member from First into Second and uses the same diagnostic order. The legacy 4+4 and stock Joint8 controls are rerun using the same native timing code. Group membership affects the math inside each program, so differences among these forced layouts include both width and composition. A 1+7 width would require a different single-medium path and was omitted.


## Native results

Every E view returned **Unknown** for whole-screen radial order. The tile planner classified all 300 tiles in each view as Joint, coalesced into one full-viewport region. There were zero First-before-Second regions and zero Second-before-First regions at all six cameras. Both families had projected support in every tile, and their conservative radial intervals overlapped within each tile. The verifier replayed 7,200 tile decisions across 24 warm census frames and matched every published packet. These conservative certificates establish the limits of the current planner on this scene. They give no count of actual same-ray intersections.

| View | First whole-screen radial interval | Second whole-screen radial interval | First / Second / Joint draw pixels |
| --- | ---: | ---: | ---: |
| Spawn | 28.00–256.58 | 44.00–248.51 | 0 / 0 / 3,686,400 |
| Main Veil | 10.00–232.46 | 56.00–228.79 | 0 / 0 / 3,686,400 |
| Dense inside | 0.00–224.05 | 0.00–212.96 | 0 / 0 / 3,686,400 |
| Reverse camera | 0.00–224.05 | 0.00–212.96 | 0 / 0 / 3,686,400 |
| Far north | 12.00–234.74 | 58.00–230.66 | 0 / 0 / 3,686,400 |
| Far south | 40.00–252.47 | 32.00–250.55 | 0 / 0 / 3,686,400 |

First3 and Second5 were both prepared in E. The only drawn program was Joint8, instantiated and drawn at every view. All its fragment WGSL fingerprints and packet rectangles match stock A for the same cameras and switch settings. E and A stayed within 1.85% in completed frame time. All five lanes issued zero routed sets and zero routed pixels.

Completed frame times in ms with Routed requested ON. Each entry is the median of two completion-checked blocks of three measured frames. OFF repeats the same qualitative result, including the far-south reversal between 3+5 and 4+4.

| View | A stock Joint8 | E chronology | F forced 3+5 | B legacy 4+4 | F2 forced 2+6 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Spawn | 172.3 | 175.5 | 70.1 | 114.2 | 64.0 |
| Main Veil | 207.1 | 210.3 | 91.8 | 147.7 | 90.2 |
| Dense inside | 333.8 | 333.2 | 132.7 | 213.1 | 106.9 |
| Reverse camera | 291.3 | 295.6 | 87.1 | 180.9 | 95.8 |
| Far north | 202.5 | 199.8 | 90.6 | 141.6 | 97.1 |
| Far south | 723.6 | 719.2 | 587.1 | 531.7 | 715.6 |

The two focused CPU chronology witnesses passed. Native output and the verifier are under `build/c5-chronology`: `telemetry.tsv` contains all 60 view/mode rows, and `checked-census.json` records projected bounds, packet order, targets, scissors and exact shader hashes.

## Disposition

Separating backend eligibility from proof admission reaches the existing planner. The existing proof cannot certify inter-family chronology on any tile at the chosen views, so E leaves this performance blocker in place. The forced 3+5 diagnostic cuts 19–70% from Joint8 frame time with Routed requested ON and beats forced 4+4 at five of six views. The far-south view favors legacy 4+4, and forced 2+6 returns to roughly Joint8 time there. A five-medium set can be economical. The observed costs vary with the field membership and camera, with width and group composition coupled in these controls.

The remaining problem is the cost of transport in regions where chronology stays unresolved. Further work would need stronger sufficient certificates or a physical decomposition that preserves a continuing ordered radiance/transmittance state. Forced publication is a cost control and carries no shipping authority. No production renderer, Emerald proof budget, support-certificate policy, or save file changed during this attribution experiment. PR #882 remains blocked on performance. Person visual acceptance remains open.
