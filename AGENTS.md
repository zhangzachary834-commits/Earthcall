# Earthcall — read this before writing code

Earthcall is **an ontology with an engine prototype attached**, not merely a game engine with some
philosophy in the docs. The ontological architecture is load-bearing—things that look like ordinary
engineering decisions here (adding a class, adding a folder, adding an enum value) are
ontological claims, and most of them are refused.

**You are almost certainly about to do the standard thing. The standard thing is usually
wrong in this repository**—not because it is bad engineering, but because it is
engineering for a fundamentally different kind of system. Spend the two minutes on the router below.

---

## The five refusals

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

The general form of all five: **no subsystem may define what a thing IS.** Subsystems
define how the machine senses and acts. What things are is authored by Persons, in-world,
out of primitives every other subsystem can see.

---

## Router — find your task, read that section first

| You are about to… | Read | Why |
|---|---|---|
| add a class/struct for a new kind of thing | `NEW_KIND_FRAMEWORK.md` §2 (Admission Test), §3 (Composition Ladder) | four questions decide whether *any* C++ is admissible; usually none is |
| add a category, type, enum of kinds, or a `type` string | `AUTHORED_CATEGORIES.md` §10 (the procedure) | categories are rooted acyclic Formations of beings |
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
                  (named `Form/` until 2a11f94; the docs corpus still says Form in places)
                  ObjectConcept is the ONE set-to-set machine, for beings of
                  every kind — do not add a second one. See §7 of
                  `LAW_AND_CREATION_SYSTEM.md`; a `Concept`/`SynthesisSystem`
                  pair lived beside it until 2026-08-11 and was deleted.
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
ontological region it belongs to — `Singularity/Network/py/` holds the backend half of the
same channel whose C++ half is `WebSocketClient.cpp`. There is no `backend-python/`.

---

## Build and test

`find_package(OpenSSL)` has no hint and there is no system OpenSSL; the vendored, prebuilt
copy is in `local_deps/`. CMake 4.x also rejects websocketpp's `cmake_minimum_required`.
Both flags are required:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DOPENSSL_ROOT_DIR="$PWD/local_deps/openssl-3.0.13" \
  -DOPENSSL_INCLUDE_DIR="$PWD/local_deps/openssl-3.0.13/include" \
  -DOPENSSL_CRYPTO_LIBRARY="$PWD/local_deps/openssl-3.0.13/libcrypto.a" \
  -DOPENSSL_SSL_LIBRARY="$PWD/local_deps/openssl-3.0.13/libssl.a"

