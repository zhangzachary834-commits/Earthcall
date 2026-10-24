# Addendum: Preserving Authored Saves and Testing Without Drift

*(Original synthesis: Gemini / Jules; source-verified corrections: Sun Squid Intake III, 2026-10-23)*

## Architectural intent and actual implementation

Authored Zones, Homes, and identities must survive testing and migration without accidental replacement. An `.ecform` or legacy `.ecsave` is a persistence representation, **not** an immutable object or a guarantee of crash durability. The checkout's `saves/` is a frozen snapshot plus fixtures; the running app may use a separate data home. See `docs/Agenda/Tasks/Specific Tasks/Serialization and Storage/Data_Home_Outside_Checkout/Data_Home_Outside_Checkout.md`.

1. **Preserve the authored state.** Tests should not write to real authored saves to make a test pass. Prefer an isolated temporary save root, and prove the test actually used it. A regression witness must check the state it purports to protect, not merely report a green exit code.

2. **Atomicity is per writer, not a blanket property of persistence.** Some SaveSystem operations have atomic-swap protections, but three writers named in the original synthesis do **not** use universal temporary-file/rename commits at this revision: `IdentityLedger::save` opens its destination with `std::ios::trunc` (`src/Identity/IdentityLedger.cpp`); `KeyStore::store` writes its destination directly (`src/Identity/KeyStore.cpp`); and `BinarySerializer::writeBinary` opens its target directly (`src/Singularity/Storage/Serialization/BinarySerializer.cpp`). A rename alone also does not prove crash durability or directory fsync. Do not describe these writers as atomic without an implementation and fault-injection witness.

3. **Two test helpers provide different protections.** `TempSaveRoot` in `tests/support/test_save_helper.hpp` creates a temporary directory and redirects `SaveSystem::setSaveRoot`; this is genuine scratch-root isolation for callers using that root. By contrast, `TestSupport::RealSaveTreeGuard` in `tests/support/test_harness.hpp` backs up **all existing top-level entries** of the actual save root, points SaveSystem at that real root, and attempts restoration on destruction. It does **not** selectively symlink Zones into a scratchpad, and it does not prevent writes to the real tree while a test runs. Its backup/restore path is a last-resort safeguard, not a substitute for isolation.

**Verification boundary:** Keep the save-tree before/after witness, inspect error paths and restoration failures, and distinguish checked-in fixtures from live data-home contents. Never claim a tested save survived merely because the test harness constructed a guard.
