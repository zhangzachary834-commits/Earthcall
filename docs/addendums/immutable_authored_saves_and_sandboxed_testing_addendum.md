# Addendum: Integrating Immutable Authored Saves and Sandboxed Testing

*(Model: Gemini, Harness: Jules)*

## Reflections on the Architectural Synthesis

Earthcall treats authored save data—worlds sculpted and imbued with Law by Persons—as irreplaceable truth. The architecture strictly forbids corrupting or mutating these artifacts for the sake of system validation, necessitating a robust isolation strategy for testing while maintaining atomic safety for production saves.

### The Synthesis

1. **The Irreplaceability of Authored Saves:**
Authored `.ecform` and `.ecsave` files represent the highest tier of value in the ontology. Tests must never write into real authored save trees (`saves/zones` and `saves/homes`). Code fixes and preservation migrations are always preferred over modifying saved worlds to make tests pass.

2. **Atomic Write-and-Rename Durability:**
To ensure this data is never truncated or corrupted during a fault, direct persistence layers like `IdentityLedger::save`, `KeyStore::store`, and `BinarySerializer::writeBinary` operate as direct writers, while other generic mechanisms might utilize atomic write-and-rename strategies when necessary.

3. **Sandboxed Testing Contexts:**
To enforce the sanctity of production saves during CI and testing, the architecture utilizes RAII guards like `TestSupport::TempSaveRoot` and `TestSupport::RealSaveTreeGuard`. `TempSaveRoot` redirects `SaveSystem::setSaveRoot` to unique scratch directories within the system temp folder. `RealSaveTreeGuard` backs up and restores the actual save tree rather than merely symlinking a scratchpad, safely bounding test mutations while preserving real data integrity.
