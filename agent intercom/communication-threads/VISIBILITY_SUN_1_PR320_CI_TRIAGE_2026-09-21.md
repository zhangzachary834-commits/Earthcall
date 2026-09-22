# Visibility Sun 1 — PR #320 CI triage

Zach — I am actively checking PR #320 before replying in chat because the chat UI is glitching.

Current topology confirmed:
- #297 has already merged into canonical and is the authoritative exact Rung-8 Visibility baseline.
- #319 and #320 are two Prism-Sun post-#297 Density landing candidates created from the same architectural moment.
- #320 explicitly declares itself the sole intended Density landing PR and says it restored a missing `sourceTransportSignedStep(...)` replay defect before opening.
- I am now reading the exact #320 CI run/job results and comparing #319 vs #320 so we do not merge both mitosis daughters or repair the wrong branch.
- If #320 has a real branch-owned failure, I will fix it on `sol/prism-density-after-rung8-20260921`, preserve `rho_source != V_transport != D_medium`, and rerun/observe CI.
- I will not merge anything behind Zach's back.

— Visibility Sun 1

# Recovered chat end-state — the message Zach's UI swallowed

PR #320 is the canonical post-#297 Density landing lane. #319 is an alternate daughter and must not also land. The previous exact-head CI was red only in the SDF range-proxy job at the WebGPU object/radiance step; Focused CPU and Slow Adapter were green. Preserve `rho_source != V_transport != D_medium`; do not substitute #299/#312 and do not revert #297.

# Hourly torch pass — 2026-09-22 02:52 PDT

A later Sun had already advanced #320 to head `54bc007181f8ed2e7f5b6405359bb12573cb1602`, so this pass did not overwrite or replay earlier renderer work.

## New evidence: the allegedly failing native witness actually completed every assertion

I downloaded the `webgpu-object-parity-log` artifact from exact-head workflow run `35707849508`. Its tail is decisive:

- V0 isolated density: low `(21,21,21)` -> high `(145,145,145)`, numeric edit reused the shader (`compiles=0 hits=1`).
- V0c production composition: clear depth `(255,255,255)`, opaque-clamped `(93,179,93)`, proving opaque depth truncated medium transport while preserving the green receiver.
- Density Timeline: `t=.05:25` -> `t=1:255`, `compiles=0 cacheHits=1`.
- Radiance live edit: `bright=253 dim=38`, cache reuse.
- Radiance Timeline: `t=.15:38 t=1:253`, cache reuse.
- Two-source radiance: `(110,0,114)`.
- Rung-8 derived visibility: off `(105,0,121)` -> on `(0,0,121)`, with blue transport preserved and no shader recompile.
- Final line: `webgpu_object_test: ALL OK`.

The test source then explicitly calls `renderer.shutdown()`, prints `ALL OK`, flushes stdout, and executes `std::_Exit(0)`. Therefore the artifact is incompatible with a genuine assertion failure in `webgpu_object_test`: every branch-owned native V/D/Rung-8 witness reached its success terminus.

GitHub nevertheless classified workflow step `Verify WebGPU object/radiance parity` as failure on run `35707849508`, causing the later authored-Perlin gates and dependent Release A/B job to skip. At this point there is no evidence justifying a semantic renderer change: changing V, D, composition, or the assertions would be cargo-culting against a log that says the executable succeeded.

## Action taken

This intercom update intentionally advances the PR head and therefore requests a fresh exact-head CI observation without mutating renderer semantics. The next Sun should inspect the new run first.

If the fresh run turns the object/radiance step green, continue through the authored-Perlin gates and Release A/B before declaring #320 ready. If the same step is classified red again while its uploaded log still ends in `ALL OK`, investigate the runner/shell exit status itself (including the process status around the `tee` pipeline) rather than weakening the native witnesses or changing the V/D constitution.

Do not merge until the complete required CI graph is green.

— Hourly Torch Sun
