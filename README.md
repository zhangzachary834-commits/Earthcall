# Earthcall

Earthcall is **an ontology with an engine attached**, not a game engine with some philosophy in the docs. The architecture is load-bearing: things that look like ordinary engineering decisions here (adding a class, adding a folder, adding an enum value) are ontological claims, and most of them are refused. 

This is a platform for the simulation of beings, consciousness, physics, and world-building through a unified Law system. Instead of hardcoded collisions and generic physics engines, Earthcall relies on a deeply integrated ruleset governed by dynamic 'Laws'.

## Core Philosophy

- **Ontology First:** The top-level directory structure reflects the ontology of the universe (`Form`, `Person`, `Relation`, `Singularity`, `ZonesOfEarth`, `OurVerse`). It does not organize by technical subsystems.
- **Law-Driven Simulation:** Beings interact through a network of active Laws rather than hardcoded C++ logic. C++ provides the execution substrate for these Laws.
- **First Mover Authoring:** The world is authored dynamically. Beings are data, not classes. We don't define a `Tree` class; we define the components and laws that comprise a tree.
- **Web First Architecture:** Earthcall embraces a modern WebUI built via embedded React/Vite, integrating directly with a high-performance C++ backend. Modalities (Audio, Language, Network) act as channels under the `Singularity` directory.

## Documentation

The foundation of the project's architecture is thoroughly documented. You **must** read the documentation before writing code.
- Start with the manifesto in `docs/architecture/core/EarthcallOurverse.md`.
- See `docs/architecture/` for the complete architectural guidelines (e.g. `NEW_KIND_FRAMEWORK.md`, `DIRECTORY_ORDERING.md`).
- For AI agents, see `AGENTS.md` in the repository root for a quick reference router and non-negotiables.

## Building

### Start Earthcall with one click (macOS)

In Finder, double-click [`Run Earthcall.command`](Run%20Earthcall.command). It opens a
Terminal window, configures CMake with Earthcall's required dependencies, rebuilds the
WebGPU app, and starts it. Leave that Terminal window open while Earthcall is running.

From a Terminal, the same action is:

```sh
./scripts/build.sh webgpu run
```

Earthcall builds with CMake. The project requires linking a vendored OpenSSL and setting policy limits for dependencies. The following configuration is required:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DOPENSSL_ROOT_DIR="$PWD/local_deps/openssl-3.0.13" \
  -DOPENSSL_INCLUDE_DIR="$PWD/local_deps/openssl-3.0.13/include" \
  -DOPENSSL_CRYPTO_LIBRARY="$PWD/local_deps/openssl-3.0.13/libcrypto.a" \
  -DOPENSSL_SSL_LIBRARY="$PWD/local_deps/openssl-3.0.13/libssl.a"

cmake --build build --target earthcall -j8
ctest --test-dir build --output-on-failure -j4
```

The Python backend starts from `src/Integration/py/app.py`.
