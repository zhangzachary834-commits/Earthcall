# Direct Screen forms: native verification

Codex / GPT-6.1 Sol / session `01a10a2b-a247-7c11-9d5f-7a8b89df6cfc` /
2026-10-04 21:20 PDT.

## Origination and implemented scope

Zach asked whether Earthcall directly admits “make my screen's pixel here display ___,”
identified the mandatory texture/Object/ShapeKind carrier under Refusals 1, 3, and 7,
and explicitly requested implementation of the direct medium. His direction is the
origin of this work. Codex extends the existing mathematical compiler and Screen
channel to manifest typed authored fields directly at framebuffer sample centres.

The production frame loop now calls `ScreenChannel::manifestOutput`. Ordinary
`AddProperty`/`Set`/`RemoveProperty` Laws grant fields or qualified bindings to fields
on any addressable Singular. No domain class, kind enum, action enum, Object face,
Material, texture, or ShapeKind is required. Pure mathematical region selectors and
piecewise intervals define where an act occurs. The shared GPU ring pool supplies
parameter/uniform slices; emitted structure keys cached pipelines.

The generic Property serializer previously discarded field mathematics, writing only
`scalar_field`/`vector_field` tags. This pass preserves the existing Field JSON payloads
and restores their typed alternatives, including typed null pointers. Old tag-only
records remain undefined; no absent mathematics is invented.

The contract and remaining authoring work are in
[Direct Screen forms](../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Direct_Screen_Forms/Direct_Screen_Forms.md).

## Executed evidence

- `cmake --build build --target earthcall_webgpu -j8`: passed with existing warnings.
- `python3 scratch/probes/direct_screen_form_probe.py`: passed with desktop GPU access.
- `DIRECT_SCREEN_FORM_RESULT PASS`: independent CPU and native GPU routes; every
  sample of a 32 × 24 gradient, numeric edits, normalized-coordinate composition and
  SDF point-substitution parity, a single selected physical pixel, interval endpoint
  inclusion, opacity composition, typed field/action JSON round-trip, actual Law
  application with a fixture Person as author, temporal admission, missing-source
  refusal, unsupported expressions, source removal, and read-only derived telemetry.
- `DIRECT_SCREEN_ENGINE_RESULT PASS`: separately initialized the actual production
  Engine in an isolated first-seed root, applied a Law authored by its fixture Person,
  ticked the live frame loop, captured the viewport, decoded its PNG, and checked every
  captured pixel independently against the authored `(u,v,.25)` expression.
- `ENGINE_SCREEN_PIXEL_RESULT pixels=3686400 maxByteError=0`: **2560 × 1440**, all
  RGBA bytes agree with mathematical expectations. This is decoded native capture,
  not a synthesized gradient or merely a success return.
- `sdf_wgsl_parameter_refresh_test`: passed, compiled and linked against current
  production app objects with the entrypoint replaced. Its existing surface,
  radiance, medium, response, and parameter-order checks guard the shared emitter.

- `channel_paths_test` and `no_black_box_test`: passed against current production
  app objects in a desktop GLFW session, covering the registered/advertised property
  surface and read-only observations.

The first sandboxed GPU attempt could not acquire a device; the successful native
runs used a desktop session. This environment failure is not counted as a rendering
verdict. The complete CTest suite was not run.

## Retained artifacts

- [Actual Engine viewport](../../scratch/verification/direct-screen-2026-10-04/engine.png)
- [Native small gradient](../../scratch/verification/direct-screen-2026-10-04/gradient.png)
- [One selected physical pixel](../../scratch/verification/direct-screen-2026-10-04/single-pixel.png)
- [Native witness log](../../scratch/verification/direct-screen-2026-10-04/native-witness.log)
- [Engine witness metadata](../../scratch/verification/direct-screen-2026-10-04/engine-screen-witness.json)
- [Independent full-frame check](../../scratch/verification/direct-screen-2026-10-04/full-frame-check.json)
- [Shader compiler regression log](../../scratch/verification/direct-screen-2026-10-04/sdf-regression.log)
- [Channel vocabulary regression](../../scratch/verification/direct-screen-2026-10-04/channel_paths_test.log)
- [No-black-box regression](../../scratch/verification/direct-screen-2026-10-04/no_black_box_test.log)
- [Serialized existing AddProperty action recipe](../../scratch/verification/direct-screen-2026-10-04/action.json)

The native fixture source was a Lexeme (`lexeme.direct-screen-field`), demonstrating
that an Object is not the admission requirement. Its author was a test Person. The
live Engine fixture's author identifier is `Person`, reported in its metadata; that
fixture is not Zach's authenticated Person. The runtime authoring Laws and temporary
first-seed files belong only to isolated test roots. No inhabited save was modified,
and the action recipe is an ActionModel fragment, not an unauthored runnable world.

## Boundaries and Person acceptance

This is one composed authored framebuffer field in the WebGPU app. It does not address
other OS applications or monitors, migrate all existing tools, establish multi-Person
visibility/authority policy, or finish direct sample elevation and interactive field
editing. `t` is explicitly bound, never borrowed from a clock. World-dependent
calls/folds/guards and unsupported mathematics refuse. The OpenGL backend explicitly
refuses this act.

First Mover channel state is ephemeral: retain an appropriately authored initialization
Law to restore bindings or local fields when a Zone activates. The native witness
proves the runtime act and typed serialization; it does not claim Person acceptance or
an inhabited Zone save/restart witness. Those checks are in the
[Person Verification List](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md).
