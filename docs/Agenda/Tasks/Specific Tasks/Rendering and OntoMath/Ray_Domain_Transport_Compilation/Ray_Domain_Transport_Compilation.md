# Ray-Domain Transport Compilation

**Origin:** Zach's Ray Transport Canon (revived Original Prism Sun, 2026-10-16): compile surviving world mathematics into the smallest lawful one-dimensional transport program per camera-ray segment. The V5 sample loop remains the exact fallback.

**Status (2026-10-17):** Rungs 0–2f are built; the original ray-segment planner has no renderer consumer, while support certificates have a separate opt-in production GPU consumer (default off). Native evidence and unresolved proof gates are recorded below. Full contracts, evidence and continuation: [plan](../../../../../plans/RAY_DOMAIN_TRANSPORT_COMPILATION_RUNG_0_1_2026-10-16.md).

## Done
- [x] Rung 0 — `OntoMath::restrictToLine` (`src/Singularity/OntoMath/LineRestriction.*`): exact polynomial subset, named refusals, symbolic ray coefficients, proved exact vanishing. Witness `ontomath_line_restriction_test` (57/57, Linux).
- [x] Rung 1 — `Rendering::VolumeRayFormCache` / `planVolumeRaySegment` / `executeVolumeSegment` / `affineSegmentTransmittance` (`src/Singularity/Screen/VolumeRayTransport.*`): exact-empty, constant closed form of the accepted lattice, affine transmittance. Witness `volume_ray_segment_transport_test` (49/49, Linux) against a CPU lattice mirror and the real WGSL emitter's flags.

- [x] Rung 2 — radial-piecewise segmentation (Zach, 2026-10-17): `OntoMath::restrictAlgebraicToLine` / `LineAlgebraicEvaluator` / `segmentLine`, plus `Rendering::scheduleVolumeRaySegment` / `executeVolumeSchedule`. Witnesses: `ontomath_line_segmentation_test` (37/37) and `volume_ray_radial_segmentation_test` (25/25). Admits 12 of 32 real densities and proves 69.8% of their random-ray samples empty, where the zero-proof grid proves 0 cells. [Plan and findings](../../../../../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md).

