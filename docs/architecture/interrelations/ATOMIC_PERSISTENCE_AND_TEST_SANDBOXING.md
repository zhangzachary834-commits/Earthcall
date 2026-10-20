# Atomic Persistence and Test Sandboxing

Earthcall's persistence layer must handle file updates without corruption, while the test suite must execute those paths without destroying actual world saves.

## The Interrelation

The persistence layer (`IdentityLedger::save()`, `KeyStore::store()`, and `BinarySerializer::writeBinary()`) employs an atomic write-and-rename pattern. It writes to `.tmp-<timestamp>`, flushes, sets permissions (`chmod 0600`), and calls `std::filesystem::rename`.

Simultaneously, tests loading states (like `ZoneManager::loadState()`) will invoke these atomic write operations, which target `SaveSystem::rootPath()`.
Because `loadState()` automatically invokes `persistZones()` and overwrites `zone.ecform` / `home.ecform`, there is a dangerous intersection between atomic persistence and test environments.

This is resolved via `TestSupport::TempSaveRoot`, which provides RAII-based sandboxing. It redirects `SaveSystem::setSaveRoot` to an isolated scratch directory, allowing the atomic persistence mechanisms to execute fully during tests without overwriting the production `saves/zones` or `saves/homes` trees.