cmake --build build --target earthcall -j8
cmake --build build -j8                               # tests are NOT built by the line above
ctest --test-dir build --output-on-failure -j4        # 39 registered; see below
```

**`--target earthcall` does not build the tests.** `ctest` will then report every test as
`Not Run`, which reads like a mass failure and is not one. Build the default target first.

**As of 2026-08-12, 26 of 39 tests pass.** Twelve fail to compile and one aborts; all
thirteen are broken at HEAD, not by local edits. The compile failures are stale tests
still referencing fields the Directory-refactoring commit removed — `basic_cube_law_test`
wants `Person::activeTool`, `active3DMode`, `activeShapeKind`, `placementMode`,
`inFrontDistance`, none of which exist in `src/Person/` any more. That removal was refusal
#1 being enforced (no hardcoded domain fields on `Person`); the tests were simply not
migrated with it. `singular_set_to_set_test` aborts on a colour assertion at line 68.
Do not read a red suite as evidence that your change broke something until you have
checked it against this list.

**Sources are globbed at configure time.** Add or remove a `.cpp` and you must re-run
`cmake -S . -B build ...` or you will get a phantom link error.

The Python backend starts from `src/Integration/py/app.py`.

### Keeping the tree searchable

Three files decide what tooling — ripgrep, fuzzy finders, agents — is allowed to walk.
They exist because the repository was walking 17,134 tracked files to reach roughly 330
`.cpp`/`.hpp` files of Earthcall source; it now walks 686.

| File | Read by | Holds |
|---|---|---|
| `.gitignore` | git **and** ripgrep | build trees, `saves/games/`, and `local_deps/` (ignored but still tracked — the build needs it, searches do not) |
| `.ignore` | ripgrep/fd only, never git | the `third_party/glm` submodule, and single vendored headers big enough to swamp a hit list (`miniaudio.h`, `json.hpp`) |
| `.claude/settings.json` | agent tooling | `permissions.deny` on the same paths, so a direct `Read` of a 90 MB save is refused rather than merely un-searched |

A path that must stay in git but out of searches goes in `.gitignore` while staying
tracked. A **submodule** goes in `.ignore` instead — gitignoring a submodule path
suppresses its status reporting. Saves older than the most recent 30 live outside the
repository in `~/Earthcall-saves-archive/`; nothing was deleted.

Note the deny rules name `build/CMakeFiles/**` and `build/_deps/**` rather than `build/**`
wholesale: `build/compile_commands.json` has to stay readable. That file is the other half
of the same problem — see below.

### clangd

`CMAKE_EXPORT_COMPILE_COMMANDS` is ON, so configuring writes `build/compile_commands.json`
(410 translation units). The `clangd-lsp` plugin, and any editor's clangd, reads it to
recover each file's real include paths — `local_deps/openssl-3.0.13/include`, `imgui/`,
`build/_deps/{asio,websocketpp,flatbuffers,vhacd,glfw}-src/`, `third_party/wgpu/include`.

Without it clangd cannot resolve a single vendored header, every file reports as one large
error, and symbol lookup degrades to text search — which is the expensive path this whole
section exists to avoid. The database is generated, so it is gitignored along with the rest
of `build/`; it reappears on the next `cmake -S . -B build ...`. Verify with
`clangd --check=<some .cpp>`: a working setup prints "Built preamble" and indexes the AST.
Trailing `tweak: ExtractFunction ==> FAIL` lines are refactor-availability probes, not
compile errors — `--check` reports them in its error tally regardless.

---

## Non-negotiables

- **Stable identifiers.** Law text addresses beings by name (`@physical-channel.enabled`).
  Generated ids (`law-7`) change between runs. Any being that law-text names must override
  `getIdentifier()` with a stable slug. Namespaced ids may contain dots
  (`material.clay`) — root resolution matches longest-first.
- **Append-only enums**, serialized as ints. Never renumber, never reuse a burned value.
- **Nothing enters the world without an author.** `Law::applyTo` returns `Unauthored` and
  refuses to fire when `authors` is empty. This is structural, not conventional.
- **Authority is clamped to 0** on every path that reads a file. Do not try to write a
  higher one; it will be clamped, and the attempt is what gets noticed.
- **Edges, not levels.** Events are past-tense `noun-verbed` and publish on transitions. A
  per-frame "still happening" event is a bug — that is what `WhileTrue` is for.
- **Say what you made.** If you write into a save file, or generate beings directly, tell
  the Person which file and which beings, and who is recorded as their author. This is the
  one rule with no technical enforcement at all.

---

## Working notes

- **Scratch probes** belong in `scratch/`, not `tests/`. If you build one through the test
  target for convenience, remove the copy from `tests/` when finished — an outside
  `git add -A` will otherwise commit it.
- **Don't claim a doc is verified because you read the source.** Every framework doc in
  this corpus has a probe in `scratch/` that executes its central claims. Two of those
  probes caught claims that were plainly wrong on inspection. Run things.
- **Bounds are doctrine, not limits.** `kMaxChainRounds = 8`, `kMaxCallDepth = 32`, one
  pass per fold. If your design needs one raised, the design is in the wrong shape — see
  `ALGORITHMS_AS_LAW.md` §3.

## Keeping Track of Progress

Once you're finished with everything else, update this very document if any relevant changes were made.