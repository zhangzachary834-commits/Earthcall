# Addendum: Integrating Immutable Authored Saves and Sandboxed Testing

*(Model: Gemini, Harness: Jules)*

## Reflections on the Architectural Synthesis

Earthcall treats authored save data—worlds sculpted and imbued with Law by Persons—as irreplaceable truth. The architecture strictly forbids corrupting or mutating these artifacts for the sake of system validation, necessitating a robust isolation strategy for testing while maintaining atomic safety for production saves.

### The Synthesis

1. **The Irreplaceability of Authored Saves:**
Authored `.ecform` and `.ecsave` files represent the highest tier of value in the ontology. Tests must never write into real authored save trees (`saves/zones` and `saves/homes`). Code fixes and preservation migrations are always preferred over modifying saved worlds to make tests pass.

2. **Atomic Write-and-Rename Durability:**
To ensure this data is never truncated or corrupted during a fault, all persistence layers (`IdentityLedger::save`, `KeyStore::store`, and `BinarySerializer::writeBinary`) employ atomic write-and-rename operations. Data is written to a temporary `.tmp-<timestamp>` file, flushed, and then atomically renamed via `std::filesystem::rename`, guaranteeing that the simulation never observes a partially written state.

3. **Sandboxed Testing Contexts:**
To enforce the sanctity of production saves during CI and testing, the architecture utilizes RAII guards like `TestSupport::TempSaveRoot` and `TestSupport::RealSaveTreeGuard`. `TempSaveRoot` redirects `SaveSystem::setSaveRoot` to unique scratch directories within the system temp folder. Rather than incurring massive I/O overhead by copying the entire 1GB+ `saves/` tree, `RealSaveTreeGuard` selectively copies or symlinks only the necessary Zone directories and target world files into the scratchpad. This allows tests to load full state without polluting the real save directory or overwhelming CI runners with disk I/O.
