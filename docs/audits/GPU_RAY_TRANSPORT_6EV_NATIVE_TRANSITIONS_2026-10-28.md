# §6ev — actual renderer transitions survive poisoned resources

**Codex / GPT-6.1 Sol / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-28T17:34:41.951061-07:00.**

Zach requested the next pass after [§6eu's live-wiring repair](GPU_RAY_TRANSPORT_6EU_LIVE_WIRING_REVIEW_2026-10-28.md). The immediate obligation was transition correctness before further performance mechanisms: a default-on renderer must survive changing authored inputs and device resources without borrowing a stale answer. This continues the [existing Intercom thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md).

**Outcome:** all **31 same-process states** pass exact comparisons against fresh state-specific renderer instances, before and after mutable GPU resources are poisoned. Independent replay checks **75,000,040 comparison bytes** of RGBA8 Screen output and float publication. Four native stage-omission controls fail at their intended output boundaries. No production renderer change was required; the experimental default-on app remains `775f91105`'s implementation. This is execution/cache validity, not a new authored-reference accuracy result or speedup.

## Actual execution and comparison boundary

The positive executable replaces only the app's diagnostic main and links its actual existing `WebGpuRenderer.cpp.o` and CMake dependencies. The source is clean `775f91105`; the receipt records `HEAD:src`, twelve source hashes, empty source patch hash, exact generated main, compiler/linker commands, fixture, object and binary hashes. Ordinary timestamp-free native WebGPU devices are used. No save, installed app, default branch, resolution policy, sample cap, error tolerance or lighting equation is changed in the app.

One renderer lives through the complete sequence. For each state:

1. Render and retain its **first frame after the transition**.
2. Construct a fresh renderer on the current device with the same current diagnostic fixture and compare its output. The forced-refusal fresh instance first creates its Kernel with a normal frame before setting the existing refuse-only ceiling to zero.
3. Upload NaN poison to publication and `0xcdcdcdcd` to mutable ray/single/overlap buffers and route-id lists. Route counts and indirect args receive bounded nonzero poison, to expose missed reset without constructing an enormous dispatch. **Immutable viewport offsets/indices are not poisoned**; they have no per-frame producer.
4. Render again and compare both the first and poisoned outputs against the fresh result, including all four float publication words wherever the routed path is intended. Inspect every returned component for finiteness and derive accepted/refused/nonempty counts independently.

The counter totals **713,130,924 submitted poison bytes**, not physical DRAM traffic. A viewport change is first exercised through the actual resize path; then its newly allocated resources are poisoned for the repeat. The new allocation's first frame is not mislabelled pre-poisoned. The positive sequence performs95 compositions, including the fresh forced-refusal prime and explicit Current control. Comparisons count repeated state-specific observations, not75 million unique pixels.

The independent checker fixes the intended sequence and eligibility itself; it does not trust a native `expected_routed` label or a stored PASS flag. Hash equality confirms exact retained arrays, and raw float replay rechecks every publication component. Fresh-instance equality is a **state-validity oracle**, not an independent mathematical integral. A common mathematical bug in both instances still requires the separate authored-reference gate.

## Transition matrix

| Family | Exercised change and recovery |
|---|---|
| Retention / switch | Baseline repeat; ON → OFF → ON |
| Clocks | `(7.25,7.25,7.25,7.25)` → `(7.25,13.5,-2,4.75)` → original; source receives its own bound coordinate |
| Emission | In-memory expression scaled by0.4 with channel revision update; restore |
| Density | Literal outer factor changed from0.75 to0; restore with channel revision update |
| Structural admission | Add zero outside the density product, forcing conservative refusal; restore matching recipe |
| Bounds | Move first medium +8 inX and shrink itsZ half-extent to70%; restore |
| Membership | Four media → three (Current) → four |
| Incident source | Literal-zero source → positive source (Current) → zero |
| Compatibility source | No source and constant non-`wi` phase (legitimate Current, zero program refusals); restore authored phase/source |
| Viewport |334×190 →321×181 →640×360 →334×190, including padded tile/strip tails |
| Opaque depth | Half-screen opaque foreground → clear |
| Refusal | Normal search ceiling → zero (all rays refused, exact Current image) → normal |
| Camera | Inside → outside/spawn → inside |
| Device | Shutdown the existing renderer, acquire a second logical GPU device while retaining the first, reinitialize on the second, repeat |

All first and poisoned Screen images match fresh state-specific images. All **26 routed states** also match their fresh float publications exactly, with zero nonfinite values. The five intended Current states have zero routed dispatches and zero program refusals. Restorations, including the replacement-device cases, return exactly to the original baseline image. The mixed-clock, emission, zero-density and bounds controls each cause an observable image change, so unchanged output cannot silently pass their sensitivity check.

The test decodes the frozen Northern Veil fixture and makes only temporary in-memory changes to its first curtain's density/emission/bounds and the four phase expressions. No edited being is serialized, installed or injected into a live world. Existing fixture identities/authorship are not rewritten. Medium clocks are supplied separately through the existing bindings.

### Correction to a witness assumption

The first development attempt demanded that lowering a **positive** density multiplier0.75→0.38 visibly dim the scene. It did not, and both fresh/reused results agreed. That expectation was not justified by the authored equations: extinction and emission are independently authored channels, while density also gates participation. The corrected control sets density to zero, which must remove that medium's contribution. The failed attempt's main, logs, receipts and outputs are retained under `development/`, explicitly outside the final31-state matrix. We corrected the test rather than altering rendering to satisfy a mistaken aesthetic/physical assumption.

The no-source/non-`wi` control also closes a limit from §6eu: unlike its earlier Phi/wi refusal, this case **actually admits and renders through Current** with no program refusal. It verifies the conservative zero-source admission boundary in a meaningful compatibility scene. The retained compatibility view differs from the original zero-source/original-phase baseline in170,520 RGB bytes (maximum51 byte levels); source and phase both change as recorded above. It does not provide a new mathematical reference for that lighting.

## Native negative controls

A **separate** executable compiles an external copy of the current renderer with one include redirected to an external derived `.inc`. Four uniquely matched, reversible edits add diagnostic omission guards; the app and positive renderer object are unchanged. The archive binds source/object/binary hashes and exact compile/link arguments. Replay reverses every edit to the positive engine's exact source hashes.

| Native omission | Observed rejection |
|---|---|
|Setup omitted on the poisoned repeat|`publication finite baseline` — NaN poison survives |
|All schedule producers omitted|`Screen stale or missing output baseline` |
|Consumer dispatch omitted|`Screen stale or missing output baseline` |
|Consumer's actual WGSL publication store omitted|`Screen stale or missing output baseline` |

Each negative exits42 as intended and retains its mismatching float publication. The fresh comparison has its omission flag cleared and its own program cache, preventing a common mutated implementation from validating itself. Missing-publication is active at initial program construction so its cached consumer really lacks the store. No validation/compiler/device-loss error is substituted for the intended semantic rejection. No negative variant is linked into the app or timed as performance evidence.

## Evidence, replay and limits

[Machine result](GPU_RAY_TRANSPORT_6EV_NATIVE_TRANSITIONS_2026-10-28.json) · [Lossless evidence archive](GPU_RAY_TRANSPORT_6EV_NATIVE_TRANSITIONS_2026-10-28.zip): **9,638,755 bytes**, SHA256 `8f34ea0be85572ef1d8333c3f178f4a1456a450c7da7ca941ba55c0532e5169b`. A manifest maps282 logical filenames onto56 content-addressed blobs, preserving every repeated output's exact bytes without storing duplicates. Includes positive and negative mains/builds/receipts/logs/raw outputs, diagnostic sources, and the corrected development failure. No executable/object binaries are committed; their hashes and build commands are retained.

```sh
python3 -B scratch/probes/ray_transport_6ev_replay.py docs/audits/GPU_RAY_TRANSPORT_6EV_NATIVE_TRANSITIONS_2026-10-28.zip
python3 -B scratch/probes/ray_transport_6ev_transitions.py build --work /private/tmp/earthcall-6ev-fresh
python3 -B scratch/probes/ray_transport_6ev_transitions.py run --work /private/tmp/earthcall-6ev-fresh --name transitions
```

Run from `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`. Stored replay is CPU validation of retained native evidence; fresh native execution requires the desktop GPU. Negative generation uses `build --negative` in another fresh external directory, then `run --mutation missing-setup|missing-producer|missing-consumer|missing-publication`. Never run another agent's GPU job concurrently; claim the shared lane first. The final replay passes31 states and4 actual native omission rejections.

**What remains before a shipped optimization:**

- **Live connection/defaultON:** implemented on the authorized feature branch; unchanged in this pass. §6eu's bounded full-resolution publication and fixed-view connected cost remain dated evidence.
- **State validity:** the matrix above passes at334×190,321×181 and640×360, on two logical native devices. Full-resolution transition coverage, arbitrary edits/medium families, resource-exhaustion and actual device-loss/recovery remain outside this witness. Mixed-clock output sensitivity proves a nontrivial clock update, not independent mathematical accuracy for every clock/member.
- **Authored numerical admission:** still OPEN. Bind the live consumer's actual rays/clips and current outputs to independent multi-view references with mean/p99/max RGB/T gates at both resolutions. Neither this fresh-instance comparison nor old references for different rays can substitute. Existing §6eu accuracy remains an80-pixel pilot.
- **Inhabited economics / Person acceptance:** still OPEN. The previous6.6–8.9% fixed-view Debug offscreen gain is not retimed or promoted to live FPS here. Measure the inhabited release build with movement, full-frame completion and pacing, then retain Zach's visual/movement judgment separately.

**Next bounded quest:** a multi-view, source-bound actual-consumer numerical pilot with contemporary Current RGB/T controls and converged references; expand coverage under a declared cost cap while retaining the full-population acceptance requirement. Then matched inhabited frame economics. Preserve the current default-on feature experiment and whole-ray Current fallback. No new proof tolerance, resolution reduction or aesthetic retuning is authorized by a cache test.

Native lane RELEASED after the positive sequence and four omission runs. All work is local; no push or PR. Person verification boxes remain pending.


**Final packaging revalidation:** three whitespace-only fixture lines were cleaned before commit. The generated mains were rebuilt and the entire31-state/four-omission native sequence rerun against the unchanged renderer; every retained RGBA8 and float-publication output is byte-identical to the earlier completed run. Final tool hashes and receipts bind the cleaned files. Earlier size-only Intercom packaging notes are superseded by this final archive hash.
