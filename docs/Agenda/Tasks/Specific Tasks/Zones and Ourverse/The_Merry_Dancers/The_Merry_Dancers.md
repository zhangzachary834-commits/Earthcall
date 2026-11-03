# The Merry Dancers

*Claude Code / Claude Opus 5.5 / session `ed9d515b-d6ac-4098-a7f9-ce71d02406b3` / 2026-11-03T12:09:36-0800. Commissioned by Zach, 2026-11-03: "THERE IS ALREADY AN AURORA ZONE (northern veils) BUT I WANT U TO MAKE ANOTHER AURORA ZONE FROM SCRATCH ... USE VOLUMETRICS AND RADIANCE FIELDS TO MAKE GORGEOUS, REALISTIC STYLE AURORA", with seven reference photographs. He added that Northern Veil "isn't that realistic/breathtaking-level" and is not the reference, and that The Veiled Hour is relevant only as volumetrics and light. The name, the design and the mathematics below are this agent's; Zach may rename or redirect any of it.*

**Status:** far-plane fix in place; the complete Zone still awaits Zach's first look in the app. Before the fix he called the curtains alone "AMAZING" at 18-24 FPS. Checks are on the [Person Verification List](../../../For%20Zach/Person%20Verification%20List.md#the-merry-dancers--first-look-2026-11-03).

## Resolved: the app clipped everything opaque past 100 m

`src/Singularity/Core/EngineRender.cpp` hard-coded `farZ = 100` beside `fov = 45`. The sky surface sits at 960, so it was clipped, along with the ridges, stars and sky reflection it carries. Media do not depend on the far plane, which is why the curtains survived. An opaque sky inside 100 m would occlude the curtains, so moving the dome closer was never a fix. Look development had wrongly used far 1000 and FOV 70, from an unused `PersonPerspective` default.

**Fix, chosen by Zach ("authored far plane"), 2026-11-03:** `view.farPlane` is an authored property on a Zone's spatial root, beside `light.*`, read by `Rendering::authoredFarPlane` (`src/Singularity/Screen/AuthoredView.hpp`). This departs from the Law-set channel value first proposed, because a channel value would leak into the next Zone and need a reset; a root property ends with the Zone and a Law can still write it. Absent means the historical 100, and only this Zone (2000) carries it among 49 data-home `zone.json` Zones. `.ecform` Zones were not scanned, but the property is new, so none can carry it unless authored since. FOV stays hard-coded at 45.

**Evidence:** `authored_far_plane_test` 20/20; `merry_dancers_test` 75/75 on snapshot and live file. Offscreen with the app's camera and the production read, 2560×1440, median of 7 frames: no clear-colour pixels in any view; 79 ms north, 80 ms up, 81 ms northeast, 60 ms south. That is about 12–17 FPS offscreen, compared with the 16–20 implied by the far-100 reproduction (62 and 51 ms) for what he measured at 18–24 FPS. The live window remains unverified. Claude Code / Claude Opus 5.5 / session `ed9d515b-d6ac-4098-a7f9-ce71d02406b3` / 2026-11-03T13:53:38-0800.

## What exists

| Thing | Where |
|---|---|
| Live Zone | `<data home>/zones/The Merry Dancers/zone.json`; not yet committed in the data-home repo |
| Snapshot | `saves/zones/The Merry Dancers/zone.json` (byte-identical at install) |
| First-seed generator | `scripts/author_merry_dancers.py`; refuses to overwrite an existing Zone unless `--look-dev` is passed for a scratch file |
| Witness | `tests/zones/merry_dancers_test.cpp` (`merry_dancers_test`), 72/72 on both the snapshot and the live file |

Beings: 3 FieldNode media, 28 Field Objects (sky, shore, 26 spruces), 3 Materials, 1 spatial root. Every medium and Object records `authors: Zach` and `injected_by: Claude Code / Claude Opus 5.5`. No C++, enum value or new method was added.

## How it is built

- **Marched sheets:** thin folded sheets whose brightness comes from the transport integral through them. One emission function carries the altitude spectrum (oxygen green low, oxygen red and nitrogen violet high, a faint pink hem). The far band has a mirrored medium below y = 0.
- **Radiance field:** the sky is one opaque surface whose colour is a function of direction: air glow, fanned rays on meridians of the magnetic zenith, stars, and the far ridges and treeline. It evaluates once per pixel.
- **The lake is not a surface.** Sky, ridges and trees are even in y, so rays pass through y = 0 into a mirror world.

## Known limits

