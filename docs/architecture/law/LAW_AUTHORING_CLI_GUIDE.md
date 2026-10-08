# Writing Earthcall Laws: a guide for humans and agents

**Current surface: 2026-10-06.** Written by Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-05 13:16 PDT.

Zach asked for a sentence authoring surface with preconfigured vocabulary, then specified general `Create` initializers, compilation by Metalaws, multiple sentences in written order, and property operations. He also tested the invisible cubes and confirmed that the corrected cube creation and original Stairmaker work. This guide turns those requests and discoveries into lessons for human authors and the agents helping them.

Your sentence becomes a **Law in the running world**. A click sentence does not immediately execute its action: it listens for a later click. Its words are authored Lexemes with meanings connected through Relations to Laws. The Terminal senses the sentence; compiler Metalaws supply the parameterized action models; the resulting Law acts through existing channels and authority gates.

## 1. Start here: make a clicked object gold

Launch **`Run Earthcall.command`** at the repository root, or run:

```bash
./scripts/build.sh webgpu run
```

Keep the Terminal beside the 3D window. Once Earthcall owns the prompt, type these into its line editor, rather than a separate shell:

```console
enter
enter LawLine
help
```

If the Terminal says you are not present, `enter Identity` and follow its key/unlock prompt, then `enter LawLine` again. Typed authoring requires your authenticated presence; previews remain open.

`enter` lists Terminal Zones. `enter LawLine` selects the Law-authoring meaning for your input. **It does not move your body or switch the visible world.** Use the app's Zone controls to choose the world you intend to work in. The prompt/footer distinguishes the Terminal's `lineZone`, the body's Zone, hearing, and the speaking author.

Submit this one line, then click a visible Object:

```text
called "Golden Touch" when clicked then Set color to gold
```

The Terminal should acknowledge an authored `law_…` identifier. Clicking supplies the event; the clicked Object is the subject, so its `color` changes. To restore blue when the pointer leaves:

```text
called "Blue Departure" when the pointer leaves then Set color to blue
```

These Laws listen to matching Objects in their active Law domain. To restrict the click to the seeded LawLine cube, use:

```text
called "Only This Cube" when clicked if Identity @law-line-cube then Set color to gold
```

That identifier is specific to the seeded example. For another Object, use its actual stable identifier from the app or completion menu.

**Before committing a sentence:** append `?` for a read-only preview, for example:

```console
called "Preview Gold" when clicked then Set color to gold ?
```

The preview does not register a Law or execute compiler Metalaws. Parameterized actions report that compilation is deferred. Previewing an event-relative condition also lacks the future click's participants; it cannot prove the Law will fire or render.

## 2. Read a sentence as four decisions

The common shape is `called "Name" when clicked if condition then action`.

| Part | What you decide |
|---|---|
| `called "Name"` | A human-readable name; optional, and not a unique identity. |
| `when clicked`, `always`, or `becomes true` | What causes evaluation/firing. |
| `if …` | What must hold; optional. |
| `then …` | The action or composed actions. |

Submitting the same name again creates another Law with another identifier. It does not edit or replace the previous Law. Inspect the Law Graph and record the identifier when keeping several versions.

### Events and continuous activation

| Phrase | Meaning |
|---|---|
| `when clicked` / `on click` | `object-clicked`; default subject is the clicked Object. |
| `when hovered` / `when the pointer enters` | `object-hover-entered`; one entry event. |
| `when the pointer leaves` / `on unhover` | `object-hover-exited`; one departure event. |
| `when touched` / `on contact` | `objects-collided`; inspect both participants. |
| `when i land` / `on landing` | `landed`. |
| `when i jump` / `on jump` | `jump-started`. |
| `when the zone opens` | `zone-entered`. |
| `always` / `constantly` / `every moment` | WhileTrue: reevaluate continuously and act while the condition holds. |
| `becomes true` / `whenever it becomes true` | OnBecomeTrue: act on entry into truth, then rearm after false. |

`becomes true` can fire on the first evaluation of an already-true condition; it is not a permanent once-only flag. Hover entry is an edge, not a per-frame “still hovered” event. Do not combine an event trigger with a continuous activation preset.

Event Laws default to `Subject`; continuous and becoming-true presets default to `Everyone`. Explicit `Subject` / `Everyone` scope clauses are available. Scope controls which beings are evaluated; a completion menu's suggested being does not change runtime event participants.

### Conditions you can write now

Use a path, comparison, and value: `if visits >= 3`, `if stair is true`, or `if not stair is true`. Numeric comparisons support `=`, `==`, `!=`, `<`, `<=`, `>`, `>=` and authored phrases such as `is greater than`, `at least`, `less than`, and `at most`.

Combine conditions with `not`, `and`, and `or`, in that precedence order. **Condition grouping parentheses are not implemented in this text surface.** Arithmetic parentheses inside an expression are supported.

