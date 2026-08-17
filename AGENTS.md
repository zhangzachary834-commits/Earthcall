# Earthcall — read this before writing code

Earthcall is **an ontology with an engine prototype attached**, not a game engine with
philosophy in the docs. The architecture is load-bearing: things that look like ordinary
engineering decisions (adding a class, a folder, an enum value, *a field*) are ontological
claims, and most are refused.

**You are almost certainly about to do the standard thing, and it is usually wrong here** —
not because it is bad engineering, but because it is engineering for a different kind of
system. Spend the two minutes on the router below.

---

## The six refusals

These come up constantly. Learn them cold; everything else is detail.

1. **No new C++ class for a domain noun.** Not `RobotEntity`, not `Vehicle`, not `Tree`,
   not `Category`. This principle also extends to hardcoded fields (e.g., `health` or `inventory` on a `Person`). Domain things and their state are *authored in-world* as data, never carved into the
   type system. → `NEW_KIND_FRAMEWORK.md`
   *(Exception: The human form. `BodyPart` and constitutive members for the `Person` vessel are invariant ontological structures, not domain nouns, and thus admitted in C++).*

2. **No new top-level directory for a subsystem.** The top level is the ontology
   (`Form`, `Person`, `Relation`, `Singularity`, `ZonesOfEarth`, `OurVerse`). A channel to
   hardware or foreign software goes *inside* `Singularity/`. → `DIRECTORY_ORDERING.md`

3. **No new enum value for a kind of thing.** `BeingKind`, `ShapeKind`,
   `ConditionNode::Kind`, `ActionNode::Kind` are **append-only and serialized as
   integers**; `ConditionNode::Kind` 12 and 13 are *burned* and must never be reused.
   Categories are authored beings, not enum members. → `AUTHORED_CATEGORIES.md`

4. **`Body` is reserved for Persons.** A `Body` is the representation of an embodied
   *someone*. Objects have visual components — geometry, fields, materials. A robot arm
   has no Body. → `NEW_KIND_FRAMEWORK.md` Floor §2

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
   second permission system, one was built here and deleted. → `NO_BLACK_BOX.md`

The general form of all six: **no subsystem may define what a thing IS.** Subsystems
define how the machine senses and acts. What things are is authored by Persons, in-world,
out of primitives every other subsystem can see. Refusal 6 is the corollary: nor may a
subsystem define what a thing's state *means* by keeping it where no law can look.

---

---
TODO: add a section about First Movers, listing them and pointing to them. Section should be fully expounded on in a dedicated companiondoc.
---

## Router — find your task, read that section first

| You are about to… | Read | Why |
|---|---|---|
| add a class/struct for a new kind of thing | `NEW_KIND_FRAMEWORK.md` §2 (Admission Test), §3 (Composition Ladder) | four questions decide whether *any* C++ is admissible; usually none is |
| add a category, type, enum of kinds, or a `type` string | `AUTHORED_CATEGORIES.md` §10 (the procedure) | categories are rooted acyclic Formations of beings |
| add a **field/member** to a being, or wonder whether one must be exposed | `NO_BLACK_BOX.md` §3 (Admission Test), §5 (the procedure) | four questions; unregistered is not "protected", it is ungoverned forever |
| decide who may *write* a property | `NO_BLACK_BOX.md` §2 → `Singularity/TransferPolicy` | one gate, three tiers; a second permission system was built here and deleted |
| implement an algorithm — a loop, search, solver, traversal | `ALGORITHMS_AS_LAW.md` §3 (four kinds of iteration) | this is not a von Neumann machine; loops compile differently |
| move existing hard-coded behavior into law | `LAW_MIGRATION_FRAMEWORK.md` §2 (the ladder) | six rungs, in order; never skip |
| write or edit a save file / seed a world | `FIRST_MOVER_AUTHORING.md` §4 (recipes), §7 (discipline) | you are acting as a First Mover; §7 is not optional |
| add a directory | `DIRECTORY_ORDERING.md` §7 (checklist) | the tree is the ontology |
| connect hardware, a device, or a foreign process | `NEW_KIND_FRAMEWORK.md` §7b | it is a *modality channel* under `Singularity/`, never a domain folder |
| understand what a Law is at all | `LAW_AND_CREATION_SYSTEM.md` | the foundation the rest assumes |
| undo a change, rewind, or ask whether something *can* be undone | `ONTOMATH_FRAMEWORK.md` §6 | the past is integrated in closed form, never replayed from a log; the refusals are the point |
| render an authored expression to a channel (sound, shader, physics) | `ONTOMATH_FRAMEWORK.md` §1, §7 | a channel reads OntoMath; it never decides what the thing is |
| ask "why is it like this?" | `core/EarthcallOurverse.md` (the manifesto), `SUBSTRATE_ORDERING.md` | the ends the architecture serves |