- **The radiance-field sky is still.** Surface colour binds no time coordinate, so only the marched sheets move. A moving sky needs either a time binding on material colour (an engine change for Zach to weigh) or a Law-driven parameter.
- **Far ridges are a matte.** They have no parallax. Marched terrain cost about 1.2 s per frame, so they live in the sky field. Flying high exposes this.
- **Only the far band is mirrored by a medium.** The arc's reflection cost about 40 ms for water the sky's reflected rays already fill. The sky's own reflection does not ripple the rays in time.
- **FOV is still the app's hard-coded 45°.** The Zone was composed at 70°, so less of the sky fits in view.
- **No spawn or facing is authored.** Zones have no spawn field that this pass found. The composition faces +Z from (0, 2, 0); the default yaw of -90 faces -Z, so a Person may need to turn around.
- **Eye height is unverified.** The ground at the origin is y = 1. Inside the shore solid the screen would be one flat colour.

## Measurements

Offscreen probe, 2560×1440, Apple GPU, production `WebGpuRenderer`, median of 9 synced frames with readback, Debug host build, 2026-11-03. These are not FPS claims and not live-window figures. **Zach reports 18-24 FPS in the app (2026-11-03; view and window size not stated)**, better than these offscreen figures imply, so do not read this table as live frame rate. **Build caveat:** this probe was linked at 11:54 PST against `build/CMakeFiles/earthcall_webgpu.dir/.../WebGpuRenderer.cpp.o` (built 11:46), which appears to include another agent's uncommitted renderer edits from the same checkout (source saved 11:40). These figures and the final stills are therefore not from committed HEAD. Earlier look-development captures used older objects, so comparisons across the session may straddle that change.

| View from (0, 2, 0) | ms |
|---|---|
| North, lake and sky | 162 |
| Zenith | 115 |
| Northeast | 101 |
| Northwest | 100 |
| South | 60 |

That is roughly 6–17 FPS, well short of Northern Veil's 33 ms through the same probe at its own spawn view. North is the worst view because the reflection and the band share the frame. Cost by part, measured on the build before the last shore and ground edits: media 56–87 ms, sky 26 ms, shore 14–19 ms, spruces under 7 ms. Hydration takes 0.23 s. The look was developed on this probe, whose surface format matches the app's non-sRGB surface, so colours should match; that remains unconfirmed by a Person.

## Findings for other lanes

1. **`MathNode::typeOf` was exponential in tree height: fixed 2026-11-03 at Zach's request ("DO 1").** The Matrix-refusal pre-loop typed every child, discarded the answers, and each case typed them again, while `MathNode::fromJson` runs the check at every node with children. `sub` now returns the pre-loop's answer. Answers are byte-identical to the unfixed binary across a 45-case table, 1,085 saved expressions and 1,375 live data-home expressions, including diagnostics. Parsing a depth-20 chain went from 0.50 s to 0.0003 s; depth 400 takes 0.07 s where the old cost was about 2^400. Witness: `ontomath_typeof_depth_test`. **Still open:** `fromJson` checks every node and builds `path` strings, so load stays quadratic in depth (0.33 s at depth 800), and recursion overflows the stack at about 1000 levels in Debug, which the old cost made unreachable. This Zone's balanced trees remain harmless and were not regenerated. Claude Code / Claude Opus 5.5 / session `ed9d515b-d6ac-4098-a7f9-ce71d02406b3` / 2026-11-03T13:30:51-0800.
2. **Routed transport was slower than Current on this form.** With an earlier four-media build of this Zone, all recipe-admitted and `routedPixels` at full frame: 125 ms routed against 67.5 ms Current (north-facing view), and 108 against 63 (zenith), media only. The cause was not found. These are single offscreen comparisons, not ABBA series. A sheet envelope 4–6× wider than Northern Veil's is one untested suspect. This Zone now has three media and stays on Current. The Intercom link journal reports an incomplete tail, so this was not posted there; the ray-transport lane should read it here.
3. **Let is inlined as text at every use,** so a Let read several times repeats its whole cost. Evaluating colour weights once per channel-independent vector, rather than per channel, cut one build from about 190 ms to about 125 ms routed. Future authors should prefer scalar × constant vector and single-use bindings.

## Next

- Zach's verdict first: everything below depends on what he sees.
- Bring the north view down: cheaper reflection, or a mirrored sheet with fewer samples if that can be authored per medium.
- Decide on time-varying sky radiance.
- Real ridge geometry with height-grid traversal, if flying near them matters.
