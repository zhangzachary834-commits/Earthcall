# Directory Ordering

**The repository tree is the ontology. Language, toolchain, and process boundaries are
implementation details and never appear at the top level.**

**Status:** Stage 1 executed (the `sight-cpp` / `backend-python` split is gone; one
source root). Stage 2 executed (non-ontological regions relocated into the `Singularity/`
modality layer: `Rendering/` → `Singularity/Screen/`, `Integration/` → `Singularity/Foreign/`,
`Perspective/` → `Singularity/Input/` & `Person/Perspective/`, `Util/` → `Singularity/Storage/`,
and `Form/` renamed to `ConstructedBeing/`).
**Companion docs:** `NEW_KIND_FRAMEWORK.md` (the same refusal applied to types rather
than folders), `SUBSTRATE_ORDERING.md` (why this is not cosmetic),
`LAW_MIGRATION_FRAMEWORK.md`, `EarthcallOurverse.md`.

---\

## 0. What this document is for

The tree used to open like this:

```
Earthcall/
  sight-cpp/          ← "the C++ one"
  backend-python/     ← "the Python one"
```

That is a **category error of exactly the kind `NEW_KIND_FRAMEWORK.md` refuses**, made
one level higher up. It asserts that the primary division in Earthcall is *which
language a file is written in* — that the C++ that manages a WebSocket and the Python
that manages a WebSocket are two different kinds of thing, while the C++ that manages a
WebSocket and the C++ that defines a Person are the same kind of thing.

Both halves of that are false. `WebSocketClient.cpp` and `engine_server.py` are the
**same being**: the Network modality's channel, split across two processes for reasons
of runtime, not of ontology. `Person.cpp` and `WebSocketClient.cpp` share nothing but a
compiler.

A directory tree is a claim about what kinds there are. This one was claiming that
Earthcall's kinds are *languages*.

**The one-sentence thesis:** *a directory earns its place at the top level by naming a
kind of being or a mode of the machine — never by naming a tool, a language, a process,
or a team.*

**The test, applied to any proposed directory:** *would this folder still exist, with
this name, if the whole system were rewritten in one language tomorrow?* `ConstructedBeing/` would.
`Person/` would. `Singularity/Network/` would. `sight-cpp/` and `backend-python/` would
not — which is the proof they were never directories, only shipping labels.

---\

## 1. The rule

```
TOP LEVEL = the ontology + the modality layer.

  A directory may sit at the top level if and only if it names
    (a) a kind of being        — ConstructedBeing, Person, Relation, ZonesOfEarth, Identity
    (b) a mode of the machine  — Singularity (and its modalities beneath)

  The Person-facing authorship surface is not a top-level region. Tools, chat,
  and controls live in Singularity/FirstMoverWindowTools/. Ourverse the being
  lives in ZonesOfEarth/Ourverse/. There is no src/OurVerse/.

  Everything else is either
    (d) inside one of those, or
    (e) explicitly marked as not-yet-ordered (§6) — the workshop and the
        foreign, named as such, so the disorder is visible rather than
        disguised as structure.
```

Clause (e) is load-bearing and is the difference between an ontology and a pose.
`build/`, `third_party/`, `local_deps/`, `Legacy/` are not ontological and never will
be. The honest move is to name them plainly rather than to invent an ontological gloss
for a folder of vendored OpenSSL. `SUBSTRATE_ORDERING.md` §2 makes the same discipline
quantitative: an artifact without provenance counts as hand-written, and *the ratio
never flatters itself.*

---\

## 2. The top level, as of Stage 2

```
Earthcall/
  src/                     the one source root — all languages
    ConstructedBeing/      Singular · Object · ObjectConcept · Formation · Property · Material
    Person/                Person · Soul · Body · Relationship · Perspective
    Relation/              Relation · RelationManager
    ZonesOfEarth/          Zone · Home · Physics · AuthorsOfLaw (Law) · Ourverse
    Singularity/           the modality layer — where language stops mattering
      Core/                Engine · EventBus · CreationChannel
      Audio/               the Sound modality (AudioSystem)
      Language/            the Symbolic modality (Lexeme, LanguageSystem)
      Network/             WebSocketClient.cpp · WebSocketServer.cpp
                             py/  engine_server.py · events.py
      OntoMath/            authored mathematics (Field, Function, CurveModel, Operations)
      Foreign/             the Foreign software modality (ForeignChannel at the root)
                             Adapters/  MacOSAccessibilityAdapter
                             API/       EarthcallAPI, SecurityManager
                             Web/       WebIntegration, RealWebView, WindowManager, IntegrationManager, web_ui
                             Sync/      AsyncStateLogger, ForeignSyncManager, InferenceLawBridge
                             py/        app.py
      Input/               the Input modality
                             Keyboard/     KeyboardHandler
                             Mouse/        MouseHandler
                             Locomotion/   LocomotionChannel
                             Interaction/  InteractionChannel, ControlPatterns
      Screen/              the Screen/Light modality (Renderer, WebGPU, GL, BrushSystem)
      Storage/             the Storage modality (SaveSystem, CloudStorage, BinaryPack, Frontier)
      Physical/            the Physical hardware modality (PhysicalChannel)
      FirstMoverWindowTools/ CreatorConsole, CreationTools, Controls, Chat, Tools
    Identity/              First Mover register, identity ledger, key pairs, claims
    Legacy/                the graveyard — not yet ontologically placed

  docs/  tests/  examples/  scripts/  saves/  scratch/  web_ui/     the workshop
  third_party/  local_deps/  imgui/                                 the foreign
  CMakeLists.txt  .gitmodules  Makefile.legacy                      the toolchain
  build/  logs/                                                     output (ignored)
  TestLab/  TestLabAI/                                               strays (§6)
```

