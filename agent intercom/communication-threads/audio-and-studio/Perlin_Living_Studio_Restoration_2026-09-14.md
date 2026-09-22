# Perlin + Living Studio restoration — 2026-09-14

Author: GPT-5.6 Sol. Zach explicitly requested restoration after reporting that the Perlin SDF hills had disappeared and that Living Studio controls still depressed visually while sound and corresponding resonator growth no longer occurred.

## Restored

- `NoiseFloorWorld`: restored only `perlin-ground-plane.field` and `fieldExtent` from the intact authored `saves/worlds/noise_floor.json`; all later Zone/Object state is retained.
- `SynthesisStudio.LivingInstrument`: migrated 68 authored Laws from the legacy world bundle into stable `saves/laws/<id>/law.json` roots and added the same stable ids to `lawRefs`.
- Model-author closure: migrated `author.gemini-spark` from the old categories-bag compatibility side channel into a nonvisual Zone-owned Object referent, preserving identifier/authorship instead of forging a Person.
- `ZoneManager::switchTo`: shared Law authors resolve as `Singular`, matching `Law::addAuthor(Singular&)`; Person authors still resolve to the actual Person when that is their being.

## Preservation

Exact pre-restoration Zone bytes are retained under `saves/backups/perlin-living-restoration-2026-09-14/`. Perlin recovery is field-scoped. Living Studio existing Objects are not regenerated or replaced; the only added Object is a previously external model-author referent needed to make the Zone's authored Law closure independently resolvable.

## Automated verification — green

- `perlin_zone_field_restore_test`: reads the restored authored Zone identity, proves `field` + `fieldExtent` are present, deserializes through the real Object reader with `hasField()==true`, verifies the `1000 x 30 x 1000` extent, and proves the current writer retains the SDF payload on round-trip.
- `synthesis_studio_zone_closure_test`: copies only the Living Zone identity and its named shared Law roots into a temporary SaveRoot, boots through `hydrateFromZoneStore()`, enters with `ZoneManager::switchTo()` and no legacy World load, proves the note/resonance/sculpture Laws are present, activates C5, observes the audio sink receive the authored `523.25 Hz` note, observes the matching C5 resonator record the strike, and proves its radius grows above the authored base radius.
- Earthcall focused CI run `34937370818` on branch head `f980f3bc42bb784173ced536e9762d58b0e7ca8b`: configure ✅, focused build ✅, focused regression witnesses ✅.

## Person witness still required

Fully quit any already-running stale Earthcall instance without saving its old live versions, relaunch the restored build, then use Creator Console → Zones. Move to `NoiseFloorWorld` and verify the rolling Perlin hills are visible. Move to `SynthesisStudio.LivingInstrument`, play C5 and several other notes, verify they are audible, and verify each corresponding floating resonator visibly swells/rises and relaxes while the button depression/release still works. This is the remaining desktop manifestation check; automated structural and behavioral witnesses are green.
