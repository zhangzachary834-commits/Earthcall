# Firmament laboratory correction: full-lattice fallback restored

**Status:** correction implemented and verified on the uncommitted laboratory changes: focused CPU/native checks pass and all 32 authored-scene native image pairs are byte-identical to the pre-adaptive renderer.

Zach explicitly commissioned the Sun to fix the correctness bugs through Astra's [review and independent native witness](FIRMAMENT_LABORATORY_REVIEW_2026-10-16.md) and the [fix-first handoff](../../agent%20intercom/communication-threads/sdf-and-rendering/NORTHERN_VEIL_COUNSEL_REQUEST_TO_GPT_6_ASTRA_2026-10-11.md#astra-to-sixth-one-sun--zach-commissions-correction-of-the-laboratory-correctness-bugs). This pass works on `firmament-laboratory` at `dcff1876cc3aab8003f70c644b113333b5b8286c`, preserving the existing review/probe/task changes. The separate sync worktree is untouched. No performance claim or author verdict is made.

## Failure and correction

The original independent executable reproduces five RGBA levels of error and 1,163,024 pixels over the one-level limit for each of the single/fused 2560×1440 absorption scenes. Density is constant; extinction is `1 + 0.9*cos(12*pi*z)`, scattering/emission zero, background white, lattice 96 samples. Source-only stride bounds cannot constrain the error in background attenuation.

Both WGSL marchers now retain the accepted step spacing and every nonzero-support sample for all positive budgets. The separate opt-in flag remains, including positive double budgets that round to GPU zero. Zero budget keeps the existing compatibility cutoff. Exact zero-support traversal remains separate and retains its existing switch.

The new tail gates are removed separately. Their finite host-range/validity bit does not qualify native Fast arithmetic, accumulated numerical error and every omitted contribution. In particular, the rate builder used positive chroma maxima, which is not an absolute-value bound when the renderer consumes signed medium chroma. There is no newly established tail-specific pixel counterexample in this pass; the observed five-level failure concerns stride. Neither a density variation threshold nor finite images completes the missing contract.

Unused density-only `adaptiveWords`, stride counts/accessors, leaf variation classification and `VolumeCertifiedBounds` are removed. The per-positive-frame host bound sweep and z/w validity/rate producer are removed; reserved output-error z/w remain zero and no WGSL reads them. The prototype `evalFastRange` API remains experimental host analysis with no production callers, explicitly excluded from transport/native certification. No new cache, GPU resource or serialized field is introduced.

Scalar-map reuse, conservative midpoint rejection and Scale's noise-free-factor-first optimization are retained. New CPU witnesses initially failed 4/61 checks: the cheap zero-factor lane could treat malformed/scalar Noise inputs as a global noise interval, then multiply by zero. That lane now refuses malformed arity/types, and Scale refuses null operands. Ordinary nonzero noise interval behavior is preserved. The final 61/61 checks cover both operand orders, vector zeros, invalid operands, refreshed scalar bindings, host noise enclosure and midpoint rejection. Removing classification assertions is not a tolerance change: the independently observed transport failure is now tested in the native suite.

## Executed verification

- Rebuilt `earthcall_webgpu` and five focused targets successfully.
- CPU proof/range: **61/61**.
- Native proof/lifecycle: **73/73**; sharing/transport: **96/96**; unified quadrature: **15/15**; object rendering: **ALL OK**, no skips.
- Authored Northern Veil and Sky-Island Sanctuary: **32/32 exact 2560×1440 image pairs**, four views, t=0/7.25, compatibility/full-lattice modes, against frozen pre-adaptive `a094e6ab0` shader/renderer/proof-builder sources with a compatible shared current core. Every comparison has zero changed bytes and nonzero lit coverage. This is offscreen fixture rendering, not live movement or whole-world acceptance.
- Unmodified independent absorption witness: **eight comparisons** (single/fused × proof off/on × 160×120/2560×1440), each **zero changed RGBA levels and zero pixels above one level**, reference/candidate centre both 5.
- New permanent absorption regressions require byte identity and no recompilation/lowering/parameter/proof upload when only budget changes; they cover proof off/on, both marchers, density edits, clock advancement and a nonwhite background. Existing bright late emission, analytic/6144-sample reference, tiny-budget, source-clock, SDF/Gradient rebinding, membership and device-recreation witnesses remain.

[Machine-readable evidence](FIRMAMENT_LABORATORY_CORRECTION_2026-10-16.json) preserves failure/final results, source/binary hashes and build commands. Artifacts are in `/private/tmp/earthcall-firmament-correction-20261016/`. Native runs use desktop GPU access, ordinary timestamps disabled, and no timing/readback mixture. The configured build is incremental, not a clean-build claim. The public host-analysis comment was clarified after compilation; it changes no declaration or executable code.

No saves, original reference probes, installed app, domain enum, sample-count control, authority mechanism, commit or push changed. No new FPS or 30-FPS movement result. AD-30/36 and Person appearance/movement acceptance remain open.

**Signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-16T14:21:39.082330-07:00 (PDT).

**Final verification signed:** Codex / GPT-6.1 Sol / session `01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca` / 2026-10-16T14:25:43.327169-07:00 (PDT).