Useful structural conditions include `Identity @law-line-cube`, `is a Person`, `is an Object`, `path near <value> within <number>`, `path between <low> and <high>`, `Related "type" to @being`, and `Overlaps @being`. The angle placeholders here are explanatory, not literal input. Canonical graph conditions `InRegion`, `Zone`, `ForAny`, and `ForAll` currently refuse their CLI text forms; author those through their existing graph/model surfaces.

## 3. Know whose property you mean

| Path | Meaning at firing time |
|---|---|
| `color`, `position.y`, `note` | A property of the current Law subject. |
| `@law-line-cube.position` | A property of that explicitly identified being. |
| `@event.subject.position` | Position of the event's subject, such as the clicked Object. |
| `@event.object` | The event's second participant, when present. |
| `my.position` | The speaking author's current feet position. |
| `my.cameraPos`, `my.cameraForward` | The author's view origin and forward vector. |

The author and the event subject are different roles. `my.position` follows the author; `@event.subject.position` follows the clicked thing. Neither freezes a coordinate when you submit the sentence.

Named roots use `@` and the longest matching dotted identifier. Use actual stable identifiers; do not guess an identifier from a displayed name. **`Identity @Zach` does not match every Person named Zach:** a keyed Person can have a `did:earthcall:…` identifier. Unresolved or ambiguous addressing must be resolved rather than silently selecting someone.

Event facts such as `@event.verb`, `@event.start`, and `@event.occurrenceId` are readable; Event properties are read-only. Existing authority and body consent gates still apply to writes. Authorship alone does not grant a Law permission to move a Person.

### Values and expressions

Values include numbers, `true`/`false`, quoted strings, and authored words such as `gold`, `blue`, `cyan`, and `purple`. Use quotes around text, especially text containing spaces or semicolons. A vector is `(1, 2, 3)`; `[1, 2, 3]` is a list. A dictionary is `{note: "hello", visits: 0}`. Literal container entries remain literals; dynamic container construction is not implied.

Parameterized expressions support qualified reads and numeric/vector `+`, `-`, `*`, `/`, with parentheses. Put spaces around arithmetic on paths: hyphens also occur in identifiers. This is the typed OntoMath expression surface, not arbitrary Python, JavaScript, interpolation, or a general function-call language.

Use `Map` to copy or calculate from another property:

```text
called "Copy Cube Colour" when clicked then Map <path: "color", expression: @law-line-cube.color>
```

`Set color to @other.color` (and arithmetic such as `set glow to @lamp.hp * 2`) copies the other path's value each time the Law fires. The authored Metalaw `law-line-compile-assignment-expression` lowers it to the same `Map` passthrough, and without that Metalaw it refuses. It is a copy, not a live alias (2026-10-05). `Set color to 1 0 0` is a supported legacy vector form; inside argument records, prefer `(1, 0, 0)`.

## 4. Add, change, and remove authored properties

Create your own state, then use it in later Laws. These three sentences can be submitted together:

```text
called "Add Note" when clicked then add property note to "hello"; called "Change Note" when hovered then modify property note to "updated"; called "Remove Note" when the pointer leaves then remove property note
```

Click an Object to add `note`, hover it to change the value, and leave it to remove the value. Inspect properties to see these changes; `note` is state, not an automatically drawn label. Likewise, a property named `glow` does not automatically produce light.

| Operation | Existing action | Important behavior |
|---|---|---|
| `add property note to "hello"` | AddProperty | Grants authored state; can overwrite existing authored state, but cannot shadow a registered engine property. |
| `modify property note to "updated"` | Set | Writes an addressable path; a never-addressable missing path refuses. |
| `remove property note` | RemoveProperty | Erases authored value; does not deregister engine paths or bypass their clear/write gates. |
| `Add visits by 1` | Add | Numeric arithmetic on existing state; initialize it first. |

An authored accessor can remain addressable after its value is removed, so Set can restore that value. Do not infer a universal “removed means forever unwritable” rule.

For ordered initialization and arithmetic within a single click:

```text
called "Count To One" when clicked then Sequence <children: [add property visits to 0, Add visits by 1]>
```

This deliberately resets to zero before adding one on every click. For a cumulative counter, initialize once through a separate setup Law, then keep only the `Add` action in the counting Law.

## 5. Create an Object with its own state

Zach's requested notation is `Create <SingularKind, properties: {registered initializers, authored: {new properties}}>`.

This click Law creates a cube whose centre is exactly three world units below the author:

```text
called "Cube Beneath Me" when clicked then Create <Object, properties: {shape.kind: Cube, position: my.position + (0, -3, 0), color: gold, authored: {purpose: "A foothold", visits: 0}}>
```

