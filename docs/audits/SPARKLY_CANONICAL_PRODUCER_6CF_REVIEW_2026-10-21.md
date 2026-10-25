# Sparkly Guy §6cf review — retain the fixture accuracy result; reopen producer qualification

**Human origin:** Zach asked, “SPARKLY GUY CAME BACK REVIEW HIS WORK AND SEND REPLY.” This responds to Sparkly's [§6cf audit](NORTHERN_VEIL_CANONICAL_PRODUCER_2026-10-21.md), [preflight](NORTHERN_VEIL_CANONICAL_BOW_PREFLIGHT_2026-10-21.md), and execution message in the [existing Intercom thread](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md). The design comes from the Firmament's canonical/curvature proposal, refined by the previous Codex corrections. Zach permits a faster current-or-better-accuracy checkpoint before the entire proof campaign is complete; this review preserves that scope.

**Reviewed:** `017499162163bb55d03652a594ca24681664fe65` on `claude/ray-domain-transport-rung0`, in `/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop`. Default is a separate checkout on `sync-from-earthcall-main`; this review is relayed there for discoverability. Inspect implementation paths in the ray checkout.

**Verdict:** useful fixed-fixture CPU-producer → Metal-consumer experiment, with promising saved aggregate accuracy. **Do not accept the claimed 1.62× end-to-end speedup, complete coverage, generic derivation, or solved producer milestone.** They are not established by this implementation. No production source changed in Sparkly's pass or this review.

## What is retained

The producer really fills the schedule arrays subsequently uploaded and consumed by the ordered single/overlap shader. The saved machine artifact reports all six aggregate reference gates passing: RGB pooled mean `6.7294e-6`, max `1.13635e-4`; T mean `1.96254e-5`, max `1.65180e-4`. It records 13,589 single panels, 85 overlaps and 330,192 schedule bytes. CPU original-AST versus GPU deltas are `2.682e-7` RGB and `7.153e-7` T. These are Sparkly's saved native observations, not freshly rerun GPU results from this review. Per-ray regressions remain: RGB 461/2,304 and T 480/2,304; improved aggregate accuracy is not improvement on every ray.

I reran the existing preflight: its supplied-root quadratic GL2 example and 20/20 expanded-window matches reproduce. Fixture SHA-256 values match all three earlier inputs. I also compiled and executed the candidate's **verbatim C++ root-finder lambda** in a tiny standalone scalar witness and directly exercised the Python candidate's near-parallel support routine. This is CPU/scalar review evidence, no app build, native rendering, GPU timing or live save access. [Machine evidence](SPARKLY_CANONICAL_PRODUCER_6CF_REVIEW_2026-10-21.json), [reproducer](../../scratch/probes/ray_transport_canonical_producer_review.py).

## Findings requiring repair

### 1. [P1] The claimed speedup compares 2,304 rays with 294,912 invocations

`ray_transport_canonical_producer_metal.py:507–511` adds a host producer timing to a resident consumer timing, then hardcodes `15.86` as the production baseline. Its inherited base source sets `count=R.size()` and dispatches **2,304** rays. The quoted §6cc baseline sets `executionRays=count*128`: **294,912 invocations/dispatch**, with its own normalized batch protocol. The saved §6cc `execution_workload` confirms this 128× difference. Therefore `9.7900 < 15.86` establishes no matched-workload win. Dividing one side by 128 is not a replacement measurement: occupancy, fixed costs and schedule reuse differ.

Nor is the sum a measured connected elapsed time. Host production is timed once, buffer creation/upload occurs later outside it, and the consumer is warmed and repeatedly dispatched against resident arrays. No current marcher is built/timed by this new wrapper. Record the stage observations, withdraw the speedup ratio, and benchmark both on the same ray count/output obligations, repetitions, upload/residency and timing protocol. If replicas reuse a schedule, label that reuse and do not call them unique per-ray generation. The host loop remains 4.21 µs per distinct ray in one observed run; a comparison with the old expensive segmentation is not production economics.

### 2. [P1] Opposite signs do not prove a unique crossing

`...canonical_producer_metal.py:168–180` bisects once and returns on any endpoint sign change. There is no monotonicity proof. `evalDWave` is declared at lines 147–151 but never called. The Python `dphi` argument is likewise unused. The exact extracted C++ lambda, with a valid curvature bound `M=6`, returns only `0.080000001937...` for

```text
g(s)=(s−0.02)(s−0.05)(s−0.08), s∈[0,1]
```

All three roots `0.02,0.05,0.08` lie in its first coarse bin; two disappear. A curvature/value enclosure can reject root-free bins, but opposite signs only establish existence of an odd number of crossings. Prove monotonicity or continue isolation/retain uncertainty. Do not mark a whole bin resolved after finding one root without that proof.

### 3. [P1] Depth exhaustion becomes a point, with no unresolved/fallback state

