# Personal Ourverse Navigator

**Zach's idea (2026-10-12), recorded verbatim in spirit and landed 2026-10-16:** a "beautiful personal Ourverse navigator with little 2D/3D islands representing each world." This is what a Person meets when Earthcall.app opens, and what a fresh data home's first run should grow into.

## Landed Implementation (2026-10-16)

The Personal Ourverse Navigator is authored as a floating archipelago inside **Ourverse Gathering** (`saves/zones/Ourverse Gathering/zone.json` and data home `~/Library/Application Support/Earthcall/zones/Ourverse Gathering/`):
- **Central Observation Plinth (`navigator-plinth-center`):** Formed of dark slate basalt (`material.navigator.plinth.deck`) with an inscribed golden compass rose inlay (`navigator-compass-ring`) resting at `[0.0, -0.2, 0.0]`. The Person stands at `[0.0, 1.8, 0.0]` facing north toward the archipelago.
- **Glowing Filaments:** Ethereal luminous pathways (`type: "filament"`) weaved between the plinth and the four world islands.
- **World Islands:**
  1. **Sanctum of Beginnings (North-West, gold):** Stepped golden spire (`island-sanctum-spire`), sunlit portal arch (`island-sanctum-arch`), and radiant gold navigation beacon (`navigator-beacon-sanctum`).
  2. **Temple of Echoes (North-North-West, azure):** Deep blue basalt crag, silver colonnade (`island-temple-col-1..4`), pediment roof, and sapphire navigation beacon (`navigator-beacon-temple`).
  3. **Cavern of Light (North-North-East, emerald):** Obsidian cavern base, cluster of glowing teal/cyan crystals (`island-cavern-crys-1..3`), and emerald navigation beacon (`navigator-beacon-cavern`).
  4. **Character Architect Forge (North-East, ember):** Magma blocks, obsidian anvil, glowing crucible, and ember navigation beacon (`navigator-beacon-forge`).
- **Interactive Law-Governed Navigation (`law-navigator-button`):**
  - Authored Law `law-navigator-button` (`saves/laws/law-navigator-button/law.json`) watches for `object-clicked` where subject is related via `instance-of` to `category.control.button`.
  - Publishes `control-activated` and `zone-navigation-requested`.
  - Engine reads `destinationZone` property ("Sanctum of Beginnings", "Temple of Echoes", etc.) and transitions active zone via `Core::Engine::instance().requestZoneSwitch`.
  - In `ZoneManager::switchTo`, `applyFormationRelations` is sequenced after world object repopulation so formation relations and category memberships bind cleanly to live objects without dangling pointers across transitions.

## Verification & Visual Witnesses
Full automated offscreen WebGPU rendering and interaction loop executed via `scratch/probes/ourverse_navigator_probe.py` (`scratch/probes/ourverse_navigator_probe.cpp`):
- `navigator_plinth_panorama.png`: 2560x1440 panoramic view of the 4 world islands from the central plinth.
- `navigator_archipelago_aerial.png`: 2560x1440 high-angle aerial view of the archipelago and glowing filaments.
- `navigator_island_sanctum.png`: Close-up of Sanctum of Beginnings gold spire and arch.
- `navigator_island_temple.png`: Close-up of Temple of Echoes colonnade and sapphire beacon.
- `navigator_island_cavern.png`: Close-up of Cavern of Light crystal spires and emerald beacon.
- `navigator_island_forge.png`: Close-up of Character Architect Forge crucible and ember beacon.
- `navigated_destination_sanctum.png`: Captured visual after clicking the Sanctum beacon and arriving inside Sanctum of Beginnings.

*Authored by Gemini under Zach's authority · session `feb60581-5952-44ea-9dfa-5d87e8fcb273` · 2026-10-16. First Mover author `author.gemini-spark`.*
