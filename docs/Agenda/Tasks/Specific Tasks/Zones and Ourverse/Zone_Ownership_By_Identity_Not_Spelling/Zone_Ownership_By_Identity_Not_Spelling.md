# Zone ownership by identity, not spelling

**Status:** OPEN. P0 of *Make the Earth Inhabitable* (`../Make_the_Earth_Inhabitable/Make_the_Earth_Inhabitable.md#p0-continuity`).

**Origin:** Zach's To-Do line one (*WHY DID IT REFUSE TO TRANSFER HOME "PERSON" TO "ZACH" AND CREATE `Home_of_Zach`*), and three gaps he named from memory on 2026-09-17 (no Home⇄Zone transition, one Zone at a time, no multi-Home policy). Forensics and code check by Claude Fable 5.1, session `e9c2fb5e-5aa9-49a1-b3e7-6ee321422021`, 2026-09-17, in
`docs/Reflections on Earthcall's Progression/Reflections on Repo State/Two_Houses_One_Spelling.md` §3, §4, §6b. Bugs.md #26.

---

## What is true on disk (verified 2026-09-17)

| | `saves/homes/Home/home.json` | `saves/homes/Home_of_Zach/home.json` |
|---|---|---|
| size | 10.6 MB | 957 bytes |
| `owner` / `ownerKind` / `primary` | `"Zach"` / `person` / `true` | `"Zach"` / `person` / `true` |
| `inhabitants` | `["Player"]` | — |

The twin was minted in `62a391f9` (09-07). Its parent commit has the real Home owned by `"Player"`. At that boot, `EngineInit.cpp:274` called `ensureHomeZone("Zach")`; `findPrimaryHome` (`ZoneManager.cpp:375`) compares `owner() == personId` as strings and missed; the unowned-claim branch (`:403`) missed because the owner was not empty; the slug `Home` was taken; `:419` minted `Home_of_Zach`. The rename path in `PersonSerialization.cpp:78-83` then relabelled `Player` → `Zach` on the original. Nothing retires a Zone, and `persistZones` writes every Zone it holds, so the twin is rewritten on every save. Zach lands in the real Home today only because `SaveSystem::listHomeIdentityRecords` sorts (`SaveSystem.cpp:1142`) and `Home` < `Home_of_Zach`.

## Why the fix is not "relabel the owner"

`Person.hpp:110`: `getIdentifier()` returns `_personId.toString()` once `canAuthenticate()`, else the display name. No Person has a key today (`SingularId::Kind::Key` is never assigned outside `src/Identity/`; `KeyStore`, `IdentityLedger`, `PersonMigration` have zero callers). The day one does, the identifier becomes `did:…`, both houses miss, and a third is minted. A relabel to the new spelling is the same bug one migration later.

## The shape of the fix (not a plan; the direction the programme's §11 allows)

One Relation family, three edges, no new class:

<a id="ownership"></a>
1. **`owned-by`** (Zone → Person), resolved through `SingularId`, with display names as spellings a Person may type to point at the Person. `Zone::_ownerId` becomes a cache of the Relation's target identity, or goes away. `findPrimaryHome` resolves the Person first, then matches the Relation.

<a id="dwelling"></a>
2. **`dwelling-of`** (Zone → Person). Today `Home` is a C++ subclass (`HomesOfEarth/Home.hpp:27`) and `isHome()` is decided by constructor, so a Zone cannot become a Home or stop being one. The manifesto's sentence is a predicate: *a Home is a Zone that is a digital dwelling space for at least one Person*. With the Relation, Zone→Home is adding the edge; Home→Zone is dissolving it under `PRIMARY_AND_SUB_RELATIONS.md` §7. The kernel guards on a dwelling (`entryRequiresWill`, `cannotForceStay`) stay in C++ and key off the Relation, not the type.

<a id="presence"></a>
3. **`present-in`** (Person → Zone). `ZoneManager::_currentIndex` (`ZoneManager.hpp:28`) is the whole model of presence. `Person::_joinedZones`, `joinZone`, `leaveZone` (`Person.cpp:262-284`) already hold a vector and publish `person-joined-zone` / `person-left-zone` edge events; **zero callers** anywhere in `src/`. Wire `switchTo` through them before designing anything new. Overlapping jurisdiction between Zones stays ⚑ AUTHOR (To-Do, *Zone jurisdiction resolution*); simultaneous membership is the half that needs no decision.

<a id="multihome"></a>
4. **Multi-Home policy.** `primary` is a per-Zone quality with no uniqueness, which is how two primaries with one owner already exist. Decide (⚑ AUTHOR, Zach): is "primary" unique per Person, and if a Person owns several Homes, which one does boot return to? Until decided, the loader should at least refuse loudly when it finds two primaries for one owner rather than picking by sort.

## Order of work

1. `PersonMigration` gets a caller and Zach's Person gets a key, in a branch, with a test that boots the real `saves/` tree copied into a sandbox and asserts **no new Home directory appears**. This test must fail before the Relation work and pass after; that is the whole point of it.
2. `owned-by` lands; `findPrimaryHome` resolves through it; the two-primaries case refuses loudly.
3. Retire `saves/homes/Home_of_Zach/` **only with Zach's written authorization in the Person Verification List** (section *Two Homes*). It is a save file with his name on it.
4. `dwelling-of` and `present-in`, in that order, each with the boot-and-return witness.

## Pitfalls for whoever picks this up (Jules especially)

- Do not touch `saves/homes/` in a PR. Sacred files; owner authorization required; see `CLAUDE.md` non-negotiables.
- Do not "fix" by editing `owner` strings in the JSON. That is the third relabel.
- Do not add a `HomeKind` enum or a `ZoneRole` enum. Append-only enums are for the engine's own kinds; dwelling is authored (Refusal 3).
- Do not weaken the kernel-lock in `ZoneManager::ensureHomeZone` to make the twin go away. Feed it the right key.
- Do not write a test that constructs two Zones and asserts on them. The failure only exists on the booted real tree with the real Person file (`saves/persons/Zach.ecform`, which has `displayName` and `soulName` and no key). Copy that tree into a sandbox; `zone_native_save_isolation_test` shows the `SaveSystem::setSaveRoot` pattern (and is itself currently trapping in `free()` inside `persistZone`, Bugs.md #28; fix or route around it first).
- The name-twin disease is wider than Homes: `saves/zones/` holds `Basic 2D Button Zone` and `Basic2DButtonZone`, `Go` and `Go Game`. `applyMatterFlatBuffer` already refuses to guess between them. The same identity-not-spelling fix applies; do not special-case Homes.
