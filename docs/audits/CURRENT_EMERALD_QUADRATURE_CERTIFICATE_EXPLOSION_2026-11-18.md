# Current Emerald Quadrature Certificate Explosion

Codex / GPT-6 / session `01a1eb67-9c6f-72c0-805f-5caceca627ad` / 2026-11-18T17:22:00-08:00. Zachary Zhang and Prism Sun commissioned this separate dossier during the C5 exit tribunal. It continues the [second-pass native proof record](C5_SUMMIT_TRIBUNAL_SECOND_PASS_2026-11-18.md#seeded-host-schedules).

## Open numerical proof obligation

`webgpu_line_quadrature_test` remains red on the current Northern Veil primary emerald field. The host planning cap is 2,048 panels. The host error budgets are 0.009 each for radiance and transmittance. The final Metal T-radius limit is 0.006 and the grid traversal cap is 4,096 panels.

| Seeded ray | Final L radius | Final T radius | Limiting quantity |
| --- | ---: | ---: | --- |
| 3 | 0.062072 | 0.014462 | Field evaluation contributed 0.020445 to the optical-depth execution radius. |
| 4 | 1.107 × 10¹² | 3.366 × 10¹¹ | Mathematical optical-depth radius 26.382 was amplified by the exponential transmittance bound. |
| 11 | 433,908 | 225,498 | Mathematical optical-depth radius 12.006 entered the same exponential bound. |

Each refused seeded ray reached the 2,048-panel cap. Ray 10 executed on Metal with 95 panels. The proof radii describe the enclosure, and the native grid witnesses independently checked the numerical reference.

| Native primary-emerald grid | Observed |
| --- | --- |
| Occupied rays | 12, all completed traversal |
| Occupied rays admitted | 0 |
| Final T radii | 0.02438–0.2688 against 0.006 |
| Execution term | Alone exceeded 0.006 on every occupied ray |
| Largest traversal | 1,463 of 4,096 panels |

The [per-ray table](C5_SUMMIT_TRIBUNAL_SECOND_PASS_2026-11-18.md#native-primary-emerald-proof-grid) gives the mathematical and execution terms for each ray. The completed grid traversals fail the final proof radius, while the seeded host schedules exhaust their refinement cap.

## Scope and causal assignment

| Reproduced comparison | Result |
| --- | --- |
| C5 with current eight-field Northern Veil | Refuses rays 3, 4 and 11. Grid admits 0/12 occupied rays. |
| Current default `d7fb75cc6` with the same eight-field save | Identical host decisions, WGSL and parameter fingerprints, and Metal outcomes. Both comparisons use the shared Perlin helper repair. |
| Historical four-field snapshot `e93a77392` | All four seeded schedules execute. Two are empty. Grid admits 2/6 occupied rays. |
| Earlier eight-field save `fcd6ba2da` with older emerald | Reproduces the historical four-field host decisions. |

Current Emerald's authored equations and bounds materially increased the conservative proof burden. Current default owns this open proof obligation. The paired evidence establishes identical refusals for PR #882 and its base. The proof budgets, execution model and authored content are preserved. The C5 exit tribunal proceeds under Zach's explicit human no-regression exception, with the retained test still red.
