# Integrating Engine Boot Sequence and Test Sandboxing

In Earthcall, the boundary between real, persistent worlds and isolated test environments must be strictly maintained to prevent tests from corrupting authored human saves. Yet, to be valid, tests must exercise the exact same logic that governs the real engine boot sequence.

This is achieved through the integration of the production boot sequence in `EngineInit.cpp` and the sandboxing RAII helpers in `tests/support/test_harness.hpp` and `test_save_helper.hpp`.

## The Production Boot Primitive

During a normal engine start (`src/Singularity/Core/EngineInit.cpp`), Earthcall discovers and restores identities by directly querying the Save System and deserializing the state:

```cpp
for (const auto& info : SaveSystem::listWorlds(SaveSystem::SaveType::PERSON)) {
    // ... filtering legacy profiles ...
    personFromJson(candidates.front(), *_person);
}
```

The crucial detail is that `EngineInit.cpp` implicitly relies on `SaveSystem::saveRoot()` to determine *where* these saves live. It does not hardcode `saves/` or use a singleton database connection. It relies on the environment's configured save root.

## Test Sandboxing via Root Redirection

When tests run, they must not execute `SaveSystem::listWorlds` against the real `saves/` directory. If a test modifies a Person, or if `ZoneManager::loadState()` invokes `persistZones()`, real data would be overwritten.

To solve this while preserving the integrity of the boot sequence, the test suite utilizes `TempSaveRoot` (defined in `tests/support/test_save_helper.hpp`):

```cpp
struct TempSaveRoot {
    std::filesystem::path path;
    TempSaveRoot() {
        path = std::filesystem::temp_directory_path() / ... ;
        std::filesystem::create_directories(path);
        SaveSystem::setSaveRoot(path.string());
    }
    ~TempSaveRoot() {
        SaveSystem::setSaveRoot("");
        std::filesystem::remove_all(path, ...);
    }
};
```

This RAII helper dynamically alters the `SaveSystem` root to a unique temporary directory for the lifetime of the test.

## Integration in `BootedEngineHarness`

With `TempSaveRoot` in place, the `TestSupport::BootedEngineHarness` can safely construct a full `Ourverse` instance and mimic the production boot sequence. Because `SaveSystem::saveRoot()` has been redirected to the scratch directory, operations like `listWorlds` and `personFromJson` (or zone persistence routines) run exactly as they would in production, but within a sandbox.

By architecting the boot sequence to rely on the contextual `saveRoot()`, and by providing an RAII helper to manipulate that context securely, Earthcall ensures that its complex test harnesses can exercise the real initialization logic without risking catastrophic corruption of the sacred save files described in the Authoring directives.
