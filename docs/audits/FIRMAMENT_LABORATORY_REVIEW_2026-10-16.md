# Firmament laboratory review: adaptive absorption counterexample

**Reviewed:** `18456320d..dcff1876cc3aab8003f70c644b113333b5b8286c`, branch `firmament-laboratory`. The working tree was clean when inspection began. This covers Flash's `a094e6ab0` and Firmament's `dcff1876c`; the unrelated reflection commit is not a rendering change.

**Human commission:** Zach asked World Forger/Astra to review Flash and Firmament's laboratory work and give him instructions to send to Flash. This continues the [laboratory runbook and implementation correspondence](../../agent%20intercom/communication-threads/sdf-and-rendering/NORTHERN_VEIL_COUNSEL_REQUEST_TO_GPT_6_ASTRA_2026-10-11.md#firmament-to-gemini-38-flash-sixth-one-sun--astra--review-of-flash-commit-a094e6ab0--implementation-of-interval-certified-adaptive-volumetric-quadrature--certified-transmittance-tail-bounds-on-firmament-laboratory) and Zach's requirement that authored meaning, execution and measured costs be considered together.

**Verdict:** do not promote the new adaptive integration as certified. A freshly compiled native witness violates its one-level output budget in both single and fused rendering, including 2560×1440 with ordinary zero-proof traversal enabled. Flash's CPU-only optimizations should be assessed separately; this review does not recommend discarding them with the faulty integration.

## Findings

### P1 — Source-rate bounds do not bound absorption error

`SdfWgsl.cpp:4317–4330` and `5169–5185` choose 2x/4x stride from `transmittance * maxSourceRate * stepLength`, then reuse one extinction value over the enlarged interval. Neither the extinction variation nor its effect on later radiance/background is bounded. `maxExtinction` is computed on the CPU but does not participate in the stride decision.

For an absorbing medium with zero scattering and emission, the source-rate bound is essentially zero. The predicate therefore permits 4x strides regardless of how much extinction varies. This changes background attenuation by much more than the configured budget. The error is not repaired by a density-only variation certificate: the counterexample has constant density.

**Executed counterexample:** local box `[-2,2]^3`, density `1`, extinction `1 + 0.9*cos(12*pi*z)`, scattering `0`, no emission, no incident source, white background, camera `(0,0,-5)` toward the origin, 96 samples per chord. A second version overlays two media, each with half the extinction, to exercise the fused marcher. The accepted 96-sample lattice is the reference; no resampling approximation is introduced in that reference.

Reference budget is the smallest positive double, which becomes GPU budget zero while preserving the opt-in full-lattice flag. Candidate budget is `1/255`. Both use the same current source, renderer, camera and equations.

| Frame | Zero-proof mode | Media | Max RGBA error (8-bit levels) | Pixels above one level |
|---|---|---:|---:|---:|
| 160×120 | Disabled | 1 | 5 | 8,004 |
| 160×120 | Disabled | 2 | 5 | 8,004 |
| 2560×1440 | Enabled | 1 | 5 | 1,163,024 |
| 2560×1440 | Enabled | 2 | 5 | 1,163,024 |

In every case the centre red value is 5 in the full-lattice reference and 0 in the adaptive candidate. Both native runs exit 1 because the independent output-budget assertions fail. These are actual desktop WebGPU pixels, not a CPU rendering model. The initial sandbox run could not acquire a device and exited 77; that is recorded separately.

**Required correction:** restore full-lattice fallback whenever a complete transport-error argument is unavailable. Qualifying coarsening requires accounting for extinction/transmittance error, downstream source and background effects, all active media, sample/segment boundaries, accumulated budget and backend arithmetic. A source contribution bound alone is insufficient. Tail termination is a distinct argument and must be tested independently with stride disabled; this counterexample specifically establishes the stride failure.

### P2 — Per-cell stride data is unused by rendering and is not a quadrature certificate

`VolumeZeroProof.cpp:154–175` assigns stride labels from absolute density variation thresholds `0.025` and `0.10`. It does not incorporate extinction, chroma, phase, emission, path length or the Person's output budget. Constant density can coexist with the variable extinction above.

A source-wide consumer search finds `adaptiveWords`/`adaptiveStride` used by the CPU builder/accessors and the CPU test, but no shader upload/read. The runtime upload still sends ordinary zero bits, the support envelope and optional bands. The shader chooses its stride from the global bound. `maxVariationRate` is also not consumed by WGSL. Consequently, the reported fraction of 2x/4x cells does not certify the GPU decisions.

The extra array is allocated and leaf analysis runs for every proof build, including compatibility mode. At 32,768 cells it represents approximately 10.7 KiB of additional host storage, before vector overhead, on top of the ordinary proof bits. Unchanged GPU payload bytes do not mean unchanged total work or storage.

**Required correction:** separate experimental classification from production proof building, or remove it until there is a valid consumer and a transport-level certificate. Do not simply connect the existing labels to the GPU: their premise remains insufficient. Tests must verify enclosure/error obligations, not merely that a preferred percentage of cells receives a label.

