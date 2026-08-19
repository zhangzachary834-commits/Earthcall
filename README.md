# Earthcall


Earthcall is **a Person-centered ontology that orders the engine attached to it.** The engine here is not the order of truth in Earthcall—the ontology is, and the engine serves as the vessel for that.


Unlike most of software history, Earthcall is built on a foundational conviction that all beings must be represented not by illusions hiding a teleologically indifferent operating system, but rather according to what they are—their actual, innate structure in reality. Instead of hardcoding everything as mere entities and behavior, Earthcall distills all conceivable creaturely beings down to their most fundamental primitives, delegating the real-world substance to be created and governed by real human beings.


Earthcall is not just an app limited to merely simulating physics, creating art, or building software, even though it is capable of all of those things. Earthcall is instead built as a vessel capable of holding any artifact of human thought. People can represent their thoughts, ideas, and intentions within dedicated primitives that compile, execute, and represent them faithfully on the hardware substrate.


The philosophy is load-bearing and encoded as the very fabric of the program: things that look like ordinary engineering decisions here (adding a class, adding a folder, adding an enum value) are ontological claims, and most of them are refused.


For maximum generativity and expressiveness, Earthcall Law system enables people to author unique behavior at runtime. Instead of hardcoded collisions and generic physics engines, Earthcall relies on a deeply integrated ruleset governed by dynamic 'Laws'. People author the conditions the Laws activate under, and author the mathematical functions and operations that Law executes upon their Person-designated targets.


Laws are executed with math operations because math is the universal language of logic. Mathematics provides a universal language for expressing logic, relationships, quantities, change, uncertainty, and structure. With this capacity, there is no need to ship hardcoded functions within Earthcal for things like “move Object X to Y location”, “change Color hexcode of Object X “, and “set Camera’s pitch/yaw/roll to ____.“ Instead, we realize those things are all variations of either a “set” operation or an “add value” operation, and author them as Laws during runtime.


## Core Philosophy


- **Ontology First:** The top-level directory structure reflects the ontology of the substrate categories (`ConstructedBeing`, `Person`, `Relation`, `Singularity`, `ZonesOfEarth`, `Identity`). It does not organize by technical subsystems. Ourverse the being lives under `ZonesOfEarth/`; the Person-facing tools live under `Singularity/FirstMoverWindowTools/`.
- **Law-Driven Simulation:** Beings interact through a network of active Laws rather than hardcoded C++ logic. C++ provides the execution substrate for these Laws.
- **First Mover Authoring:** The world is authored dynamically. Beings are data, not classes. We don't define a `Tree` class; we define the components and laws that comprise a tree.
- **Web First Architecture:** Earthcall embraces a modern WebUI built via embedded React/Vite, integrating directly with a high-performance C++ backend. Modalities (Audio, Language, Network) act as channels under the `Singularity` directory.


## Documentation


The foundation of the project's architecture is thoroughly documented. You **must** read the documentation before writing code.
- Start with the manifesto in `docs/core/EarthcallOurverse.md`.
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


The Python backend starts from `src/Singularity/Foreign/py/app.py`.



