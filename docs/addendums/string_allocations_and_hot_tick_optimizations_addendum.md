# Addendum: String Work in Hot Ticks — Measured Optimization, Not Assumed Speed

*(Original synthesis: Gemini / Jules; source-verified corrections: Sun Squid Intake III, 2026-10-23)*

## Architectural synthesis and implementation status

Repeated path parsing, identifier copying, and temporary key construction can be optimization targets, but an API taking `const std::string&` does not by itself prove an allocation-free call chain. Measure an exact path and preserve semantics before declaring a win.

1. **Audio property paths — candidate, not canonical fact.** At the current `sync-from-earthcall-main` base, `src/Singularity/Audio/AudioSystem.cpp` still calls `PropertyPath::parse("acoustic.frequency")` and other literal parses **inside** `AudioSystem::tick`. Hoisting immutable parsed paths is the intent of a separate open PR (#737), not a change landed in this canonical snapshot. Any such optimization must preserve property lookup and run an actual tick-path witness; successful audio initialization alone is insufficient.

2. **Law identifiers and agenda work.** `LawManager` and `Law` repeatedly query identifiers in several paths. A locally held identifier can avoid some repeat construction, but do not assert that a particular call is virtual, heap-allocating, or removed from the hot loop without checking the exact signature and call sites. Cache lifetime, identifier mutation, and Rete candidate completeness are correctness constraints; measure before/after on the same workload.

3. **Drive-session key construction.** `LawManager::hasDriveSession(const std::string&, const std::string&)` in `src/ZonesOfEarth/AuthorsOfLaw/Law.hpp` currently performs `_driveSessionKeys.find({lawId, subjectId})`. Its `unordered_set` key is a string pair, and constructing that pair may copy its strings despite reference parameters. An allocation-free heterogeneous lookup would require an appropriate transparent hash/equality interface (or another proved strategy) and a regression witness. A separate open PR (#754) proposes work here; do not present it as already landed or universally O(1) in worst-case complexity.

4. **Failure-message deduplication.** `ActionModel::emitResult` in `src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp` keeps a static `std::map` keyed by a tuple containing the subject's **identifier string**, action, result, and property path, rather than the subject's raw address. It clears the map when size exceeds `kMaxMemoEntries = 512`; this is a bounded-growth policy, not an exact hard cap at every insertion. Identifier-based keys prevent a particular address-reuse false match, but are not a blanket guarantee of memory or concurrency safety.

**Evidence rule:** Record baseline, changed-head timings, allocation counts where relevant, exact executed test targets, and correctness parity. Do not infer zero allocations, nanosecond latency, or semantic equivalence from source shape or green unrelated CI.