`migrate_saves.cpp`, formerly listed here as a root-level stray, was moved to
`scratch/scripts/migrate/migrate_saves.cpp` on 2026-08-13 — it built no CMake target, so it was a
one-off tool rather than live source, matching the same "move, never discard" precedent
as `scratch/attic/` below. On 2026-08-14 the scratch root was subdivided (`probes/`,
`legacy/`, `scripts/`, `fixtures/`, `audits/`, `experiments/`); see `scratch/README.md`.

`scratch/attic/` holds what used to sit loose at `sight-cpp/`'s root — one-off
probes (`test_parse.cpp`, `test_variant.cpp`), fixtures (`save.json`), logs, and
five stale compiled binaries that were tracked (`dump_save`, `pack_save`,
`earthcall_webgpu`, `test_parse`, `test_parse2`). They were moved rather than
deleted: a refactor may relocate, never discard. Untracking the binaries is a
separate decision for a separate commit.

---\

## 3. Where language goes

**Rule: language is a leaf, never a branch.** Within an ontological region, source of a
second language lives in a subfolder named for that language, at the leaf:

```
src/Singularity/Network/
  WebSocketClient.cpp        ← the engine's half of the channel
  WebSocketServer.cpp
  py/
    engine_server.py         ← the backend's half of the SAME channel
    events.py
```

This is the flagship case and worth stating plainly: **those files are now neighbours
because they are the same thing.** They were previously separated by the width of the
whole repository, and nothing about the system justified it except that one of them had
to be interpreted.

The `py/` leaf exists for tooling, not for ontology — Python needs package-shaped
directories to import, and fighting that buys nothing. A leaf-level concession to a
language's packaging model is a different claim from a top-level split: the first says
*"this is how Python is spelled here,"* the second says *"Python is a kind of thing."*

**The wire test, inherited from `NEW_KIND_FRAMEWORK.md` §7c:** a message crossing
between the two halves of a channel must be indistinguishable in kind from any other
message about the world — property writes, `ECA` events, Relation assertions. If the
schema names the language, the process, or the domain, the split has grown back inside
the protocol where the directory tree can no longer show it to you.

---\

## 4. Where processes go

A process boundary — engine vs. backend, client vs. server, native vs. WASM — is a
**runtime deployment fact**, in the same category as a build flag. It is expressed in
`CMakeLists.txt` and in launch scripts, which is where deployment facts belong. It is
never expressed in the source tree's shape.

The reason is the same as for language: the beings on both sides of a process boundary
are frequently the *same* beings. A Zone that exists in the engine and is mirrored to
the web UI is one Zone. Two folders would make it two.

---\

## 5. Stage 2 — the completed placements

The four non-ontological regions formerly at `src/` top-level have been fully relocated
into their proper ontological homes:

| Former Region | Relocated To | Status & Rationale |
|---|---|---|
| `Rendering/` | `Singularity/Screen/` | **Done.** Output channel for the Screen/Light modality. Rendering is how Earthcall acts in the light modality. |
| `Integration/` | `Singularity/Foreign/` | **Done.** The Singularity-level modality holding hardwired connectors to external applications (`ForeignChannel`, `EarthcallAPI`, `SecurityManager`). |
| `Perspective/` | split | **Done.** `KeyboardHandler`, `MouseHandler` → `Singularity/Input/`; `PersonPerspective`, `AvatarHandler` → `Person/Perspective/`. |
| `Util/` | `Singularity/Storage/` | **Done.** Persistence and serialization moved to the Storage channel (`SaveSystem`, `CloudStorage`, `BinaryPack`, `Frontier`). |
| `Form/` | `ConstructedBeing/` | **Done.** Renamed to clarify domain of constructed entities (`Singular`, `Object`, `ObjectConcept`, `Formation`, `Property`, `Material`). |

| Subsystem | Action | Why |
|---|---|---|
| `Legacy/` | dissolve | the graveyard; burn down gradually. |

**Stage 2's exit test achieved:** nothing at `src/` top level is named after a technology.

