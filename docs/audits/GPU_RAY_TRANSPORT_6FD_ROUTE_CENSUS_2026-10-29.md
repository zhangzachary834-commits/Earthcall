# §6fd step 1 — actual-route mask census: where the 48.9% fallback tiles really come from

Answers item 2 of Sixth-One Sun's [§6fc quest](GPU_RAY_TRANSPORT_6FC_SONNET_REVIEW_AND_ROUTING_BOUNDARIES_2026-10-29.md#next-sonnet-quest) (census before any routing change), building on the Firmament's mask-routing counsel and §6fc's correction that unconditional empty-tile skipping is not bit-identical. Origin of the question: the Firmament counsel; origin of "census first, one guarded change, keep zero-budget refusal": Sixth-One Sun. The observer, controls and numbers below are mine.

## Method
Kernel diagnostic, **off by default** (`KernelRoutedTransport::enableRouteObserver`): a pipeline derived from the production `kClassifyWgsl` text by replacing **only its routing tail** and additionally recording the AABB-hit mask (the old classifier has no such field). Per tile it histograms `activeMemberMask | aabbMask<<4 | overlap<<8 | orderedS2<<9`; a second one-thread dispatch adds the production S1/S2/fallback list counters. Nothing in production shaders, routes, budgets, defaults or fallback changed. Harness: `scratch/probes/ray_transport_6fb_engine.{cpp,py}` mode `census`.

Native Release Engine, wgpu-native/Metal, 2560×1440, the frozen 64-pose route (poses 0..63, clock 7.25+i/60): 64 observed frames + 4 control frames, **8.96 s** of the declared 300 s cap, no validation/device errors. Controls that all passed: every tile of every strip classified (230,400 tiles/frame); observer-derived S1/S2/fallback equals the production counters on **every** frame; strip counter = 30; observer ON publication bit-identical to OFF at poses 0 and 32 (`de154bd4fd9d2c0b`, `7c14eadd2a2e79c9`, matching §6fb's publications). Raw: [JSON](GPU_RAY_TRANSPORT_6FD_ROUTE_CENSUS_2026-10-29.json). Occupancy only: no timing, no cost claim.

## Result (14,745,600 tiles = 64 × 230,400)
| Route | Tiles | Share |
|---|---:|---:|
| S1 (mask 1) | 4,599,130 | 31.19% |
| S2 (mask 3, ordered) | 2,942,684 | 19.96% |
| **Fallback** | **7,203,786** | **48.85%** |

Fallback tiles by reason (disjoint classes, share of all tiles / of fallback):
| Class | Tiles | All | Of fallback |
|---|---:|---:|---:|
| **Single member ≠ 0** (mask 2: 2.73 M, 4: 0.53 M, 8: 6 k) | 3,267,390 | 22.2% | **45.4%** |
| **Empty by wave test** (mask 0) | 2,058,435 | 14.0% | 28.6% |
| Two members, no overlap, not ordered (0|2, 0|3, 2|3 masks 5, 9, 12) | 1,218,462 | 8.3% | 16.9% |
| Overlapping members | 658,394 | 4.5% | 9.1% |
| ≥3 members, no overlap | 1,105 | 0.0% | 0.0% |

Observations that qualify the counsel: (1) **No tile has an empty AABB mask** — every "empty" tile has at least two members' boxes hit (AABB masks 9, 11, 13, 15), so the Firmament's AABB-based skip of empty tiles would skip **none** of these 2.06 M; their emptiness comes only from the classifier's wave test, so any skip must reuse that proof (and §6fc's budget/charge/packet contract). (2) An old mask-1 tile often has AABB mask 15 (1.54 M tiles), confirming §6fc's warning that AABB-based re-routing would add empty-member charges absent from S1. (3) The largest fallback class is **single non-zero member** tiles; mask 2 alone is 2.73 M tiles (18.5% of all tiles). Generalizing S1 to carry member IDs would reroute 45% of fallback tiles, but needs IDs through producer payloads and consumer masks (quest item 4). (4) Empty-by-wave tiles (28.6% of fallback) need **no member-ID plumbing**, but only a proof-admission contract.

## Unfavorable / not shown
Tile counts are not cost: fallback per-tile cost differs by class, and the §6fa 18.08 ms is source-order inference from an older, pre-coalescing trace. This census gives no savings estimate and no speedup. It does not show a wrong image. Per-tile occupancy on this 64-pose route does not transfer to other views.

## Next (one guarded ablation, not yet implemented)
Recommended first change: **empty-by-wave tiles (mask 0)**, because it needs no payload/consumer changes. Contract options to decide before coding: (a) raw-identical (must reproduce the 96-unit budget charges and base-offset words, which cheaply means a small dedicated writer rather than skipping), or (b) new proof admission for `cfg.budget ≥ 96` with the budget-0 forced refusal retained by falling back to the original route; the latter changes unused packet words and needs the same bitwise publication/snapshot gates as §6fb plus poisoned scratch. Then member-ID S1 for mask 2 as the second, larger candidate. Gate native publication/snapshot parity before any clean timing; no tolerance changes.

No push/PR; no live save, default checkout or installed app touched. GPU lane RELEASED.

Signed: Claude Code / Sonnet 5.5 / session `78cc5135-a805-4233-84ab-54fb16681c11` / 2026-10-29 (PDT).