`shape.kind`, `position`, and `color` are existing property paths. `purpose` and `visits` are newly authored state. The equivalent `registered: {…}` group is available inside `properties`. Registered literal initializers compile to Set; expressions compile to Map. Authored literals compile to AddProperty; authored expressions compile to AddProperty followed by Map.

Inside Create, bare initializer paths refer to the **newborn**. Qualified `my.…` and `@event.subject.…` reads keep their source meaning. Initializer actions retain ordinary runtime refusals; successful compilation does not guarantee every initializer writes successfully. Inspect the action trace if a created Object has unexpected state.

### Make a visible probe before diagnosing below-ground placement

Aim roughly level in first-person view and submit:

```text
called "Visible Probe" becomes true if is a Person then Create <Object, properties: {shape.kind: Cube, position: my.cameraPos + my.cameraForward * 3, color: gold, authored: {visibilityProbe: true}}>
```

In the one-Person lesson, this creates a cube three units along the author's view. The first evaluation fires; a second still-true evaluation does not create another cube. This makes a useful visibility check without continuously multiplying Objects.

The continuously firing version is:

```text
called "Cubes Below Me" always if is a Person then Create <Object, properties: {shape.kind: Cube, position: my.position + (0, -3, 0), color: gold}>
```

**This keeps creating cubes while enabled.** Disable/delete it when finished. Standing still causes overlapping cubes, not a growing visible pile. Feet position minus three can be underneath an opaque floor. In a multi-Person domain, `is a Person` matches each Person while `my` still means the author; this lesson is not an exact author-identity filter.

### Creation boundaries

`Object` has the configured direct-birth rule. `Create <@prototype, properties: {…}>` can derive from a live, resolvable prototype through the existing concrete-kind adapter. An arbitrary kind word does not conjure a constructor. Person birth is not available through an ad hoc Object/clone sentence; Persons correspond to actual humans registered through identity channels. Relation creation still requires participants; this initializer syntax does not supply universal participant slots.

`Cube` is an authored word denoting the existing geometry compatibility value. It does not make shape enums the ontology. In particular, `shape.r`, `shape.ry`, and `shape.rz` are not generic cube resizing controls. The sky example below uses an existing ellipsoid manifestation; authored form can also use the broader OntoMath surfaces described in the [2D/3D app guide](../Design/Building%202D%20and%203D%20Apps%20with%20Earthcall%20Guide.md).

## 5a. Direct 2D Screen forms — fields written in the CLI

**Added at Zach's request, 2026-10-06.** Rebuild/restart WebGPU to load the generic value constructor channel. The seed appends its Lexemes and compiler Laws to the native LawLine Zone while keeping your existing beings and Laws. Unlock your identity, then `enter LawLine`.

Start with a full-frame gradient:

```text
called "Screen Gradient" becomes true if is a Person then add property @screen-channel.output.color to VectorField <pieces: [Piece <value: $((u, v, 0.25))>]>
```

This displays colour directly in Earthcall's physical framebuffer. The authored field does not need an Object, Material, FaceTexture, or ShapeKind. A previously configured `output.colorPath` takes precedence over local colour; use the clear recipe below before this small lesson if another output binding is active.

`$(…)` **quotes mathematics for later sampling**. The Terminal retains the existing typed OntoMath tree; Screen binds its coordinates at each physical pixel. This notation belongs in Earthcall's line editor. Without quotation, a mathematical expression is evaluated as an ordinary initializer instead.

| Word / notation | Meaning |
|---|---|
| `VectorField <pieces: […]>` | A typed vector-valued field; Screen colour expects a vector. |
| `ScalarField <pieces: […]>` | A typed scalar-valued field; Screen opacity expects a scalar. |
| `Piece <value: $(…), where: $(…)>` | Value applies where the signed selector is ≤ 0; omitted `where` admits the whole domain. |
| `x`, `y`, `p` | Physical pixel centre coordinates; origin top-left, `p = (x, y, 0)`. |
| `u`, `v` | Normalized physical coordinates, `x / width`, `y / height`. |
| `width`, `height` | Current framebuffer dimensions; formulas adapt to resizing. |
| `t` | Only the explicitly authored time value/binding; no implicit clock. |

The existing `y`/yes shorthand and the mathematical `y` coordinate are separate authored meanings. Resolver Metalaws choose by the quotation context. Preview may report that this meaning stays open until Enter; it executes no resolver or compiler.

The first applicable Piece wins. Put fine details before a full-screen background. Undefined regions leave the existing scene intact. Select physical pixel column 7, row 9 by quoting `Distance <a: $(p), b: $((7.5, 9.5, 0))> - 0.1`; the half-unit offset selects that pixel centre. [Gold Pixel: the complete one-line program](../../../examples/law_line_screen_pixel.txt).

