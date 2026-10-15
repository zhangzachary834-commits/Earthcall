# Support-focused proof allocation: bounded native trial

**Status: all three bounded implementation trials rejected on measured cost; production renderer restored. Exact native images pass; no new FPS gain.**

Zach commissioned Astra's review and explicitly authorized the handoff to the Sun. This implements the [support-focused allocation brief](../../agent%20intercom/communication-threads/sdf-and-rendering/NORTHERN_VEIL_COUNSEL_REQUEST_TO_GPT_6_ASTRA_2026-10-11.md#astra-to-the-sun--support-focused-allocation-brief-after-firmament-review), following Firmament's subbox screen and the [existing demand/support plan](../plans/VOLUME_DEMAND_AND_SUPPORT_IMPLEMENTATION_2026-10-15.md). No new research agents or author verdicts are introduced. The [host Euclidean range entrypoint](VOLUME_EUCLIDEAN_RANGE_2026-10-15.md) remains excluded from native proof consumption.

## Explicit representation

| Quantity | Meaning and consumer |
|---|---|
| Authored origin/half extent | Admission-box events, sample lattice and integration; unchanged |
| `gridMin/gridMax` | Local physical domain of the packed bitmap cells; CPU cell proof, shader indexing, leaf exits and band geometry agree on this domain |
| `supportMin/supportMax` | Separately padded envelope of every unproven cell; ray support bounds, never substituted as the bitmap normalization domain |
| Column bands | Envelope of all unknown cells in a chosen slab of the same grid; prior axis-ranking/rate formula and outside-only eligibility remain |

The experiment adds six explicit grid-domain floats after the six existing support floats. Packed words retain their format; support is at words+0..5, grid at words+6..11, optional bands at words+12. `attachVolumeZeroProof`, both camera lanes, shader index/exit readers and dirty-upload accounting use this layout. Instance layout and authored parameter order are unchanged. Array dimensions still use the existing packed ten-bit axes. Representation bytes include metadata and bands, not only bitmap bits.

A point outside the bitmap domain returns Unknown and evaluates density exactly, rather than clamping an unrelated edge bit. In particular, native near-parallel rays can still traverse the admission box outside the cropped domain. Those rays invalidate the absolute claim that outside-support cells are unreachable. The existing near-parallel fallback and authored quadrature remain.

## Isolated allocation candidate

Build the existing full-box proof with **the old `evalRange` operators and current density semantics**. Its padded unknown-cell envelope contains every original unknown cell. Clip that envelope to the admission box, allocate the same near-cubic target inside it, and certify the new enlarged cells using the same producer. The complement of the cropped grid is covered by the original zero-cell union. Empty/unknown/invalid/no-benefit cases retain the original proof. Pilot and final proof dependencies are the same density revision and half extent, cached together by the existing owner.

This uses no new Y-invariance theorem or mathematical operator. The grid keeps all three axes; the existing near-cubic selector is retained. It does not bundle a macro index, full-Y cells, inside-column activation or arithmetic cleanup. The prior coarse-index rejection remains distinct evidence. The current column eligibility formula is unchanged; its geometric inputs naturally reflect the new grid, and the inside lane remains disabled.

Construction is deliberately separate from warmed rendering: this two-pass candidate can rebuild substantially more range work on admission/edit. The experiment records three direct host constructions per medium, cold frame submission/preparation and proof/parameter upload bytes, edit costs, then separate readback-free sequential ABBA batches after image parity. Host construction timings taken alongside correctness work are descriptive observations, not isolated paired CPU speedup claims. In this trial the retained-proof `rangeEvaluations` counter omits a discarded focused-build attempt when that attempt falls back to the pilot; wall-clock construction samples include that work. The count is therefore a lower bound in fallback cases, not a complete work counter. A promoted implementation would have to correct that accounting.

The candidate inherits the legacy scalar-density contribution rule without selecting AD-30. A complete retained Fast-profile numerical/coordinate/consumer enclosure remains unfinished under AD-36. Finite captures corroborate this trial; they cannot silently authorize broader native proof permission. Any remaining admission obligation will be reported explicitly and the candidate kept experimental.

## Results and disposition

### First increment: isotropic support allocation — rejected

All 32 comparisons at 2560×1440 are byte-identical, across both frozen Zones/four views/two times/both modes. CPU proof/domain checks pass 52/52; native proof/boundaries 73/73, sharing/power 76/76 and quadrature 15/15 pass without device skips.

Predeclared aurora/inside, two sequential ABBA series of 24-frame batches:

- Control mean 136.2811 ms, candidate 160.0650 ms: **+17.4521% time**.
- Control mean 141.1648 ms, candidate 165.8016 ms: **+17.4525% time**.

