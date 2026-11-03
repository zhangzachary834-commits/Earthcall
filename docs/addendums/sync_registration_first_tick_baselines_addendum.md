# Integrating Sync Registration, File Watcher Baselines, and First-Tick Guards

Origin: Jules architectural interrelation proposal; corrected against Earthcall canonical `59232506f781`.

## Registration is not the same as baseline capture

`FileWatcher::syncRegister(LawManager&)` in `src/Singularity/Storage/FileWatcher.cpp` checks whether `file-watcher` already exists and, if absent, adds a new `FileWatcher` Law. **It does not call `rescanBaseline()`.** Baseline capture is implemented by `FileWatcher::rescanBaseline()` and is invoked by `propSetWatchPath()` when the watched path changes; tests can invoke it explicitly. `tick()` first respects the poll interval, then `checkNow()` compares the current scan with `_trackedFiles` and publishes `file-created`, `file-modified`, or `file-deleted` events. With an initially empty tracked-file map, the first eligible scan can classify already-existing files as created. A quiet initial scan for the default watch path is therefore **not guaranteed by registration**; it would need an explicit baseline call and a regression test.

## The interaction channel has a different guard

`InteractionChannel::syncRegister` installs world readings and registers the channel; it does not initialize the windowing system. In `src/Singularity/Input/Interaction/InteractionChannel.hpp`, the channel and its `Sense` default `windowWidth` and `windowHeight` to **1**, so normalized `propPointerU/V()` start with nonzero denominators. During observation, `InteractionChannel.cpp` further clamps sensed dimensions with `std::max(1, sense.windowWidth)` and the height equivalent before publishing changes. The guard prevents zero-denominator/nonfinite normalized coordinates; floating-point division by zero need not raise a C++ exception.

## Interrelation

Both systems cross an initialization-to-observation boundary, but with different current contracts. The input channel actively establishes a safe numeric default; the file watcher exposes an explicit baseline operation without automatically invoking it during `syncRegister`. Treating these as identical first-tick guarantees would conceal a real behavioral difference. A future FileWatcher baseline-at-registration change should be independently tested for existing-file silence and post-baseline create/modify/delete events.
