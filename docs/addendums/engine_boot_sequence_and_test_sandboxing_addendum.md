# Engine Boot, Person Presence, and Test Save Sandboxing

The production boot path and the test harness share important runtime wiring, but they are not identical. In particular, constructing `TestSupport::BootedEngineHarness` does **not** automatically install a temporary SaveSystem root or execute the entire production `Engine::initLogic()` path.

## Production identity restoration

In `src/Singularity/Core/EngineInit.cpp`, `Engine::initLogic()` constructs a Person if necessary, enumerates `SaveSystem::listWorlds(SaveSystem::SaveType::PERSON)`, filters superseded legacy profiles, and calls `personFromJson` only when exactly **one** candidate remains. It refuses to guess among multiple candidates. A restored profile is **not** equivalent to authenticated presence: keyed identity requires an explicit unlock through the identity-presence path.

SaveSystem's configured root determines which profiles are visible to the enumeration. Tests that intend to exercise real boot restoration must isolate that root **before** invoking the boot path, and must explicitly test the authentication and multiple-profile cases as needed.

## Explicit sandbox setup

`TempSaveRoot` is defined in `tests/support/test_save_helper.hpp`. Its constructor creates a unique temporary directory and calls `SaveSystem::setSaveRoot(path.string())`. Its destructor resets the root to `""` and removes the directory. **It does not restore an arbitrary previously configured non-default root**, so it should not be described as a fully nestable save-root stack.

`tests/support/test_harness.hpp` provides `TestSupport::BootedEngineHarness`, which wires a LawManager, Physics, authored laws, Universe providers, and other boot-like services. It does **not** itself create `TempSaveRoot` or deserialize Person profiles through `Engine::initLogic()`.

For tests that can read or persist authored worlds, create an isolated root before constructing/loading the relevant systems, restore the prior root deliberately when required, and ensure the temporary fixture contains only intended test saves. Avoid using a green harness test as evidence of complete production boot-path equivalence.
