# Volume support: observed Metal profile and coordinate refusals

Zach commissioned this pass with “YAAAAASSSSSS” after the Sun proposed tracing actual shader transformations, coordinate rounding and bin selection. It continues [the first conditional numerical contract](VOLUME_SUPPORT_NUMERICAL_CONTRACT_2026-10-12.md) and [the reply to Astra](../../agent%20intercom/communication-threads/sdf-and-rendering/NORTHERN_VEIL_COUNSEL_REQUEST_TO_GPT_6_ASTRA_2026-10-11.md#sixth-one-sun-returns-the-bounded-certificate-pass--production-gate-refuses). Zach originated the fidelity/minimum-work direction; Astra required backend admission and conservative refusals. The Sun implemented native compiler observation and the additional coordinate/profile witnesses.

**Result:** the native profile is now observed, rather than guessed. The retained volume shaders use **Metal Fast arithmetic and Fast floating-point functions** on Apple M5. Eight observed/plain image pairs at 320×180 match every byte. The research tool now passes **24 witness groups**, including coordinate cancellation and refusal of apparent single-bin membership. A complete enclosure of the actual Fast-mode transformations is still not established: **zero production skips admitted, no runtime guard, no new FPS gain**. This completes an observation/refusal pass, not the full production certificate.

## Current baseline and boundaries

HEAD at native observation: `1b469427c4061b1114881ab25589532110235871`. Zach committed the previous research; only the separately authored Author Decisions additions were dirty when this pass began. AD-30–32 remain OPEN. Astra's additional AD-33–36 describe structural author review and explicitly do not impose a blanket hold on independent engineering. This pass resolves none of those decisions and introduces no whole-contribution skip, extinction-floor removal or source-chroma migration.

The app's ordinary device uses `wgpu-native`, not Dawn: `CMakeLists.txt` links `third_party/wgpu/lib/libwgpu_native.a`; the packaged tag is `v29.0.1.1`. The [pinned upstream dependencies](https://github.com/gfx-rs/wgpu-native/blob/v29.0.1.1/Cargo.toml) name wgpu/naga 29.0.1. The matching [Metal translation source](https://github.com/gfx-rs/wgpu/blob/v29.0.1/wgpu-hal/src/metal/device.rs#L198-L224) generates MSL, sets language version and preserve-invariance, and supplies compile options to Metal. Upstream source guides the investigation; the actual runtime observation below establishes the option values for this vendored binary.

## Diagnostic implemented and run

- [Native observer](../../scratch/probes/volume_metal_compile_observer.mm) is linked only into a scratch executable. It intercepts the concrete device's `newLibraryWithSource:options:error:` **inside that process**, reads options/source, calls the original implementation once with identical arguments, and writes exclusive numbered MSL/JSON records. No source substitution, option setter or application hook is installed. It does not affect another process or the shipped app.
- [Driver](../../scripts/volume_backend_contract_probe.py) copies the existing read-only capture fixture into a fresh external directory and links the configured app objects. The second binary contains a no-op installation stub. It reads the existing MsgPack Zone envelope once during startup; it does not rewrite the save or alter codecs.
- Both lanes render spawn, inside, reverse and side at clocks 0 and 7.25. Native timestamps remain disabled. These are compilation/pixel diagnostics, with image readback; their logs must not be treated as performance runs.

The visible/native compiler boundary records **104 successful library compilations**, including **16 records whose translated modules contain volume evaluators**. These are compilation records, not 16 distinct mathematical programs. Every draw has its own successful volume records. All observed options agree:

| Observed item | Value |
|---|---|
| Device | Apple M5 |
| Concrete device class | AGXG17GDevice |
| `mathMode` | 2 — Fast |
| `mathFloatingPointFunctions` | 0 — Fast |
| legacy `fastMathEnabled` | true |
| `languageVersion` | 196610 — 3.2 |
| `preserveInvariance` | true |

These enum meanings are declared in the installed Apple SDK's `Metal.framework/Headers/MTLLibrary.h`. Apple's [Fast arithmetic mode](https://developer.apple.com/documentation/metal/mtlmathmode/fast) permits unsafe arithmetic optimizations. Preserve-invariance concerns its own contract; it does not establish this support certificate's rounding/rewriting enclosure. Observing Fast mode is **not evidence of an incorrect frame or a WGSL conformance failure**. It identifies transformations our conditional model does not yet account for.

Every observed/plain pair matches exactly: **1,843,200 RGBA bytes compared across eight 320×180 images**. This establishes that this compiler observer did not perturb those finite images. It is not a full-resolution correctness certificate, not evidence for an uninstalled guard, and not an explanation of the earlier shader-instrumentation observer effect. Production resolution and rendering behavior are unchanged.

## Actual producer and additional conditional enclosure

The captured volume MSL contains the original producer `worldP = ro + rd * sampleT`, then medium-local subtraction `worldP - mediumInst.origin.xyz`, before the volume evaluator. The folded clamp remains weighted `metal::sin/cos` of affine spatial inputs, guarded division, abs and clamp. Naga's MSL text still carries expressions into Metal; inspecting it does not reveal every later compiler transformation or establish local variables as precision barriers.

The existing [rational research tool](../../scripts/volume_support_certificate_research.py) now encloses **one conditional local-coordinate producer**: `ro + rd*sampleT - origin`, with already materialized finite f32 inputs. It includes the rounded/exact product and every additive grouping of the three atoms. Upstream ray normalization, sample-lattice arithmetic and distributions through downstream affine consumers remain outside that model. It is intentionally not labeled a complete production producer proof.

Constructive f32 witness:

```text
ro = origin = 2^25; rd = sampleT = 1
exact real local coordinate = 1
nearest_f32(ro + rd*sampleT) - origin = 0
(ro - origin) + rd*sampleT = 1
conditional directed/all-tree enclosure = [0, 4]
```

The enclosure also includes allowed upward rounding. Apparent ideal membership in one support bin therefore cannot choose that bin: the uncertainty crosses edges and the witness refuses. This is a CPU arithmetic witness, **not a measured GPU coordinate error**. It demonstrates a necessary obligation rather than accusing the current scene of this large-coordinate case.

Six new witness groups cover cancellation/regrouping, uncertain coordinate-bin selection, producer overflow, observed Fast arithmetic/functions refusal, Safe/Precise options alone still refusing, and unavailable options. The previous 18 remain passing. `profile_obligations` consumes observed metadata and names the remaining obligations; `production_zero` still returns Unknown for every profile. Even Safe/Precise options cannot grant admission without the complete transformation/coordinate/binding enclosure.

The initial local-grid counts are **not recomputed or promoted**. They still describe the earlier closed scalar model and stored historical report. This pass screens admitted production opportunity as **zero**. It neither assigns a native skip rate nor predicts savings from the conditional table.

## What remains and why the runtime candidate stops

The former generic “profile unresolved” statement is now narrower: **the retained binary's numerical option profile is known; its complete Fast-mode execution enclosure is unresolved**. Closing it requires accounting for supported arithmetic transformations, actual ray/sample/local-coordinate production, affine intrinsic arguments, division and consumer boundaries under that profile. The normative [WGSL reassociation/fusion permissions](https://www.w3.org/TR/2026/CRD-WGSL-20260921/#reassociation-and-fusion) also prevent treating source grouping as a universal guarantee.

Changing compiler settings to obtain a simpler model would create a new rendering candidate. It could change pixels and costs; this pass does not do so or claim that Safe mode alone solves admission. Another route is a proof whose complete execution uncertainty is enclosed under the retained profile. Either route must establish useful demanded coverage before a guard benchmark. Do not silently widen OntoMath's mathematical intervals, assert that finite matching captures prove the guard, or revive rejected caches/transport skips.

AD-30 still governs any new whole-contribution support premise. A scalar-local proof can be researched independently, but its scope cannot be promoted into permission to omit other authored channels. Existing renderer/proof behavior is preserved; this audit does not certify or invalidate all inherited proofs.

## Reproduction, verification and integrity

```sh
cmake --build build --target earthcall_webgpu -j8
python3 scripts/volume_backend_contract_probe.py --output /private/tmp/volume-backend-new-run
```

The driver requires a desktop GPU and a **new directory outside the repository**; files are not deleted or overwritten. Optional `--width`/`--height` choose diagnostic dimensions, not production rendering policy. The observer must stay scratch-only. No CMake target or shipped caller references it.

Authoritative artifacts: `/private/tmp/earthcall-volume-backend-contract-20261012-pass2/report.json`, `contract-refusals.json`, `verified-provenance.json`, eight pairs of `.rgba`, and per-draw `*-modules/module-*.metal/.json`. Raw shaders stay in those bounded artifacts; the audit reports only relevant slices and values. The provenance records HEAD, original input/observer/driver hashes, the vendored library, both executable hashes and the research witness script. After native capture, the driver gained stricter per-draw record assertions and binary-hash reporting; the saved records were rechecked against those assertions without rerunning GPU captures. Both execution-time and later driver hashes are preserved.

Executed checks: app builds successfully; all 104 libraries created; all eight draws contain volume compilation records; all eight image pairs match; 24 CPU witness groups pass; input renderer/compiler/save hashes remain unchanged; focused whitespace and document links pass. This is not a newly run full CTest suite, full-resolution image gate, timing run or live Person acceptance. No new production source, saves, original `nv_probe`, ordinary timestamp feature, subagents, commits or pushes. Concurrent Author Decisions additions are preserved.

Zach's approximately 21 FPS report remains qualitative live evidence from the default window with most auroras visible, angle unrecorded. This fixture is not that view. The existing continuous movement/appearance and 30 FPS checks remain open; no new visible change is expected from this pass.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-12T11:26:00-07:00 (PDT).