The runner stopped remaining views after the primary gate failed. These are readback-free completion-fenced offscreen batch means, not isolated GPU durations or live FPS. The recorded final-frame warm lowering, compilation, parameter and proof uploads are zero; the fixture does not aggregate those counters across every timed frame.

Cold first-frame CPU preparation/submission is approximately 1.43–1.53 seconds for controls and 4.43–4.58 seconds for candidates; it includes shader admission/driver work and proof construction, so it is not an isolated proof duration. All runs upload 1,020 parameter bytes; inside proof payload totals are 22,892 control versus 22,840 candidate bytes, including metadata. Smaller bytes do not imply faster rendering. Direct three-sample host proof construction separately records each medium; absolute times are descriptive under concurrent correctness load. The structural density-replacement native witness reports 1,510.58 ms first-frame CPU work, two shader compiles, one proof build, 408 parameter bytes and 7,024 proof bytes; this is not a layout-preserving numeric edit.

The first candidate is rejected and archived at `candidate-source/`, with its exact binaries/images/economics. This does not refute finer full-axis/coarse layouts.

### Second increment: full-axis allocation — rejected

The next candidate uses repeated occupied pilot-slice masks solely as a resource hint. Empty outer slices are ignored; remaining slice equality is not elevated into a mathematical invariance theorem. Every final cell is certified again across its **entire collapsed axis** with the existing scalar range producer. Resolution between the other two axes follows pilot boundary-transition counts and the existing packed-axis limit, within the requested bit budget. No axis name or Zone identity is hardcoded.

A first allocation draft underfilled the requested budget under extreme anisotropy; a subsequent slab witness also exposed excessive reconstruction work (8,678 range evaluations for 4,095 retained cells). Those failed CPU logs are preserved. The final resource selector declines the proposed two-axis layout if either remaining pilot direction has no boundary transitions, preserving the slab's cheap original proof. The original assertion is unchanged. This gate affects allocation economics, never whether a cell is certified. The guarded version passes CPU 52/52, native proof/boundaries 73/73 and all 32 exact native images. The re-proving semantics were unchanged by the allocation screen.

Paired aurora/inside means: **121.0035 -> 177.0099 ms (+46.2850%)** and **137.4454 -> 186.8874 ms (+35.9722%)**. Primary gate failed; remaining timing views were not run. The structural density-replacement witness reports 1,816.44 ms CPU preparation/submission, two compiles, one proof refresh, 408 parameter bytes and 9,876 proof bytes. As above, this is not an isolated numeric edit or isolated GPU duration.

### Third increment: guarded AND-parent traversal — rejected

The final candidate reserves budget for aligned 6×6×6 parents, clipped at grid edges (one axis has only one cell). Each parent is the AND of every fine child. No new density theorem is introduced. Fine plus parent **logical certificate bits and their two separately padded 24-bit word banks** fit the 32k logical-bit/packed-word budget for the selected hierarchies; support/grid metadata and bands are additional and are included in the reported total bytes. Stored f32 words hold 24 logical bits each, so logical-bit count is not physical storage-bit count.

Storage-present and instance arithmetic-eligibility flags are distinct. Column-axis readers mask off these new bits. Parent word offsets follow support/grid metadata and optional bands in both lanes; upload accounting includes every bank. Near-parallel or exceptional cases retain the existing fine walk. The parent-exit region is shrunk by a fine-cell margin plus the existing translated-coordinate padding. Existing inside-column exclusion remains. These screens are not a complete Fast-profile numerical proof.

CPU 56/56 includes exhaustive parent-implies-all-children and combined packed-budget checks. Native proof/boundaries pass 73/73 and all 32 native images are exact. Paired means are **117.2294 -> 146.9839 ms (+25.3814%)** and **133.5377 -> 161.3725 ms (+20.8441%)**. The primary gate fails in both series. Other timing views were not run.

The parent implementation can add one parent-word lookup after an eligible fine-zero result, along with index/exit arithmetic; reached band paths and grid metadata have their own accesses. Whole uninstrumented batches include all executed work. **Dynamic native fine/parent/band access and hardware instruction counts were not measured**, and CPU probe query counters are not substituted for them. No isolated cause for the regression is claimed. These variants were each paired against the retained original renderer; their separate historical runs are not a matched causal comparison between variants.

### Construction and representation costs

Direct three-sample constructor observations per aurora, in ms (under concurrent correctness load, not a clean paired CPU experiment):

