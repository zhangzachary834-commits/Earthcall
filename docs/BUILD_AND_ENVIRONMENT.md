# Build, Test, and Environment — required reading

Split out of `AGENTS.md` so that file stays short enough to be read in full. This is the
workshop half: how to build, what the test suite means, and what tooling is allowed to walk
the tree. `AGENTS.md` holds the ontology and the refusals, and links here.

*(Note: unlike the framework corpus, this file lives at `docs/`, not `docs/architecture/` —
it describes the workshop, not the ontology.)*

---

## Build

### One-click WebGPU launch (macOS)

Double-click `Run Earthcall.command` at the repository root. It opens Terminal, configures
CMake with the required dependency paths below, incrementally builds `earthcall_webgpu`,
and launches it. Keep the Terminal window open while the app runs; it displays any build
or launch failure instead of closing immediately. The same action from a terminal is
`./scripts/build.sh webgpu run`.

### One-click WASM launch (macOS)

Double-click `Run Earthcall WASM.command` at the repository root. It uses Emscripten from
the Terminal environment (or the usual `~/emsdk/emsdk_env.sh` installation), configures and
incrementally builds `earthcall_wasm`, then serves the tracked browser entry point at
`http://localhost:8000/web_ui/wasm.html`. Keep the Terminal window open while using the app.
If no Emscripten SDK is available, the launcher names that requirement and leaves the error
visible.

### One-click Python-backend launch (macOS)

Double-click `Run Earthcall Python.command` at the repository root. It starts
`src/Singularity/Foreign/py/app.py` using that application's `venv`. If the environment is
absent, run `src/Singularity/Foreign/py/setup_env.sh` once first; the launcher reports this
rather than falling back to an unpinned system Python.

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

cmake --build build --target earthcall_webgpu -j8       # THE APP. `earthcall` is the
                                                       # OpenGL build, where every
                                                       # analytic shape (sphere, torus,
                                                       # field) falls back to its cached
                                                       # tessellation -- nothing is
                                                       # raymarched. Run Earthcall.command
                                                       # and scripts/build.sh webgpu run
                                                       # both use earthcall_webgpu.
