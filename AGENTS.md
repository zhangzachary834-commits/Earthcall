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

**A framework name is not a directory name.** `docs/architecture/Integration/` and
`src/Singularity/Foreign/` are the same effort at two different scopes, and neither is a
misnaming of the other. *Integration* is the tentative name of the whole conceptual system
— sensing an external app, projecting it into Zones, reconstructing its behavior, syncing
back — and it names a **docs** directory, which is the workshop, not the ontology.
*Foreign* is the Singularity-level modality layer holding the hardwired connectors to
external software. So there is no `src/Integration/` (that would be refusal #2) and no
`Singularity/Integration` either. Do not "fix" the mismatch by renaming one to the other;
this has been attempted. See `docs/architecture/Integration/INTEGRATION_FRAMEWORK.md` §0.

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
ctest --test-dir build --output-on-failure -j4        # 44 registered; see below
```

**`--target earthcall` does not build the tests.** `ctest` will then report every test as
`Not Run`, which reads like a mass failure and is not one. Build the default target first.

**As of 2026-08-13, 43 of 44 tests pass and the default build is clean.** The thirteen
that were broken were stale against three refactors, not against each other:
`Rendering/` → `Singularity/Screen/` and `Util/` → `Singularity/Storage/`;
`Object::GeometryType` → `ShapeKind`; the placement and tool fields off `Person` and onto
`Singularity::Core::CreationChannel` (refusal #1 being enforced); `Zone` off `Object` and
onto `Singular`, losing the tint and brush a canvas has and a space does not.

The one remaining failure is **`webgpu_particle_test`, and it is deliberate**. It calls
`WebGpuRenderer::drawParticles(FieldNode&, int)`, which has never existed in any commit —
the test was written against an unbuilt feature and has never compiled. It is kept as that
feature's specification, listed in `PENDING_FEATURE_TESTS` in `CMakeLists.txt`, and
excluded from the default target so the build is not red for work nobody has started;
`ctest` reports it `Not Run`. Take a name off that list when its feature lands. Do not read
a red suite as evidence that your change broke something until you have checked it here.

**Three tests exist because of what got past the suite, not to pad it.** Each guards a
class of silent failure this repository has actually shipped, so keep them honest rather
than convenient:

| Test | Guards against |
|---|---|
| `paint_test` | paint written through a *shared* material (repaints the world), and a `color` property that does not read back what was written — `propSetColor` was an empty function for a month |
| `object_roundtrip_test` | a field `to_json` writes and `from_json` drops. `faceColors` was write-only for a month with the write side making it look covered; `serialization_compat_test` covers the msgpack/Frontier *plumbing* and cannot see this |
| `channel_paths_test` | the law-authoring picker offering a property path no registry answers. `CreationChannel::activeShapeKind` was advertised and unregistered, so every law reading it silently fell back |

If you add a field to `Object` that `to_json` writes, add it to `object_roundtrip_test`. If
you hand-list a path in `knownPathOptions()`, `channel_paths_test` will hold you to it.

**Sources are globbed at configure time.** Add or remove a `.cpp` and you must re-run
`cmake -S . -B build ...` or you will get a phantom link error.

The Python backend starts from `src/Singularity/Foreign/py/app.py`.

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

clangd also writes its own index — `.cache/clangd/index/*.idx`, one per translation
unit — separate from `compile_commands.json` and rewritten on nearly every reindex. As of
2026-08-13 it is gitignored (`.cache/`); it was tracked for some prior stretch (582 files),
which meant routine reindexing showed up as unrelated modified files in every `git status`.
Untracking does not affect clangd — the index regenerates locally regardless.

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
- **Kernel guards on the body are not settings.** A modality channel that reaches a
  Person's body enforces its Person guards in C++, unconditionally — no parameter, no flag,
  and never as law text, which could be authored away. The audio channel's infrasound floor
  (`ONTOMATH_FRAMEWORK.md` §7a) is the worked example: it refuses and says which frequency,
  rather than silently filtering a Person's mathematics. Guards constrain the path to the
  body, never the mathematics — a Person may still author and integrate a 7 Hz field.
- **Paint is on the Material, and materials are shared.** An Object's per-face textures
  live on the `Material` being it names by identifier — `material.default` unless it says
  otherwise — so writing paint through the material you *resolve* repaints every object
  naming the same one. Painting therefore goes through `Object::setFaceColor` /
  `Object::ownMaterial`, which diverge the object onto its own `material.<identifier>` on
  the first stroke, carrying the shared appearance over. Do not reach for
  `materials.resolveOrDefault(obj->materialId())` and paint into the result; that is the
  bug, not the shortcut. `Object::faceColors` is the object's own colour and what the
  `color` property reads; `setFaceColor` writes both halves so the property does not lie.
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

## The Agenda
- When relevant, look at the Agenda and its To Do List. The To-Do List is inside Earthcall/docs/Agenda/Tasks
- For example, when a prompt says something like "What does Earthcall need to do next" or "next steps for Earthcall", you should look at the To-do list, unless the prompt specifically asks otherwise. 
- Add to the Agenda items however you see fit. Do not delete anything from it, instead mark it with checkmarks and a "done and verified" note
- If the To-Do List does not list a task that another document, you should add that task to the To-Do list  

## Extra Housekeeping notes
-- If you 

## Keeping Track of Progress
- Once you're finished with everything else, update this very document if any relevant changes were made.