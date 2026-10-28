# Property-only Singular and Object Screen pipeline

Codex / GPT-6.1 Sol / session `01a10a2b-a247-7c11-9d5f-7a8b89df6cfc` /
2026-10-17T16:27:11-07:00. Commissioned by Zach; native verification, not Person acceptance.

## Human requirement and parent

Continuing [Direct Screen forms](../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Direct_Screen_Forms/Direct_Screen_Forms.md#singular-bearers-and-agent-authoring-route--2026-10-17),
Zach required that “the pipeline works with a base Singular/Object with only properties
added on without using a single shapekind specification.” This pass tests that exact
authoring dependency through the production pipeline. The existing mechanism passed;
no renderer, domain class, enum, visual grammar or inhabited save was changed.

## Exact scope

Checkout `/Users/zacharyzhang/Documents/GitHub/Earthcall`, branch
`sync-from-earthcall-main`, HEAD `63f1c03c145080029b96c31d7d8814716d17d005`, with the
documentation and probe changes from this session. Rebuilt Debug `earthcall_webgpu`.
The native report records the probe SHA-256 and capture timestamp
`2026-10-17T23:26:33.866704+00:00`.

`Singular` is an abstract C++ interface. The first source is the existing concrete
non-Object `Lexeme`, identity `property-screen-singular`, used through `Singular&`.
The second is a plain `Object("property-screen-object")`, also used through that base
interface. Their fixture constructors establish identities; real Terminal sentences
grant one authored `region` dictionary containing typed color and selector fields.
Neither source joins the Zone's geometry-render list. Both are retained through the
ordinary Zone Singular store and resolved through the production Universe/provider.

The plain Object retains its existing constructor defaults. This witness makes **no
shape or material specification**, calls no shape/material setter, and does not use
that default geometry to display the field. It proves the direct channel accepts an
Object as a property bearer; it does not claim legacy Object geometry was removed.
The non-Object source separately proves that Object geometry is unnecessary.

## Executed route and checks

`Terminal → authored value/compiler Metalaws → AddProperty/Set Laws → qualified source
PropertyPath → ScreenChannel::manifestOutput → production WebGPU → completed viewport
PNG → decoded RGB comparisons`.

For each bearer, authored programs:

1. Grant a region whose signed polynomial selects two disconnected normalized bands;
   its color field supplies `(u,v,0.25)` within that support and blue elsewhere.
2. Bind `output.colorPath` to the bearer's region color. Assert the source gained only
   its one `region` Property. Evaluate 64 CPU samples against an independent interval
   definition, checking both color and observation membership.
3. Change the existing blue coefficient to `0.75` through a canonical field path.
   Request a new-token actual framebuffer observation and check its color; independently
   capture/check the complete edited viewport.
4. Replace the region's color/support and observation selector with distance mathematics
   selecting physical pixel `(7,9)`. A two-location observation selects only that pixel;
   the complete frame has exactly one red pixel over blue.
5. Withdraw the output binding and verify `output.drawn` becomes false.

All submitted source programs are retained in
[property-bearer-programs.txt](../../scratch/verification/singular-screen-properties-2026-10-17/property-bearer-programs.txt).
The harness asserts those programs contain no shape/material specification. Pixel
support belongs to the color Piece's `where`; the separate ScalarField selects sensed
pixels. The fixture deliberately authors matching mathematics, rather than relying on
an implicit clipping connection.

| Source / phase | Native dimensions | RGB pixels checked | Maximum RGB byte error |
| --- | --- | ---: | ---: |
| Non-Object Singular / disconnected bands | 2560×1440 | 3,686,400 | 0 |
| Non-Object Singular / color edit | 2560×1440 | 3,686,400 | 0 |
| Non-Object Singular / single pixel | 2560×1440 | 3,686,400 | 0 |
| Plain Object / disconnected bands | 2560×1440 | 3,686,400 | 0 |
| Plain Object / color edit | 2560×1440 | 3,686,400 | 0 |
| Plain Object / single pixel | 2560×1440 | 3,686,400 | 0 |

Native checks passed with `SINGULAR_SCREEN_PROPERTIES_RESULT PASS`. A separate
Pillow/NumPy decoder then checked all **22,118,400 RGB pixels** using integer interval
sets and direct channel-byte expectations, with zero error; both single-pixel images
contain exactly one red pixel. This script reads native PNGs and never generates
replacement images. [Native report](../../scratch/verification/singular-screen-properties-2026-10-17/result.json),
[native log](../../scratch/verification/singular-screen-properties-2026-10-17/native-witness.log),
[independent image checks](../../scratch/verification/singular-screen-properties-2026-10-17/independent-png-check.json),
[plain Object capture](../../scratch/verification/singular-screen-properties-2026-10-17/property-screen-object-bands.png).

Also passed: C++ witness syntax check, Python syntax check, and all three existing
`test_seed_law_line.py` preservation/isolation checks. These are focused witnesses;
the whole CTest suite and the earlier probe modes were not rerun.

## Reproduce and interpret failures

```sh
cmake --build build --target earthcall_webgpu -j8
python3 scratch/probes/law_line_screen_probe.py --property-bearers
# Use the bundled workspace Python with Pillow/NumPy for this independent check:
python3 scratch/probes/check_singular_screen_captures.py
```

The native run requires desktop display/GPU access. The sandboxed attempt stalled in
macOS display-service initialization and was stopped; it supplies no rendering verdict.
The first desktop attempt was rejected because the old fixture demanded a new capture
on an immediate startup tick. The new mode focuses its fixture window and waits at most
30 actual ticks for a **new existing PNG**, refusing stale captures. The final run above
passes; production rendering was not altered to accommodate the witness.

The runner explicitly sets `EARTHCALL_DATA` and SaveSystem's root to the isolated
first-seed directory recorded in the report. It uses the existing public author DID
and sanctioned presence test seam, never a human private key. The two source identities
and adopted Laws exist only in that fixture. Their grant Laws record its fixture Person
as author; they are not claims of human key-unlock or new inhabited-world authorship.
Retained images/programs/reports are verification artifacts, not installed saves.

## Boundaries and successor guidance

Begin new direct 2D authoring with [Singular-borne form](../architecture/Design/Building%202D%20and%203D%20Apps%20with%20Earthcall%20Guide.md#51-start-with-a-singular-and-its-mathematical-form).
Keep this property-bearer probe when changing path resolution, typed fields, Universe
enumeration or direct Screen lowering. Do not satisfy it by introducing a ShapeKind
carrier or by rendering an image outside the production pipeline.

One explicitly composed output remains the admitted model. Automatic multi-source
composition, multi-Person authority, generalized live projected pixel l-values and
inhabited restart acceptance remain open. The [Person list](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md#property-only-direct-screen-sources-2026-10-17)
retains visual/intent acceptance separately from these native checks.
