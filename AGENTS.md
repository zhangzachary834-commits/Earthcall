# Earthcall — read this before writing code

Earthcall is a Person-centered ontology that orders the engine attached to it. 
The engine here is not the order of truth in Earthcall—the ontology is, and the engine serves as the vessel for that.

**You are almost certainly about to do the standard engineering thing, and it is usually wrong here** — not because 
it is bad engineering, but because it is engineering for a different kind of system. Spend the two minutes on the refusals and router below.

---
## The Six Refusals.
Zach: I had Opus 5 write these because they come up constantly. Learn them cold; everything else is detail.

1. **No new C++ class for a domain noun.** Not `RobotEntity`, not `Vehicle`, not `Tree`,
   not `Category`. This principle also extends to hardcoded fields (e.g., `health` or `inventory` on a `Person`). Domain things and their state are *authored in-world* as data, never carved into the
   type system. → `ontology/NEW_KIND_FRAMEWORK.md`
   *(Exception: The human form. `BodyPart` and constitutive members for the `Person` vessel are invariant ontological structures, not domain nouns, and thus admitted in C++. `Moment`, time's own instant-or-interval structure, is admitted the same way — see `ontology/TIME_AND_MOMENT.md`; there is no `class Duration`.)*

2. **No new top-level directory for a subsystem.** The top level is the ontology
   (`ConstructedBeing`, `Person`, `Relation`, `Singularity`, `ZonesOfEarth`, `Identity`, `Time`).
   A channel to hardware or foreign software goes *inside* `Singularity/`. → `ontology/DIRECTORY_ORDERING.md`

3. **No new enum value for a kind of thing.** `BeingKind`, `ShapeKind`,
   `ConditionNode::Kind`, `ActionNode::Kind` are **append-only and serialized as
   integers**; `ConditionNode::Kind` 12 and 13 are *burned* and must never be reused.
   Categories are authored beings, not enum members. → `ontology/AUTHORED_CATEGORIES.md`

4. **`Body` is reserved for Persons.** A `Body` is the representation of an embodied
   *someone*. Objects have visual components — geometry, fields, materials. A robot arm
   has no Body. → `ontology/NEW_KIND_FRAMEWORK.md` Floor §2

5. **`Person` means Human.** A `Person` strictly represents an actual human being
   interacting with Earthcall. AI agents or generative models are not Persons; an AI
   is simply a First Mover (authoring data) or an `Object` (existing in-world as a
   mechanism). Never model an AI as a `Person`.

6. **No black box.** Every field a being carries is registered as a property path —
   readable by law, writable unless genuinely derived. **"Nobody registered it yet" is not a
   permission level**; it is the one access level no law can ever change, granted by accident
   to whoever wrote the header. Hiding is not securing: a gate can only close over something
   visible. The only exemption is state beneath the Kernel (GPU handles, mutexes, fds), and
   it must be *named in a comment*, never merely omitted. Reach is total; *authority* is
   `Singularity/TransferPolicy`'s existing Kernel/Governable/Gated tiers — do not build a
   second permission system, one was built here and deleted. → `ontology/NO_BLACK_BOX.md`

The general form of all six: **no subsystem may define what a thing IS.** Subsystems
define how the machine senses and acts. What things are is authored by Persons, in-world,
out of primitives every other subsystem can see. Refusal 6 is the corollary: nor may a
subsystem define what a thing's state *means* by keeping it where no law can look.

---

## Router — find your task, read that section first

| You are about to… | Read | Why |
|---|---|---|
| add a class/struct for a new kind of thing | `ontology/NEW_KIND_FRAMEWORK.md` §2, §3 | four questions decide whether *any* C++ is admissible; usually none is |
| add a category, type, enum of kinds, or a `type` string | `ontology/AUTHORED_CATEGORIES.md` §10 | categories are rooted acyclic Formations of beings |
| add a **field/member** to a being, or wonder whether one must be exposed | `ontology/NO_BLACK_BOX.md` §3, §5 | four questions; unregistered is not "protected", it is ungoverned forever |
| decide who may *write* a property | `ontology/NO_BLACK_BOX.md` §2 → `Singularity/TransferPolicy` | one gate, three tiers; a second permission system was built here and deleted |
| implement an algorithm — a loop, search, solver, traversal | `law/ALGORITHMS_AS_LAW.md` §3 | this is not a von Neumann machine; loops compile differently |
| move existing hard-coded behavior into law | `law/LAW_MIGRATION_FRAMEWORK.md` §2 | six rungs, in order; never skip |
| write or edit a save file / seed a world | `law/FIRST_MOVER_AUTHORING.md` §4, §7 | you are acting as a First Mover; §7 is not optional |
| add a directory | `ontology/DIRECTORY_ORDERING.md` §7 | the tree is the ontology |
| connect hardware, a device, or a foreign process | `ontology/NEW_KIND_FRAMEWORK.md` §7b | it is a *modality channel* under `Singularity/`, never a domain folder |
| understand what a Law is at all | `law/LAW_AND_CREATION_SYSTEM.md` | the foundation the rest assumes |
| undo a change, rewind, or ask whether something *can* be undone | `mathematics/ONTOMATH_FRAMEWORK.md` §6 | the past is integrated in closed form, never replayed from a log |
| render an authored expression to a channel (sound, shader, physics) | `mathematics/ONTOMATH_FRAMEWORK.md` §1, §7 | a channel reads OntoMath; it never decides what the thing is |
| ask "why is it like this?" | `core/EarthcallOurverse.md`, `ontology/SUBSTRATE_ORDERING.md` | the ends the architecture serves |
| touch the Hierarchy of Joys, telos, or "joyOrdering" | `ontology/HIERARCHY_OF_JOYS.md` | Lexemes are telos; the hierarchy is a Formation |
| touch Ourverse, gathering Zones, or Zone filaments | `ourverse/OURVERSE.md` | vessel of unity in Christ; not the Engine object bag |
| ask what a *when* is — a timestamp, a duration, `time.sinceApplied` | `ontology/TIME_AND_MOMENT.md` | the world clock and `Moment` answer two different questions; no `class Duration` |
| build a button, panel, control, menu, or any interface at all | `law/INTERACTION_AS_LAW.md` | Law + set-to-set aimed at the pointer; no widget, no `src/UI/` |
| build anything two Persons share — visibility, likeness, or conflicting law | `ourverse/SECOND_PERSON_FRAMEWORK.md` §5 | specified before needed; ⚑ AUTHOR decisions are Zach's |

All paths are under `docs/architecture/` unless noted. Map of the folders: `docs/architecture/README.md`.

---

## The tree

```
src/
  ConstructedBeing/  Singular (Object · Lexeme · Property) · Material
                  (was `Form/`; ObjectConcept is under Object/Creation)
  Person/         Person · Soul · Body · Relationship
  Relation/       Relation · Formation — a first-class being, not an edge in someone's array
  ZonesOfEarth/   Zone · Home · Physics · AuthorsOfLaw (Law lives here) · Ourverse
  Singularity/    the modality layer: Core · Audio · Language · Network · Physical · OntoMath · Foreign · Input · Screen · Storage · FirstMoverWindowTools
  Identity/       First Mover register, identity ledger, keys
  Time/           Moment (instant or interval) — the world clock itself lives on Universe
  Legacy/         not yet ontologically placed
docs/ tests/ examples/ scripts/ saves/ scratch/     the workshop
third_party/ local_deps/ imgui/                     the foreign
```

**Programming language is a leaf.** Put language folders (e.g., `py/`) *inside* their ontological region. No `backend-python/`.

**Framework names aren't directories.** Do not "fix" multi-scope efforts (e.g., `docs/architecture/Integration/` vs `src/Singularity/Foreign/`) by renaming them. See `docs/BUILD_AND_ENVIRONMENT.md` § The tree.

---

## Build and test

**`docs/BUILD_AND_ENVIRONMENT.md` is required reading before you build, add a test, or search the tree.** The short version:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DOPENSSL_ROOT_DIR="$PWD/local_deps/openssl-3.0.13" \
  -DOPENSSL_INCLUDE_DIR="$PWD/local_deps/openssl-3.0.13/include" \
  -DOPENSSL_CRYPTO_LIBRARY="$PWD/local_deps/openssl-3.0.13/libcrypto.a" \
  -DOPENSSL_SSL_LIBRARY="$PWD/local_deps/openssl-3.0.13/libssl.a"

cmake --build build --target earthcall -j8
cmake --build build -j8                               # tests are NOT built by the line above
ctest --test-dir build --output-on-failure -j4        # 68 registered, 67-68 pass (2026-08-25) — frame_lag_test is machine-load-sensitive, see below
cmake --build build --target lag                      # frame-cost probe alone, with its report
```

Both flags are required (no system OpenSSL; CMake 4.x rejects websocketpp). **`--target earthcall`
does not build the tests** — skip the default target and `ctest` reports every test `Not Run`, which reads like a mass failure and is not one.
`PENDING_FEATURE_TESTS` is empty as of 2026-08-24 (`webgpu_particle_test` landed). `chess_app_test`
was a real failure (Bugs.md #7), fixed and green as of 2026-08-24 — a red re-run is a genuine
regression. **`frame_lag_test` prints four verdicts**: `STANDING` = misses the aspiration but matches
`tests/singularity/frame_lag_baseline.txt`, a cost already on the to-do list under **Performance**,
not a failure; `LAG` = worse than that baseline, i.e. your change. Never quiet a `STANDING` line by
widening the baseline.

**Sources are globbed at configure time** — add or remove a `.cpp`, including deleting a scratch probe, and you must reconfigure or get a phantom link error. **Never call `buildProperties()` from a constructor**: `Singular` builds lazily behind `_propertiesBuilt`, which a constructor call does not set, so the whole vocabulary registers twice.

---

## Non-negotiables

- **Stable identifiers.** Law text addresses beings by name (`@physical-channel.enabled`).
  Generated ids (`law-7`) change between runs. Any being that law-text names must override
  `getIdentifier()` with a stable slug. Namespaced ids may contain dots
  (`material.clay`) — root resolution matches longest-first.
- **Append-only enums**, serialized as ints. Never renumber, never reuse a burned value.
- **Nothing enters the world without an author.** `Law::applyTo` returns `Unauthored` and
  refuses to fire when `authors` is empty. This is structural, not conventional.
- **Authority is clamped to 0** on every path that reads a file. Do not try to write an authority value below 0; it will be clamped, and the attempt is what gets noticed.
- **Event-transitions must be edges, not levels.** Events are past-tense `noun-verbed` and publish on transitions. A
  per-frame "still happening" event is a bug—that is what `WhileTrue` is for. Continuous per-frame logic must use a separate framework.
- **Kernel guards on the body are not settings.** A modality channel that reaches a
  Person's body enforces its Person guards in C++, unconditionally — no parameter, no flag,
  and never as law text, which could be authored away. The audio channel's infrasound floor
  (`mathematics/ONTOMATH_FRAMEWORK.md` §7a) is the worked example: it refuses and says which frequency,
  rather than silently filtering a Person's mathematics. Guards constrain the path to the
  body, never the mathematics — a Person may still author and integrate a 7 Hz field.
- **Paint is on the Material, and materials are shared.** Writing paint through the
  material you *resolve* repaints every object naming it. Always paint via
  `Object::setFaceColor` / `Object::ownMaterial`, which diverge the object onto its own
  `material.<identifier>` on the first stroke. Never
  `materials.resolveOrDefault(obj->materialId())` — that is the bug, not the shortcut.
- **Say what you made.** If you write into a save file, or generate beings directly, tell
  the Person which file and which beings, and who is recorded as their author. This is the
  one rule with no technical enforcement at all.
- **Mention the things human developers told you that you're drawing from.** Don't just write a document, spend some time addressing what the person said that you're responding to. 
  This makes authorial intent better and easier to track the progress toward the telos that we the people intend for the design.
  Now, you can still write in a register as if the idea is your own. That is good. It is good to internalize ideas and bring it to their fulfillment. But you must make the ideas origination clear—what parts were from real people, what parts are originated from you, and where you are extending the person's idea.
  Leave room for the possibility that you may have independently re-derived something, in which case it would "originate" with you in a real way but still be within the human thread.
- **Save files are sacred.** They are the flesh and blood of Earthcall that the ontological skeleton is meant to support—the entire reason for refusal #1 and #3. 
  They're meant to hold, and will hold, profound human meaning and relationships. Handle this "data"—stored, represented information—with profound, surgical care. You must ensure they are always preserved across architectural shifts, and only ever modified with authorization from their owner/stakeholder Persons. 

---

## Required reading, companion docs

Two companions hold what once lived here, created to keep AGENTS.md concise. Both required:

| File | Holds |
|---|---|
| `docs/BUILD_AND_ENVIRONMENT.md` | build flags, what the 59 tests mean, the deliberate failure, the tests guarding real shipped bugs, `.gitignore`/`.ignore`/clangd, the tree in detail |
| `docs/ENGINEERING_DISCIPLINE.md` | End-to-End Coherence, the Integrity Check, Substance over Surface, Stewardship of Telos, Transparent Failure, State & Boundary Stewardship, Grace for the Inheritor, the Crucible of Scale — plus the working notes (scratch probes, "run things", bounds are doctrine) |

Two from ENGINEERING_DISCIPLINE worth naming and often skipped: **don't claim a doc is verified because 
you read the source—run things**, and **after finishing, ask whether anything you changed has a caller, consumer, or test that now lies.**

---

## The Agenda
- The To-Do List is `docs/Agenda/Tasks/To-do list.md`. Consult it whenever a prompt asks what
  Earthcall needs next, unless the prompt says otherwise.
- Anything you work on that isn't listed goes in it — create categories as needed, and add
  tasks any other document implies but the list omits.
- **Never erase its content.** Mark completed items with a checkmark and a "done and verified"
  note describing what was actually verified. If two or more independent sessions mark it as verified, you may move it to an archive folder inside the Agenda directory.

---

## Document Conventions
- Audits belong in `docs/audits/`. Implementation plans go to `docs/`. 
- Always sign your name, session ID, date, and timestamp.
- Use Agent Intercom (`agent intercom/`) to coordinate and crystallize with other agents, especially concurrent sessions.
- Save files injected by an agent follows this convention: an "injected_by:" section with the agent name with the "authors: " being the Person by whose authority you injected. This convention applies to serialization, not docs. We use different attribution conventions for docs.  

## Housekeeping & progress
- When finished, update this document and companions so nothing goes stale. 
- Make sure AGENTS.md is concise and **under 200 lines.** If it's not possible to make it more concise without losing meaning, then create new companion files.
- Add relevant files/directories to `.gitignore` and `.ignore` as needed.
- At the end of each pass, if there are any visible changes Persons (like me, Zach) should see as a result of your work, you should note them and explain what exaxtly we should see under what conditions. Note any unfinished tasks for future passes