## What holds up, and what remains unverified

- Flash's one-time scalar-map extraction is a coherent way to remove repeated host allocation within a range query. The midpoint check only refuses potential proofs, so its direction preserves the conservative fallback. Their performance claims still need their recorded matched build/run artifacts to be independently certified here.
- The new Scale zero-annihilation path retains range evaluation of the other operand using a cheaper noise enclosure. Add focused compatibility, invalid-input and operand-order witnesses before claiming complete equivalence. No new failure of that optimization was established in this review.
- I freshly compiled and ran the current CPU proof test against the reviewed core sources: **58/58 pass**. Its new stride-classification assertions pass despite the native error above. The branch needs an independent transport-error oracle.
- Flash's reported five-suite/32-image checks describe the earlier increment pass. They are not fresh validation of the later adaptive commit. This review did not rerun the entire native suite or validate Flash's ABBA datasets.
- `computeVolumeCertifiedBounds` runs over active media/channels in each positive-budget flush. Measure this warmed CPU cost, separately from cold proof construction, edits/uploads and GPU completion. No new frame-rate result was measured here.
- Finite host ranges and an `isfinite` validity bit do not complete the previously unresolved native arithmetic/coordinate contract. The Fast-profile boundary and open AD-30/36 scope still apply. Keep unsupported bounds on the exact fallback.
- Flash's rejected shader experiments have reported measured outcomes, but register pressure/occupancy causation remains a hypothesis without native compiler/hardware evidence.
- This review found no save changes in the reviewed branch delta. It changed no production source, authored equation, installed app or author decision. Existing Person appearance/movement acceptance remains outstanding; reproducing the defect requires no Person judgment.

## Reproduce and send to Flash

The [native probe](../../scratch/probes/firmament_absorption_review.cpp) uses synthetic in-memory media and adapts the existing shared-subexpression test's GPU/readback plumbing. The [build driver](../../scratch/probes/build_firmament_absorption_review.py) recompiles the probe plus ScalarForm, VolumeZeroProof, SdfWgsl and WebGpuRenderer using configured app compiler/link arguments, and reuses the other existing app objects. Use a compatible configured app build; this is not a full clean-build witness.

From the repository root, choose a fresh external directory:

```sh
python3 scratch/probes/build_firmament_absorption_review.py /private/tmp/flash-absorption-review
REVIEW_NATIVE_SIZE=1 REVIEW_PROOF_ON=1 /private/tmp/flash-absorption-review/absorption-probe
```

Desktop GPU access is required. Exit 77 is unavailable execution, not a pass. On the reviewed commit, expect exit 1 and the two 5-level failures above. The original executed commands, source hashes, complete small/full native logs and CPU result are preserved in [machine-readable evidence](FIRMAMENT_LABORATORY_REVIEW_2026-10-16.json); original artifacts are in `/private/tmp/astra-firmament-review-20261016/`.

**Message Zach can send to Flash:**

> Review `docs/audits/FIRMAMENT_LABORATORY_REVIEW_2026-10-16.md` before further optimization. Astra reproduced an actual output-budget failure at `dcff1876c`: 5 levels of error and 1,163,024 failing pixels at 2560×1440, for both one and two pure absorbing media. First reproduce the supplied witness, then isolate/disable unqualified adaptive stride while preserving the independent CPU optimization work. Add the counterexample as a regression and establish a transport-level error contract covering extinction/transmittance and downstream/background effects before re-enabling coarsening. Validate tail termination separately with stride disabled. Per-cell density stride labels are currently unused and are not sufficient certificates; remove/gate that work until justified. Use an independent full-lattice reference, check both render paths, proof on/off, variable extinction/density, bright late emission, nonblack backgrounds, medium boundaries, edits/clocks/membership and device recreation. Run exact compatibility-mode captures and bounded-mode error checks on the final commit before any ABBA timing. Record source/binary hashes, all failures and measured cold/edit/warm costs; do not infer GPU occupancy or FPS from source counts. Do not change saves or silently settle AD-30/36. No new research moons are needed for this verification pass.

**Signed:** Codex / GPT-6 Astra (World Forger) / session `01a12cb8-ac95-7b13-827d-2c7f289aa560` / 2026-10-16T12:43:43.028604-07:00 (PDT).


## Correction commissioned after this review

The original failing evidence above remains historical and unchanged. Zach then commissioned the Sun's [implemented correction and verification](FIRMAMENT_LABORATORY_CORRECTION_2026-10-16.md): unqualified strides/tails and unused classification are removed, CPU optimizations retained/hardened. The unmodified absorption probe now reports zero error in eight single/fused, proof off/on and small/native comparisons. Final focused checks pass 61/61 CPU, 73/73 native proof, 96/96 sharing/transport, 15/15 quadrature and object ALL OK. Read that correction's final authored-scene status before relying on its cross-scene coverage; this append does not rewrite the original defect or promote a new certificate.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-16T14:22:15.045552-07:00 (PDT).
