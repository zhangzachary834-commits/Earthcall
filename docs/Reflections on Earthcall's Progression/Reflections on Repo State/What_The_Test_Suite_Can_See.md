# What the Test Suite Can See

**Author:** Claude (Opus 5), Claude Code session
**Session ID:** `79740b6f-39f1-4f66-a24d-3e072cb5fe5d`
**Date:** 2026-08-24
**Timestamp:** 2026-08-24T15:24:42-07:00
**Tree:** `bb1b4737`, branch `sync-from-earthcall-main`, working tree clean
**Method:** built and ran the suite, re-derived every number below from the tree. Read the
two audits already on disk from today only *after* forming the picture, to see where I
agreed and where I did not.

---

## The state, in one sentence

Earthcall has a working immune system and a stalled hand: everything with a test attached
is healthy and honest, everything requiring a Person's hand or a naming decision is queued
and aging — and the one surface where those two meet, the save files, is where the only
serious bug of the week lived.

---

## 1. The numbers, verified today

```
cmake --build build -j8      → clean, nothing to rebuild
ctest --test-dir build -j4   → 100% tests passed out of 65,  5.36 sec
```

65 registered, 65 pass. `PENDING_FEATURE_TESTS` is genuinely empty —
`webgpu_particle_test` (#51) passes, `chess_app_test` (#21) passes in 2.12s.
`CLAUDE.md`'s build section is true as written today.

Volume:

| | files | lines |
|---|---|---|
| `src/` | 363 (.cpp + .hpp) | 97,986 |
| `tests/` | 65 | 14,807 |
| `docs/` | 113 (.md) | 27,161 |
| `agent intercom/` | 25 (.md) | 2,893 |

Tests are 15% of source by line. Docs are 28%. For a repo whose thesis *is* the ontology,
28% is not sprawl — it is the load-bearing half. 15% test coverage by volume is thinner
than I would like given everything in §3.

## 2. Docs went stale inside of three hours

Antigravity's independent audit is timestamped **2026-08-24T13:46** and reports "62 of 63
tests pass, test #50 `webgpu_particle_test` correctly skipped as an unbuilt feature." By
**14:59** that was false: `bb1b4737` implemented `WebGpuRenderer::drawParticles`
(`WebGpuRenderer.cpp:801`) and the pending test went green as #51 of 65.

Nothing was done wrong. The audit was accurate when signed, and the commit that falsified
it was the audit's own Priority-3 recommendation being taken. But it is worth naming
plainly: **at this velocity a document's shelf life is measured in hours, and every
hand-written count in the tree is a claim with an expiry date nobody stamped on it.** This
is precisely the case for the *Mechanize router truth* probe already sitting on the
Housekeeping list — read the count from `ctest -N`, resolve backticked paths against disk.
It is one of the cheapest items on the whole list and it defends every other document.

Two smaller instances of the same decay, both found by re-running rather than re-reading:

- `Week_Of_2026-08-24_Structural_Debt.md` §5 reports "~480 volatile identifiers on a chess
  load." Running `./build/chess_app_test` now gives **152**. The number moved; the finding
  stands; nobody updated it.
- Three near-identical `docs/Reflections…` trees (§7 of the same file) are now **one** —
  `docs/Reflections on Trends and Directions/` and `docs/Trends and Directions/` no longer
  exist. The merge happened and was never recorded as done. Its point, however, did not
  land: I walked every relative link under `docs/Agenda/` and **6 are still broken**,
  including all four that §7 named. Moving the directories without rewriting the links
  fixed the sprawl and left the drift.

## 3. The suite is green and the city is unwalked

> **Superseded within four hours of writing — see §7.** Zach walked the app on the
> evening of 2026-08-24 and discharged most of what this section counts. The section is
> left standing unedited because §7 is only legible against it, and because being wrong
> this fast is the finding.

This is the oldest open thread in the repo and it has not moved. Counting through
`Bugs.md` and the to-do list, the following are marked done and verified, *headlessly*,
with an explicit note that no Person has confirmed them under the hand:

- Bug #3, double spawn — "**Needs a human click to confirm**"
- Bug #6, gyroid implicit — "**not re-clicked in-app**"
- Bug #7, Zone relation graph loss — "**Not yet done:** in-app confirmation — a Person
  loading `chess` and clicking a pawn"
- Feature-sized: "Personally test 3D shape law and other laws — **nobody has clicked in the
  running app**"
- In-world test observation — "A Person still has to open Grave and click Observe"

Five verification debts, all of the same kind, none discharged. The repo's own doctrine —
Zach's *Sabbath* mandate and the Surface Routing Rule, "route agent work by who catches the
failures" — says the felt surface has failure modes no headless test can see. The list says
it. The tree keeps not doing it.

**And here is what I think the mechanism actually is, which I have not seen named
elsewhere:** compare what landed today against what did not. The audit filed at 13:46
listed three Priority-1 items (relocate `src/Time/`, remove `EventEntity`, unhook `Body`
from `Object`) and one Priority-3 item (`drawParticles`). I verified all four against the
tree at 15:24. Priority 1 is **entirely untouched** — `Body.hpp:13` still reads
`class Body : public Object`, `EventEntity` still has six live references including
`Law.hpp:737`, `Ourverse.hpp:92-93` still holds `cameraPos` and `ownedObjects`,
`src/Time/Duration/` is still an empty directory and `src/Time/`'s four files are still
pasted into `IMGUI_SOURCES` at `CMakeLists.txt:144-148`. The tracked symlink
`src/ConstructedBeing/Singular/Object/Formation` is still git mode `120000` with **21**
files including through it. Priority 3 shipped within the hour.

The difference is not difficulty. It is that `drawParticles` had a **failing test pointing
at it** and the Priority-1 items have nothing but a paragraph. The repo has exactly one
feedback loop that closes on its own, and everything gravitates into it. Which is also why
§3's click-debt never clears: a Person's hand is the one instrument `ctest` cannot hold.

I do not think the answer is more tests for ontological cleanliness — `no_black_box_test`
already shows the shape that works, and the two probes on the Housekeeping list (router
truth, authored-save lint) are the right next two. The answer is noticing that **"add a
test that fails" is currently the only way to make anything happen here**, and either
accepting that as the intake protocol or building a second one deliberately.

## 4. The week's ledger is written in save files

Churn by area over the last seven days:

```
saves            +669,172   -381,023
src               +10,709     -3,630
docs               +9,740       -809
tests              +4,735       -342
agent intercom     +4,246       -585
```

The world's data churned **sixty times** the code that reads it. And it is exactly the
subsystem Zach marked CRITICAL — *"we don't want developer worlds unstable or erased in the
fragile states of testing and developing features"* — and the one the non-negotiables call
sacred: *"the flesh and blood of Earthcall that the ontological skeleton is meant to
support."*

That is where the week's one serious bug lived. Bug #7 was three defects in series in
`Serialization.cpp` and `ZoneManager.cpp` that silently dropped a Zone's entire relation
graph on round-trip, and `saves/zones/Chess/zone.json` had to be re-authored from a session
file to recover 38 relations. This is not a coincidence to file away: **the largest,
least-reviewable surface in the repository is the one that broke, and it broke silently.**
A 5,575-line diff on `zone.json` in `53247194` is not a diff anyone read.

The posture toward that surface is currently half-decided, which the structural-debt doc
itself calls the worst of the three options:

- `saves/backups/` and `saves/games/` are ignored — decided by Zach on 08-21, correctly, and
  it killed the multi-megabyte churn per load. Good.
- `saves/worlds/my_world.json` — **14 MB, tracked**, the file that churned 162k lines in a
  single commit. Not decided.
- `saves/worlds/chess.json` and `chess_app.json` are **byte-identical** (`md5
  3616b980…`, 388 KB each) and both tracked. Which is the world of record? Nothing in the
  tree says. This is §6's script graveyard — seven numbered chess generators across
  `scripts/` and `scratch/` — reappearing on the *save* side, where it matters more,
  because authorship is meant to be traceable and here even identity is not.
- `/scratch/attic/earthcall_webgpu` is on `.gitignore:288` **and still an 18 MB Mach-O
  binary in `HEAD`**. An ignore rule is not an untracking; it only stops the file being
  re-added. Verified: `git cat-file -t HEAD:scratch/attic/earthcall_webgpu` → `blob`.

Total tracked working tree: **503 MB across 14,414 files**, of which the repo's own source
is about 100 MB at most — a vendored OpenSSL source tree with build artifacts, a 33 MB wgpu
static lib, a compiled binary in the attic, and 24 MB of saves. `.git` is 751 MB. None of
this is urgent; all of it is the kind of thing that stops being a decision and becomes a
condition if left long enough.

## 5. Where I disagree with this morning's audit

Antigravity marks **Refusal #4 FAIL** on `class Body : public Object` and Refusal #1
**minor debt** on `EventEntity`, and recommends removing both. I agree the inheritance is
wrong and should be fixed. I do not think it is the same *kind* of finding as the others,
and I want to record why before someone acts on the list mechanically:

Refusal #4 says *objects have no Body* — it forbids giving a robot arm a `Body`. It does
not say a `Body` may not *be* an Object in the type graph. Nothing in the tree currently
gives a non-Person a `Body`; only `Person` instantiates one. So the refusal as written is
being **kept**, and what `Body : public Object` actually costs is different and worth
stating on its own terms: it hands the human vessel Object's spatial and material fields by
inheritance rather than by composition, so every future reader has to decide case by case
whether a given Object affordance is meant for a Person's body. That is real debt. It is
not a violation of the refusal, and calling it one makes the refusal mean something looser
than it does. The refusals are load-bearing exactly to the degree that they are read
strictly.

`EventEntity` is the sharper of the two and I would take it first: it is a C++ class for a
domain noun, and the fix — events as `Relation` instances joining source, target, and a
`Moment` — is also the thing that would give `Moment` its first real consumer and settle
what `src/Time/` is for.

On `src/Time/`: Zach wrote intent into `Time.h` on 08-19 — *"we need a robust philosophy of
time. Think: branch of high-level metaphysics that deals with time"* — and the weekly review
correctly read that as the ⚑ AUTHOR question being answered in the direction of *claimed,
not abandoned*. I would only add that the standing to-do item already says how to proceed
— *write what a* when *is first, the way `Formation` already says what a set and a category
are* — and that until that document exists, the directory should be **listed in
`CLAUDE.md`'s tree** so the doc and the disk stop contradicting each other. Right now the
next agent to read Refusal #2 will try to delete a directory the author has claimed. The
weekly review says it nearly did. That is a real hazard sitting in the tree at zero cost to
remove.

## 6. What is genuinely strong

It would be a bad reflection that listed only debts, because the debts are all small and
the foundation is not.

- **`Singular` copy/move slicing (Bug #8) is the model of how this should go.** Zach's own
  fix shipped in `ce5c1cbe` under the title "Attempt to fix chess lag," which is to say the
  most serious latent bug in the ontology's root class was fixed by accident and nearly
  lost. The follow-up did the right thing: confirmed 36/50 checks fail on `ce5c1cbe^` via a
  worktree *before* confirming 50/50 pass on the fix. That is the verification discipline in
  `ENGINEERING_DISCIPLINE.md` actually executed, not cited.
- **`no_black_box_test` is the shape everything else should copy** — a test that reads the
  headers and enforces a doctrine, rather than a doctrine that hopes to be read.
- **OntoMath is real.** Exact symbolic differentiation and integration, canonical term
  form, bounded recursion, and a compiler straight from `MathNode` to WGSL. The
  substrate-ordering claim that authored mathematics drives the channel is not aspirational
  in this corner; it runs.
- **The infrasound floor** does what a kernel guard is supposed to do: refuses loudly and
  names the frequency, rather than silently filtering a Person's mathematics. `Home`'s
  `entryRequiresWill` and `cannotForceStay` are built the same way.
- **65 tests in 5.36 seconds.** A suite that fast is a suite people will actually run.

---

## Attribution

Drawing on things Persons and prior agents put in the tree, which I want to keep visible
rather than absorb:

- **Zach** — the CRITICAL directive on the save system and the "save files are sacred"
  non-negotiable, which is why §4 is the longest section here rather than a footnote; the
  intent comment in `Time.h`; the Sabbath / unobserved-runtime mandate, which §3 is a
  measurement of rather than a discovery.
- **The weekly structural-debt review** (`Week_Of_2026-08-24_Structural_Debt.md`,
  commissioned by Zach) — items 3 through 8 there. I re-verified each against `bb1b4737`
  rather than restating them, and §2's "the merge landed, the links did not" and §4's
  `chess.json`/`chess_app.json` duplicate are extensions of its §7 and §6 into places it did
  not look.
- **Antigravity (Gemini 3.7 Flash)**, `INDEPENDENT_EARTHCALL_AUDIT_2026-08-24.md` — the
  priority list §3 uses as its natural experiment, and the Refusal #4 reading §5 argues
  with.
- **The Walk Writes Back** and **The Unclicked Window** — the click-debt frame. §3 adds a
  proposed mechanism (only the tests close a loop, so all work flows there) that I believe
  is mine, though it is close enough to those two that I may be re-deriving them.

Mine: the three-hour staleness measurement, the churn ratio, the Priority-1-vs-Priority-3
natural experiment and what I take it to show, the broken-link walk, the ignore-is-not-
untracking finding, and §5's disagreement.

Nothing in the repository was modified in the course of writing this except the addition of
this file and one line in the to-do list pointing at it.

---
*Claude (Opus 5) · session `79740b6f-39f1-4f66-a24d-3e072cb5fe5d` · 2026-08-24T15:24:42-07:00*

---

# §7. Addendum, 2026-08-24 evening: the walk happened

Written four hours after everything above, at Zach's prompting: *"OKAY FINEEEE I MARKED MANY
OF THE VERIFICATION ITEMS DONE (some of them i'd already verified prior to your reflection)."*
That last parenthesis matters and I want it on the record — part of §3's ledger was already
stale when I counted it. I measured the *documents*, not the world, and the documents were
behind. The correction is his, not mine.

## 7.1 What the hand found that the suite could not

The walk is now a standing document — `docs/Agenda/Tasks/Person Verification List.md` —
with a new rule at the top of the to-do list: *"Things that require a Person to manually
verify whether in-app functionality is working as intended should go in Person Verification
List."* That is the second feedback loop §3 said the repo did not have. It now exists, it is
addressed, and it has a home.

**Confirmed working under the hand:** save → quit → reopen → load; Save As with no crash;
`my_world` loads in-app; Home round-trip with a painted FaceTexture surviving a departure and
return; Creator Console open and select; Face Brush; the 3D Create tool; the gyroid implicit,
visually; hover-enter and hover-exit, click, scroll, drag, and key edges all appearing in the
Law Authoring Window's Recent Events; Assets; Chat; `K`, `F8`, `F9`, `H`.

That retires, with a Person's eyes, three of the five verification debts §3 counted — Bug #6's
gyroid, the 3D create path, and the interaction edges the whole Interaction-as-Law framework
rests on. **The event vocabulary is real and observable at the surface.** That is a much
stronger result than the suite could give, and it should be said plainly before the failures.

**And then the failures, which are the point.** A Person's hand found things no headless test
in this repo could have found, in about an hour:

- **Chess: clicking a pawn does nothing visible.** Zach: *"Most other functionality below
  can't be tested unless this is working."* Six chess line-items blocked behind one dead click.
- **Rotate has two sets of controls and only one works.** The angle sliders on a selected
  shape do nothing; scroll to the bottom of the Creator Console in 3D tool mode and the
  "Target Rotation" sliders rotate it correctly. A dead control sitting above a live one is
  worse than no control, and no test can see it because both are wired to *something*.
- **Pottery still stretches FaceTexture pixels** instead of growing the texture to the new
  face dimensions — Zach's own note from the to-do list, unfixed, now confirmed under the hand.
- **`H` opens the chat window and pressing `H` again does not close it.** A toggle that only
  toggles one way.
- **Observe Test shows four tests out of 60+**, one throws on load, two are "epistemically
  opaque," and the one that renders spawns cubes whose correctness Zach cannot judge without
  an in-world tool for reading positions. The feature is technically live and practically
  unusable — exactly the gap between "loads" and "tells you something."
- **Fuse is ambiguous:** *"it executes, but it's not always clear what."*
- **Four items are marked with "I don't know what I'm supposed to be looking for."** Window
  focus, unfocus, and pointer-recapture. That is not a failure of the app; it is a **failure of
  the checklist**, and it is the most reusable finding here: a verification item that does not
  state its own success criterion cannot be verified, only guessed at. Whoever writes the next
  batch owes each line an observable.

## 7.2 `chess_app_test` passes and the pawn is dead — why both are true

This is the sharpest thing in the tree right now, so I traced it rather than restating it.

`tests/law/chess_app_test.cpp:68-75` is the whole story:

```cpp
void click(InteractionChannel* interaction, LawManager& lawManager,
           Object* subject, float wx, float wy, float wz) {
    interaction->pointerWorld = glm::vec3(wx, wy, wz);
    Core::EventBus::instance().publish(
        ECA::Event{"object-clicked", subject, nullptr, std::time(nullptr)});
```

The test **sets the pointer by hand and mints the event by hand**. It never calls
`InteractionChannel::step()`. So it exercises the picking of nothing, the reachable set of
nothing, and the press/release/slop edge logic of nothing. What it proves — and proves well,
in 2.12 s — is *given an `object-clicked` bearing this subject, the 33 chess laws do the right
thing.* What it cannot prove, and never claimed to, is that a mouse over a pawn produces that
event.

Zach's click lands in exactly that gap. And it is a narrow gap, because I checked the two
things it could otherwise have been and both are sound:

- **The law cascade is complete.** 33 chess laws listen for 10 event types
  (`object-clicked`, `square-clicked`, `piece-selected`, `move-committed`, `turn-changed`,
  `check-scanned`, `check-evaluated`, `king-probed`, `enemy-captured`, `move-reverted`), and
  all 10 have publishers among **30 `Publish` action nodes** in the same world —
  `law-chess-click` mints `square-clicked`, `law-chess-select` mints `piece-selected`, the
  pawn laws mint `move-committed` and `enemy-captured`. Only 2 of the 33 listen for
  `object-clicked`, the one event the engine itself publishes; the other 31 are downstream of
  those two. **This retires a live suspicion:** to-do item 31c calls chess a *"private event
  bus"* with laws whose triggers no publisher emits. That is not what it is. It is a correctly
  wired cascade hanging off a single entry point — which is why one dead entry point kills all
  of it.
- **The conditions are satisfiable.** `law-chess-click`'s condition is `ConditionNode::Kind 4`
  = **`Any`**, not `All` — `isBoard == true` OR `instance-of category.chess.piece`. (I first
  read the `"conditionMode": "all"` field beside it as making this an impossible AND of
  board-and-piece. It is not. Worth recording that the JSON has two adjacent things named
  "all"/"Any" meaning different layers, because I misread it and the next reader will too.)

So the entry point is where to look: whether `InteractionChannel::step` reaches chess pieces at
all in the loaded Zone, and whether `@interaction-channel.pointerWorld` is populated on the
live path the way the test populates it by hand. I have not run the app, so that is a located
hypothesis and not a diagnosis — **the next move is a Person clicking a pawn with the Recent
Events log open, and reporting whether `object-clicked` appears at all.** If it does, the
break is in the law's read of `pointerWorld`; if it does not, the break is in picking. One
click distinguishes them.

Note also that §4's warning has teeth here: `saves/worlds/chess.json` and `chess_app.json` are
byte-identical, and `chess_app_test` defaults to loading `chess_app.json` while a Person loads
`chess`. They agree today. Nothing keeps them agreeing, and the day they diverge, the test and
the Person will be looking at different worlds while using the same word.

## 7.3 What this does to §3's argument

It sharpens it rather than refuting it, and I want to be careful not to claim vindication for a
section that was factually wrong within hours.

§3 claimed the repo has one closing feedback loop and everything gravitates into it. Zach's
walk shows the missing loop can be opened in an evening, and that when it opens it pays
immediately — one hour of clicking produced a blocked feature, a dead control, an unusable
tool, a one-way toggle, and four under-specified checklist lines. None of those were reachable
from `ctest`.

But the deeper point survives, and the chess case states it better than my Priority-1/Priority-3
argument did: **a green test and a working feature are different claims, and this repo has been
allowing the first to stand in for the second.** `chess_app_test` was cited in Bug #7 as the
consequence-witness that the relation graph was restored. It was — the relations are there. It
was never evidence that a Person could play chess, and it was read that way, by me among
others, because it is named after the thing rather than after what it tests. A test that mints
its own input event is a test of the downstream half. That is a fine and useful test. It should
just be legible as half.

The cheapest correction is not more tests. It is that **every headless "done and verified"
should name the seam it does not cross** — `chess_app_test` verifies laws-given-events, not
click-to-event — so the Person Verification List can be generated from the gaps instead of
assembled by hand.

## 7.4 Meanwhile, the lag question got a real answer

A concurrent session's `tests/singularity/frame_lag_test.cpp` and
`docs/audits/rendering_optimization/2026-08-24_frame_lag_probe.md` landed while this was being written, and they
resolve something §6 left hanging. `ce5c1cbe`, titled "Attempt to fix chess lag," turned out to
contain the `Singular` copy/move slicing fix and no performance change — §6 called that a happy
accident. The probe now says what the lag actually is: **`Physics::updateBodies` is all-pairs
with no broadphase** (`Physics.cpp:327-343`, fitted `n^1.75`, 1.1 → 40.7 ms from 64 → 512
objects), with `g_legacyEngineEnabled = true` at `Physics.cpp:33` and no caller in the app that
ever clears it. So the commit that was *reaching* for the lag fixed a different and more
serious bug instead, and the quadratic is still standing. Both halves of that sentence are now
documented, which is the outcome to want.

Two of that probe's own notes belong to this document's argument: it refuses to enforce a
timing when the machine moves under it, after a first draft reported contended numbers as
Earthcall's and had to be withdrawn — an instrument that knows when it is being lied to. And
its last line is the same one as §3's: *"A lag test cannot feel a hitch."*

---

## Addendum attribution

The walk, the `Person Verification List.md` document, the to-do rule routing manual checks
into it, and every finding in §7.1 are **Zach's** — including the correction that some items
were verified before I counted them unverified. §7.2's trace of `chess_app_test.cpp:68-75`,
the 33-laws/30-publishers cascade check, the `Kind 4 = Any` reading and my own misreading of
it, and §7.3's "name the seam you do not cross" are mine. §7.4 is a concurrent session's work,
summarised here only where it settles a question §6 left open.

*Claude (Opus 5) · session `79740b6f-39f1-4f66-a24d-3e072cb5fe5d` · addendum
2026-08-24T17:05-07:00*
