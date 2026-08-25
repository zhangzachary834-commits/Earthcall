# Structural Debt from the Week of 2026-08-17 → 08-24 — Handoff

**Status**: Open (2026-08-24). Found during a weekly review of `4265279e..ce5c1cbe`
commissioned by Zach. Each item below is small, verified against the tree, and independent —
take them in any order. The one large finding from that review has its own doc:
[Zone_Relation_Graph_Loss.md](Zone_Relation_Graph_Loss.md).

---

## 1. `Singular` copy/move slicing — fixed, unguarded, and mis-labelled

**Priority: highest in this file.** `ce5c1cbe` ("Attempt to fix chess lag") contains no
performance change. What it actually contains is a fix to the most serious latent bug in the
ontology's root class. Before it, `Singular.cpp`:

```cpp
Singular::Singular(const Singular& o) : _telosId(o._telosId) {}
```

Copying **any** `Singular` silently dropped `designatedZones`, `_stakeholders`,
`_dynamicProperties`, `_dataStructures`, and `name`. Every authored property, gone, on any
copy — and `Zone`, `Relation`, and `Object` all have copy constructors that run through it.
The to-do list already recorded "Singular copy drops the map" as a known gap under authored
properties (2026-08-23); this is that gap, and it is now closed in code.

**Done (2026-08-24):**
- [x] Regression test: `tests/constructed-being/singular_copy_move_test.cpp` — a concrete
      `Singular` subclass, dynamic properties of five `PropertyValue` kinds
      (`int`/`float`/`bool`/`std::string`/`glm::vec3`), `name` / `designatedZones` /
      `_stakeholders` / a `DataStructure` / `_telosId`, all copy-constructed, copy-assigned,
      move-constructed, and move-assigned (50 checks). Confirmed 36/50 fail on `ce5c1cbe^`
      (via `git worktree add` against that commit) before confirming 50/50 pass here.
- [x] Added to `Bugs.md` #8 with the real title ("`Singular` copy and move silently dropped
      every authored field"), marked done and verified.

## 2. `applyAndMaybeDrive` — right change, possible new hot loop

Same commit removed `if (!record.changedSomething()) return;` from
`LawManager::applyAndMaybeDrive` (`Law.cpp:1755`). **This is correct** and should stay:
`maybeStartDriveSession` says so itself — *"Driving is the law's AUTHORED choice, not an
inference from what it reads."* Inferring intent from writes was the bug.

But `maybeStartDriveSession` (`Law.cpp:1886`) now runs a **linear scan of `_driveSessions`**
on every application of every driving law, where before it usually returned early. In a
104-object / 35-law zone that is a plausible source of the very lag the commit is named for.

- [ ] If chess still lags after the relation graph is restored, profile here first. The fix is
      an index keyed on `(lawId, subjectId)`, not restoring the guard.

## 3. `src/Time/` — a top-level directory holding an empty class

```cpp
// src/Time/Time.h
// Placeholder for a first-order vessel of Time in Earthcall
class Time { };
```

Refusal 2: the top level **is** the ontology, and `CLAUDE.md`'s tree lists six nouns — Time is
not among them. An empty placeholder class staking out a directory is exactly the standard
move the repo refuses.

`Moment` is a different matter and should survive: a `Moment` as a `Singular` whose bounds are
exact `OntoMath::ScalarForm` with a `double` cache for cheap ordering is real ontology, not a
domain noun, and `Kind { Instant = 0, Interval = 1 }` is correctly marked append-only.

**Update, same day:** Zach wrote intent into the file while this review was being drafted —
*"Ok so we need a robust philosophy of time. Think: branch of high-level metaphysics that deals
with time."* That settles the ⚑ AUTHOR question in one direction: Time is claimed, not
abandoned. **Do not delete `Time.h`.** It also matches the standing to-do item — *"do not start
by unifying clocks… write what a* when *is first, the way `Formation` already says what a set and
a category are"* — so the empty class is a placeholder for a doc that has not been written, not
for code that has not been typed.

- [ ] Write the *when* first (`docs/architecture/` — the Time framework), then let the class
      shape follow it. `Moment` is the one piece that already exists and already earns its place.
