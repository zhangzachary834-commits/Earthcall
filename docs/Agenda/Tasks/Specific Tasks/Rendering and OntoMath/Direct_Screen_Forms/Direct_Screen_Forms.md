# Direct authored Screen forms

Codex / GPT-6.1 Sol / session `01a10a2b-a247-7c11-9d5f-7a8b89df6cfc` /
2026-10-04 21:10 PDT. Implementation and verification session; see evidence below.

Zach asked whether “make my screen's pixel here display ___” was directly authorable,
then explicitly requested the direct medium after identifying the Object/Material/
ShapeKind admission requirement under Refusals 1, 3, and 7. That direction is Zach's.
Codex implements the Screen act by extending the existing OntoMath GPU compiler,
rather than inventing a visual being, category, action opcode, or expression language.

The existing paint path remains meaningful for painting an Object's own surface.
This path supplies the previously missing direct framebuffer manifestation.

## Contract

`ScreenChannel::manifestOutput` is called by the production `EngineRender` loop after
world/volume composition and before compatibility HUD/tools. It reads ordinary authored
Properties and asks the Renderer to sample their mathematics. No Object geometry,
Material, FaceTexture, texture sampling, or ShapeKind participates in this act.
The existing GPU ring pool supplies aligned uniform/storage slices; this path adds no
per-frame driver buffer allocation, and its bytes contribute to the existing Screen metrics.
The backend's fullscreen triangle only dispatches fragment invocations; it is Kernel
machinery, never the author's form or bounds.

The channel admits one composed output expression. A Person authors its pieces and
region selectors. It does not invent a multi-source stacking/ownership model; compatibility
HUD/tools still render later. This pass addresses Earthcall's framebuffer, not other
applications, OS windows, or a physical monitor independently of the app.

| Authored Property on `@screen-channel` | Meaning |
| --- | --- |
| `output.colorPath` | Qualified PropertyPath to an AST `VectorField` on any addressable Singular |
| `output.opacityPath` | Optional qualified PropertyPath to an AST `ScalarField` |
| `output.timePath` | Optional qualified PropertyPath to a finite numeric temporal coordinate |
| `output.color`, `output.opacity`, `output.time` | Local typed values when the corresponding path binding is absent |

Use existing `AddProperty`, `Set`, and `RemoveProperty`. A supplied path takes precedence
and must resolve; a malformed binding refuses rather than falling back to a local value.
Missing color withdraws the act. Missing opacity means opaque where color is defined.
Missing time leaves `t` unbound; an expression reading it refuses. `enabled` governs
Screen as before. No new enum entries or class for a domain noun were added.

| Coordinate | Actual binding |
| --- | --- |
| `x`, `y` | Physical framebuffer pixel centres, origin top-left: `(column+.5, row+.5)` |
| `z` | Zero: this admitted domain is two-dimensional |
| `p` | `(x,y,0)` |
| `u`, `v` | `(x/width, y/height)` |
| `width`, `height` | Current physical framebuffer dimensions |
| `t` | Only the explicitly supplied temporal coordinate; no borrowed clock |

Window points and physical pixels are deliberately distinct on Retina. For one physical
pixel at column 7, row 9, author a piece whose `whereLEZero` is
`distance(p, (7.5,9.5,0)) - .1`, with the desired color as its value. A circle, union,
intersection, arbitrary expression, or interval is the same mathematical vocabulary;
there is no enum of pixel region kinds. The first applicable piece wins. Interval
endpoint inclusion flags are honored. Undefined color/opacity regions leave prior
scene pixels intact. Opacity uses ordinary source-over composition; the target's UNORM
format defines its representable range, and arithmetic/transport uses float32.

Derived observations `output.width`, `output.height`, `output.drawn`, and
`output.lastRefusal` are registered read-only. A refusal is reported there and on stderr
when it changes. No old compiled field is drawn after a failed binding or compilation.
`output.drawn` reports accepted GPU submission, not completion or Person acceptance.

## Authoring and persistence

These are typed `PropertyValue` fields, serialized with the existing `vector_field` /
`scalar_field` alternatives. The previous tag-only serializer dropped mathematical contents; this pass fixes that
shared serializer with the existing Field JSON payloads. Old tag-only records remain
undefined because they contain no recoverable mathematics. Ordinary sources can therefore retain the mathematics in
normal authored persistence. First Mover channels are engine-owned and excluded from
Zone Law-root storage: preserve an initialization Law that grants the bindings (or local
fields) when the Zone activates. Do not claim that setting ephemeral channel state alone
persists its authoring intent.

The probe emits `direct-screen-form-action.json`: a real serialized existing
`AddProperty` ActionModel that grants `output.color` as a typed gradient field. It is an
action recipe, not a world, an inhabited save, or an unauthored runnable Law. Use it in
an appropriately authored Law through existing Law loading/authoring. Law Line can
already carry a typed field via an authored value Lexeme; this pass does not add an
ad hoc mathematical-field literal grammar or claim a finished interactive field editor.

## Compiler and cache

The new compiler entrypoint in `SdfWgsl` shares the existing typed MathNode emitter and
pure mathematical WGSL library with the surface renderer. Screen has its own coordinate
context and preserves definedness independently of color. Numeric coefficients and
bounds become storage-buffer parameters. Generated WGSL is the exact pipeline cache
key: numeric edits reuse a pipeline, structural edits select a different one, and every
submission rereads parameters. Nothing relies on a mutable field pointer as an
invalidation proof. Driver objects and pooled buffer slices are named Kernel state.

