# Persistence Boundaries and Test Save Isolation

Earthcall's persistence safety depends on **which writer** and **which root** are actually used. Atomicity and sandboxing are separate properties.

## Source-checked boundaries

- [`ZoneManager::saveState`](../../../src/ZonesOfEarth/ZoneManager.cpp) stages matter before committing the semantic root; its implementation documents an atomic-rename boundary. That does **not** establish atomicity for every Earthcall file writer.
- [`IdentityLedger::save`](../../../src/Identity/IdentityLedger.cpp) writes JSON directly to its configured `_path` using `std::ofstream(..., std::ios::trunc)`. [`KeyStore::store`](../../../src/Identity/KeyStore.cpp) also writes directly to its key directory, with permissions applied separately. [`BinarySerializer::writeBinary`](../../../src/Singularity/Storage/Serialization/BinarySerializer.cpp) writes directly to its supplied filepath. Do not describe these as universally using `.tmp` plus rename.
- [`SaveSystem::setSaveRoot`](../../../src/Singularity/Storage/SaveSystem.cpp) redirects paths resolved through `SaveSystem::rootPath()`. It does not redirect arbitrary `_path`, key-store directories, or caller-supplied filepaths.
- [`TempSaveRoot`](../../../tests/support/test_save_helper.hpp) is a test helper in the global namespace: it creates a temporary directory, sets the SaveSystem root, then resets the root to the default and removes the directory. It does not preserve a previously configured nondefault root.
- [`ZoneManager::loadState`](../../../src/ZonesOfEarth/ZoneManager.cpp) can persist zone identities on the `snapshotRestore` path; it does **not** unconditionally call `persistZones()` on every load. Other load/save side effects still warrant isolated fixtures.

## Engineering consequence

Tests that may write authored Zone/Home identity must establish a sandbox before invoking the relevant save/load path and verify the effective root. Separately sandbox any direct identity, key-store, or explicit-path writer. A successful atomic file replacement is not evidence that the replacement targeted the correct tree. Never run a mutating test against real authored saves as a substitute for this proof.