cmake --build build -j8                               # tests are NOT built by the line above
ctest --test-dir build --output-on-failure -j4        # 81 registered, 80 pass (~35-115 s depending on load) — smooth_tessellation_cache_test is the one failure, pre-existing, Bugs.md #11; frame_lag_test is machine-load-sensitive
cmake --build build --target lag                       # just the frame-cost probe, with its report
```

**`--target earthcall` does not build the tests.** `ctest` will then report every test as
`Not Run`, which reads like a mass failure and is not one. Build the default target first.

**Sources are globbed at configure time.** Add or remove a `.cpp` and you must re-run
`cmake -S . -B build ...` or you will get a phantom link error. This bites hardest after you
delete a scratch probe you had temporarily built through the test target: the stale target
survives in the cache and the build fails on a missing source until you reconfigure.

The Python backend starts from `src/Singularity/Foreign/py/app.py`.

---

## The test suite

**As of 2026-08-24, 66 of 66 tests pass and the default build is clean.** `zone_facetexture_test` guards Home/Zone identity materials (FaceTextures persist across session loads). `chess_app_test` guards the authored chess world (`saves/worlds/chess_app.json`) and is green again — see below. The thirteen
that were broken were stale against three refactors, not against each other:
`Rendering/` → `Singularity/Screen/` and `Util/` → `Singularity/Storage/`;
`Object::GeometryType` → `ShapeKind`; the placement and tool fields off `Person` and onto
`Singularity::Core::CreationChannel` (refusal #1 being enforced); `Zone` off `Object` and
onto `Singular`, losing the tint and brush a canvas has and a space does not.

There are no known failures, deliberate or otherwise.

**`chess_app_test` was a real, open regression (Bugs.md #7, 2026-08-24) and is now fixed and
guarded.** The Zone identity store lost the relation graph — every `saves/zones/*/zone.json`
carried `formationRelations: []` while `saves/worlds/chess.json` carried 38 — so
`law-chess-click` and `law-chess-select` reported `conditions-failed`. Three defects fixed in
series in `Serialization.cpp` / `ZoneManager.cpp`; full trace in
`docs/audits/ZONE_RELATION_GRAPH_LOSS_AUDIT_2026-08-24.md`, resolution recorded in
`docs/Agenda/Tasks/Specific Tasks/Zone_Relation_Graph_Loss.md`. Guarded independently by
`tests/zones/zone_relation_roundtrip_test.cpp` so `chess_app_test` is not the only witness. If
this test goes red again, it is a real regression — read the audit before touching either
file.

**`webgpu_particle_test` landed 2026-08-24.** It called `WebGpuRenderer::drawParticles(FieldNode&, int)`,
which had never existed in any commit — the test was written against an unbuilt feature and
had never compiled, and was excluded from the default target via `PENDING_FEATURE_TESTS` in
`CMakeLists.txt` so the build was not red for work nobody had started. `drawParticles` now
exists (`src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`): it visualizes a `FieldNode`'s
`VectorField` as GPU points, reusing the existing flat-colour pipeline machinery
(`flatPipeline`/`drawFlat`) with `WGPUPrimitiveTopology_PointList` rather than building a
parallel one. Particle positions are procedural — an index-seeded xorshift hash places each
particle inside the field's origin/scale box, then carries it along `baseFlow` by a random
phase — so the call is stateless (no simulation buffer to step forward, no `dt` the caller
would need to supply) and deterministic for a given `(field, count)`. `PENDING_FEATURE_TESTS`
is now empty; add a name back to it if a future test lands ahead of its feature, and take it
off the moment that feature lands. Do not read a red suite as evidence that your change broke
something until you have checked it here.

**`frame_lag_test` landed 2026-08-24 and is the one test in the suite that measures
durations.** It is also the slow one — ~18 s, most of the suite's 33 s — and it carries
`RUN_SERIAL TRUE`, so `ctest -j4` gives it the machine to itself rather than measuring it
while three other tests fight it for cores. Run it alone with `cmake --build build --target
lag`, or `ctest --test-dir build -L lag`.

It asks four questions, ordered by how much each leans on the clock: does per-frame cost
grow faster than the world does (a *ratio* of times, so a slow machine and a fast one
agree); on an idle world, is the frame still finding work (no clock at all); does the
authored chess world hold a 60 Hz frame without a hitch or drift; how long does opening
that world take.

Its output has **four** verdicts, not two, and the distinction is load-bearing:

| | means |
|---|---|
| `ok` | meets the aspiration — what a frame *ought* to cost |
| `STANDING` | misses the aspiration, matches `tests/singularity/frame_lag_baseline.txt`. A known cost, already on the to-do list under **Performance**, reprinted every run. **Not a failure.** |
| `LAG` | worse than the baseline. *This* is the failure: something you just did made Earthcall slower |
| `IMPROVED` | comfortably better than the baseline — re-record it or the tripwire stays slack |

So the suite is green while today's real lag is read aloud on every run. Do not silence a
`STANDING` line by widening the baseline; fix the item or leave it standing.

**It knows when it cannot trust the clock.** A fixed reference workload is timed before and
after the measurements. If the machine's speed moved by more than 1.4x in between — a build,
a browser, another agent — every timing verdict is still printed but **none of them may fail
the run**, and `--rebaseline` is refused outright. The idle-world invariants never touch a
clock and are enforced either way. Every wall-clock budget is also divided by that
calibration factor, so the baseline file means roughly the same thing on another machine.

Re-record with `./build/frame_lag_test --rebaseline`, on a quiet machine, only downward, and
say so in the commit message. `./build/frame_lag_test --calibrate` prints this machine's
speed against the reference and stops; if that number will not hold still, nothing else the
test measures will either. Pass a save path as the first argument to measure a different
world (the baseline is only ever written from the default one).

What it found on its first run is written up in `docs/audits/2026-08-24_frame_lag_probe.md`
and tracked in the to-do list's **Performance** section: `Physics::updateBodies` is all-pairs
with no broadphase, so `Zone::update` costs 1.1 / 3.3 / 11.1 / 40.7 ms at 64 / 128 / 256 / 512
objects — a fitted `n^1.75`. The chess world itself is fine at 35 objects (6-7 ms simulation
half, no hitch, 1.4-2.1 s to load). `-O0` is a real caveat on every absolute number and is
named in both. The audit's §2 also records a draft of itself that had to be withdrawn because
it was written from a laptop at load average 90 — read it before you report a frame rate.

### Tests that exist because of what got past the suite, not to pad it

Each guards a class of silent failure this repository has actually shipped, so keep them
honest rather than convenient:

| Test | Guards against |
|---|---|
| `paint_test` | paint written through a *shared* material (repaints the world), and a `color` property that does not read back what was written — `propSetColor` was an empty function for a month |
| `object_roundtrip_test` | a field `to_json` writes and `from_json` drops. `faceColors` was write-only for a month with the write side making it look covered; `serialization_compat_test` covers the msgpack/Frontier *plumbing* and cannot see this |
| `channel_paths_test` | the law-authoring picker offering a property path no registry answers. `CreationChannel::activeShapeKind` was advertised and unregistered, so every law reading it silently fell back |
| `no_black_box_test` | refusal #6 — a being that registers nothing, a registry built twice, a setter that accepts a write and drops it, and a registered property the picker never offers |
| `ground_plane_test` | a subsystem deciding, by list index, which being is the floor. `Zone::update` (formerly `World::update`) fell back to `_objects[1]` when nothing carried `baseline=ground` — and a Zone starts empty, so the **second being a Person spawned** silently became the ground. `Physics::integrate` then clamped its *centre* to its own *top*, lifting it half a height per substep, raising the floor, lifting it again: the whole world climbed at 30 m/s and every later spawn was teleported up to it |
| `test_observation_load_test` | loading a test dump so a Person can see it. The Developer window called `loadState`, which erases Home, and never wrote `Person.position`, so `LocomotionChannel` snapped the camera back onto wherever the Person was standing. The live office is `ZoneManager::loadTestObservation` |
| `world_switch_test` | two saved sessions mixing. json/.ecsave twins and a 0-byte file listed as separate worlds; a refused load retitled the live world; same-named Zones share identity (loading the other session does not rewind the Zone) |
| `save_roundtrip_test` | Person Save As / Load. `saveStateWithLog` used to skip the first two Zone objects; `loadState` used to move the camera and not `Person.position`. json+.ecsave of one stem must list as one world; objects in a non-active Zone must survive |
| `unsaved_preserve_test` | load used to erase unsaved work. Identity-stable Zones are kept across session load; `loadState` also writes `saves/backups/before-load.json` so Restore unsaved can rewind, and loading that slot does not re-stash over itself |
| `zone_identity_test` | Home was copied into every "world" file, so loading another session showed an empty Home. Zones now have `saves/zones/<id>/zone.json`; sessions reference them; fork/diff are first-class |
| `frame_lag_test` | the frame quietly getting dearer. Guards three things nothing else could see: that per-frame cost stays sub-quadratic in the population, that an *idle* world fires no laws and grows no objects (CLAUDE.md's edge-not-level rule, measured rather than asserted), and that no being registers a property path twice (the `buildProperties()`-in-a-constructor bug, which doubles the cost of every law evaluation and shows up nowhere else) |
| `zone_home_ontology_test` | manifesto Home/Zone: primary Home kernel-locked per Person (not "any owned Zone"); owner is Person/Relationship/Community; community-home / community-zone authored kinds; AuthorZone mints extras; unused `class Home` retired |

If you add a field to `Object` that `to_json` writes, add it to `object_roundtrip_test`.

`channel_paths_test` and `no_black_box_test` are **inverses, and neither implies the
other**: the first walks advertised → registered (nothing is offered that cannot answer),
the second walks registered → advertised (nothing governable goes unoffered). A path can
fail either direction independently. See `docs/architecture/ontology/NO_BLACK_BOX.md` §7.

`no_black_box_test`'s first run found two live bugs, which is the argument for it existing:
`CreationChannel` registered all 21 of its properties **twice**, and 15 of those 21 were
governable but unreachable from the authoring picker. Both are fixed — the channel is now
*probed* into `knownPathOptions()` rather than hand-listed, so that drift cannot recur.

### Two traps the suite now holds you to

**Do not call `buildProperties()` from a constructor.** `Singular` builds the registry
lazily behind `_propertiesBuilt`, which a constructor call does not set, so everything is
registered a second time on first access — the authoring picker then offers every path
twice. `ForeignChannel.cpp` carries the standing note; `PhysicsLawBridge` and
`PhysicalChannel` follow it; `CreationChannel` was the one channel that ignored it.

**Scratch probes belong in `scratch/probes/`, not `tests/`.** If you build one through the test
target for convenience, remove the copy from `tests/` *and reconfigure* when finished — an
outside `git add -A` will otherwise commit it, and the stale CMake target will break the
next full build.

**`tests/` is grouped by ontological region** (`constructed-being/`, `person/`, `zones/`,
`law/`, `singularity/`, `identity/`, plus `support/` for shared headers). CMake still
globs `tests/**/*.cpp`; the ctest name is the file stem. See `tests/README.md`.

---

## Two live-system notes

**`Core::Engine`'s core subsystems are now actually constructed (fixed 2026-08-13).**
`_lawManager`, `_player`, `_camera`, `_mouseHandler`, `_keyboardHandler` were declared as
`unique_ptr` members but never allocated anywhere post-"Game" refactor — `Engine::initLogic()`'s
first line (`_lawManager->connectToEventBus()`) was a null-pointer deref waiting for the first
caller. Nothing had called it, so nothing had noticed: `Tool::Pottery3D`/`Rotate3D`/etc. (the
already-"reconnected" 3D tools) were never actually invoked from `Engine::tick()` either, and
`getPerson()`/`getLawManager()` weren't even *defined*, only declared — so nothing exercised the
null. They're allocated now, in `EngineInit.cpp::initLogic()`, before anything touches them; the
live `earthcall` binary runs to its main loop as of this fix (verified by running it). If you
hit a null `_camera`/`_player`/etc. deref elsewhere, that subsystem is now real — look for a
missing accessor definition (`Engine.cpp`, next to `getMouseHandler()`/`getCamera()`) before
assuming the pointer itself is the problem.

**A restored, working example of a Law actually firing end-to-end lives at
`saves/tests/shape_generator_3d_law.json`** — the "Tool: Shape Generator 3D" law from
`basic_cube_law_test.cpp`, seeded through the real `ZoneManager::saveState` and round-trip
verified through `ZoneManager::loadState`. Load it from the "Developer: Test World Saves" panel
(`DeveloperToolsWindow.cpp`) to see a Person-authored law spawn a cube on click. The same file's
sibling panel, "Developer: 3D Create Tool", is `Tool::ShapeGenerator3D` restored as a direct-spawn
developer bypass (see its doc comment in `Tool.cpp` for how the two paths stay distinct — L key
arms the law path, the panel's own controls drive the direct-spawn path).

---

## Keeping the tree searchable

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

---

## clangd

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

## The tree, in detail

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

**`ConstructedBeing/`** was named `Form/` until 2a11f94; the docs corpus still says Form in
places. `Object`, `Lexeme`, and `Property` live under `ConstructedBeing/Singular/`;
`ObjectConcept` is the ONE set-to-set machine (`Singular/Object/Creation`) — do not add
a second one. See §7 of `LAW_AND_CREATION_SYSTEM.md`; a `Concept`/`SynthesisSystem` pair
lived beside it until 2026-08-11 and was deleted. Formation is `Relation/Formation`
(all includes point directly to `Relation/Formation/Formation.hpp`). Lexeme is a Singular, not a Language-channel type.

**Test harnesses (`TestLabInterfaces/`, `TestLabAI/`)** sit at the repository root as external harness interfaces (renamed from `TestLab/` in commit e813b6b6 to distinguish interface tools from the ontology).

