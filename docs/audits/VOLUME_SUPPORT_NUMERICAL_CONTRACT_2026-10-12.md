# Volume support: conditional numerical contract and production refusal

Zach commissioned the next testing/implementation pass after reading **Performance as Rightly Ordered Truth**, with Astra's hold advice governing dependent work. This answers [Astra's bounded certificate brief](../../agent%20intercom/communication-threads/sdf-and-rendering/NORTHERN_VEIL_COUNSEL_REQUEST_TO_GPT_6_ASTRA_2026-10-11.md#astra-to-the-sun--bounded-certificate-brief-and-author-decisions). Zach originated the minimum-work, compile-once, native-fidelity direction; Astra specified numerical admission and refusal-first research; the Sun developed the rational model and two-endpoint band below.

**Result:** the CPU research tool passes 18 named witness groups, but admits **zero production skips**. A conditional operation model is implemented; the complete backend/coordinate contract requested by Astra is not established. Stop this runtime candidate here. The retained renderer, authored equations, midpoint lattice, ordinary timestamp policy and saves are unchanged. No new native correctness, performance or FPS result is claimed.

## Recognized expression and emitted operations

[Research tool](../../scripts/volume_support_certificate_research.py) recognizes authored AST structure:

```text
clamp(1 - guarded_div(abs(z + sum(A_i * trig(k_i*x + s_i))), w), 0, 1)
```

The two spatial axes must differ; `trig` is sin or cos. Recognition uses existing operations, variables and tree shape, never Zone/object identity or fixed parameter slots. It accepts at most seven trig terms and one unguarded density Piece for this fixture screen. Unsupported structure declines the screen. The certificate target is **this scalar clamp**, not an entire medium's contribution. Clock/source/shifted-point bindings cannot be substituted for spatial bindings.

The current [WGSL emitter](../../src/Singularity/Screen/WebGPU/SdfWgsl.cpp) records binary32 parameters through `Emit::param(float)`; `emitScalarForm` emits weighted trig of affine inputs; `MathNode::Div` emits `select(0, a/b, abs(b) >= kDegenerateDivisor)`; Abs and Clamp retain their operations. The tool models converted finite parameter values, the guard threshold, affine input rounding, intrinsic uncertainty, weighted products and sum trees. It does not change OntoMath's mathematical interval semantics.

## Conditional closed arithmetic model

The normative starting point is the [WGSL floating-point contract](https://www.w3.org/TR/2026/CRD-WGSL-20260921/#floating-point-evaluation): runtime rounding direction is not fixed; subnormal inputs/results may flush; f32 division has 2.5-ULP accuracy over its stated denominator range; f32 sin/cos have absolute error at most 2^-11 on [-pi, pi], with accuracy unspecified outside that range. Reassociation and fusion must also be addressed. These rules justify conservative uncertainty, not an empirical epsilon.

Implemented research enclosure:

- Exact rational endpoints; directed binary32 conversion verified by rational comparison and adjacent-float repair. Nonfinite inputs and possible overflow decline.
- Basic rounded add/multiply intervals, including optional subnormal flush to zero. Division encloses three maximum ULPs plus directed rounding, and declines zero or denominators outside [2^-126, 2^126] in magnitude.
- A rational pi bracket from Machin's identity and alternating-series remainder. Trig uses a 16-term rational Taylor midpoint, Lagrange remainder, derivative bound one, the normative 2^-11 intrinsic allowance and outward conversion. Every possible argument must remain inside the inner pi bracket. No host-libm endpoint epsilon constructs the range.
- All additive parenthesizations/permutations of the weighted trig atoms and displacement coordinate. Product intervals include exact products as well as rounded products, enclosing contractions within this **closed model**. Abs, guarded division, subtraction and Clamp boundaries remain ordered in that model.

The model starts with already materialized binary32 coordinate intervals. It does **not** establish an exhaustive enclosure for the production local-point producer, all transformations permitted across the emitted module, or every backend's parameter conversion/execution behavior. Function calls and local variables are not treated as verified numerical barriers. Enumerating the final sum trees does not prove closure over broader transformations. No runtime flag in the tool can assert a production profile: `production_zero` always returns Unknown.

## Two-endpoint scalar-zero band within the model

For one fold-axis bin, let weighted intrinsic intervals be `T_i=[lo_i,hi_i]`, with displacement constrained by `abs(z)<=Z`. Define:

```text
M = Z + sum(max(abs(lo_i), abs(hi_i)))
E_0 = 0
E_(j+1) = E_j + upperULP(M + E_j) + 3*2^-126
tau = w / (1 - 3*2^-23)
band = [-sum(hi_i)-E_n-tau, -sum(lo_i)+E_n+tau]
```

There are `n` additions for `n` trig terms plus z, regardless of parenthesization. Each step reserves one maximum ULP and up to two input flushes plus one output flush. Width must be finite, positive, within the division accuracy range and at least the converted degeneracy threshold. Possible sum/quotient overflow declines. For finite normal quotient q, `2.5*ULP(q) <= 3*2^-23*q`; tau therefore suffices to keep every enclosed quotient at least one. Round the final band endpoints outward. A displacement interval **strictly outside** that band gives an exact scalar zero under the closed model.

Proposed bin selection compares a coordinate against stored binary32 edges directly, avoiding an unbounded float-to-index expression. A coordinate enclosure crossing an edge declines; a future all-possible-bin union is not implemented. Runtime use would additionally require the same coordinate producer/profile, bounds and dependency identity. No such runtime selector or guard is installed.

## Refusal witnesses and what they establish

The final executable report passes **18 named groups**: uncertainty-margin refusal; separated model scalar zero; band margin refusal; uncertain bin edge; principal-domain boundary; unsupported intrinsic domain; guarded small denominator; nonfinite parameter; overflow; unsupported division range; subnormal flushing; stale numeric dependency refusal; an edit invalidating a formerly zero value; independent clock; shifted SDF point; source coordinate domain; unresolved production profile; and independent endpoint/midpoint samples for 98 rational trig intervals.

Constructive uncertainty witness: `w=1`, `A=1`, `sin(0)`, `z` approximately 1.0001. Real arithmetic suggests absence. A result displaced by the permitted negative intrinsic error can leave a positive clamp. The guard therefore refuses this margin. This is an allowed-error model witness, not a measured GPU failure. The edited-amplitude case changes both the dependency key and the conditional zero conclusion. Binding mismatches refuse stale reuse rather than silently sharing.

The independent host `math.sin/cos` samples corroborate the rational construction; they are ordinary host floating-point samples, not GPU conformance or proof of the backend. Every grid query accepted by the band also passes a separately evaluated interval expression enumerating additive trees. Both checks share the declared closed model and cannot establish its production admission.

## Conditional screen; admitted opportunity is zero

The fixture screen uses 256 fold-axis bins and 128 displacement positions per member, with directed coordinate enclosures. Only bins whose complete intrinsic intervals fit the supported domain are evaluated:

| Fixture member | Model-enclosed bins / 256 | Unknown bins | Conditional queries | Conditional scalar zeros | Production admitted |
|---|---:|---:|---:|---:|---:|
| Emerald curtain | 30 | 226 | 3,840 | 3,516 | 0 |
| Cyan ribbon | 29 | 227 | 3,712 | 3,421 | 0 |
| Violet crest | 35 | 221 | 4,480 | 4,064 | 0 |
| Crimson fringe | 63 | 193 | 8,064 | 7,337 | 0 |
| Total | 157 / 1,024 | 867 | 20,096 | 18,338 | **0** |

The remaining bins fail intrinsic-domain admission. This is not a ray/sample demand screen: it includes empty space already eliminated by retained proofs, excludes unsupported bins, and has no live camera, transport, noise demand or opacity history. **18,338/20,096 is neither a GPU skip rate nor a remaining-work fraction.** Earlier 42.21% and 21.86% opportunity estimates remain separate exploratory results, not updated admission claims.

A hypothetical full table would need 8,192 bytes of endpoint pairs, 4,112 bytes of four 257-edge arrays and 128 bytes of validity bits: **12,432 bytes**, before additional profile/dependency metadata and alignment. A binary search takes about eight comparisons plus domain/profile checks, table accesses and displacement comparisons. The tool stores research JSON only; no GPU buffer or layout is implemented. Guard cost and break-even have not been measured. With zero admitted production opportunity there is no justified native guard benchmark in this pass.

## Author dependencies and next boundary

[AD-30–32 remain OPEN](../Agenda/Tasks/For%20Zach/Zach%20Author%20Decisions.md#northern-veil-authored-channel-semantics--2026-10-12). This research does not choose universal density support, remove the extinction floor, or alter missing source-chroma semantics. A scalar zero alone does not authorize new whole-contribution skipping. Such skipping must close over all channels under Zach's adopted semantics. Expression-local reuse can be considered independently, but still needs a sound backend contract and consumer analysis.

The next dependency is a verified numerical profile covering the actual emitted operation transformations and coordinate producer, or a different expression-level proof whose complete allowed execution is enclosed. Principal-domain restrictions must still leave useful demanded coverage. Until then, retain the evaluator and stop this candidate. Do not add a larger cache or revive rejected transport/noise experiments on this result.

Zach clarified that his approximately 21 FPS observation used the **default window size**, with **most auroras visible**, and no recorded exact angle. Physical framebuffer/camera/build/motion remain unmeasured. No reconstruction of that view, new visual result, matched gain or 30 FPS movement acceptance follows here.

## Reproduction and integrity

Read-only decoded fixture: `/private/tmp/earthcall-sixth-one-sun-01a122d7/proof-pass/zone.json`. Reuse the tool with a **new** output filename; it refuses overwrites:

```sh
python3 scripts/volume_support_certificate_research.py --fixture /private/tmp/earthcall-sixth-one-sun-01a122d7/proof-pass/zone.json --output /private/tmp/volume-support-new-report.json
```

Authoritative result: `/private/tmp/earthcall-sixth-one-sun-01a122d7/proof-pass/certificate-research/verified-research.json` and adjacent `verified-research.log`, with `provenance.json`. Earlier research/banded/final files in that directory precede the final tightening and are not the authoritative result. The fixture was already decoded for prior diagnostics; repository saves remain MsgPack and no codec changes are introduced.

Provenance records HEAD `7df0702273bb50ee08bf976d0bf4cdb4ae3523fd`, tool SHA256 `738b1041985a8d992fd9045d13118b2332e15923a336afa23b783f9c8b3e53f6`, decoded fixture SHA256 `30115723790b9e64f34b3830c7f8f8bb4acdde1bdf3952890a3092274ca7edf3`, and Northern Veil save SHA256 `6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`. Production volume sources remain the retained baseline. No native run, subagents, commits, pushes, save mutations or original `nv_probe` edits in this pass. Concurrent unrelated changes are preserved. Global whitespace checking still reports another agent's pre-existing Crucible trailing whitespace; focused checks apply to this pass's files.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-12T00:40:00-07:00 (PDT).


## Later native-profile follow-up

[Native observation and coordinate refusals](VOLUME_SUPPORT_NATIVE_PROFILE_2026-10-12.md) records the retained binary's actual Metal Fast/Fast option profile, eight paired 320×180 images and six additional witness groups (24 total in the evolved tool). The complete production execution enclosure still refuses admission. The 18-group report/hash and local-grid counts above remain the original dated result; the current script has subsequent additions and a new hash. No historical conditional coverage is promoted into production skips.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-12T11:27:35-07:00 (PDT).
