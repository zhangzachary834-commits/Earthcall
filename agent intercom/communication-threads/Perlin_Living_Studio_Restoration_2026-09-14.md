# Perlin + Living Studio restoration — 2026-09-14

Author: GPT-5.6 Sol. Zach explicitly requested restoration after reporting that the Perlin SDF hills had disappeared and that Living Studio controls still depressed visually while sound and corresponding resonator growth no longer occurred.

## Restored

- `NoiseFloorWorld`: restored only `perlin-ground-plane.field` and `fieldExtent` from the intact authored `saves/worlds/noise_floor.json`; all later Zone/Object state is retained.
- `SynthesisStudio.LivingInstrument`: migrated 68 authored Laws from the legacy world bundle into stable `saves/laws/<id>/law.json` roots and added the same stable ids to `lawRefs`.
- Model-author closure: migrated author.gemini-spark from the old categories-bag compatibility side channel into nonvisual Zone-owned Object referent(s), preserving identifier/authorship instead of forging a Person.
- `ZoneManager::switchTo`: shared Law authors resolve as `Singular`, matching `Law::addAuthor(Singular&)`; Person authors still resolve to the actual Person when that is their being.

## Preservation

Exact pre-restoration Zone bytes are retained under `saves/backups/perlin-living-restoration-2026-09-14/`. Perlin recovery is field-scoped. Living Studio existing Objects are not regenerated or replaced; the only added Object is a previously external model-author referent needed to make the Zone's authored Law closure independently resolvable.

## Verification still required

Automated live-path tests must enter Living Studio with `ZoneManager::switchTo()` and no legacy world load, then prove note audio and resonator growth. A Person desktop witness should confirm the Perlin hills, audible notes, and corresponding sphere growth after a fresh restart.
