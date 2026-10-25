# Integrating Persistence Boundaries and Test Sandboxing

Earthcall has both atomic-commit save paths and direct-write paths; they must not be conflated. `ZoneManager::saveState` uses a staged matter/semantic-root commit, while `IdentityLedger::save`, `KeyStore::store`, and `BinarySerializer::writeBinary` contain direct file writes. See [the source-checked interrelation](../architecture/interrelations/ATOMIC_PERSISTENCE_AND_TEST_SANDBOXING.md).

`TempSaveRoot` in `tests/support/test_save_helper.hpp` redirects `SaveSystem` paths for scoped tests. It does **not** automatically redirect independent identity/key paths or guarantee that every writer is atomic.