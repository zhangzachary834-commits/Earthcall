# §6eq — the Screen publication boundary executes on the native renderer

Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-28, America/Los_Angeles; exact timestamp in the machine record.

Zach requested the next pass after [§6ep](GPU_RAY_TRANSPORT_6EP_REFERENCE_BINDINGS_2026-10-28.md). This executes the first native step proposed by [Sonnet Bridge](SONNET_BRIDGE_ROUTED_MODE2_TO_SCREEN_2026-10-27.md#proposed-minimal-patch-not-applied-each-with-witnesses), with Firmament's separate-consumer direction retained. Zach's authored-faithfulness requirement determines the gate: refusal must preserve Current's actual pixels, not merely a CPU model of its equations.

**Outcome:**16 retained source-bound native captures,11 positive pixel checks and two targeted native mutations pass/reject as intended. At2560×1440, the all-refused prototype matches the unmodified renderer across **all14,745,600 RGBA bytes**, with zero differing bytes. A separate full-resolution publication test returns the correct unique address for **all3,686,400 pixels**. This is an executed isolated Screen connection, not a connected Mode2 integrator or a performance result.

## What actually ran

Two standalone executables share the same diagnostic main and source-matching engine tree `3603add4018961529a91bc9592d76f9a553500b1`. The control links the **original, unmodified WebGpuRenderer object**. The bridge links an external source-derived copy of that renderer with only the publication entry point and buffer layout additions. The patched renderer retains its original CMake optimization/FP/language flags; only the common main uses its normal standalone-probe optimization. Neither the checkout's engine sources nor the shared build tree was edited.

The real renderer still calls `composeVolumeSet` and lowers the four authored Northern Veil media. Its native viewport rays, proxy rasterization/culling, opaque-depth texture read, clip rejection, authored evaluators, Current marcher, output discard and premultiplied blend pipeline remain. The extra read-only group0/binding3 supplies `vec4(L,alpha)` per pixel. After the original clip tests and before transport state initialization, `alpha<0` falls through to Current; accepted values use the existing discard thresholds and return into the same blend pipeline.

All data were read from the frozen checkout fixture, with its authored source expression enabled, clock7.25, and the stated camera. The witness target is **RGBA8Unorm over a diagnostic clear color** `(.125,.25,.5,1)`, encoded as `(32,64,128,255)`. No opaque geometry was drawn. That clear color is a controlled composition background, not a lighting edit or a change to the installed world. Inside camera:`0,50,75,40,62,110`; outside/spawn camera:`0,2,0,0,60,90`. Ordinary timestamps remain disabled. The default native macOS WebGPU adapter was used; its backend/vendor information was not queried.

The probe initializes a whole-viewport publication buffer on the CPU before any pipeline is selected. This is **synthetic protocol input**, not output from routed compute. It uploads once per process and binds the same buffer during three frozen frames; only the final captured image is retained and compared. It proves neither GPU producer completeness nor cross-frame validity. Hardware handles/counters are explicitly named scratch Kernel state, not hidden authored properties.

## Native checks

| Check | Viewport | Independent expectation | Result |
|---|---|---|---|
| Patched renderer, lane off |334×190 |Original renderer |0 changed bytes |
| All-refused `(0,0,0,-1)` |334×190 |Original renderer |0 |
| Refused with large poisoned RGB |334×190 |Original renderer |0 |
| Accepted unique pixel addresses |334×190 |Linear index encoded in RGB |0 |
| Accepted zero alpha and zero RGB |334×190 |Discard preserves clear color |0 |
| Accepted zero alpha, nonzero RGB |334×190 |Premultiplied additive contribution |0 |
| Accepted `(L,alpha)=(.125,.25,0,.5)` |334×190 |RGBA`(48,96,64,255)` over clear |0 |
| Publication budget refused before pipeline selection |334×190 |Original renderer; lane-off WGSL |0 |
| All-refused at native viewport |2560×1440 |Original renderer |0 /14,745,600 bytes |
| Accepted unique pixel addresses at native viewport |2560×1440 |Every pixel's linear index |0 /14,745,600 bytes |
| All-refused outside the proxy bounds |640×360 |Original renderer |0 |

Three separate original-renderer controls support those11 comparisons. The334×190 image exercises padded readback rows; its dimensions are not claimed to be odd numbers. The full-resolution publication buffer is58,982,400 bytes (56.25MiB), within this diagnostic's64MiB cap. Budget refusal is simulated by setting that cap16 bytes below the request; it selects the lane-off layout/shader **before** compilation. This is a budget-admission witness, not a GPU out-of-memory or device-loss test.

The address pattern was strengthened before final qualification: it encodes index`y*width+x` in the low/middle/high RGB bytes, with alpha1. All tested indices are below`2^24`, so the values are injective. A repeating checkerboard would not distinguish every address. Final evidence uses a fresh build and captures of the injective version; initial smoke/periodic-pattern trials remain scratch preparation, not the retained final suite.

The normal publication shaders strip back **byte-for-byte** to the encountered lane-off WGSL after removing the one binding and entry block:18 shader-text restorations across the retained positive cases. Budget-refused WGSL is itself byte-identical to lane off. These are shader-source checks alongside native pixels, not claims about compiled ISA identity, cache reuse across changing eligibility, or zero overhead. Shader dumps record `volumePipeline` encounters, which can include cache activity; counters are not presented as compile counts.

## Native counterexamples that must fail

1. **Discard before refusal:** the negative sentinel is incorrectly treated as an empty accepted result. The native process succeeds, produces the known clear background, and the Current-equivalence validator rejects **179,662 changed bytes**, exit2 at `Screen refusal pixel mismatch`.
2. **Ignore publications:** the native process successfully runs Current despite an accepted address buffer. The actual image equals the original Current control; the independent address validator rejects **189,870 changed bytes**, exit2 at `Screen publication pixel mismatch`.

An environment/shader failure is not counted as a successful mutation. Receipts/logs retain native return0, buffer binding/initialization, actual WGSL and the intended validator error. Normal positives return0 with exact expectations.

## Evidence and replay

[Machine record](GPU_RAY_TRANSPORT_6EQ_SCREEN_PUBLICATION_2026-10-28.json), [compact evidence ZIP](GPU_RAY_TRANSPORT_6EQ_SCREEN_PUBLICATION_2026-10-28.zip), [build/capture/validator](../../scratch/probes/ray_transport_6eq_screen_bridge.py), [CPU replay](../../scratch/probes/ray_transport_6eq_replay.py). The archive retains exact generated renderer/main sources, compiler/link arguments, binary/build/fixture/source fingerprints before and after each retained capture, shader texts, native logs, raw RGBA8 images and validator results. Eight unique images are stored by SHA256 rather than duplicating identical captures. The compact archive is about5.8MB; raw executables/build objects are not committed.

```sh
python3 -B scratch/probes/ray_transport_6eq_replay.py \
 --record docs/audits/GPU_RAY_TRANSPORT_6EQ_SCREEN_PUBLICATION_2026-10-28.json \
 --archive docs/audits/GPU_RAY_TRANSPORT_6EQ_SCREEN_PUBLICATION_2026-10-28.zip \
 --output /tmp/6eq-replay.json
```

Replay reconstructs generated source from the original source, validates receipts and all stored pixels, restores shader text, and confirms the two intended semantic rejections. It does not rerun the GPU. Fresh native reproduction uses `build --build-root <source-matching-checkout> --work <fresh-external-dir>`, then the recorded bounded `capture`/`verify` cases with the new binary/output paths. Each capture has a180-second process timeout. Build/test artifacts stay external. Native lane ownership must be checked in Intercom before repeating the suite.

## Remaining bridge and shipping account

**Scratch publication/refusal is now witnessed (W1 and the all-refused portion of W4).** Synthetic address/discard/blend checks establish parts of the publication/composition protocol. They do not establish optical accuracy of accepted routed answers, f32 intermediate equality, or the general W5/W6 integration contracts. Final RGBA8 equality can conceal smaller intermediate differences; this is the observable pixel gate for these frozen cases.

All three shipping gates remain **OPEN**. The next runtime step is **device ray and authoritative opaque-depth equality (W3)** against the unchanged fragment path, including invalid/empty clips. Then connect the real qualified consumer through production channel lowering, with mixed accepted/refused ownership, complete producer writes (W2), opaque-scene composition, clock/revision/resize/device validity and actual full-population accuracy. The production-parameter versus scratch-density layout distinction still applies. No proposed renderer-specific OntoMath plan/API has been adopted here.

The full230,400-ray reference remains unlaunched at the prior~8-hour estimate; the qualified reference pilot coverage is unchanged. Native frame economics and inhabited movement/Person acceptance remain open. CPU buffer filling, immutable per-process eligibility, capture/readback and shader dumping are diagnostic scaffolding, not a shipping pipeline or a clean timing scope. The mere presence of an all-refused branch may cost GPU time; matching pixels does not prove free execution.

No engine source, authored save, installed app/default or live-world lighting/resolution changed. There is no new Person-visible feature/check; existing Person checks remain open. This evidence/coordination is local, with no push by this pass. The native GPU lane is **RELEASED** after the final captures.