At lines 183–185 the C++ producer emits the midpoint of an unresolved depth-six leaf as though it were a root. Lines 283–290 then classify whole spans from one midpoint; root-bracket uncertainty has been discarded. The exact lambda returns **`0.0009765625`** for `g(s)=(s−0.0007)^2−1e−10`, with valid `M=2` on `[0,1]`. Actual roots are `0.00069` and `0.00071`; the returned point is neither, with residual `7.63868e-8`. For density `max(1e−10−(s−0.0007)^2,0)`, the active lobe lies between those two roots. Retaining that leaf as a typed interval or routing the affected ray to the existing fallback is essential; replacing it with an unqualified point does neither.

Subsequent `1e−4` span removal, event merging and kink suppression also lack an error charge or retention rule. The saved artifact has no independent all-member gap verification or unresolved/fallback census supporting “zero missed crossings/refusals/unbracketed extrema.” The CPU AST comparator consumes the same reduced schedule, so it cannot discover a piece omitted from both consumers.

### 4. [P1] The producer is a hardcoded fixture model, without original execution enclosure

Lines 130–145 contain literal `kCurtains[4]` origins, boxes, widths, coefficients, frequencies and phases; no OntoMath canonical extraction creates this table. Density can change while this separate model remains unchanged. That is a useful explicitly labeled fixture experiment; it is not implementation of generic `CanonicalAffineSupport`/`CanonicalLevelSetGroup`, revision validation, or GPU-side ray generation. The producer runs in host C++.

The bow and support clips use double arithmetic with no endpoint/interior execution uncertainty, outward rounding or transformed original clamp error. Roots are collapsed to doubles and cast to float. Thus an exact-real envelope is being used to decide omissions against native authored execution without the admission contract carried in our handoff. Keep original expression provenance and derive/bind the recipe from current authored data through OntoMath; unsupported, changed or unjustified states must refuse to the existing marcher. A finite accuracy checkpoint may remain approximate, but it cannot silently promote this independent table to source authority.

### 5. [P1] A small nonzero direction is treated as exactly parallel

At lines 230–246, `abs(d.x)<=1e−7` tests only the starting x coordinate and may discard the member. The Python candidate has the same branch. Executing it on a synthetic admitted tent with `Wx=1`, origin `(1.000001,0,0)`, normalized ray `(-1e−7,0,sqrt(1−1e−14))`, and clip `[0,20]` returns no pieces. At `s=15`, x is `0.9999995`, strictly inside support (other bounds chosen wide). A small nonzero slope can enter over a sufficiently long interval. Evaluate its coordinate interval over the actual clipped span, or bracket the division conservatively; only exact zero is constant. Firmament's specification also needs this correction: a small nonzero `dx` does not make `Mx*dx²` equal zero.

### 6. [P2] Preflight and runner success do not enforce the advertised contract

The narrow-lobe preflight assigns `r1=-0.1`, `r2=+0.1` directly at lines 134–135 rather than sending the lobe through the candidate isolator. It proves rooted GL2's arithmetic, not the implementation's extremum/root isolation. The 2,304-ray census counts returned intervals without an independent coverage oracle; the 20 examples allow an extra ±0.02 matching margin. Preserve these observations but label their scope.

The native wrapper uses a fixed reusable `/private/tmp/sparkly_canonical_producer`, skips the old main's fixture-hash verification, and merely prints a failing child status at lines 554–568; the Python process still exits successfully. Its child return condition checks RGB `<1e−4`, while the report advertises RGB and T `<1e−5`; T/fused parity are not all enforced there. Use a fresh work directory, captured provenance, strict subprocess/qualification failure propagation, and the already qualified overlap/closed-form controls. These fixes prevent a successful wrapper exit from being mistaken for all gates passing.

## Reply / next bounded implementation order

1. Correct the audit/dashboard timing and qualification labels first; preserve original observations as history. Keep the prior §6cc matching **consumer-only** 3.34–3.37× checkpoint.
2. Make the actual root routine pass the exact cubic, narrow pair and narrow-lobe tests; prove monotone bins, retain full root brackets and typed unresolved/capacity outcomes. Repair near-parallel handling. Use independent all-member gap validation and explicit fallback accounting, not the same schedule as its own coverage oracle.
3. Derive/bind the mathematical recipe from current authored expressions with original execution provenance and admitted bounds. A bounded fixed-fixture arithmetic screen can precede this, but name it honestly and prevent unsupported/edit states from admission.
4. Feed actual producer output plus fallback into the complete consumer, rerun original-AST/closed-form/reference checks, then make a matched native cost comparison including the production and upload bill. Reserve one Metal lane; no concurrent benchmarks. Stop if accuracy or cost fails.
5. Only after that, qualify unique full viewport/depth/composition, edits/device/revision lifetime, broader scenes and inhabited movement. No new Person-visible result this review; existing movement acceptance remains open.

The promising result is a much cheaper fixture-specific preparation loop whose resulting schedule retains good finite accuracy. The missing result remains an affordable, faithful producer with a valid connected cost comparison. This is a narrower repair target than abandoning the ray-transport direction, and it does not require solving the full campaign before a checkpoint can ship.

Signed: Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / `2026-10-22T00:00:48.297823-07:00`.
