# Ourverse Gathering was replaced, not patched, and its backup sits in `zones/`

**Opened:** 2026-10-16 by the [full-system audit](../../../../../audits/FULL_SYSTEM_AUDIT_2026-10-16.md#1-findings-most-severe-first) (F3, F4, F9). Claude Code / Claude Opus 5.5 / session `d6fe31dc-53d9-49c1-a4b7-87479d9d605b`.

## Facts (data home `~/Library/Application Support/Earthcall`)

- Commit `9135fd6` (author `Gemini <gemini@antigravity>`, 2026-10-16 17:45, "Install Personal Ourverse Navigator into Ourverse Gathering") rewrote `zones/Ourverse Gathering/zone.json` from 36 MB to 45 KB and deleted `zone.ecform`.
- Objects went 122→28 and materials 122→18. None of the old object ids survive.
- The removed objects are 119 tori and 3 cubes with generated ids (`object-555`…) and `Player`/`law-30` stakeholders.
- The new Zone's `qualities.injected_by` reads "Gemini 3.8 Flash (High) - Personal Ourverse Navigator Prototyping Loop authorized by Zach, 2026-10-16". No other record of that authorization was found.
- Full copies remain in data-home git `71a8503` and in `zones/Ourverse Gathering.bak.20261016/` (`zone.ecform` and `zone.json`, 122 objects each).
- **Hazard:** `SaveSystem::listZoneIdentityRecords` treats every `zones/*/` folder containing a zone file as a Zone. The `.bak` folder carries identifier `Ourverse Gathering`, so the next launch should see two Zones with one identifier. This is inferred, not launched.

## Steps (in order)

1. **Zach** confirms whether the 122 old objects were meant to go.
2. Move the `.bak` folder to `backups/`, which is gitignored and not enumerated.
3. Restore or keep the Zone per step 1, then commit the data home.

No agent should restore or delete either copy before step 1.

## Status 2026-10-16: step 2 done at Zach's request

On Zach's "fix the low hanging fruit", the backup was **moved** (not altered) to `backups/Ourverse Gathering.bak.20261016/`, so it can no longer load as a duplicate Zone. Step 1 (Zach's decision) and step 3 remain.
