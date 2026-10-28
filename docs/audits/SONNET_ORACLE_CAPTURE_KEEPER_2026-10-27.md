# Sonnet Oracle Keeper — identity-bound reference wrapper for distinct native ray captures

Claude Code / Claude Sonnet 5.5 (`claude-sonnet-5-5`) / session `37985354-1fde-47c7-9707-50ceed335c11` / 2026-10-27 PDT. Commissioned by Sixth-One Sun (Codex / GPT-6.1 Sol) on Zach's approval (Quest 2). Base: local checkpoint `46ebde497`; follows [§6em](GPU_RAY_TRANSPORT_6EM_CORRECTIVE_TAKEOVER_2026-10-27.md), which asks the next owner for "a bounded denser set of distinct recorded native rays with … finite coverage and actual reference/parity checks", and builds on Opus 5.5's [full-workload reference](NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27.md) (the 316-ray finding that holdouts missed). CPU only. No GPU execution, capture or timing; no large reference run; no engine, save, app, default, lighting or resolution change; no edit to shared ledgers.

## What exists (all new, all mine)

| File | Role |
|---|---|
| `scratch/probes/sonnet_oracle_capture_gate.py` | Validates a capture, binds Current/candidate/reference rows to it, then calls the **unchanged** shared `ray_transport_full_workload_accuracy.evaluate` (all rows, both resolutions, mean/p99/max, RGB pooled and T). Only adds refusals. |
| `scratch/probes/sonnet_oracle_controls.py` | 22 small CPU controls (below). |
| `scratch/probes/sonnet_oracle_plan.py` | Prints compute/memory estimate and the proposed invocation. Never launches anything. |

The reference itself is the existing `veil_projected_reference.cpp`, **unchanged** (raw authored ASTs through `MathNode::evaluate`; box containment; f32 `p`; `max(X,1e-6)·[D>0]`; authored zero source refused otherwise). It already takes any ray count (`count=min(rays,limit)`) and one finest grid yields both resolutions (stride 2), so no second run is needed for the half level.

## Capture contract the wrapper enforces (a proposal for Surveyor/Sun to freeze)

JSON (or `.gz`): `schema="sonnet-oracle-capture-1"`, `zone_sha256` (must equal the live `zone.ecform` hash), `time`, `medium_clocks{id→clock}`, `origin`, `ray_count` (must equal `len(rays)`; optionally the frozen plan count), `rays`: rows `[px,py,dx,dy,dz,a,b, …]` padded to **23 columns** because the unchanged probe indexes `r[7..22]` for its own printout. Per ray: finite, integer distinct pixel, unit direction (±1e-4), `b>a≥0`. `rays_sha256` is the file's sha256 (as in the shared checker). Candidate and Current rows are `[px,py,Lr,Lg,Lb,T]` in exact capture order (the layout the shared checker's `native_rows` produces; I found by reading `evaluate` that it compares `a[2..4]` to reference L and `a[5]` to reference T — a layout mismatch would silently swap T and Lb, so the wrapper binds pixels per index).

## Executed evidence (CPU, this session)

`python3 -B scratch/probes/sonnet_oracle_controls.py` → **`ORACLE CONTROLS PASS: 22`**, 0.1 s. These test gate logic and the reference scheme on synthetic rays; they are **not** evidence about Northern Veil.

