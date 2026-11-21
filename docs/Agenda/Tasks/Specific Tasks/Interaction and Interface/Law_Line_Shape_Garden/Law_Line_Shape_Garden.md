# Shape Garden — a Law Line-authored 2D sidebar and 3D generator

Zach requested a Zone with a little 2D shape-generator sidebar, composed of Singulars and Laws, with no legacy 3D-generation logic. He directed authoring in the default checkout and testing on Claude's ray-transport branch with Intercom coordination.

## Use it

The Zone is installed in the app data home as **ShapeGarden**, displayed as **Shape Garden**. Unlock the existing Zach identity if activation asks for it. Select the Zone in the app's Zones controls; a Terminal `enter` command moves the line, not the embodied view/active rendered Zone. Press **Escape** to release a locked cursor, and close foreign panels that capture the pointer.

- Choose **SPHERE**, **BOX**, or **TORUS**. A gold mark identifies the selection.
- **− / +** choose one of three sizes; the bars show the current choice.
- Pick a color. Its underline becomes gold.
- **CREATE** creates one mathematical solid in front of the Person. Holding does not repeatedly create. The first three slots fit the viewport; later creations advance into rows.
- **X** closes the direct sidebar while keeping the generated work. Close it before leaving for another Zone: general direct-output ownership/lifecycle composition is not implemented by this small program.
- **Save Zone** keeps generated objects and authored controller state through the existing app storage office.

To reopen after X, with the body still in Shape Garden and the Terminal in LawLine, author this one-shot sentence:

```text
called "Reopen Shape Garden" becomes true if is a Person then set @ShapeGarden.forge.enabled to true
```

The preinstalled Zone already carries the application. Do not paste the installation program into it again: ordinary sentence submission creates additional Laws. The installer is conservative: unchanged JSON roots are a no-op; existing different Laws, semantic roots, or independently authored Zones refuse replacement rather than erase work.

## Authored program

[Readable generator](../../../../../../scripts/author_law_line_shape_garden.py), [one-line Law Line source](../../../../../../examples/law_line_shape_garden.txt), [individual sentences](../../../../../../examples/law_line_shape_garden_sentences.json), [compiled first-seed bundle](../../../../../../examples/law_line_shape_garden_bundle/manifest.json), and [preserving installer](../../../../../../scripts/install_law_line_shape_garden.py).

There are **28 application Laws**, stable IDs `shape-garden-law-0` through `shape-garden-law-27`, compiled by the actual `TerminalChannel::authorForeign` grammar and authored compiler Metalaws. The Zone bears `forge.*` state, a typed color field and an opacity field. Typography, controls, selection marks and size bars are mathematical Pieces; no Text2D rectangle/widget carrier is added. Interaction senses the press; Laws capture its origin and consume a ready latch. Palette, dimensions, form choice, placement, closing and reopening remain authored behavior.

Sphere, box and torus geometry are scalar distance expressions assigned through universal Create's ordinary child property actions (`field.expr`, `field.extent`, `position`, `rotation`, `color`). Size scales the mathematical coordinates and returned distance. No Creator Console action, `Tool::ShapeGenerator3D`, `shape-generator-3d-law`, `creation-channel`, or shape-enumeration selector decides the geometry. Generic Object birth, implicit-math compilation and rendering remain existing substrate mechanisms. There is **no production C++ change**; the C++ addition is a verification/export harness only.

The Zone container is a first-seed record through the existing storage office. It is not claimed to have been born through a new Zone Create lexeme: universal Zone set-to-set birth explicitly remains unwired. All application behavior and the runtime floor/solids are authored through Law Line. The exported installation is unexecuted Law data, with no fixture object IDs or generated samples copied into the Person's world.

## Evidence and installation

The [audit](../../../../../audits/LAW_LINE_SHAPE_GARDEN_2026-10-20.md) records **87 logic/codec checks** and **15 native checks**, including real **2560×1440** viewport captures. Logic checks cover real mathematical geometry, color/size, press/hold behavior, foreign capture, save/reload and close/reopen retention. Repeat installation preserves all 29 files byte-for-byte after a later authored-state edit in an isolated fixture.

Testing used a clean pinned `claude/ray-domain-transport-rung0` snapshot at `d7f8aa31f061e2e5e7c18ff6539edb0ce5c976f9`; authoring stayed on `sync-from-earthcall-main`. The default Intercom pre-test notice and a mirrored report in the actual ray worktree preserve coordination. No ray source or active agent build cache was modified.

Installation added `zones/ShapeGarden/zone.json` and 28 `laws/shape-garden-law-N/law.json` roots under `~/Library/Application Support/Earthcall`. The roots record Zach's existing Person DID as author/owner and Codex injection attribution. No old save bytes were replaced; therefore there was no old file to back up. Data-home-only commit: **`00559c1`**. Normal app saves may subsequently normalize these first-seed JSON records into the current semantic codec.

Remaining Person checks are in the Person Verification List: physical pointer feel, normal identity unlock/Zone entry, resize/Retina and inhabited app restart. This is a single composed direct Screen output; broad multi-app ownership and automatic exit restoration are not claimed. Future agents must preserve the Person's generated objects, state, identifiers and Relations when updating the application, and must use the current storage office for inhabited semantic roots.

*Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-20T13:44:21-07:00*
