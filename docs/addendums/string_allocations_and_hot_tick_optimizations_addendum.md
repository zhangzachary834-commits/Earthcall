# Addendum: Integrating String Allocations and Hot Tick Optimizations

*(Model: Gemini, Harness: Jules)*

## Reflections on the Architectural Synthesis

Avoiding redundant string allocations and tokenization in Earthcall's hot tick loops is an optimization direction; individual proposals must not be described as already landed or measured.

### The Synthesis

1. **Pre-Parsing Property Paths:**
Canonical `AudioSystem::tick()` still parses literal property paths. Pre-parsing and reusing resolved identifiers is proposed; reductions in allocations and tick cost require measurements and regression witnesses.

2. **Virtual Call and Identifier Caching:**
Where `LawManager` repeatedly requests the same stable law identifier in one operation, a local `const std::string lawId = law.getIdentifier();` is a candidate optimization. Its safety depends on identifier stability and use-site semantics; allocation and dispatch savings are not established without profiling.

3. **Proposed Drive-Session Lookup Optimization:**
A string-keyed unordered lookup for `hasDriveSession` is a proposal, not canonical implementation. `const std::string&` parameters alone do not prevent constructing temporary composite keys. Any replacement must preserve session identity and invalidation semantics; unordered-map lookup is expected constant time, not a universal O(1) guarantee.

4. **Identifier-Keyed Failure De-duplication:**
Canonical `ActionModel::emitResult` stores failure timestamps in a `std::map` keyed by value-based identifiers, action, failure result, and property path, rather than by raw object addresses. This avoids conflating distinct beings merely because an address was reused. Its 512-entry threshold is **not a strict size bound**: a new key is inserted before the threshold check, and the map is cleared only when a failure is eligible to be logged after the de-duplication interval. Many distinct failures within that interval can grow the map beyond the threshold. A genuinely bounded policy would require separate implementation, stress tests, and profiling; the existing mechanism establishes neither zero allocations nor a hard memory cap.
