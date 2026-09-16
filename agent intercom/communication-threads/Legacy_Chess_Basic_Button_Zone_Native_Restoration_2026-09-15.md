# Legacy Chess + Basic 2D Button Zone-native restoration — 2026-09-15

Author: GPT-5.6 Sol. Zach asked to make the legacy `chess_app` world and the Basic Button Zone independently enterable through the ordinary fresh-boot Zone path, following the same preservation principle used for Living Studio.

## Preserved sources

No legacy World/session file is rewritten. The intact authored sources remain:

- `saves/worlds/chess_app.json`
- `saves/worlds/basic_2d_button_zone.json`

Exact pre-restoration Zone identities are preserved under `saves/backups/chess-button-zone-native-restoration-2026-09-15/`.

## Chess

The current `saves/zones/Chess/zone.json` retained its 39 spatial authored Objects but had only 5 relations and no `lawRefs`. The intact legacy session contains 69 authored Chess Laws, 143 formation relations, and 21 unique extra-spatial support/category/author/state beings.

The preservation migration:

- retains the existing 39 current Chess Objects rather than regenerating them;
- migrates the 21 stable support referents into the Zone closure as nonvisual/offscreen Object referents, including `grok-4.6` and `state.chess`;
- restores the intact 143-relation authored graph;
- migrates all 69 Laws into `saves/laws/<law-id>/law.json` with their original Law JSON, triggers, and `grok-4.6` Object authorship;
- records those 69 stable ids in `Chess.lawRefs`.

The legacy `categories` compatibility bag made those support Objects extra-spatial at runtime. Their Zone-owned compatibility copies remain zero-size Shape2D beings and are parked outside authored screen space rather than being turned into visible world cubes. This is a compatibility rung until Category becomes an independently rooted save being.

## Basic 2D Button

The canonical identity is `saves/zones/Basic2DButtonZone/zone.json`, matching the legacy session's `currentZoneId`/`zoneRefs`. The separate historical directory `saves/zones/Basic 2D Button Zone/` is deliberately left untouched rather than guessed to be equivalent.

The canonical Zone retained `my-2d-button` but had lost its authored `speedDir` property and all four authored Laws. The migration:

- restores only `speedDir` from the intact legacy Object;
- restores `Antigravity` as a nonvisual model/First-Mover Object author referent;
- deliberately does **not** import the legacy compatibility Object named `Zach`, because a real Person must never be forged or shadowed as an Object;
- migrates `law-move-right`, `law-move-left`, `law-bounce-right`, and `law-bounce-left` into shared Law roots with original `object-clicked` triggers and authorship;
- records those four ids in `Basic2DButtonZone.lawRefs`.

## Generic lifecycle repair

Chess's complete relation graph includes Law-to-category relations. Zone boot hydration necessarily occurs before a Zone's shared Law roots are activated, so those edges cannot bind on the first pass. Legacy `loadState()` already performs an idempotent second relation hydration after loading Laws. Zone-native `ZoneManager::switchTo()` now does the same generic second pass after activating shared roots; this is not Chess-specific.

## Automated witnesses

- `chess_zone_native_fresh_boot_test`: boots the complete checked-in Zone store with no legacy World load, enters `Chess`, requires all 69 Chess Laws and all 143 relations, verifies both piece/category and Law/category relations, and walks the e2 pawn to e4 through `object-clicked`.
- `basic_button_zone_native_fresh_boot_test`: boots the complete Zone store with no legacy World load, enters canonical `Basic2DButtonZone`, verifies all four Laws plus `speedDir`, proves no compatibility Object named `Zach` exists, clicks the orange button, and requires `x2D` to move from 100 to 120.

Both witnesses are part of focused macOS CI.

## Person witness

Fully quit any stale Earthcall instance **without saving its old Chess or Basic2DButtonZone copies**, relaunch the restored build, and use Creator Console → Zones only. Move to `Chess`, play several legal moves (including e2→e4), and verify selection/movement/capture still behave without loading `chess_app`. Then Move to `Basic2DButtonZone`, click the orange 2D button repeatedly, and verify it advances horizontally and still reverses at its authored bounds. Legacy Load is not part of this witness.