**Stage 3, further out:** `src/` itself dissolves and the repository root *is* the
ontology, with the workshop and the foreign as its only non-ontological neighbours.
That is a one-line CMake change whenever it is wanted; it is held back only because a
root with 200 entries is worse to navigate than a root with 20 until the stage-2
regrouping has thinned it.

---\

## 6. The not-yet-ordered, named honestly

| Directory | What it is | Standing |
|---|---|---|
| `third_party/`, `local_deps/`, `imgui/` | vendored foreign source | permanent. Foreign code is foreign; pretending otherwise would be the mirror error of the one this document fixes. |
| `build/`, `logs/` | machine-specific output | permanent, and git-ignored. |
| `docs/`, `tests/`, `examples/`, `scripts/`, `scratch/`, `saves/`, `web_ui/` | the workshop — things *about* the world rather than *in* it | permanent. `SUBSTRATE_ORDERING.md` contemplates a future where the world reads its own tests and docs as beings; until it does, they are workshop. |
| `Legacy/`, `Legacy Depricated/` | superseded code, retained | temporary by intent. Named honestly, which is why it is tolerable. |
| `../../../TestLabInterfaces/`, `TestLabAI/` | standalone experiments with their own `main` | temporary. Fold in or retire. |

---\

## 6b. Building from the new root

The move required exactly **two** edits to `CMakeLists.txt` — `../imgui` → `imgui`,
twice — because every `#include` in the tree was already written relative to `src/`
(`#include "ConstructedBeing/Object/Object.hpp"`), and `src/` is still called `src/`. Nothing else
in 299 source files changed. That the whole rearrangement cost two lines is the
strongest available evidence that `sight-cpp/` was a label rather than a boundary.

Two build-environment facts are **not** consequences of the move, but you will meet
them on a fresh configure because the old `CMakeCache.txt` (which carried them) lived
in `sight-cpp/build/` and went with it:

- **OpenSSL** is vendored and prebuilt at `local_deps/openssl-3.0.13/` (`libssl.a`,
  `libcrypto.a`). There is no system or Homebrew OpenSSL on this machine, and
  `find_package(OpenSSL REQUIRED)` at `CMakeLists.txt:16` carries no hint, so it must
  be pointed at the vendored copy.
- **CMake 4.x** (the pip-installed one at `~/Library/Python/3.9/…/cmake`) rejects
  websocketpp's `cmake_minimum_required(VERSION 3.0)`, which arrives through
  `FetchContent`.

The working invocation:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DOPENSSL_ROOT_DIR="$PWD/local_deps/openssl-3.0.13" \
  -DOPENSSL_INCLUDE_DIR="$PWD/local_deps/openssl-3.0.13/include" \
  -DOPENSSL_CRYPTO_LIBRARY="$PWD/local_deps/openssl-3.0.13/libcrypto.a" \
  -DOPENSSL_SSL_LIBRARY="$PWD/local_deps/openssl-3.0.13/libssl.a"
cmake --build build --target earthcall -j8
```

The Python backend now starts from `src/Singularity/Foreign/py/`:

```sh
python3 src/Singularity/Foreign/py/app.py
```

`app.py` puts `src/Singularity/Network/py/` on `sys.path` and imports
`engine_server` / `events` from there — the only source edit the Python move needed,
and it exists precisely because the two halves of the Network channel are now
correctly separated by a modality boundary instead of incorrectly joined by a
language one.

---\

## 7. Adding a directory — the checklist

```
1. Does it name a kind of being, or a mode of the machine?
       NO  → it is not a top-level directory. Put it inside the region that
             uses it, or under the workshop / foreign heading.
2. Would it survive a one-language rewrite, with this name?
       NO  → it names a tool, a language, a process, or a team. Refused.
3. Is it a modality?
       YES → it goes under Singularity/, and NEW_KIND_FRAMEWORK.md §2's
             Admission Test governs whether it may exist at all.
4. Is it a domain noun (Robotics/, Vehicles/, Plants/)?
       YES → REFUSED outright. NEW_KIND_FRAMEWORK.md Floor §6. Domain nouns
             are authored in-world as concepts, never carved into the tree.
5. Is it a second language for something that already exists?
       YES → it is a py/ (or ts/, or rs/) leaf inside the existing region.
             Never a peer, never a root.
```

---\

## 8. Why this is not housekeeping

`NEW_KIND_FRAMEWORK.md` §11 makes the argument for types; it holds unchanged for
folders, because a folder is where a type goes to become permanent.

A tree split by language teaches every reader — every contributor, every AI agent
starting cold, and eventually the author on a tired day — that the system's real joints
are its toolchain's joints. That lesson gets applied. It is precisely the reasoning that
produced `src/Robotics/` as a peer of the ontology: the tree said top-level directories
are for subsystems, so a subsystem asked for one, and it was not wrong about what the
tree said.

The tree is the first document anyone reads and the only one nobody skips. Make it say
the true thing.