Mathematical constructor words are ordinary authored Lexemes: `Distance <a: $(…), b: $(…)>`, `Abs <value: $(…)>`, `Clamp <value: $(…), low: $(…), high: $(…)>`, plus Dot, Cross, Hadamard, Normalize, Length, Project, Union, Intersection, Difference, Pow, Sqrt, Tan, and Component. `Component` takes `value: $(…)` and an `index` string naming an existing axis (e.g. `"x"`). Sin/Cos/Exp/Ln use the existing ScalarForm transcendental factors: `Sin <variable: "t", scale: 2, shift: 0>`; scale/shift are optional. This form applies to a named coordinate, not an arbitrary function-call argument. Check current completion signatures.

**Try [Luminous Lens](../../../examples/law_line_screen_lens.txt):** paste its single line once. It creates a dark blue gradient, a blue inner disc, cyan and pulsing gold rings, four golden points, and a luminous diamond at the centre. Its second Law uses Flow to advance the explicitly installed `output.time`. This is one composed field; it does not establish multiple independent display owners or an OS-wide screen controller.

To restore the 3D scene, first disable/delete `Lens Time` if you installed it, then paste [Clear Direct Screen](../../../examples/law_line_screen_clear.txt). That Law removes the direct colour, opacity, and time bindings/values. Disable/delete previous setup Laws if you do not want them to reapply when you reenter/restart. Save Zone retains your setup Laws, not the engine-owned channel's transient state; the setup Law must run again on activation.

The new compiler slot is `value`. Metalaws return exactly one `value` (serialized PropertyValue), `literal` (structural record), or `math` (existing MathNode) envelope. Typed fields, Piece selectors, and constructor lowering are authored templates. Coordinate words carry `sentence.math`; their meanings are not a parser switch. Wrong known field result types, unused/duplicate arguments, missing compilers, and conflicting compiler outputs refuse. Quoted live PropertyPath captures refuse; Screen samples explicitly admitted coordinates. Backend-specific unsupported mathematics and unbound time still refuse at manifestation.

Focused verification: four CLI suites passed (the LawLine fixture has 220 checks). Native WebGPU captures of the gradient and both Lens times each matched all 3,686,400 pixels with zero byte error; the selected physical gold pixel also matched exactly. See [the audit and captures](../../audits/LAW_LINE_DIRECT_SCREEN_AUTHORING_2026-10-06.md). Real Person unlock/typing, resize, and save/restart acceptance remain open.

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-06 18:22 PDT — Zach requested direct 2D CLI wizardry; the field/value seam and Luminous Lens program implement that direction.*

## 5b. A named displayed region, its source, and its sensed pixels

Zach asked for the unfinished direct Screen addressing layer to be authorable through Metalaws. `ScreenRegion`, `ScreenSample`, and `ScreenSampleAtTime` are now authored Lexemes denoting ordinary value Laws, with three separate compiler Metalaws. Their lowering is data in the LawLine seed; the parser has no switch for these names.

A `ScreenRegion` supplies an ordinary dictionary with typed `color` and `selector` predicates. It can belong to an addressable Singular, including your Person. It is neither a new C++ class nor a new kind. Its colour field is the explicit edit destination; its selector independently says which displayed samples you want to observe. Colour is displayed by binding `@screen-channel.output.colorPath` to `"@bearer.region.color"`. The colour field's Piece guards determine its drawn support; naming an observation selector does not silently clip or rewrite that source.

After clearing earlier direct output, paste [My Named Region](../../../examples/law_line_screen_region.txt). Its first sentence makes a gold disc of radius 12.25 physical pixels centred at `(120.5,120.5)`, binds its source and requests a readback. Its second sentence waits for that request to succeed, grants `my.haloReading`, then uses the existing authored Set/Map compiler to carry the observation onto your Person. The region source in this example is on Screen, an existing Singular; the observation becomes an aspect of your Person through Law. To make the source Person-owned too, grant it as `my.halo` and use your actual stable qualified identifier in the quoted output and sample binding strings. Quoted `"my.halo"` is text and does not expand the CLI's `my` path alias.

[Recolour My Region](../../../examples/law_line_screen_region_edit.txt) writes the three existing RGB coefficients, makes the disc cyan, and requests another observation with a new token. Granular field paths expose the existing serialized mathematics:

```text
called "Red Coefficient" becomes true if is a Person then set @screen-channel.halo.color.astDefinition.pieces.0.mathNode.children.0.scalarForm.terms.0.c to 0.5
```

These are canonical field edits, validated and committed through the root's setter/storage. Tiny coefficient changes are preserved exactly. Invalid operations, dropped JSON members, wrong known result types, nonfinite values and absent indices refuse; derived getters remain read-only. A field edit replaces that bearer slot's typed value, without mutating other slots holding the old shared field pointer. Whole field replacement remains available through `VectorField`/`ScalarField`. Containers retain their existing explicit shared-binding semantics.

