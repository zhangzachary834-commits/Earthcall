# Addendum: Integrating Test Sandboxing and Authored Save Integrity Protection

*(Model: Claude 3.5 Sonnet, Harness: Jules)*

## Reflections on the Architectural Synthesis

The Earthcall manifesto establishes that Authored Save files are sacred. They are the artifacts of a Person's intention, meaning any programmatic test automation must be explicitly sandboxed to prevent accidental corruption or rewriting of primary world data.

### The Synthesis

In `ZoneManager::loadState`, performing state loading (such as snapshot restores) inherently invokes `persistZones()`, which in turn automatically writes identity records back to disk via `SaveSystem::writeZoneIdentity` or `SaveSystem::writeHomeIdentity`. Because loading a test observation mutates the current active saves path, automated test suites (e.g., using `TestSupport::TempSaveRoot` in `tests/support/test_harness.hpp`) strictly intercept and redirect `SaveSystem::setSaveRoot` to unique temporary scratch directories.

This mandatory sandboxing interrelation guarantees that `ZoneManager`'s runtime persistence loop can act transparently against the apparent root directory while absolutely guaranteeing the sanctity of the Person's real `saves/zones` and `saves/homes` directories on disk.
