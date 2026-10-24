# Test Sandboxing and Authored Save Integrity

**Scope:** Test isolation around `SaveSystem` and `ZoneManager`; an authored save is not disposable test output.

## Why reading can become writing

`ZoneManager::loadState` in `src/ZonesOfEarth/ZoneManager.cpp` has stateful load paths. In particular, **snapshot restoration** calls `persistZones()`, which may write Zone or Home identities. Other admission paths can also persist identities. It is inaccurate to claim that every invocation of `loadState` unconditionally rewrites all identities; actual effects depend on the path and guards.

`TempSaveRoot` is defined in `tests/support/test_save_helper.hpp`, **not** `test_harness.hpp`. Its constructor creates a temporary directory and calls `SaveSystem::setSaveRoot`; its destructor resets that root to the default and removes the directory. This helps tests that actually instantiate the guard before touching saves. It does not globally intercept all save writes, protect arbitrary absolute paths, or prove that every test uses isolation.

## Witness and safety requirements

Tests that hydrate, snapshot, or persist Zones must establish their save root before any possible write and verify that no authored `saves/zones` or `saves/homes` paths changed. Tests using other helpers or direct filesystem paths need separate guards. A green test result alone is not evidence that a real authored save tree was untouched. See also the repository's `RealSaveTreeGuard` guidance for legacy tests that can reach real save paths.