| Authored request / derived observation | Contract |
|---|---|
| `sample.request` | A `ScreenSample <region: "@bearer.region", x: 108, y: 108, width: 25, height: 25, limit: 625, token: "gold-halo">` record on Screen. |
| `region.selector` | AST ScalarField: defined finite values ≤ 0 select; positive/undefined values exclude. No implicit world guards, calls or folds. |
| `x`, `y`, `width`, `height` | Explicit integer rectangle in physical pixels; top-left origin, entirely inside the current framebuffer. Membership is evaluated at pixel centres. |
| `limit` | Author's maximum selected samples; exceeding it refuses with an empty list, never a partial success. Scan ceiling is 1,048,576 locations; count ceiling is 65,536 samples; readback ceiling is 256 MiB. |
| `token` | Nonempty explicit request identity. Change it to recapture; both success and refusal consume it. Changing source, selector or rectangle with the same token does not reinterpret an old observation. |
| `ScreenSampleAtTime` | Same arguments plus an explicit finite `time`; use when the selector reads `t`. Ordinary ScreenSample leaves `t` unbound. |
| `sample.result` | Registered read-only snapshot: `ok`, `refusal`, `frame`, `width`, `height`, `stage`, `token`, `region`, `count`, `samples`; success also records selector mathematics and admitted time. |
| `sample.result.samples.0.color` | Actual completed-viewport RGBA8 RGB bytes normalized to [0,1], with integer `x`/`y` and separate `alpha` on each row-major sample. Not a fresh evaluation of authored colour. |
| `sample.frame`, `sample.lastToken`, `sample.scanCeiling`, `sample.countCeiling`, `sample.readbackByteCeiling` | Read-only channel observations/bounds. |

Sampling happens after Engine's scene, direct output and compatibility HUD/2D draws, before the separate Dear ImGui overlay and presentation. The observation identifies that completed viewport stage, not an OS monitor or another app. It is a historical snapshot, not a continuously live region cache. Membership is recomputed for every request. Reads do not repaint; edits target the named source field, because composited pixels have no unique inverse to authored sources. `haloReading` is an ordinary Law-carried predicate; its editable copy cannot mutate the channel's protected `sample.result` through shared-container aliasing. The observation preserves the selector, frame and dimensions used to obtain it. Membership is evaluated by CPU mathematics; the displayed mask uses WebGPU float32 mathematics, which can differ at exactly-zero boundaries. Sampling always reports the actual displayed bytes, including background at such a boundary; it does not substitute an expected source colour.