- **Reference scheme (Python mirror of the probe's Simpson, 2^12 grid):** continuous tent matches the closed form to <1e-6 with finest-vs-half below the ceiling; `D≤0` everywhere gives T=1, L=0 even with huge X; `D>0, X=0` floors to σ=1e-6 (T=exp(−1e-4) to 1e-12).
- **Grazing support:** a support narrower than the half-grid spacing centred on an odd finest node gives T=0.98386 finest vs 1.0 half (Δ=1.6e-2). The convergence guard **refuses** it rather than letting a reference that depends on grid alignment judge a candidate.
- **Localized tail error:** one ray at 30× error among 400. Mean and p99 gates pass, RGB-max and T-max fail, `beyond_current_max_indices=[203]`. The same error is invisible to a stride-20 sample, which passes through the same evaluator — recorded as proof that sampled-max acceptance is wrong, not as a method.
- **Refusals:** missing candidate/Current ray; reordered candidate/Current; stale fixture hash, clock, pose, ray-file hash, frozen count; truncated capture with declared count; differing medium clocks; duplicate pixel; NaN ray; ray narrower than probe width; non-finite output; failed reference convergence; attempt to widen the convergence ceiling. Strictly-better candidate and identical-Current pass at both resolutions.
- The shared checker's `--help` still loads; I did not modify it.

Not executed: the C++ probe (needs a build tree absent from this worktree), any real capture, any gate on real data.

## Compute/memory estimate (extrapolated from recorded rates, not measured here)

Rates: 3.3e8 evaluations in 109 s on 4 threads (≈3.03e6/s, [projected reference](NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21.md)); 4.73e9 evaluations for 32,784 rays at 2^16 (≈1.44e5/ray, Opus's audit; cost scales with 2^k+1 nodes and ray count). `sonnet_oracle_plan.py` gives, at 2^16, 4 threads: 32,784 rays ≈ 4.73e9 evals ≈ **26 min**; peak ≈ 2.1 MB per worker (`sig` + `src` for one ray), ≈ 8.4 MB for 4; output 64 B/ray; the probe parses the whole ray file into a JSON DOM (~200 B/ray estimated). Cost is linear in ray count: 131,136 rays ≈ 104 min. Dropping to 2^15 halves cost but Opus's 2^16/2^15 pair is the validated convergence pair.

Proposed invocation (after the capture and cost cap are frozen), from the worktree root with a `veilref` built per the wide-reference audit:

```sh
veilref CAPTURE.json 16 1000000 4 REF.json
python3 -B scratch/probes/sonnet_oracle_capture_gate.py --capture CAPTURE.json --reference REF.json \
  --candidate CAND.json --current CURRENT.json --zone "saves/zones/Northern Veil/zone.ecform" \
  --expected-count N --expected-clock 7.25 --output GATE.json
```

## Unresolved math decisions (for the Sun / Zach)

- **D1 — medium-specific clocks.** The unchanged probe applies one `time` to every medium. The wrapper therefore *refuses* a capture whose `medium_clocks` differ. Supporting distinct per-medium clocks needs a probe change (a variable-map lookup per medium); I did not fork the probe because I could not compile-check it here. Decide whether Northern Veil captures can differ per medium before the capture is frozen.
- **D2 — convergence ceiling.** I set a per-ray finest-vs-half refusal bound of **1e-6** (observed full-workload 6.5e-8 RGB / 1.1e-7 T; Current's error scale ~1e-4). This is my choice, not a council-set value; it can be tightened but never widened by the code. Confirm or replace it before the first real run.
- **D3 — jump discontinuities.** Where σ jumps (positive-density support edges with X not tending to 0, or a floor-dominated edge), Simpson is only O(h) and the half/finest pair can both be wrong in the same direction. The guard catches grid-alignment dependence (grazing control) but cannot prove convergence for a *resolved* jump. If real captures have such edges, an exact-edge refinement (root-find D=0 per member, split the integral) is the right fix; it is a new reference and needs approval.
- **D4 — "contemporary Current".** The wrapper trusts the supplied Current rows; it verifies identity only through the capture binding (pixel order, count). Binding Current to the same build/clock/fixture needs the Surveyor's capture record, which does not exist yet.
- **D5 — gate on projected-model scope.** As in the prior audits, passing means agreement with the projected-model numerical reference at one pose and clock, not a native execution certificate or FPS claim.

## Handoff

Branch `claude/sonnet-oracle-ray-transport-c7ecc9`, worktree `.claude/worktrees/sonnet-oracle-ray-transport-c7ecc9`; commit hash is in my final reply (not self-referenceable here). I did not push, merge or touch the To-do list, compass, AGENTS or Intercom; the Sun may want a To-do bullet pointing at this report. No Person-visible change; nothing for the Person Verification List.
