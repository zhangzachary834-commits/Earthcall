# Cathedral surface frame cost

**Commission** — Zachary Zhang asked for surface SDF optimization after the Northern Veil merge.  
**Author** — Codex / GPT-6 / Sixth-One Sun  
**Session** — `01a148a7-bd85-7550-a416-0e859a729c1a`  
**Timestamp** — 2026-11-03T21:18:00-08:00

Source/CPU preparation is complete. All 178 surface Fields compile and match re-collected parameters. Thirty-six require gradient-corrected marching, and none emit analytic gradients. The spatial-root expression supplies authored source radiance. [Audit, source-bound census, and prepared probe](../../../../../audits/CATHEDRAL_SURFACE_SDF_FIRST_PASS_2026-11-03.md)

- [x] Read the saved scene and distinguish geometry and radiance callers.
- [x] Build correct-checkout Release production objects and execute the CPU compiler census.
- [x] Prepare an explicitly invoked isolated Field-subset native screen.
- [x] Resume native work after Zach closes Chrome and releases the CPU-only boundary.
- [ ] Capture Zach's actual slow camera and drawable size for the inhabited comparison.
- [ ] Validate baseline shaders and resolve the scalar-Map diagnostic on `material.logos.bounds.quantized`.
- [x] Separate Field-subset and full saved-object costs, then locate the larger CPU staging-allocation bottleneck with a warmed stack sample.
- [x] Qualify bounded mesh upload pages, with independent native pixels, offset-negative control, fixed-order image parity, and four ABBA blocks.
- [x] Compare upload-only fidelity with independent pixels and exact fixed-order full-scene RGBA alongside paired ABBA complete cost.
- [x] Record Zach's live 38 FPS report with expensive Laws disabled; keep Law optimization separate as requested.
- [x] Make the Finder launcher default to Release even over a cached Debug build, retaining an explicit Debug override.
- [x] For any later derivative/intersection change, qualify intersections, normals, depth, seams, and authored-reference accuracy separately.
- [ ] Validate the full inhabited scene and record Zach's appearance, motion, and frame-pacing judgment.
- [ ] Investigate stable logical mesh ordering for repeatable coplanar results across processes.

Preserve authored geometry and native resolution. Mathematical differentiation belongs with OntoMath. Retain original value arithmetic and fall back where derivative validity is undecided. Broader spatial proof structures require measured profitability before adoption.

**Native checkpoint, 2026-11-03T22:52:00-08:00.** Upload pages reduce the full saved-object screen from 62.112 to 30.042 ms, with CPU submission 39.592 → 15.654 ms. Fixed-order images match every RGBA byte. Natural process-dependent batch ordering remains visible in both renderers. Run `build_release/earthcall_webgpu` for the pending Person check. The [audit continuation](../../../../../audits/CATHEDRAL_SURFACE_SDF_FIRST_PASS_2026-11-03.md#native-continuation-and-retained-upload-repair) records scope, raw controls, padding cost, and the separate scalar-Map follow-up.

**Native SDF Rung 1 & 2 checkpoint, 2026-11-04T02:26:00-08:00.** Forward-mode algebraic Jet differentiation across `ScalarForm`, RPN, and the full `geom::SdfNode` CSG tree brings analytic gradient coverage on Cathedral of the Living Logos from 0/36 (and 0/178 overall) to **36/36 (100%) of gradient fields and 178/178 (100%) of all fields**, with 0 parameter mismatches. Single-pass `sdfEvalGrad` replaces 4-sample numerical finite differences during raymarching and normal evaluation, dead volumetric evaluation is stripped from pure surface march loops, and `paramOffset` is hoisted. Full-world Metal warm frame times reach 29.74–30.84 ms on Cathedral and 9.30–9.52 ms on The Merry Dancers, with >99.1% exact byte parity. [Audit details](../../../../../audits/CATHEDRAL_SURFACE_SDF_FIRST_PASS_2026-11-03.md#sdf-surface-truth-mastery-rung-1--rung-2-implementation-and-native-offscreen-validation).

**Corrective ABBA & Rung 3 & 4 checkpoint, 2026-11-04T16:30:00-08:00.** Following The Firmament's corrective decoupling of scalar `sdfEval(p)` from `sdfEvalGrad(p)` and single-pass analytical primitive formulas, Rung 3 and Rung 4 were implemented and verified on Metal. Rung 3 lowers single-leaf `Sphere`, `Box`, and `Convex` polyhedra to $O(1)$ closed-form ray-domain hits (0 march iterations) for primary and shadow rays. Rung 4 conservatively prunes child 1 in `Union` and `SmoothUnion` subtrees with $\ge 3$ leaves using CPU-derived bounding spheres $\|\mathbf{p} - \mathbf{c}_B\| - R_B$, with $\mathbf{c}_B$ and $R_B$ registered as storage buffer parameters to preserve byte-identical WGSL across value edits. Paired Metal ABBA benchmark (16 warm frames): Cathedral Field Subset reaches **7.81 ms** (down from 8.38 ms baseline and 9.23 ms initial), Cathedral Full World reaches **30.74 ms** (median 29.34 ms, warm 28.20–38.71 ms), and The Merry Dancers Full World reaches **10.00 ms** (median 10.01 ms, warm 9.89–10.08 ms). All unit and parity suites pass (25/25 shapes in `webgpu_sdf_parity_test`, 0 failures in `webgpu_cathedral_open_hand_test`). [Audit details](../../../../../audits/CATHEDRAL_SURFACE_SDF_FIRST_PASS_2026-11-03.md#4-paired-abba-performance-gate-metal-25601440).
Antigravity / Gemini 3.8 Flash / session `c802a0d5-3c06-450d-b47b-ebdc7aef86d0` / 2026-11-04.