Remove `@screen-channel.sample.request` to stop requests; clear output using the earlier recipe. Save your initialization/derivation Laws, not ephemeral channel results. Interactive selection, inverse painting, multiple output owners and multi-Person composition still need their own specified semantics. [Contract and verification](../../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Direct_Screen_Forms/Direct_Screen_Forms.md#named-regions-and-completed-viewport-observations--2026-10-07).

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-07 18:54 PDT — Zach originated the named-region direction; Codex implements explicit source editing, bounded sensing and the authored derivation recipe.*

### A whole pixel-art editor from one paste

[Atelier](../../../examples/law_line_pixel_art_editor.txt) authors a 16×16 direct Screen canvas, twelve ink swatches, eraser, one-step undo/redo, undoable clear, viewport PNG export and close. It is **one line containing 276 cooperating Law sentences**, compiled by the existing authored Metalaws. Paste it once after rebuilding/restarting, unlocking, `enter LawLine`, closing pointer-capturing panels and unlocking the cursor with Escape. Press Enter after pasting and allow the substantial program to finish compiling. Earlier display/world-click programs should be stopped if you do not want them running alongside it.

Click a left swatch, then click/drag over the canvas. Right-hand tiles are undo, redo, clear, PNG, eraser, close from top to bottom. Artwork lives in your authored `atelier.*` properties; the generator installs no save or C++ app. The initialization marker preserves existing installed state. [Complete controls, limits and verification](../../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Law_Line_Pixel_Art_Editor/Law_Line_Pixel_Art_Editor.md).

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18`; Zach requested the whole editor through Law Line.*

## 6. Several Laws on one line, or several actions in one Law

Use a top-level **semicolon** between complete Law sentences:

```text
called "Hover Gold" when hovered then Set color to gold; called "Leave Blue" when the pointer leaves then Set color to blue
```

The authored `law-line-compile-sentences` Metalaw compiles the batch, and Laws are registered exactly once in the written order. Quoted semicolons and separators inside nested records/lists/parentheses are preserved. Every sentence is parsed before registration begins. A later adoption refusal can still leave earlier Laws adopted: this is not an atomic transaction.

**Registration order does not promise general event firing order.** To guarantee one action follows another in the same Law, use Sequence:

```text
called "Ordered Paint" when clicked then Sequence <children: [Set color to blue, Set color to gold]>
```

Its final colour is gold. `then Set color to blue and Set color to gold` is also sequential composition. Parallel and Synthesize are existing action models; their names are not promises of CPU threads, atomicity, or speed.

Do not mix immediate commands such as `delete`, `enter`, or vocabulary search into a Law batch.

## 7. Grow Stairmaker, then its sky spiral

Paste the single line from [the original Stairmaker](../../../examples/law_line_stairway.txt). It contains four Laws:

1. Click an Object without `stair: true`: create three blue cube steps at offsets `(0, 1, 0)`, `(1, 2, 0)`, and `(2, 3, 0)` from the clicked Object; mark each `stair: true`.
2. Enter a step with the pointer: paint it gold.
3. Leave a step: paint it blue.
4. Click a step: raise it by 0.5 units.

The reusable idea is visible in this smaller lesson:

```text
called "One Step" when clicked if not stair is true then Create <Object, properties: {shape.kind: Cube, position: @event.subject.position + (0, 1, 0), color: blue, authored: {stair: true}}>
```

The condition prevents the newborn step from triggering this particular growth rule. The property is authored meaning shared by several Laws; it is not a hardcoded Stair class.

Keep the original program enabled and paste [the sky spiral add-on](../../../examples/law_line_sky_stairway.txt) once. Clicking an existing step grows eight broad cyan/violet jewel steps winding upward to a gold crown. Each new jewel can grow another spiral once. Hover turns a jewel by 25 degrees; departure restores its stored colour and orientation. The original click-to-rise action remains active.

The add-on stores `skyGrown`, `skyStep`, and `skyColor` as authored properties. It uses an existing ellipsoid geometry value (`shape.kind: 5`) with authored radii; that number is compatibility data, not a request to add another geometry enum. Its full one-line source is maintained in the example file so this guide does not carry a second divergent copy.

Zach confirmed the original Stairmaker after the active-Zone creation fix. The enhanced spiral has focused and native Engine evidence; its appearance and feel in Zach's scene remain a separate Person check.

## 8. Action reference

An action fragment belongs after `then`, or inside a composed action's `children`. The tables are **reference signatures**: replace `‹…›` placeholders with real paths, values, or actions. They are not complete pasteable Laws.

### Direct sentence forms

| Kind | Fragment | What it does |
|---|---|---|
| Set | `Set color to gold` | Writes a value. |
| Add | `Add position.y by 0.5` | Adds a numeric amount. |
| Scale | `Scale visits by 2` | Multiplies numeric state. |
| Spawn | `Spawn @‹concept identifier›` | Uses an existing ObjectConcept; not a generic Singular birth rule. |
| Publish | `Publish "demo-event" about @‹being›` | Publishes an event; `about` is optional and otherwise uses the current subject. |
| Create | `Create <Object, properties: {color: gold}>` | Derives a newborn using configured birth semantics. |
| AddProperty | `add property note to "hello"` | Authors state. |
| RemoveProperty | `remove property note` | Removes/clears state through the existing action. |
| Destroy | `Destroy @‹being›` | Requests destruction when the Law fires, subject to existing limits. |
| AddRelation | `AddRelation @‹source› "‹type›" @‹target›` | Uses the existing Relation action. |

### Authored named-argument forms

These sixteen signatures are authored `sentence.arguments` data with corresponding compiler Metalaws, rather than sixteen new CLI implementation branches.

| Kind | Fragment |
|---|---|
| Lerp | `Lerp <path: "‹path›", operand: ‹value›, factor: ‹number›>` |
| Drive | `Drive <path: "‹path›", curve: {form: 1, coeffs: [0, 1]}, input: "‹path›">` |
| Sequence | `Sequence <children: [‹action›, ‹action›]>` |
| Parallel | `Parallel <children: [‹action›, ‹action›]>` |
| Map | `Map <path: "‹path›", expression: ‹expression›>` |
| Flow | `Flow <path: "‹path›", expression: ‹rate expression›>` |
| AddElement | `AddElement <container: "‹being›", element: "‹being›">` |
| RemoveElement | `RemoveElement <container: "‹being›", element: "‹being›">` |
| Synthesize | `Synthesize <children: [‹action›, ‹action›]>` |
| PlayAudio | `PlayAudio <frequencyPath: "‹path›", amplitudePath: "‹path›", timbre: "sine">` |
| AuthorZone | `AuthorZone <identifier: "‹id›", zoneKind: "‹authored kind›", owner: "‹being›", ownerKind: "‹kind›">` |
| WritePixel | `WritePixel <facePath: "‹path›", uPath: "‹path›", vPath: "‹path›", colorPath: "‹path›">` |
| ElevatePixels | `ElevatePixels <name: "‹property›", facePath: "‹path›", selector: {‹existing Piecewise model›}>` |
| FileRead | `FileRead <input: "‹file path property›", path: "‹destination property›">` |
| FileWrite | `FileWrite <path: "‹file path property›", input: "‹content property›">` |
| CodecTransform | `CodecTransform <operation: "‹codec operation›", input: "‹source property›", path: "‹destination property›">` |

`Drive.input` is optional; its curve uses the existing model (form 0 constant, 1 polynomial, 2 sinusoid). `container` defaults to the subject when omitted. `PlayAudio.timbre` defaults to `sine`. `AuthorZone.owner` and `ownerKind` are optional. Codec operations include `jsonCompact`, `jsonPretty`, and `base64`.

`Flow` integrates a rate using world delta time; `Map` writes the expression value. Neither sentence spelling supplies an implicit animation schedule. Choose activation, scope, and bounds deliberately. Reactive continuous hover candidate propagation remains open; use the verified entry/exit gestures for the sky lesson rather than presenting a proposed hover-Flow as proven animation.

Path-taking channel actions require the actual registered source/destination properties and live channel context. A literal file name does not substitute for a “file path property” parameter. Parsing audio, file, pixel, membership, or Zone operations does not prove their execution or permission; inspect their runtime outcome and existing TransferPolicy gates.

### Publish and listen to your own event

An explicitly quoted event name can be authored without assuming an engine publisher:

```text
called "Announce Click" when clicked then Publish "demo-click"; called "Respond To Demo" on "demo-click" then Set color to gold
```

The first Law supplies the event the second listens for. An arbitrary `on tick` sentence does not establish that the running Engine publishes a `tick` event. Prefer `always` for the continuous lesson.

## 9. Editor, deletion, and persistence

| Input | Purpose |
|---|---|
| `help`, `?` alone, or F1 | Show current help. |
| `?? word` | Search the live authored vocabulary. |
| Trailing `?` | Preview syntax without registration or compiler execution. |
| Tab | Accept a completion/advance a blank; opens choices when needed. |
| ↑ / ↓, menu wheel, PgUp / PgDn | Choose/page completion entries; closed-menu arrows walk history. |
| → / End | Accept ghost text. |
| Click a completion row | Select an offered entry. |
| Esc | Close the menu; another Esc clears the line. |
| Ctrl-R | Search history. |
| Ctrl-W / Ctrl-U / Ctrl-K | Delete word / to start / to end. |
| Ctrl-C | Clear; twice quits. |

To remove the continuous cube example, type:

```console
delete "Cubes Below Me"
```

Read the selected Law and answer `yes` to confirm, or `no` to keep it. Use the exact Law identifier when names repeat. Immediate `delete` has a confirmation flow; a `Destroy` action in a Law is different and acts on later firing.

Submitted Laws are adopted into the active runtime Zone. **Use the app's Save Zone control to retain Laws and world changes after restart**, including deletion. A Terminal acknowledgment alone is not a persistence witness. Verify after restarting the same Zone. History is in `saves/logs/terminal-history.txt`; runtime Terminal output is in `saves/logs/earthcall-terminal.log`. History is a convenience, not the authoritative world save.

## 10. When it accepts the sentence but nothing happens

| Symptom | Check next |
|---|---|
| “Authored” but no colour change | Did the matching event occur after submission? Is the Law enabled in the active Zone? Inspect trigger, condition, subject, and action outcome. |
| `Identity @Zach` never matches | Resolve the actual keyed identifier; a display name is not identity. |
| `my` places it somewhere unexpected | Check the speaking author; use event-relative paths when the clicked Object is the intended origin. |
| Count rises but no cube is visible | Check newborn position and owning/rendered Zone; then view direction, floor occlusion, overlapping births, and material/form. Assets are not viewport proof. |
| Below-me cube is hidden | The author's position is at the feet; minus three can be underground. Try the view-relative probe. |
| Both below-me and the visible probe are absent | Rebuild/restart the corrected WebGPU app; older builds could route births into inactive `World`. Do not change the intended placement to conceal routing failure. |
| Missing/conflicting compiler refusal | Inspect enabled `law-line-compile-*` Laws and their authored templates; do not bypass them with a bespoke fallback. |
| Unknown word/path or missing argument | Search with `??`, use completion, and read the precise refusal; vocabulary is contextual authored data. |
| `glow` changes but no light appears | A numeric property does not by itself connect to visual form. Inspect the Laws that manifest it. |
| Works until restart | Save the intended Zone, then verify reload. |

Diagnosis has several separate witnesses: syntax accepted → compiler selected → Law adopted → event/condition matched → action succeeded → state changed → effect manifested → saved/reloaded. Inspect the first missing witness rather than assuming the last one from the first.

## 11. For agents: author with the human, extend the shared mechanism

Start with the Person's requested meaning: origin, trigger, scope, state, effect, stopping condition, and what they should visibly witness. Give them a real sentence using vocabulary present in their Zone. Explain the gesture that fires it and how to stop a continuous example. Do not invent a convenient alias and present it as installed.

### Source map

| Resource | What to inspect |
|---|---|
| [AGENTS.md](../../../AGENTS.md), [build guide](../../BUILD_AND_ENVIRONMENT.md), [engineering discipline](../../ENGINEERING_DISCIPLINE.md) | Repository constraints before searching, building, or implementing. |
| [Law Line task](../../Agenda/Tasks/Specific%20Tasks/Law%20and%20Reasoning/Law_Line/Law_Line.md) | Implementation history, exact limits, pending rungs, and future-agent directions. |
| [Terminal sources](../../../src/Singularity/Terminal/) | LawSentence structural parsing, TerminalChannel compilation/adoption, LineEditor interaction. |
| [seed_law_line.py](../../../scripts/seed_law_line.py) | Authored Lexemes, denotes Relations, presets, signatures, and compiler templates. |
| [AuthorsOfLaw](../../../src/ZonesOfEarth/AuthorsOfLaw/) | Shared Law and ActionModel execution; follow declarations to actual implementations. |
| [law_line_zone_test.cpp](../../../tests/singularity/law_line_zone_test.cpp) | Isolated real Terminal/Metalaw/tick checks, keyed identity, batches, creation, and pointer programs. |
| [native probe](../../../scratch/probes/law_line_visibility_probe.py) | Existing isolated production Engine viewport witness, including `--stairway`. |

The compiler seam is `compilation.input` → authored Metalaw → `compilation.template` / `compilation.error` → structural substitution and existing ActionModel validation. Create lowering and named arguments belong in those authored templates. Missing/conflicting outputs refuse. Preview and completion must not execute compiler Laws. Some legacy sentence forms still use the older parser; this is not a claim that every opcode has already migrated to Metalaw.

For new domain meaning, author state, Lexemes, Relations, and Laws. Do not introduce a Stair class, `spawnBelowMe` verb, new enum kind, hidden field, or second permission system. If an actual substrate invariant is missing, follow the Seven Refusals and router before proposing implementation. Preserve the distinction between a human Person and an agent acting as a granted First Mover.

For foreign/MCP authoring, use the existing authorized mutation surface through ForeignActuationGuard. A foreign sentence's `my` resolves to its own actual author, not borrowed local Person identity. The foreign `law_sentence` surface currently refuses batches: one authorized identifier per submission. Do not substitute direct save edits for a refused foreign mutation. Never expose identity keys or secrets in examples or logs.

When implementation is needed, build the WebGPU app with the repository flags and run focused checks appropriate to the seam. For documentation examples, submit through the real Terminal/compiler path in an isolated fixture. An action-model deserialization test is not proof of audio, file, rendering, or Person consent. Native visibility needs a real Engine capture; readability and feel require the Person. Record remaining checks in [Person Verification](../../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md).

Existing inhabited saves must be patched by stable identity, preserving bytes and relationships, with owner authorization and the [First Mover authoring rules](FIRST_MOVER_AUTHORING.md). State exactly which file/beings were authored, who the recorded human author is, and who injected them. This guide creates documentation only; it does not modify a world save or register a Law on the reader's behalf.

## Evidence and remaining boundaries

The guide describes the inspected 2026-10-05 source and authored vocabulary. The original cube/Stairmaker behavior has Zach's live confirmation. The [creation routing audit](../../audits/LAW_CREATE_ACTIVE_ZONE_ROUTING_FIX_2026-10-05.md) and [sky spiral audit](../../audits/SKY_STAIRWAY_AUTHORED_PROGRAM_2026-10-05.md) retain focused and native evidence; they do not establish every advanced action in every world. Full Timeline authoring, the unimplemented condition text forms, reactive hover candidate propagation, and wider creation semantics remain tracked in the [Law Line task](../../Agenda/Tasks/Specific%20Tasks/Law%20and%20Reasoning/Law_Line/Law_Line.md).

**Guide example verification (2026-10-05):** all 13 `text`-fenced lesson lines were submitted through the production Terminal/Metalaw path in a scratch copy of the isolated LawLine harness. They adopted the expected 17 Laws; the trailing-question preview adopted none. The existing 162 checks plus these 14 checks passed **176/176**. This verifies submission and preserves the existing functional witnesses; it does not add a new native capture or prove execution of every placeholder action in the reference table. [Recorded result](../../../scratch/verification/law-authoring-guide-2026-10-05/result.json). First-reader and exercise acceptance remain in Person Verification.
