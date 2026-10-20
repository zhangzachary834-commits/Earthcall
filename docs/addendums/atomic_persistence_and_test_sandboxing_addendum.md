# Integrating Atomic Persistence and Test Sandboxing

Earthcall's atomic write-and-rename persistence operations (e.g., in `IdentityLedger`) are safely isolated during tests by utilizing `TestSupport::TempSaveRoot` to sandbox the `SaveSystem::rootPath()`.