All paths are under `docs/architecture/` unless noted.

---

## The tree

```
src/
  ConstructedBeing/  Singular · Object · ObjectConcept · Formation · Property · Material
                  (was `Form/`; ObjectConcept is the ONE set-to-set machine)
  Person/         Person · Soul · Body · Relationship
  Relation/       Relation — a first-class being, not an edge in someone's array
  ZonesOfEarth/   Zone · Home · World · Physics · AuthorsOfLaw (Law lives here)
  Singularity/    the modality layer: Core · Audio · Language · Network · Physical · OntoMath · Foreign · Input · Screen · Storage
  OurVerse/       the Person-facing authorship surface (tools, chat, controls)
  Identity/       First Mover register, identity ledger, keys
  Legacy/         not yet ontologically placed
docs/ tests/ examples/ scripts/ saves/ scratch/     the workshop
third_party/ local_deps/ imgui/                     the foreign
```

**Language is a leaf, never a branch.** Python lives in `py/` subfolders *inside* the
ontological region it belongs to. There is no `backend-python/`.

**A framework name is not a directory name.** `docs/architecture/Integration/` and
`src/Singularity/Foreign/` are one effort at two scopes — do not "fix" it by renaming
either. Details in `docs/BUILD_AND_ENVIRONMENT.md` § The tree, in detail.

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
ctest --test-dir build --output-on-failure -j4        # 46 registered, 45 pass
```

Both flags are required (no system OpenSSL; CMake 4.x rejects websocketpp).
**`--target earthcall` does not build the tests** — skip the default target and `ctest`
reports every test `Not Run`, which reads like a mass failure and is not one. The one true
failure, `webgpu_particle_test`, is **deliberate** (`PENDING_FEATURE_TESTS`); never read a
red suite as proof you broke something before checking there.

**Sources are globbed at configure time** — add or remove a `.cpp`, including deleting a
scratch probe, and you must reconfigure or get a phantom link error. **Never call
`buildProperties()` from a constructor**: `Singular` builds lazily behind `_propertiesBuilt`,
which a constructor call does not set, so the whole vocabulary registers twice.

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
  (`ONTOMATH_FRAMEWORK.md` §7a) is the worked example: it refuses and says which frequency,
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

---

## Required reading

Two companion files hold what used to live here, so this file stays short enough to read in
full. Both are required:

| File | Holds |
|---|---|
| `docs/BUILD_AND_ENVIRONMENT.md` | build flags, what the 46 tests mean, the deliberate failure, the five tests guarding real shipped bugs, `.gitignore`/`.ignore`/clangd, the tree in detail |
| `docs/ENGINEERING_DISCIPLINE.md` | End-to-End Coherence, the Integrity Check, Substance over Surface, Stewardship of Telos, Transparent Failure, State & Boundary Stewardship, Grace for the Inheritor, the Crucible of Scale — plus the working notes (scratch probes, "run things", bounds are doctrine) |

Two from that second file are worth naming here because they are the ones most often
skipped: **don't claim a doc is verified because you read the source — run things**, and
**after finishing, ask whether anything you changed has a caller, consumer, or test that now
lies.**

---

## The Agenda
- The To-Do List is `docs/Agenda/Tasks/To-do list`. Consult it whenever a prompt asks what
  Earthcall needs next, unless the prompt says otherwise.
- Anything you work on that isn't listed goes in it — create categories as needed, and add
  tasks any other document implies but the list omits.
- **Never erase its content.** Mark completed items with a checkmark and a "done and verified"
  note describing what was actually verified. If two or more independent sessions mark it as verified, you may move it to an archive folder inside the Agenda directory.

---

## Housekeeping & progress
- Create a new directory when a new file needs one for better organization.
- If you are doing an audit, write your audit to /docs/audit. If you wrote an implementation plan, write that plan to its appropriate directory inside /docs
- When finished, update this document (and its two required-reading companions) if anything
  here went stale. Keep this file **under 200 lines** — move detail into the companions rather than growing it.
- Every so often, add relevant directories/files to the gitignore and claudeignores/agentignores. If you ever, by chance, happen to catch a directory or file that should be ignored by either git or the agents.
- If during your session you come up with anything that you see as relevant for future agents to know up front, you should add it to this document AGENTS.md or the relevant companion documents.
- If you are doing a task: When you're done with your pass, if there's anything from the original plan left unfinished, make sure to note that explicitly for another pass.
- Use the Agent Intercom system to leave notes for other AI agents. A feature in the intercom system that allows you to communicate with other agents and ping each other in real time is being implemented.