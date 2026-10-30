# Addendum: Integrating File Watcher Tick, Time-Interval Polling Gating, Time Moments, and Law Property Predication

## The actual clock boundary

The [FileWatcher](../../src/Singularity/Storage/FileWatcher.cpp) is a Law-backed host-filesystem observer. `FileWatcher::tick()` first checks `_enabled`, then measures elapsed time with `std::chrono::steady_clock`. It calls `checkNow()` only when that elapsed duration reaches `_pollIntervalMs` (initialized to 250 ms). This is wall-clock throttling of directory scans, not a loop that polls continuously.

`checkNow()` resolves the configured path, compares observed files with its baseline using modification times and sizes, updates tracking, and publishes `file-created`, `file-modified`, or `file-deleted` events through `Core::EventBus`. Registered C++ callbacks also receive these detected changes.

## Authored control versus simulation time

The writable computed property `watcher.checkNow` calls `checkNow()` synchronously when set to `true`, then resets the trigger. This bypasses the *interval check* in `tick()`, but **not** the watcher's `_enabled` guard or path checks. The [file-watcher test](../../tests/singularity/file_watcher_test.cpp) exercises this through `lawSetValue` and verifies a deletion event.

Events currently use a default-constructed `Moment{}`. The implementation does **not** show the polling interval being scheduled by, quantized to, or synchronized with Timeline Moments; `_pollIntervalMs` measures host steady-clock time. The useful architectural connection is that a Law-exposed trigger can request an immediate filesystem observation while the ordinary path remains rate-limited. Do not infer stronger temporal ordering or complete event logging from this boundary alone.

## Verification anchors

- [FileWatcher.cpp](../../src/Singularity/Storage/FileWatcher.cpp): `tick`, `checkNow`, `propSetCheckNowTrigger`, `buildProperties`.
- [file_watcher_test.cpp](../../tests/singularity/file_watcher_test.cpp): Law-property trigger witness.