- [ ] Add `Time` to the tree in `CLAUDE.md` § The tree, so the doc and the directory stop
      disagreeing. Whatever else is true, a top-level directory the ontology file does not list
      is drift, and the next agent to read Refusal #2 will try to delete it — as this review
      nearly did.
- [x] Either way: `src/Time/Time.cpp`, `Time.h`, `Moment.cpp`, `Moment.hpp` are pasted into the
      middle of `IMGUI_SOURCES` at `CMakeLists.txt:145-148`. `GLOB_RECURSE` already collects
      them; CMake dedupes within `earthcall_core` so it links, but this is an IDE writing into
      the wrong list. Remove those four lines.
      *Done and verified: Cleaned IMGUI_SOURCES in CMakeLists.txt (2026-08-25).*
- [ ] `src/Time/Duration/` is an empty directory. Remove it or fill it.

## 4. A tracked symlink is holding the Relation move together

```
src/ConstructedBeing/Singular/Object/Formation -> ../../../Relation/Formation   (git mode 120000)
```

`3b47e580` moved Formation to `src/Relation/Formation/` — the right claim, since a Formation is
Relation-shaped, not a part of Object. But **21 files still include the old path**, and a
committed symlink is holding them up: `Zone.hpp`, `Person.hpp`, `Law.hpp`, `Material.hpp`,
`Ourverse.hpp`, `Singular.cpp`, `ConditionModel.cpp`, `JoyHierarchy.cpp`, `LawGraphWindow.cpp`,
`Tool.hpp`, `Body.hpp/.cpp`, `BodyPart.hpp`, `Community.hpp`, `PropertyValueJson.cpp`, and six
tests.

In a repo where the directory tree *is* the ontological claim, this commits the claim and its
denial in the same tree. It also will not survive a checkout that does not honour symlinks.

- [x] `grep -rl "Object/Formation/Formation.hpp" src tests` → rewrite to
      `Relation/Formation/Formation.hpp`, then `git rm` the symlink and reconfigure.
      Verify: full build + `ctest`. Roughly twenty minutes; do it before the include graph sets.
      *Done and verified: Rewrote all includes across src/, tests/, and scratch/ to Relation/Formation/Formation.hpp and Menu headers, git rm of symlink completed, full build and 65/65 ctest passed (2026-08-25).*

## 5. ~480 volatile identifiers on a chess load

```
WARNING: Object initialized without a stable string identifier.
         Assigned volatile ID 'object-459'. This object should not be
         reliably targeted by Law text.
```

The pieces the laws address (`piece-white-pawn-4-1`) do carry stable slugs, so this is not what
breaks `chess_app_test`. But it is the **Stable identifiers** non-negotiable failing at scale
in the one world most dependent on law-text addressing, and it buries every other diagnostic
printed during a load.

- [x] Find which beings are minted without a slug on this path (board squares? geometry
      children?) and either give them stable slugs or stop registering them as addressable.
- [x] Collapse the warning to one summary line per load — *"142 beings took volatile ids"* —
      with the full list behind a flag.
      *Done and verified: Passed stored object IDs directly to Object constructors during JSON deserialization (`zoneObjectsFromJson`), gave CategoryManager, EngineInit baseline objects, and BodyPart structured stable IDs, added verbose flag + atomic volatile ID counting in `ObjectIdentity.hpp`, and added summary line to `ZoneManager` load report. Eliminated ~480 warning spam lines down to a single clean load report; verified full build, 65/65 ctest (100%), and lag probe (0 broken invariants, 0 regressions) (2026-08-25).*

## 6. The chess authoring scripts are a graveyard

```
scripts/  author_chess.py  author_chess_clean.py  author_chess_first_mover.py
          chess_generator.py  generate_chess_v2.py  generate_chess_v3.py  generate_chess_v4.py
scratch/  generate_chess_v5.py  generate_chess_v7.py  rewrite_v3.py  rewrite_v4.py
          rewrite_v7.py  patch_chess_v2.py  fix_grok.py  read_chess.py
```

Seven numbered generators across two directories; `generate_chess_v2/v3/v4.py` are byte-identical
at 347 lines. Exactly one of these is the First Mover of record for `saves/worlds/chess.json`, and
from outside there is no way to tell which — which matters more here than in an ordinary repo,
because authorship is meant to be traceable.

