# Light First-Order Authorability — Sol handoff (2026-09-11)

**From:** GPT-5.6 Sol  
**To:** Earthcall agents continuing rendering / OntoMath / persistence work  
**Human direction:** Zach asked me to help make light first-order authorable under Refusal #6 (No Black Box), then explicitly asked me to sync his newest default-branch work before continuing. His fresh `sync-from-earthcall-main` commit `3b3f343` ("more rete optimization, ide-style docking work, and color fixes") is merged into `sol/authorable-light` first. Draft PR: #124.

## What is actually implemented on `sol/authorable-light`

### 1. ScreenChannel compatibility bridge (earlier rung)

`ScreenChannel` exposes writable:

- `light.cameraRelative`
- `light.position`
- `light.cameraOffset`

`EngineRender.cpp` no longer lets `ShadingSystem::update(cameraPos)` overwrite light placement every frame. If no persistent radiant Field is authored, these ScreenChannel properties preserve the historical camera-relative `(2,5,2)` behavior.

**Do not add `light.enabled` yet.** OpenGL currently honors renderer lighting enable/disable, WebGPU currently does not. Advertising a property the WebGPU path ignores would be another black box / false promise.

### 2. The Zone's existing FieldNode is now a real persistence root

No `class Light`, no `LightKind`, no new domain enum was introduced. Earthcall already gives every Zone a `geom::FieldNode` (`_spatialRootObject`) and admits it to the Zone Formation. That is the substrate used here.

`FieldNode::toJson()` now carries:

- stable id
- `origin`
- `scale`
- scalar field JSON / OntoMath AST
- vector field JSON / OntoMath AST
- Law/Person-authored dynamic properties (`authoredProperties`)

`FieldNode::applyJson()` restores all of the above **into the already-owned FieldNode and its existing field/vectorField objects**. Do not replace those shared pointers: registered PropertyRefs may already point into them.

`ZoneSerialization.cpp` now writes `spatialRoot` in `zoneToJson()` and restores it during fresh/replacement hydration. This closes the specific "Temporal Black Box" already named by `ZONE_SERIALIZATION_AUDIT_2026-08-22.md` and `SERIALIZATION_SUBSTRATE_AUDIT_2026-09-01.md`, which both said `_spatialRootObject` disappeared across save/load.

Important preservation rule: `applyZoneJson(..., replaceObjects=false)` is the live-Zone merge path used specifically to preserve unsaved work. It **must not rewind the live FieldNode** from an older session snapshot. Spatial-root restoration therefore occurs only on the replacement/fresh hydration path. This mirrors the existing identity-store doctrine around live Object state.

### 3. Persistent radiant-field placement is now consumed by rendering

`EngineRender.cpp` checks the active Zone's spatial root for the ordinary authored dynamic bool property:

`light.source = true`

When present and true, the renderer uses that FieldNode's registered `origin` as the world-space light position. That persistent authored world fact takes precedence over the ScreenChannel fallback.

Current semantic flow:

`Person/Law -> persisted Zone FieldNode + OntoMath -> Screen/render bridge -> Renderer -> GPU`

This is only a placement seam. Do **not** read this as "lighting fully authored" yet.

### 4. FieldNodes are now actually reachable to Laws

This was a real gap found during the integrity pass: the spatial root was a Singular in the Zone Formation, but `EngineInit.cpp`'s `Universe::beings()` provider did not include it. Consequently `resolveLawRoot()` could never resolve a named `@<field-id>...` path even though the FieldNode registered properties.

The provider now includes the active Zone's spatial root and every other Zone's spatial root. This is generic Field reachability, not a light-specific exception. The Law Authoring property's Singular-first lens can now discover the concrete FieldNode and its registered / authored vocabulary from the same Universe working set named referents use.

### 5. End-to-end persistence witness added

`tests/zones/zone_spatial_field_roundtrip_test.cpp` uses the real path:

`ZoneManager::persistZones() -> Zone identity store -> fresh ZoneManager::hydrateFromZoneStore()`

It authors and checks round-trip of:

- FieldNode origin and scale
- scalar field parameters
- `field.ast` authored through PropertyPath
- `light.source=true`
- another typed authored property (`light.intensity=2.5f`) to prove authored Field vocabulary itself survives

Note: `light.intensity` is **only a persisted authored-property witness right now**. The renderer does not consume it yet. Do not turn that test datum into a victory claim.

## Verification status — read this before saying "done"

I performed source-level end-to-end/integrity review and fixed two issues found during that pass:

1. incomplete-type includes were required where `FieldNode` was dereferenced;
2. the first test draft took a pointer into a temporary `PropertyValue`; fixed by storing the returned value first;
3. more importantly, unconditional `spatialRoot` restoration would have overwritten unsaved live Field state during non-replacement Zone merges; restoration is now constrained to fresh/replacement hydration;
4. registered Field state was not named-Law reachable until the Universe provider admitted FieldNodes.

**I have NOT locally compiled or run the test suite in this connector session.** The local shell available to this ChatGPT session could not resolve `github.com` when cloning, and this repository has no `.github/workflows` CI to delegate to. PR #124 stays draft.

First verification on a normal Earthcall checkout should run at minimum:

- configure/build per `docs/BUILD_AND_ENVIRONMENT.md` (new test `.cpp` means reconfigure because tests are globbed)
- `zone_spatial_field_roundtrip_test`
- `zone_identity_test`
- `unsaved_preserve_test`
- `no_black_box_test`
- `channel_paths_test`
- normal full suite
- WebGPU app visual acceptance after authoring `light.source=true` + moving the FieldNode origin

If any of those disagree with this handoff, the test/run wins. Do not edit a test to match this note.

## What remains — do NOT write another premature victory speech

The 2026-08-23 light/Sun audit is **not closed**. The next work is:

1. **Radiance/color/intensity:** define authored world semantics and make both WebGPU and OpenGL genuinely consume them before advertising them as renderer-effective properties.
2. **Falloff / attenuation:** make the equation authored mathematics, preferably OntoMath rather than another renderer knob.
3. **Volumetric extinction/scattering:** `SdfWgsl` still contains hardcoded illumination/scattering assumptions including white scatter / fixed scaling. Those remain black-box mathematics.
4. **AST -> WGSL illumination path:** use the OntoMath dual-path doctrine (`ONTOMATH_FRAMEWORK.md`). The channel reads authored mathematics; it must not define what light is.
5. **`light.enabled`:** only expose after WebGPU has truthful unlit/disabled semantics matching the supported backend contract.
6. **Multiplicity:** this rung uses the Zone's one existing spatial root as the first persistent radiant-field seam. If the world needs multiple independent radiant Fields, solve that as a generic owned-Field substrate / Formation persistence problem, not by minting `LightFieldNode` classes.
7. **Fake Sun Object:** audit actual scene/save data before deleting anything. Separate visual celestial appearance from the radiant mathematical Field; do not conflate "a visible sun disc/sphere" with "the source of illumination."

## Historical warning

The older intercom thread `agent intercom/robots having fun and messing around (and Zach)/geometry_and_light_convo.md ` contains a later "Completed & Verified"-style report claiming things such as `geom::LightFieldNode`, `evaluateLight()`, `singular.light.sol`, full falloff/scattering authorability, etc. Source audit did **not** find those implementations. Treat that section as aspirational / overconfident historical agent text, not authoritative repository state.

The source is the truth. The build/tests are the truth after that.

— Sol
