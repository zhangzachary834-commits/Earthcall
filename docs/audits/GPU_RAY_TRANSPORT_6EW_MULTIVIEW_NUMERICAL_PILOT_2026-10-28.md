# §6ew — the live consumer passes a broader native numerical pilot

**Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-28T19:23:50.132756-07:00.**

Zach requested the next pass after [§6ev's transition controls](GPU_RAY_TRANSPORT_6EV_NATIVE_TRANSITIONS_2026-10-28.md). This follows his criterion of faithfulness to authored equations, rather than choosing the more pleasing image. The [existing Intercom thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md) holds the declared scope and reference cap before execution.

**Result:**24 accepted full2560×1440 native captures,320 selected pixel cases,880 new reference rows (774 distinct reference inputs), and36 continuity execution rows. All **162 scoped aggregate checks** pass: float RGB/T mean/p99/max on each lane's own inputs in4camera/depth cases, common ideal-camera targets in3clear views, and actual unmodified Screen RGB errors for those clear views, at both2^16 and2^15 reference resolutions. This expands numerical evidence; it does **not** close the full-population gate, establish pointwise dominance or measure inhabited performance. No production renderer change was required.

## Native measurement without substituting the implementation

Four camera/depth cases use the real **2560×1440 viewport**: inside `(0,50,75)→(40,62,110)`, turned `(40,64,110)→(-50,60,95)`, outside `(0,2,0)→(0,60,90)`, and the inside camera with an opaque half-screen foreground. All media and source clocks are7.25. Diagnostics sample every fourth row/column, producing640×360 records; this is not a640×360 viewport. The frozen authored fixture is unchanged.

Candidate rays are copied **after the actual production setup pass**, through host encoder buffer copies. Candidate computeWGSL is unchanged. The full58,982,400-byte publication has the same native SHA256 with observation ON/OFF in every scene. Raw retained candidate publication is the stride-four subset; replay verifies that subset and the native full-buffer hash equality, not an absent full raw buffer.

Current is harder to observe. A storage-buffer observer changed one red byte at pixel `(856,242)`,43→44, outside the sampled grid. It was **rejected**, not accepted with a wider tolerance. An initial four-attachment design required52bytes/sample and refused at the native32-byte capacity guard. These attempts are preserved under `rejected/`.

The accepted observer uses the original RGBA8 color attachment plus one unblended RGBA32Float attachment (20bytes/sample total). One unchanged diagnostic shader receives a runtime uniform selector **only in its post-integrator observation function**: mode0 returns origin/t0, mode1 direction/t1, mode2 T/RGB. Each mode has its own process/receipt. Late transparent discard becomes a zero-color/zero-alpha return so diagnostic data can be retained; other mathematical operations remain original. All three Current modes reproduce the unmodified Current image **exactly** in all4scenes. Candidate observation also reproduces its unmodified image exactly:235,929,600 retained Screen comparison bytes overall. Sampled float records reconstruct black-background Screen values exactly; the opaque foreground is excluded from that reconstruction, while its complete image still participates in observer equality.

These are **diagnostic Current float values with exact Screen controls**, not a theorem that every hidden intermediate bit of the uninstrumented shader is unchanged. The runtime selector does not enter integration. The positive controls link the actual existing app renderer object; the observed copy is separately source/object/binary-bound. Ordinary GPU timestamps remain disabled. No observed variant is installed or timed as an optimization.

## Independent reference and fixed cohort

Before references, each case freezes64uniform paired-valid positions and adds positions with the largest native T differences until80unique pixels are selected. The eligibility rule requires finite valid inputs and candidate publication. Paired-valid sampled coverage is230,400inside,230,400turned,167,871outside,230,400front; missing/unpaired pixels are not silently qualified. This is a deliberately bounded pilot, not a population maximum test.

Both lanes get **their own captured f32 directions and clip intervals**. A separate common target for each clear view uses a direct Decimal80 ideal60-degree look-at camera and exact fixture-union slab bounds, rounded once to binary64 for the existing reference. There is no invented ideal opaque-depth oracle. Own-input and common-target errors remain distinct.

The CPU reference is Opus's original OntoMath AST/Simpson generator and the previously qualified, closed worker-local map-reuse derivation. Original `ScalarForm.cpp` is rebuilt with `-O3` without relaxed arithmetic and linked first, preserving the established weak-definition selection policy. Fresh canonical and worker executions reproduce all18 historical continuity rays bit-for-bit. The reference preserves the existing evaluator's vector/scalar precision; “converged” describes nested numerical integration, not a rigorous real-number enclosure.

Eleven80-row jobs give880 rows and nested2^16/2^15 results. Including two18-ray continuity executions, the run uses **916 rows /115.286765 seconds**, below the declared960-row/600-second cap. Maximum finest-versus-half spread is **1.05600224e-7**, below the unchanged1e-6 check. Every input, result, executable, evaluator and receipt is bound by hashes. Four independent metadata negatives reject wrong input pairing, changed reference binary, missing rows and nonfinite reference data.

T comparison uses **1−published alpha** for both lanes; Current alpha is formed at f32 precision from its observed T. RGB statistics include both per-ray maximum component error and flattened component errors. The six required T/per-ray-RGB mean/p99/max metrics pass, as do the additional component statistics, at both reference resolutions.

## Results and limits

Own-input maximum errors at2^16:

| View | Current T | Candidate T | Current RGB | Candidate RGB |
|---|---|---|---|---|
|inside|0.003709047|7.006963e-05|0.002397558|4.822734e-05|
|turned|0.002091275|0.0001125921|0.001342068|7.225332e-05|
|outside|0.002479769|2.46576e-05|0.001591387|1.820051e-05|
|front|0.003709047|7.006963e-05|0.002397558|4.822734e-05|

Own-input maxRGB is about18.6–87.4× lower. Common ideal-camera maxRGB is26.65× lower inside,17.71× turned,87.23× outside. All corresponding mean/p99/max metrics improve at2^16 and2^15. These ratios describe the selected cohorts, not the complete scene.

**Individual float results are not uniformly better.** At the finest common target, candidate RGB error is higher for10/80 inside pixels,2/80 turned,2/80 outside; T is higher for11/80,2/80,2/80. Largest RGB increases are4.57835e-5,2.45216e-5,5.35697e-6 respectively. No candidate error exceeds the corresponding Current cohort maximum. Keep these regressions visible rather than selecting only aggregate wins.

The actual unmodified RGBA8 Screen samples also improve mean/p99/max against the common ideal target, using the clamped display RGB reference. None of the selected clear-view pixels has a worse **per-pixel maximum RGB error**; this does not imply every channel/float or every unreferenced pixel improves. For example inside Screen max error drops0.00413095→0.00198735. The full image is retained for observer checks, but only selected pixels have independent numerical references.

## What remains before merge-quality promotion

- **Connected default-on transport:** implemented, with Current fallback; preserved unchanged in this pass.
- **Transition validity:** §6ev's bounded31-state poison matrix remains the applicable evidence; it is not expanded to full-resolution transitions here.
- **Numerical coverage:** this pilot now spans4views and774distinct inputs, with all scoped aggregate gates passing. Full native population, wider fields/clocks, generic transport error certification and pointwise behavior remain OPEN. Do not replace a full-workload maximum gate with this sampled maximum.
- **Inhabited economics / Person acceptance:** OPEN. The earlier6.6–8.9% fixed-view Debug offscreen gain is not retimed here and is not live FPS. Release-build movement, complete frame/pacing costs and Zach's judgment remain separate.

A whole230,400-ray reference grid still projects roughly8hours with this CPU method (an estimate, not a launched job); a full native viewport is much larger. Before a large job, declare its cost cap and improve reference execution without changing arithmetic or admission. The still-per-leaf numeric-map allocation at `ScalarForm.cpp:1798` is a concrete next reference-cost investigation. In parallel in the campaign—not concurrent native jobs—prepare matched inhabited release-frame measurement. Neither improvement authorizes loosening fidelity, retuning light, reducing resolution or hiding individual regressions.

## Replay and evidence

```sh
python3 -B scratch/probes/ray_transport_6ew_replay.py docs/audits/GPU_RAY_TRANSPORT_6EW_MULTIVIEW_NUMERICAL_PILOT_2026-10-28.zip
```

Run from `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport` at the matching historical source/tool revision. Replay reconstructs native selection and every input, rechecks source/binary/receipt identities and continuity, recomputes all metrics and rejects the four metadata controls. It is CPU replay of retained observations, not fresh native execution.

[Machine result](GPU_RAY_TRANSPORT_6EW_MULTIVIEW_NUMERICAL_PILOT_2026-10-28.json) · [Content-addressed lossless evidence](GPU_RAY_TRANSPORT_6EW_MULTIVIEW_NUMERICAL_PILOT_2026-10-28.zip), **53,586,861bytes**, SHA256 `977cf69fb71691f8d70daa62681b7a05fda0ca46221aa46519806dddec71a905`.231 logical files /181unique payloads. Includes full Screen images, sampled native rays/floats, original/observed generated sources and receipts, reference generator/evaluator material, inputs/results/logs, rejected observer attempts and replay tools. Executable/object binaries are represented by hashes and build commands, not committed.

Native lane RELEASED after capture; no source/default/save/install/push or PR change. Person checks stay pending. **Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-28T19:23:50.132756-07:00.**
