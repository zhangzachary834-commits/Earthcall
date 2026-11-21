# Earthcall

Earthcall is **a Person-centered ontology that orders the engine attached to it.** The ontology is the order of truth; the engine serves as its vessel. Persons author beings, meanings, relationships, mathematical forms, and Laws, and the machine gives those intentions an executable, visible, audible expression.

Zachary Zhang conceived Earthcall as a computational ontology grounded in the God-created relationship between human intention and the machine. Its **Ourverse** is a vessel of shared human life ordered toward Christ: a creation meant to glorify God, hold encounters with Christ and each other, and give those encounters a language in which their meaning can be articulated. The [Earthcall Ourverse Manifesto](docs/core/Earthcall%20Ourverse%20Manifesto/EarthcallOurverse.md) carries that human origin and telos.

Earthcall's ambition is to hold artifacts of human thought across art, software, physical simulation, language, and shared worlds. The governing question is what minimal set of invariants can preserve the greatest expressive ceiling. A tree, an instrument, or a button can be authored through existing beings, Relations, Properties, mathematics, and Laws. Their substance belongs to the authored world; C++ supplies the irreducible mechanisms through which that world senses and acts.

## The authored world

The philosophy is load-bearing. Adding a class, a directory, or an enum of domain kinds makes an ontological claim. Earthcall therefore keeps its source regions aligned with the foundational distinctions they serve:

| Region | What it holds |
|---|---|
| `ConstructedBeing/` | Singulars such as Objects and Lexemes, creation mechanisms, and Materials; Property is a non-Singular predicate beneath `Singular/Property/`. |
| `Person/` | Actual human Persons, Soul, Body, Voice, Perspective, and human relationships. |
| `Relation/` | First-class Relations and Formations: meaningful connections and structured wholes with their own identity. |
| `ZonesOfEarth/` | Zones, Homes, Ourverse, Physics, and AuthorsOfLaw. |
| `Time/` | Relative Timelines, instant-or-interval Moments, and Events as transition Moments. |
| `Identity/` | Identity, signing keys, and Person-granted First Mover standing. |
| `Singularity/` | Machine modalities, mathematical execution, storage, and First Mover tools. |

**Laws are authored data.** Persons specify when a Law listens, what must hold, and which operations it performs on its designated targets. Set, composition, flow, creation, and mathematical expressions give different intentions a common executable language. Compiler Metalaws connect authored words to action models; the modality channels carry out the admitted effects.

**OntoMath holds mathematical meaning.** Scalar/vector fields, piecewise expressions, matrices, and admitted calculus are shared mathematical structures. Screen, Audio, Physics, and other consumers bind coordinates and execute the mathematics they support. A backend's representation tag does not define an authored category.

**Persons remain humans.** An AI helping author Earthcall acts as a Person-granted First Mover; an in-world mechanism is an authored being. Neither is another Person. Authorship, ownership, governance, dependency, and Constitution are distinct grounds of standing, and bodily actuation requires its own consent. The [agent guidance](AGENTS.md) and [architecture corpus](docs/architecture/README.md) develop these boundaries.

## What you can author in the current prototype

Earthcall currently runs as a native C++/WebGPU application with a Terminal authoring channel and additional foreign/web integration surfaces. The following routes lead to working mechanisms, concrete examples, and their documented evidence:

| Authoring route | Start here |
|---|---|
| Write a Law sentence, preview it, or submit cooperating Laws in order | [Law authoring guide](docs/architecture/law/LAW_AUTHORING_CLI_GUIDE.md) |
| Create Objects with authored initial state, or derive admitted Singular kinds from explicit prototypes | [Universal set-to-set creation](docs/Agenda/Tasks/Specific%20Tasks/Interaction%20and%20Interface/Singular_and_Object_Set_to_Set_Creation/Singular_and_Object_Set_to_Set_Creation.md) |
| Express color, opacity, and named pixel regions directly as mathematical fields | [Direct Screen forms](docs/Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Direct_Screen_Forms/Direct_Screen_Forms.md) |
| Build a complete small pixel-art editor through authored Laws | [The Law Line atelier](docs/Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Law_Line_Pixel_Art_Editor/Law_Line_Pixel_Art_Editor.md) |
| Author richer 2D/3D forms, interactions, Materials, and assemblies | [Building 2D and 3D apps with Earthcall](docs/architecture/Design/Building%202D%20and%203D%20Apps%20with%20Earthcall%20Guide.md) |
| Connect foreign software through semantic structures | [HTML as Lexeme Formation](docs/architecture/Integration/HTML_LEXEME_FORMATION_BRIDGE.md) |

![Native capture of the Law-authored pixel-art atelier](scratch/verification/law-line-pixel-art-editor-2026-10-07/art-cyan.png)

*A 16×16 pixel-art atelier authored as one pasted line containing 276 cooperating Laws. Its canvas, palette, hit regions, painting, one-step undo/redo, and controls are authored state and behavior; Screen reads the mathematical field directly. This is a retained production Engine capture from the October 7–8 native witness. Zach confirmed the editor worked on October 8; [the task](docs/Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Law_Line_Pixel_Art_Editor/Law_Line_Pixel_Art_Editor.md) records the evidence and remaining acceptance checks.*