AST traversal/lowering is currently performed each active frame; pipeline compilation
is cached, but CPU expression lowering and small GPU uploads still have optimization
work ahead. Cache resources are reclaimed on renderer shutdown, matching the existing
renderer cache lifecycle. Keep any future memoization content-sensitive and declare its
invalidation in the derived-state ledger.

Unsupported world-dependent guards, calls, folds, stochastic draws, unknown operations,
wrong result types, and unbound variables refuse. Pure `whereLEZero` region selectors
are supported. The OpenGL backend explicitly refuses direct fields; it does not fabricate
a texture or approximate the form. Do not change these refusals into silent substitutions.

## Verification

Run after building `earthcall_webgpu`:

```sh
python3 scratch/probes/direct_screen_form_probe.py
```

The script links the witness against the actual production app objects with the entrypoint
replaced, in an isolated temporary first-seed root. It exercises independent CPU and
native GPU paths, every gradient sample, a single physical pixel, interval endpoint
inclusion, opacity, field edits through real authored Laws, typed serialization, temporal
admission, source withdrawal, and explicit refusals. It separately boots the actual
Engine, applies a Law authored by its fixture Person, captures its viewport, and checks
that `Engine::tick` publishes direct-output observations. The fixture author is reported
in `engine-screen-witness.json`; no existing Person or inhabited save is modified.

Native execution passed on 2026-10-04: the 2560 × 1440 production capture matched all
3,686,400 pixels with zero byte error, and the existing shader parameter-refresh
regression passed. See [the audit and retained artifacts](../../../../../audits/DIRECT_SCREEN_FORMS_NATIVE_VERIFICATION_2026-10-04.md).
Full-suite and Person acceptance remain separate from this focused native witness.

## Remaining work

- CLI field/selector construction is implemented below; interactive editing and Person acceptance remain open.
- Authored composition, placement relative to other manifestations, and multi-Person
  visibility/authority semantics before introducing additional direct output sources.
- Law-addressable observation/elevation of direct output samples without confusing an
  ephemeral display observation with a persistent authored color value.
- Content-sensitive CPU lowering and unchanged-parameter upload elimination.
- Shared generic GPU lowering of world-dependent calls/folds, when their semantics are
  explicitly admitted, and further non-WebGPU backend support.

Person checks: [Person Verification List](../../../For%20Zach/Person%20Verification%20List.md).

## CLI field and selector authoring — 2026-10-06

Zach requested the missing Metalaw-driven sentence layer and “super cool 2D wizardry.” Nested value Lexemes now use the existing compiler seam with `slot = value`. Compiler output is a single `value`, `literal`, or `math` envelope, with ordinary authored template substitution. `$(…)` is structural quotation of existing OntoMath mathematics; coordinate words carry authored `sentence.math` data. No Screen-specific parser branch, visual class, action kind, or region enum was added.

`VectorField`, `ScalarField`, and `Piece`, fifteen general mathematical operations, Component, four transcendental-factor constructors, and nine coordinate Lexemes are seeded with their corresponding meanings. Parameterized value lowering lives in 23 enabled compiler Metalaws; constructor meanings and coordinate meanings are disabled value Laws. Two further resolver Metalaws preserve the existing `y`/yes shorthand while reading `y` as the vertical coordinate in quoted mathematics. Preview supplies structural placeholders without executing any compiler; missing/conflicting compilers, unused arguments, and statically wrong result types refuse. Quoted live property captures refuse; explicitly admitted coordinate names remain the channel's contract.

The [CLI guide](../../../../../architecture/law/LAW_AUTHORING_CLI_GUIDE.md#5a-direct-2d-screen-forms--fields-written-in-the-cli) documents the surface. [Gold Pixel](../../../../../../examples/law_line_screen_pixel.txt) selects one physical pixel. [Luminous Lens](../../../../../../examples/law_line_screen_lens.txt) composes a blue disc, gradient, cyan/gold rings, diamond, and four stars with an explicitly authored time/Flow Law. [Clear Direct Screen](../../../../../../examples/law_line_screen_clear.txt) withdraws channel output. Stop Lens Time separately; saving initialization Laws preserves the intended restart behavior.

The shared seed appends **32 Lexemes, 32 denotes Relations, and 57 Law refs** to `saves/zones/LawLine/zone.ecform`, creates only missing Law files under `saves/laws/`, and preserves existing root bytes. Zach's existing keyed identity is the recorded human author; Codex is the injector. Backup: `scratch/backups/law-line/LawLine-zone-before-law-line-patch-20261006-181244.ecform`. A second seed run leaves the native Zone bytes identical. This is an authorized vocabulary/Metalaw bootstrap; it does not install the Lens or overwrite Person-authored programs.

Verification and native capture are recorded in [the audit](../../../../../audits/LAW_LINE_DIRECT_SCREEN_AUTHORING_2026-10-06.md). Remaining: actual Person unlock/typing, appearance, resize, stopping, and save/restart acceptance; multi-source composition/authority and other backends remain outside this rung.

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-06 18:22 PDT.*
