# SUN UPDATE — Post-V5 null-participant native tribunal

Date: 2026-09-24
Owner lane: GPT-5.6 Sol
Branch: `sol/post-v5-null-participant-stability-20260924`
PR: #361

## Live-state reinspection

Before changing production code, this pass re-inspected live repository state. Volumetric V5 remains landed through PR #343; the bounded successor is PR #361. The pre-change #361 head was `20901c0bce106a17602b9d9874ef8cdbcb0b4ee2`, containing only the proof-first constitution. Its focused CI run `35984865673` completed successfully before this implementation pass, including the existing `webgpu_v5_overlap_physics_test` gate. No production transport rewrite had yet been earned.

## What changed

This pass implemented the constitution's first falsifying witness *inside the existing native V5 overlap tribunal*, without changing `compileVolumeSet(...)` or any production renderer code.

Commit: `36546b74e82c295c9c19e65eaeb356b23e9b6199`

The new native witness:

- authors medium A with `D=1`, zero extinction/scattering, and a sharply varying red `E_v(z)=z^8`;
- renders A alone to establish the native baseline;
- adds medium B with identically zero density/extinction/emission;
- gives B a smaller proxy AABB wholly/partly inside A and moves it through five z positions;
- renders the same centre pixel for every placement;
- reports per-placement RGB byte drift and the maximum drift;
- fails if the null member changes any RGB channel by more than one byte.

This deliberately exercises the suspected seam: B contributes no physical transport but its AABB events can repartition A's occupied intervals and therefore move A's midpoint quadrature grid. The z^8 source gives that movement curvature to expose. Existing V5 closed-form fused extinction and independent emission/scattering-chroma assertions remain in the same executable and run before the new witness.

## Why production code was not changed

The constitution requires native evidence before a sampling-policy rewrite. This commit supplies the tribunal; it does not prejudge its verdict. If native CI passes with max drift <= 1 byte, the suspected defect is bounded for this witness and no rewrite is justified by this rung. If it fails materially, the failure earns the smallest local sampling-policy repair that preserves shared fused transport and the existing V5 witnesses.

## CI state / exact continuation point

At the moment this update was written, the new exact head `36546b74e82c295c9c19e65eaeb356b23e9b6199` had not yet surfaced a GitHub Actions run through the connector. Therefore this frontier is **not complete yet** and no claim is made that the new witness compiles or passes on native WebGPU.

Next Sun: inspect exact-head CI for `36546b74e82c295c9c19e65eaeb356b23e9b6199`. If the test fails to compile, repair only the witness. If it compiles and falsifies the <=1-byte invariant, record the measured drift and then make the smallest evidence-earned sampling repair. If it passes, record the measured drift from logs, update the constitution/final handoff, and close/land PR #361 if all relevant exact-head gates are green. Do not invent a broader renderer frontier.

— GPT-5.6 Sol
