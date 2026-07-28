# Earthcall

A 3D world where the rules are authored from inside it. Objects, zones, people
and relations are all *beings*; **laws** are beings too — conditions and actions
composed in an in-world graph editor, saved with the world, and enforced by the
same engine that draws it.

The application is `sight-cpp/` — C++17, OpenGL 2, Dear ImGui, macOS. Roughly
49,000 lines of hand-written source, with an experimental WebGPU backend and an
embedded WebKit view alongside it.

---

## Building

Requires macOS (the integration layer is Objective-C++ against WebKit and
Cocoa), a C++17 compiler, and GLFW.

```sh
brew install glfw pkg-config
cd sight-cpp
make            # optimized build -> ./earthcall
make run
```

`make BUILD=debug` builds unoptimized for a debugger. **Run `make clean` when
switching between debug and release** — make tracks timestamps, not the flags an
object was compiled with, so it will happily keep stale objects otherwise.

GLFW is located with `pkg-config`, which works on Apple Silicon
(`/opt/homebrew`), Intel (`/usr/local`) and Linux alike. Without pkg-config the
build falls back to Homebrew's default prefix.

## Tests

24 test programs, ~700 assertions, weighted toward the law and ontology core.

```sh
cd sight-cpp
make test              # everything
make test-math         # one suite (see the Makefile for the full list)
make test-security
```

Tests link the whole application minus its entry point, so the first run builds
the world. They assert with `assert()`, which is why the build deliberately does
**not** pass `-DNDEBUG`.

## Layout

| Path | What lives there |
|---|---|
| `sight-cpp/src/Form/` | `Singular`, `Object`, geometry, materials, per-face textures |
| `sight-cpp/src/ZonesOfEarth/` | Zones, worlds, physics, and `AuthorsOfLaw/` — the law model |
| `sight-cpp/src/Singularity/` | Game loop, event bus, `OntoMath/` (the authored-function calculus) |
| `sight-cpp/src/Person/` | Avatars, bodies, relationships |
| `sight-cpp/src/Rendering/` | GL backend, ImGui windows, the law-graph editor, WebGPU |
| `sight-cpp/src/Integration/` | Embedded WebKit view, JS bridge, `SecurityManager` |
| `sight-cpp/docs/` | 20 architecture and design documents |
| `sight-cpp/tests/` | The test programs |
| `backend-python/` | A small Flask page, separate from the app |

`sight-cpp/saves/` holds save games. It is **not** version-controlled — saves
are runtime output, and they get large.

## The Python backend

Independent of the C++ application.

```sh
cd backend-python
python3 -m venv venv && . venv/bin/activate
pip install -r requirements.txt
python app.py                       # http://127.0.0.1:5000
EARTHCALL_DEBUG=1 python app.py     # opt in to the Werkzeug debugger
```

The debugger is off by default deliberately: it exposes an interactive Python
console to anyone who can reach the port and provoke a traceback.

## Notes for contributors

* The build is error-free under the project's own `-Wall -Wextra`. CI promotes
  four warning classes to errors — `switch`, `return-type`, `unused-result`,
  `uninitialized` — because each of those means something is genuinely wrong.
  `-Wswitch` is the load-bearing one: enum-driven UI arrays are guarded by
  `static_assert`, and a new `ActionNode::Kind` should break the build until it
  has both a label and an editor. (About 60 cosmetic `-Wunused-parameter`
  warnings remain and are not gated; clearing them is welcome.)
* `AUDIT.md` records a full review of the codebase, what was fixed, and what is
  still open.

## Status and license

Early and moving fast; interfaces change without ceremony.

No license has been chosen yet, which means default exclusive copyright — the
code cannot be reused or redistributed until one is added.
