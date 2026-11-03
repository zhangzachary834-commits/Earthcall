# The Merry Dancers

*Claude Code / Claude Opus 5.5 / session `ed9d515b-d6ac-4098-a7f9-ce71d02406b3` / 2026-11-03T12:09:36-0800. Commissioned by Zach, 2026-11-03: "THERE IS ALREADY AN AURORA ZONE (northern veils) BUT I WANT U TO MAKE ANOTHER AURORA ZONE FROM SCRATCH ... USE VOLUMETRICS AND RADIANCE FIELDS TO MAKE GORGEOUS, REALISTIC STYLE AURORA", with seven reference photographs. He added that Northern Veil "isn't that realistic/breathtaking-level" and is not the reference, and that The Veiled Hour is relevant only as volumetrics and light. The name, the design and the mathematics below are this agent's; Zach may rename or redirect any of it.*

**Status:** first seed installed. Person verdict open: see the [Person Verification List](../../../For%20Zach/Person%20Verification%20List.md#the-merry-dancers--first-look-2026-11-03).

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
- **No spawn or facing is authored.** Zones have no spawn field that this pass found. The composition faces +Z from (0, 2, 0); the default yaw of -90 faces -Z, so a Person may need to turn around.
- **Eye height is unverified.** The ground at the origin is y = 1. Inside the shore solid the screen would be one flat colour.

## Measurements

Offscreen probe, 2560×1440, Apple GPU, production `WebGpuRenderer`, median of 9 synced frames with readback, Debug host build, 2026-11-03. These are not FPS claims and not live-window figures; the app has not been run on this Zone. **Build caveat:** this probe was linked at 11:54 PST against `build/CMakeFiles/earthcall_webgpu.dir/.../WebGpuRenderer.cpp.o` (built 11:46), which appears to include another agent's uncommitted renderer edits from the same checkout (source saved 11:40). These figures and the final stills are therefore not from committed HEAD. Earlier look-development captures used older objects, so comparisons across the session may straddle that change.

| View from (0, 2, 0) | ms |
|---|---|
| North, lake and sky | 162 |
| Zenith | 115 |
| Northeast | 101 |
| Northwest | 100 |
| South | 60 |

That is roughly 6–17 FPS, well short of Northern Veil's 33 ms through the same probe at its own spawn view. North is the worst view because the reflection and the band share the frame. Cost by part, measured on the build before the last shore and ground edits: media 56–87 ms, sky 26 ms, shore 14–19 ms, spruces under 7 ms. Hydration takes 0.23 s. The look was developed on this probe, whose surface format matches the app's non-sRGB surface, so colours should match; that remains unconfirmed by a Person.

## Findings for other lanes

1. **`MathNode::typeOf` is exponential in tree height.** The pre-loop in `src/Singularity/OntoMath/ScalarForm.cpp` types every child, then each case types them again, and `MathNode::fromJson` runs the check at every node with children. An unflattened sky took about a minute to parse. Let does not double, so this Zone authors around the defect with balanced trees and single-use Lets. The engine fix is unmade, because it touches shared math code outside this commission.
2. **Routed transport was slower than Current on this form.** With an earlier four-media build of this Zone, all recipe-admitted and `routedPixels` at full frame: 125 ms routed against 67.5 ms Current (north-facing view), and 108 against 63 (zenith), media only. The cause was not found. These are single offscreen comparisons, not ABBA series. A sheet envelope 4–6× wider than Northern Veil's is one untested suspect. This Zone now has three media and stays on Current. The Intercom link journal reports an incomplete tail, so this was not posted there; the ray-transport lane should read it here.
3. **Let is inlined as text at every use,** so a Let read several times repeats its whole cost. Evaluating colour weights once per channel-independent vector, rather than per channel, cut one build from about 190 ms to about 125 ms routed. Future authors should prefer scalar × constant vector and single-use bindings.

## Next

- Zach's verdict first: everything below depends on what he sees.
- Bring the north view down: cheaper reflection, or a mirrored sheet with fewer samples if that can be authored per medium.
- Decide on time-varying sky radiance.
- Real ridge geometry with height-grid traversal, if flying near them matters.