## Open, in recommended order (pitfalls inline)
- [ ] **Rung 2b — GPU closed-form radial support intervals.** One sphere/ray quadratic per member per ray trims traversal inside the authored box. Never move box events or sample spacing. Widen roots outward for f32. Native pixels and ABBA cost are required. Do NOT port branch-and-bound to the fragment shader: CPU scheduling costs ~0.2 ms per ray in Debug.
- [x] **Rung 2c — per-ray affine-argument transcendentals** (sin/cos/exp; Ln stays refused). 14 of 32 real densities are now admitted. See [Rung 2 plan §6a](../../../../../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6a-rungs-2c-and-2d--implemented-2026-10-17-second-pass).
- [x] **Rung 2d — cost**: leaf-range memo and two-phase schedule kept; analytic proposals measured CPU-neutral and default off. Release mean for the original twelve media is 24–27 µs/ray.
- [x] **Rung 2e — Perlin on a ray** via OntoMath's own box theorem, plus a sign-free memo, density-sign targets and proposals re-enabled. 28 of 32 real densities admitted. Northern Veil: Rung 2 proves 97.5–99.0% of random-ray samples empty against the grid's 95.1–97.0%, roughly halving the remaining work, and 59–85% of that gain comes from the thin curtain factor. [§6b](../../../../../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6b-rung-2e--perlin-noise-on-a-ray-and-what-it-showed-about-northern-veil-2026-10-17-third-pass).
- [x] **Rung 2f — Lipschitz-certified support tracing, CPU prototype** (`traceVolumeSupport`, interval derivatives, Cauchy–Schwarz norms). It matches full segmentation over 28 real media (70.65% vs 70.60%), and on Northern Veil needs 18–30 curtain evaluations per ray. [§6c](../../../../../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6c-rung-2f--lipschitz-certified-support-tracing-cpu-prototype-2026-10-17-fourth-pass).
- [x] **Rung 2f-native groundwork**: authored-tier support certificates (box-mode evaluator, spatial Lipschitz bounds, source operands) and a certificate-only tracer matching per-ray tracing at 70.65% over 28 media (Northern Veil 28–39 curtain evaluations/ray). Also a **cloud software WebGPU lane** (lavapipe + wgpu-native at the vendored tag) that runs the real WGSL offscreen. [§6d](../../../../../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6d-rung-2f-native-groundwork--authored-tier-certificates-and-a-cloud-webgpu-lane-2026-10-17-fifth-pass).
- [ ] **⚑ AUTHOR — null members vs unified quadrature.** `webgpu_v5_overlap_physics_test` (PR #361's tribunal, last changed 10-07) contradicts per-segment spacing (10-09): a null overlapping member shifts another medium by up to 42 bytes on lavapipe. Rung 2 proofs could exclude proved-empty members from spacing, but that changes the lattice. Zach decides; confirm on Metal first.
- [x] Compare the Perlin residency assertion on Metal: Codex reproduced `!sawInitialRangeUpload` on the M5; Opus subsequently fixed the empty-batch mirror invalidation, with lavapipe passing and Metal revalidation pending (see the residency item below).
- [x] **Rung 2f-native — the GPU tracer, opt-in** (`ScreenChannel volumeSupportCertificatesEnabled`, default OFF). `sdfwgsl::lowerVolumeSupportKillers` lowers each candidate switch with the production emitter; the renderer attaches per-box certificate headers {sign, δ, 1/L, offset}; both marchers consult them where no grid proof covers a sample, jump `(sign·g − δ)/L` minus a coordinate pad, and skip *checks* by the converse bound. Lavapipe witness `webgpu_volume_support_certificates_test` 56/56 byte-identical (Northern Veil fused/single, 6 views, translated, edits, resize, refusal, full lattice, 2560×1440, Prism Cathedral wing B). [§6e](../../../../../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6e-rung-2f-native--the-gpu-certificate-tracer-opt-in-2026-10-17-sixth-pass).
- [x] **Codex P1 repair — execution provenance.** Literal-only divisors, exponents and clamp bounds; `OntoMath::executionBound` and per-leaf authored provenance; margin-aware segmentation, Rung 1 and certificates (δ from authored operands, clock-magnitude bucket). It costs ≤ 0.08 points of proof over 28 media, with 0 violations. [§6f](../../../../../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6f-execution-provenance-the-codex-p1-repair-2026-10-17-seventh-pass).
- [x] **Native Mac revalidation of the P1 repair:** rebuilt at `a597da4cd`, certificates 57/57 and zero-proof 73/73 on Apple M5 Metal; the cancellation case is black on this device and does not supply native positive-density coverage ([record](../../../../../audits/RAY_DOMAIN_TRANSPORT_METAL_VERIFICATION_2026-10-17.md#post-repair-metal-revalidation--opus-seventh-pass)).
- [x] **`webgpu_perlin_exact_gradient_test` residency assertion** (line 817, lavapipe AND Metal): an empty SDF range batch cleared the persistent buffer's mirror, so identical bytes re-uploaded. The mirror now stays; lavapipe ALL OK and M5 Metal ALL OK after rebuilding at `a597da4cd` (6/6 camera hit coverage; 27 diagnostic color-byte differences at 45 degrees, so full RGBA parity is not established) ([§6f](../../../../../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6f-execution-provenance-the-codex-p1-repair-2026-10-17-seventh-pass)).
- [x] **Rung 2f-native eligibility policy** — a Kernel cost estimate over the shader's own loop and the `volumeSupportCertificatesAutoEligibility` switch (default on within the off-by-default lane). Northern Veil stays on the grid lane, and Prism wing B gets 1.5–5.2× cheaper on lavapipe. [§6g](../../../../../plans/RAY_DOMAIN_TRANSPORT_RUNG_2_RADIAL_PIECEWISE_SEGMENTATION_2026-10-17.md#6g-eligibility-where-certificates-pay-2026-10-18-eighth-pass).
- [ ] Model the shared-noise prelude in the eligibility estimate (the noise cloud is a safe-direction miss).
- [x] Metal ABBA and polynomial-cancellation witness: 68/68 twice at `993f92a6`, 36,669 lit pixels, byte-identical. Seven Prism media improve; zero-eligible Northern Veil is 18–23% slower, so neutral-cost/default promotion remains blocked ([record](../../../../../audits/RAY_DOMAIN_TRANSPORT_METAL_VERIFICATION_2026-10-17.md#auto-eligibility-metal-verification--opus-eighth-pass)).
- [ ] Remove or explain the zero-eligible Metal overhead with an effective-eligibility baseline program path; preserve generic cache/payload identity, byte parity and paired cost evidence.
- [x] **Metal run of the pre-repair certificate lane:** 56/56 twice, with report-only ABBA recorded; Opus has since implemented the P1 repair, while its Metal revalidation, device enclosure and Person acceptance remain open ([native record](../../../../../audits/RAY_DOMAIN_TRANSPORT_METAL_VERIFICATION_2026-10-17.md)).
- [ ] Lower Select-piece killers (roles 4+2i/5+2i) and the density-sign killer to WGSL; today they are not candidates.
- Note: `volume_ray_radial_segmentation_test` takes ~90 s in Debug, because it runs the full 28-medium census.
- [ ] Outward-round lone constant terms in `ScalarForm::evalRange` (`Term::evalRange` starts from an un-widened float). This is the proof consumers' office; see Rung 2 plan §5.2.
- [ ] **Rung 1b — GPU consumer for structurally constant, unlit media.** Classify per revision rather than per ray (exact zero direction components are measure-zero at pixel centres). Eligibility joins the module identity. Before retention you need native pixels vs the current lattice (not byte-identical, so state the ~1e-6 relative T outcome at 8-bit), zero-proof and cutoff interplay, ABBA cost, and device reconstruction. A scene with one enabled incident source gains nothing.
- [ ] **Rung 1c — constant σ with polynomial source.** The arithmetico-geometric closed form cancels catastrophically for thin media, so it needs a stable evaluation or a certified threshold.
- [ ] **Rung 3 — ray-domain sharing** keyed on (form, binding identity). Source, occluder and receiver coordinates are never shared with medium coordinates.
- [ ] **Rung 4+ —** affine-argument transcendentals, polynomial σ, certified quadrature/tails. Approximation is a separate office and needs an author-visible policy.
- [ ] Reduce `ScalarForm::plus` quadratic re-normalization before any live consumer lowers heavy polynomials (`(x+y+z)^6` takes ~66 ms in Debug).

*Claude Code · Claude Opus 5.5 · session `session_01HvZSR64QghrdyCpq2M7qvX` · 2026-10-16; Rung 2f-native GPU tracer and the emitter precedence fix 2026-10-17; Codex P1 execution-provenance repair 2026-10-17 (findings: Codex / GPT-6.1 Sol / `01a148a7-bd85-7550-a416-0e859a729c1a`).*



## Source-review findings awaiting the independently assigned live pass

**Historical scope:** the signed review and Metal evidence below describe pre-repair HEAD `ee9057845`. The current checklist above records Opus's subsequent P1 and Perlin repairs; their native Metal revalidation remains pending.

Zach commissioned the Sun's [October 16 source review](../../../../../audits/RAY_DOMAIN_TRANSPORT_SOURCE_REVIEW_2026-10-16.md), expressly excluding tests/builds. Two P1 findings concern cancellation-derived exact-empty proofs and CPU-folded clamp thresholds differing from the emitted bound tree. Concrete MathNode cases and proposed validation are in that review and the existing Opus handoff. No implementation was changed or new native/performance result established; keep the certificate lane off pending correction/qualification. Existing Person checks and AD-30/36 remain open.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-16T22:35:31.712804-07:00 (PDT).


## Metal economics and eligibility proposal — October 17

Zach commissioned execution of Opus's Mac handoff. [Native record](../../../../../audits/RAY_DOMAIN_TRANSPORT_METAL_VERIFICATION_2026-10-17.md): stock certificates 56/56 twice, zero-proof 73/73, sharing 76/76; overlap passes with black null comparisons; the Perlin residency assertion reproduces on Metal. The separate CPU cancellation counterexample also reproduces and blocks omission-proof qualification; two proposed GPU cases were not reproduced on the M5.

Native report-only grid → grid+certificate medians: Northern Veil fused **1.93 → 2.87 ms**, single **0.50 → 0.85 ms**; Prism wing B fused **2.86 → 1.25 ms**, with all eight selected individual media cheaper. These are completion/readback-inclusive fixed-view costs, not isolated GPU time or live FPS.

**Proposal only:** after correcting execution-equivalence/error admission, derive per-medium cost evidence from existing Kernel facts (proven-cell coverage, killer count and lowered evaluator work), then measure crossover with paired native runs. Do not encode Zone names, infer truth from a cost score, or hardcode an unmeasured coverage threshold. Any variable resource-allocation policy must follow Zach's authored-Law direction; this pass implements no policy and changes no default. A no-killer/unknown-proof medium always keeps the exact path. New retained ranking state would need explicit dependency/invalidation evidence.

- [x] Correct or conservatively refuse canceled-operation execution-zero proofs; preserve original-DAG provenance before a renderer consumer or promotion. (§6f, `09143a5d`.)
- [x] Qualify computed clamp thresholds and native arithmetic error; same-M5 black parity does not discharge the source-review premise. (Literal-only bounds plus the declared execution model, §6f; a lit polynomial witness is added in §6g.)
- [x] Investigate the reproduced Perlin initial range-upload assertion without loosening its check. (A residency bug, fixed in `94554ae9`; M5 ALL OK.)
- [ ] Obtain Zach's verdict on the three Zone appearance pairs; all visual boxes remain open.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-17T00:18:36-07:00 (PDT).


**Sky-Island fidelity follow-up:** Zach's acceptance criterion is mathematical faithfulness rather than preserving the old aesthetic brightness. [Independent native and 80-digit witnesses](../../../../../audits/SKY_SANCTUARY_AUTHORED_EQUATION_FIDELITY_2026-10-17.md) support the darker after shading for the authored grouping; no coefficients or defaults were changed. The general omission-proof and device-error gates above remain open. **Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-17T00:37:58-07:00 (PDT).


## Post-repair native verification — October 17

The three requested witnesses pass on M5 Metal: certificates 57/57, zero-proof 73/73, Perlin ALL OK. [Exact scope and caveats](../../../../../audits/RAY_DOMAIN_TRANSPORT_METAL_VERIFICATION_2026-10-17.md#post-repair-metal-revalidation--opus-seventh-pass) supersede the historical pending-native status below the earlier signed records. No new performance or Person acceptance is claimed.

- [~] Characterize Perlin's 27 Metal color-byte differences before claiming full native RGBA parity: Opus reproduced the pre-fix 18-byte lavapipe baseline, but the native pre-fix 45-degree baseline remains unexecuted; stock coverage/root assertions pass.
- [x] Obtain a nonblack Metal cancellation witness: the polynomial identity at A=4096 has 36,669 lit pixels, 1 killer and 0 byte differences in both `993f92a6` M5 runs. The simple cancellation case remains black; this is finite-case coverage, not universal execution equivalence.

**Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-17T12:12:06-07:00 (PDT).


**Eighth-pass native return:** [68/68 twice, lit polynomial witness and paired auto-mode costs](../../../../../audits/RAY_DOMAIN_TRANSPORT_METAL_VERIFICATION_2026-10-17.md#auto-eligibility-metal-verification--opus-eighth-pass) complete the requested measurements, while zero-eligible Metal overhead is a remaining performance gate. Default-off certificates and existing Person checks remain. **Signed:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-17T19:22:42-07:00 (PDT).
