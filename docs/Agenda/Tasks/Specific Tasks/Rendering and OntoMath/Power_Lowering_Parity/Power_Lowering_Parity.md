# Generic power lowering parity

Zach commissioned a rendering/Intercom catch-up. During that review, Codex found source-contract discrepancies in the generic `ontoPow` helper introduced by `07435d0ce`. **Current status: route-aware normal/domain repair implemented and natively witnessed; full CPU-double/native and subnormal parity remain unresolved.** The original finding and planning chronology follow; the implementation record is below.

At the original finding, `SdfWgsl.cpp` used the helper in ScalarForm, MathNode and RPN power routes. It returned zero for all bases with magnitude below `1e-12`, and treats negative-base exponents within `1e-4` of an integer as integers. Existing CPU `MathNode::Op::Pow` tests exact integrality for negative bases and otherwise uses `std::pow`. Positive tiny bases with negative exponents and negative bases with nearby noninteger exponents expose discrepancies. CPU Term/RPN routes need their own documented contract rather than assuming all routes are identical.

Source-level examples with uploaded f32 values: `(1e-13)^(-1)` is about `1e13` under the existing CPU scalar rule but zero under the helper branch; `(-2)^(2.00005)` returns zero under the existing MathNode scalar rule but about `4.00014` under the helper branch. These are inspected/replayed source paths, not native captures or evidence of a specific inhabited-world frame defect.

Follow-up: reproduce through actual scalar/vector/ScalarForm/RPN consumers as admitted; cover zero/tiny bases, negative exponents, exact and nearby noninteger exponents, and nonfinite/domain refusal. Repair the implementation against the appropriate existing OntoMath contract without inventing tolerance-based authored semantics. Preserve parameter slots and independent lowerings. Validate native behavior, then establish a new compatible performance baseline if generated code changes. Keep this separate from shared-group scheduling experiments and preserve saves.

[Review, exact provenance and scope](../../../../../audits/RENDERING_INTERCOM_CATCHUP_2026-10-15.md). No production change is implemented by this task entry.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T01:38:17.654972-07:00 (PDT).

**October 15 planning continuation:** Firmament confirms the route differences and supplies a proposed repair, not an adopted contract. [Stage A of the implementation plan](../../../../../plans/VOLUME_DEMAND_AND_SUPPORT_IMPLEMENTATION_2026-10-15.md#4-stage-a--correctness-baseline-and-reproducible-inputs) requires actual route/native witnesses first: its proposed `1e38` zero-to-negative sentinel is finite and does not preserve CPU infinity; `8388609` is a representable odd binary32 integer, contrary to the reply's broader evenness claim. Host checks reproduce those two qualifications. No helper repair or route unification is implemented, and this correctness work must establish a compatible baseline before scheduling economics. Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T03:21:44.030398-07:00 (PDT).

## Implemented route correction and native boundary

[Implementation, numerical cases and native evidence](../../../../../audits/VOLUME_POWER_AND_OPTIMIZATION_TRIALS_2026-10-15.md) replace the epsilon-defined base/exponent domains and near-integer approximation with exact binary32 classification. Scalar/vector MathNode retains its negative/noninteger zero rule; ScalarForm Term and actual legacy RPN retain their different `std::pow` domain behavior. Signed zero/infinity, tiny normal bases, exact integrality and parity around 2^23/2^24 are covered. Both shader prefixes share one helper implementation; numeric slots are preserved. Missing power operands refuse safely.

The first expanded native witness exposed CPU-subnormal/native-zero output at `FLT_MAX^-1`. Its failure is preserved and a separate direct intrinsic witness confirms the retained native behavior. This case is explicitly reported outside CPU parity; native compiler options and authored mathematics were not changed to hide it. CPU-double equivalence, exhaustive native arithmetic enclosure and subnormal parity remain open, so this task is partial. Subsequent optimization trials use a frozen power-corrected reference; all were rejected independently.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-15T05:56:25.134858-07:00 (PDT).