| Medium | Control / isotropic | Control / full-axis | Control / parent |
|---|---:|---:|---:|
| Emerald | 241.9 / 946.0 | 237.1 / 857.4 | 238.7 / 788.6 |
| Cyan | 253.5 / 1,067.9 | 233.3 / 909.2 | 232.7 / 831.8 |
| Violet | 281.2 / 1,027.4 | 221.4 / 1,045.5 | 216.6 / 943.5 |
| Crimson | 341.5 / 1,637.2 | 326.6 / 1,843.8 | 316.4 / 1,606.0 |

For the parent version, inside payloads are 5,484–5,512 bytes per curtain; outside payloads including bands are 8,284–9,176 bytes. Controls are respectively 5,544–5,928 and 6,548–7,160 bytes. The larger outside bands matter even when bitmap bytes fit their budget. Full raw per-medium construction samples and cold/edit/upload records are in the durable JSON. The final parent density-replacement witness records 1,215.72 ms first-frame CPU work, two compiles, one proof refresh, 408 parameter bytes and 9,768 proof bytes. Workload/load differences prevent interpreting those individual edit times as a comparative speedup.

### Restoration and retained tooling

**All experimental producer, header, shader and renderer changes are removed.** Their source hashes match the starting `40b43c4b` snapshot, including the earlier power correction and unchanged host-only Euclidean analysis. The original CPU proof target is restored and passes 48/48. Four generic translated/parallel/support-edge native regressions remain, with first-edit preparation/upload diagnostics. Final rebuilt native proof passes **73/73**, sharing/power **76/76**, unified quadrature **15/15**, all without skips. The checkout app is rebuilt; the installed app and live saves remain untouched.

Retained external-tool changes support compatible control builder substitution, explicit constructor footprint reporting, separate cold-admission diagnostics and strict early rejection gates. The hierarchy-footprint helper detects an optional experimental hierarchy at compile time, so the tool also compiles with the restored legacy proof type; no hierarchy or app hook is installed by that helper. A restored-type host-only construction smoke is recorded separately. Construction reporting requires explicit metadata counts rather than silently assuming a layout.

No new operator, GPU skip eligibility, author verdict or Person movement acceptance is promoted. The experiment still lacks a complete numerical enclosure for new coordinate/exit calculations under retained Fast arithmetic and the full proof/consumer scope under AD-30/36. Economic rejection is already sufficient here; exact captures would not by themselves settle those obligations.

### Limits on the rejection

This implements coarse-support-envelope cropping, an occupied-slice/transition selector and AND-parent traversal. It **does not** implement Firmament's tighter slab-bisection domain or a post-tightening 16×16×16 selector, nor reproduce every hardcoded 172×1×178/28×1×28 layout. Therefore the result rejects these specific implemented layouts and payload/traversal costs, not every possible support-focused scheme. The CPU model's omitted macro reads, absolute outside-support-unreachability claim and unproved Y-invariance are not revived. No further research agents were commissioned.

## Reproduction and scope

Artifacts and frozen control sources are under `/private/tmp/earthcall-support-allocation-20261015/`, starting from HEAD `40b43c4ba114249a1d86ce7e0796c746533429f8`. The paired harness now substitutes **VolumeZeroProof.cpp as well as SdfWgsl.cpp and WebGpuRenderer.cpp**: sharing a candidate builder with the old shader would compare incompatible bitmap domains. Both variants compile their respective sources against one compatible current header/core layout. Exact compiler/link arguments, source/binary/input fingerprints, images and raw timing logs are preserved. [Durable numerical evidence](VOLUME_SUPPORT_ALLOCATION_TRIAL_2026-10-15.json) records all 96 exact image pairs, 24 primary timing runs, construction/edit observations and final restoration. Experimental source patches against the recorded baseline are preserved for review: [isotropic](VOLUME_SUPPORT_ALLOCATION_ISOTROPIC_2026-10-15.patch), [full-axis](VOLUME_SUPPORT_ALLOCATION_FULL_AXIS_2026-10-15.patch), [parent](VOLUME_SUPPORT_ALLOCATION_PARENT_2026-10-15.patch). These are rejected/not-admitted artifacts, not recommended production patches.

The retained external harness separates build/images/proof/cost invocations. Proof-cost mode takes explicit `--proof-metadata-floats CONTROL CANDIDATE`; this experiment uses 6 and 12. The first proof-cost invocation's archived runner selected those same two counts internally before the explicit option was added. JSON is developer evidence from one-time fixture decoding/reporting, not a new save codec or per-frame authored fingerprint.

No installed app, live save, original reference probe, timestamp feature, domain enum, authority mechanism, commit or push is changed. Offscreen completion throughput does not establish visible surface wait, isolated volume GPU duration, live FPS or Person movement acceptance.

**Signed:** Codex / GPT-6.1 Sol (Sixth-one Sun) / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T11:27:20.577924-07:00 (PDT).

**Final result signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T12:12:33.020544-07:00 (PDT).
