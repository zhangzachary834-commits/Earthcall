# Create sent newborns into inactive World

Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-05 11:55 PDT.

## Origin and failure

Zach confirmed that the continuous below-me Law created named `object-XXX` Objects and face assets, but none appeared in the 3D scene. He then tried the camera-forward Visible Probe and still saw no cube. This falsified treating floor occlusion or gaze direction as a complete explanation.

`ActionModel.cpp::resolveZone` first accepted an explicit Zone subject, then searched the Universe for a Zone with identifier `World`, and only then used the first Zone. EngineInit intentionally provides the active/rendered Zone first and subsequently exposes inactive Zones for named reach and governance. `saves/zones/World/zone.json` is a real inactive Zone in the current store. The resolver's legacy spelling preference therefore overrode the active destination. Creation and assets were real; the renderer drew the active Zone's Objects while the newborns were held elsewhere.

The earlier native probe drew the newborn directly and used a boot harness that omitted inactive Zones. It proved geometry/placement rendering but missed residence selection. Its floor-occlusion observation is valid for that fixture and did not diagnose Zach's live failure. The shared boot harness now enumerates inactive Zones and their fields like EngineInit.

## Change

The shared resolver preserves explicit Zone subjects and otherwise selects the first Zone in the existing Universe domain. EngineInit already guarantees active Zone first. The hardcoded preference for the spelling `World` is removed. This corrects generic Create, prototype Create, Spawn, and the Zone resolver used by AddRelation without adding a grammar branch, a new enum, or a new domain type. Authored compiler Metalaws and placement expressions are unchanged.

This repairs the existing implicit destination protocol; it does not redefine ownership or derived Zone location. Explicitly targeting the World Zone remains supported. No Person-authored Law, existing cube, or inhabited save was rewritten or moved.

## Reproduction and verification

An isolated copy of the existing LawLine and World seeds reproduces the failure before the patch: after the actual Terminal-authored continuous sentence fires, active LawLine remains at **one Object** and inactive World holds **one newborn**. After the patch, active LawLine has **two Objects** and inactive World remains empty.

The native probe now also supports `python3 scratch/probes/law_line_visibility_probe.py --engine`. It runs the full production Engine in a separate uninhabited working directory and save root, with an unkeyed test Person profile named Zach and copied seed roots. It never loads real identity keys. It enters LawLine, submits the actual `examples/law_line_visible_probe.txt` through Terminal/Metalaws, advances Engine ticks, and captures the real viewport through ScreenRecorder. The fixture author is that simulated Person; the test profile's `injected_by` identifies Codex / GPT-6.1 Sol. Only temporary fixture saves are generated.

**Full Engine result: PASS.** The probe is held in the active rendered Zone and inactive World remains empty. Native 2560×1440 PNGs decode; the central viewport contains **zero gold pixels** before submission and **484,416 gold pixels** after it. The after image visibly shows the cube's gold front face. This verifies the actual Engine path rather than drawing the Object directly.

- [Measurements and routing counts](../../scratch/verification/law-birth-routing-2026-10-05/result.json)
- [Full Engine before](../../scratch/verification/law-birth-routing-2026-10-05/engine-before.png)
- [Full Engine after](../../scratch/verification/law-birth-routing-2026-10-05/engine-after.png)
- [Before-fix routing log](../../scratch/verification/law-birth-routing-2026-10-05/before.log)
- [Full Engine execution](../../scratch/verification/law-birth-routing-2026-10-05/engine.log)
- [Focused test log](../../scratch/verification/law-birth-routing-2026-10-05/ctest.log)

The rebuilt `earthcall_webgpu` target succeeds. **Seven focused tests pass:** `action_spawn_test`, `chess_zone_native_boot_test`, `law_creation_test`, `shape_generator_law_test`, `universal_singular_creation_test`, `law_line_zone_test`, and `terminal_zones_test`. Regression tests exercise Create and Spawn with an active Zone plus inactive World, retain explicit Zone destination selection, and cover prototype birth. The booted LawLine regression includes both seed Zones and asserts inactive World receives no births. Person scene acceptance after restarting the rebuilt app remains in [Person Verification](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md).

A broader twelve-test run was stopped, not counted as passing. Legacy `chess_app_test` fails its black-pawn capture assertion at line 213. In a separate isolated Chess/World fixture, the same assertion fails both with the old resolver/old harness and with the corrected resolver/current harness; this is baseline debt, not evidence of a routing regression. The comparison replaces assertion abort with an exception so stack unwinding preserves the fixture and avoids the long native abort delay. Both [baseline](../../scratch/verification/law-birth-routing-2026-10-05/chess-baseline-isolated.log) and [current](../../scratch/verification/law-birth-routing-2026-10-05/chess-current-isolated.log) logs are retained. The native Zone boot chess test passes.

The broader run also encountered costly full-store pre-load serialization in `person_not_object_world_test`; a one-second stack sample placed it in `ZoneManager::saveState`'s JSON dump, with roughly 10 GB resident memory. That check and the unrestricted baseline attempt were stopped. All real `saves/zones` and `saves/homes` file membership and SHA-256 bytes were then compared with the pre-test backup: **no differences**. No restore over concurrent work was needed. The full suite remains unverified; [legacy test follow-up](../Agenda/Tasks/Specific%20Tasks/Law%20and%20Reasoning/Legacy_Test_Store_Isolation/Legacy_Test_Store_Isolation.md) records the separate work.

Future agents: preserve the destination ordering declared at EngineInit's Universe provider; do not restore a special name preference. Any replacement birth/residence architecture must remain authored and distinct from spatial bounds and ownership. Keep the inactive-Zone fixture in the shared harness and the full Engine capture consumer when changing Create or render routing.
