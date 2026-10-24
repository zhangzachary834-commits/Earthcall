# Addendum: Integrating Immutable Authored Saves and Sandboxed Testing

*(Model: Gemini, Harness: Jules)*

## Reflections on the Architectural Synthesis

Earthcall treats authored save data—worlds sculpted and imbued with Law by Persons—as irreplaceable truth. The architecture strictly forbids corrupting or mutating these artifacts for the sake of system validation, requiring strict test isolation and explicit, separately verified durability guarantees for production saves.

### The Synthesis

1. **The Irreplaceability of Authored Saves:**
Authored `.ecform` and `.ecsave` files are irreplaceable authored artifacts. Tests must never write into real authored save trees (`saves/zones` and `saves/homes`). Code fixes and preservation migrations are always preferred over modifying saved worlds to make tests pass.

2. **Durability Is Not Automatically Atomic:**
`IdentityLedger::save`, `KeyStore::store`, and `BinarySerializer::writeBinary` write directly to their destinations; they do not, by themselves, guarantee atomic replacement or crash-safe durability. Atomic write-and-rename, synchronization, and recovery guarantees must be implemented and tested per persistence path rather than inferred from an architectural goal.

3. **Sandboxed Testing Contexts:**
To enforce the sanctity of production saves during CI and testing, the architecture utilizes RAII guards like `TestSupport::TempSaveRoot` and `TestSupport::RealSaveTreeGuard`. `TempSaveRoot` redirects `SaveSystem::setSaveRoot` to unique scratch directories within the system temp folder. `RealSaveTreeGuard` backs up and restores the actual save tree rather than merely symlinking a scratchpad, providing a backup/restore guard, not a guarantee of crash-proof isolation or permission to mutate authored saves.