- [x] Keep the one that authored the shipped world, name it so in a header comment, move the rest
      to `scratch/attic/`. `scratch/` is a workshop; `scripts/` should not hold v2 through v4.
      *Done and verified: Kept `scripts/author_chess.py` (authored `saves/worlds/chess_app.json`) and `scripts/author_chess_clean.py` (authored `saves/worlds/chess_first_mover.json`) with clear provenance header comments; moved redundant v2–v7 generator/rewrite scripts to `scratch/attic/` via git mv (2026-08-25).*

## 7. Three near-identical documentation trees

```
docs/Reflections on Earthcall's Progression/{Reflections on Repo State, Reflections on Trajectory, ...}
docs/Reflections on Trends and Directions/{Reflections on Repo State, Reflections on Trajectory}
docs/Trends and Directions/{Reflections on Repo State}
```

Same subfolder names under three different parents; cross-links in the to-do list already point
into two of them.

This is not cosmetic — it has already broken links. Four of the five dead relative links in
`docs/Agenda/Tasks/` point into these trees:

```
To-do list.md -> ../../Reflections on Trends and Directions/Reflections on Trajectory/The_Second_Person_and_the_Speed_of_Frameworks.md
To-do list.md -> ../../Reflections on Trends and Directions/Reflections on Trajectory/The_Walk_Writes_Back.md   (x2)
To-do list.md -> ../../../agent intercom/Claude's monestary/The_Gap_In_The_Ground.md   ("monestary" was renamed to "Monastery" in 3b47e580)
Bugs.md       -> Specific Tasks/Self_Lifting_Floor_Bug.md   (missing the ../)
```

All five predate this review and none were introduced by it.

- [ ] Pick one parent, move the rest under it, fix the relative links. **⚑ AUTHOR — the naming
      is Zach's to choose.**
- [ ] Fold a link-resolver into the **Mechanize router truth** probe already on the Housekeeping
      list. It was scoped to backticked paths in the router trio; broken markdown links in
      `docs/Agenda/` are the same drift, catchable by the same twenty lines.

## 8. `saves/` is 37 MB of unreviewable blobs in git

`saves/worlds/my_world.json` alone is 14 MB; `saves/backups/` is 13 MB. `my_world.json` churned
161,992 lines in `a71042db` and 87,915 in `ce5c1cbe`. Those diffs cannot be read, reviewed, or
merged — and they sit directly on top of the subsystem Zach marked CRITICAL.

- [ ] Not urgent, but decide before it is: `.gitignore` the derived `.ecsave`/backup artifacts and
      keep one small canonical fixture world under version control, **or** commit to the blobs
      deliberately and say so in `BUILD_AND_ENVIRONMENT.md`. Silence is the worst of the three.

---

## Recurring pattern worth a decision — pointer plus shadow string

Three places grew the same shape this week, each for a good local reason (deserialization must
resolve later), each hand-rolled:

| Being | Pointer | Shadow string | Reconciled? |
|---|---|---|---|
| `Relation` | `_a` / `_b` | `_savedA` / `_savedB` | Yes — cleared on `bind()` |
| `Home` | `_stakes` (`Formation`) | `_stakeIds` | **No** |
| `Home` | — | `_inhabitants` | n/a |

Each is now a being that can be in two states — bound and unbound — where every reader must know
which. `Relation` handles it; `Home` never reconciles `_stakes` against `_stakeIds` at all.

- [ ] If this is going to be the pattern, give it one named type with one invariant and one
      resolver hook, rather than three versions. If it is not, fix `Home`'s now — while there are
      seven Zones instead of seven hundred. **⚑ AUTHOR — which way this goes is Zach's.**

---

## Attribution

Findings from a weekly review commissioned by Zach on 2026-08-24, with the standing instruction
to reach an independent position before reading any other model's write-up — no
`docs/Reflections on…` file was opened. The `Singular` slicing fix and the `applyAndMaybeDrive`
change are Zach's own (`ce5c1cbe`); what is added here is that the first needs a guard and the
second needs a profile. The Relation reforge (`18ffc9f5`) and the Home kernel guards
(`a71042db`) were examined and are sound — the only note against the reforge is §"Relation.type
is still a string tag", which Zach's own commit already names in the header comment and which is
tracked in the to-do list, not here. The remaining items are mine.
