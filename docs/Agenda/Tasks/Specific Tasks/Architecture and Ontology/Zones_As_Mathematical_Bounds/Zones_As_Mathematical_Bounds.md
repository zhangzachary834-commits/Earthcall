# Zones as Mathematical Bounds

*Claude Opus 5.5 · session `b0dcb70f-a02a-4081-8589-0aae3ab30551` · 2026-09-23. From Zach's direction the same day: Person's location must be decoupled from whether a Zone is active, and Zones become OntoMath bounds in a continuum ("a Dimensional Zone").*

**Status:** Rung 1 done and green (`tests/zones/dimensional_zone_test.cpp`, mutation-checked). Rungs 2–6 planned.

- Plan, model, rungs, refusals, derived-state declaration: [plan](../../../../../plans/ZONES_AS_MATHEMATICAL_BOUNDS_PLAN_2026-09-23.md)
- Code: `Zone` (`within`, `dimension.*`, `placement.*`, `extent`, `extent.lo/hi`), `ZonesOfEarth/ZoneBounds.cpp` (locator, `@world.zoneId` / `zonePath` / `dimensionalZoneId`), `ZoneSerialization.cpp` (optional `dimensions` / `placement` / `extent` keys).
- **Next: Rung 2**. Law-writable `running` plus derived `present`. The law closure becomes the union of present and running Zones (today `switchTo` swaps it wholesale; see `ZoneManager::switchTo` and `ZoneLawMembership.cpp`). Physics steps every running Zone.
- **Pitfalls for the next agent:**
  - A child Zone's object view includes its parents' objects (`switchTo` repopulates from `globalObjects`). Residence therefore prefers the store the object is *designated* to.
  - Never store location. It is a reading.
  - Never write a save's bounds without its owner's authorization (Rung 6).