Earthcall is an actively developed prototype. Each feature's task distinguishes implementation, focused tests, native evidence, Person acceptance, and future work. Shared-memory custody, general Property access mediation, signed movement-consent machinery, broader direct Screen composition, and parts of multi-Person governance remain open; the [agent compass](docs/AGENT_COMPASS.md) maps those boundaries.

## Start Earthcall

### Native WebGPU app on macOS

In Finder, double-click [Run Earthcall.command](Run%20Earthcall.command). It opens Terminal, configures the required dependencies, incrementally builds `earthcall_webgpu`, and starts the app. Leave that Terminal window open while Earthcall is running.

From the repository root, the same action is:

```sh
./scripts/build.sh webgpu run
```

### Author your first Law

Use the running app's Terminal line editor. If it asks for authenticated presence, enter `Identity` and follow its key/unlock prompt, then enter the Law authoring domain:

```console
enter Identity
enter LawLine
help
```

`enter LawLine` selects the Terminal's authoring vocabulary; choose the visible world separately through the app's Zone controls. Submit this sentence, then click a visible Object:

```text
called "Golden Touch" when clicked then Set color to gold
```

The sentence registers a Law; the later click supplies its event and subject. Save Zone retains submitted Laws. Follow the [guide](docs/architecture/law/LAW_AUTHORING_CLI_GUIDE.md) for previews, targeting, persistence, creation, mathematical values, and troubleshooting.

For the pictured atelier, follow its [launch instructions](docs/Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Law_Line_Pixel_Art_Editor/Law_Line_Pixel_Art_Editor.md#launch-and-controls), then paste [the editor program](examples/law_line_pixel_art_editor.txt) once into Law Line and press Enter. Installing it again creates additional Laws; artwork persistence has its own acceptance check.

### Build and test manually

Run from the repository root. The vendored OpenSSL paths and CMake policy setting are required:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DOPENSSL_ROOT_DIR="$PWD/local_deps/openssl-3.0.22" \
  -DOPENSSL_INCLUDE_DIR="$PWD/local_deps/openssl-3.0.22/include" \
  -DOPENSSL_CRYPTO_LIBRARY="$PWD/local_deps/openssl-3.0.22/libcrypto.a" \
  -DOPENSSL_SSL_LIBRARY="$PWD/local_deps/openssl-3.0.22/libssl.a"

cmake --build build --target earthcall_webgpu -j8

# Build the active test targets before running the suite.
cmake --build build -j8
ctest --test-dir build --output-on-failure -j4
```

`./scripts/build.sh test` performs configuration, the default build, and CTest. For focused work, build and select the relevant named tests. GPU/GL tests need a desktop GPU/display session. `ctest --test-dir build -N` inventories the local configuration; it does not execute tests. Reconfigure after adding or removing source files. [Build and Environment](docs/BUILD_AND_ENVIRONMENT.md) owns the detailed recipe and test interpretation.

The native app target is `earthcall_webgpu`; `earthcall` is the OpenGL build. Browser/WASM and Python-backend launchers are documented in [Build and Environment](docs/BUILD_AND_ENVIRONMENT.md); the Python entrypoint lives at `src/Singularity/Foreign/py/app.py`.

## Read and contribute

Start with the human purpose, then follow the route for your work:

- [Zach's Earthcall Ourverse Manifesto](docs/core/Earthcall%20Ourverse%20Manifesto/EarthcallOurverse.md) — the project's origin, ends, and foundational beings.
- [Architecture index](docs/architecture/README.md) — ontology, Law, mathematics, Ourverse, interaction, and integration.
- [AGENTS.md](AGENTS.md) — the Seven Refusals, task router, and non-negotiables; read before writing code.
- [Agent compass](docs/AGENT_COMPASS.md), [Build and Environment](docs/BUILD_AND_ENVIRONMENT.md), and [Engineering Discipline](docs/ENGINEERING_DISCIPLINE.md) — current mechanisms, implementation boundaries, and evidence standards.
- [Agenda](docs/Agenda/Tasks/To-do%20list.md) — priorities and linked task records.
- [Person Verification](docs/Agenda/Tasks/For%20Zach/Person%20Verification%20List.md) — checks that need a human hand, eye, or judgment.
- [Agent Intercom](agent%20intercom/README.md) — the repository's shared, append-only conversations, with a searchable browser and commands to trace replies across documents.

Authored saves carry human meaning and relationships. Changes to existing worlds must preserve them through targeted patches, retained originals, preservation checks, and atomic replacement. Record who authored what, keep unresolved human decisions explicit, and test the actual consumer of a change.

*README refresh: Codex · GPT-6.1 Sol · session `01a122ec-b377-7391-ad6f-86d11b501d1b` · 2026-10-09 16:34 PDT, commissioned by Zach; preserves his ontology and Christ-centered telos while reconciling the public introduction with current routes and workshop instructions.*
