# Law Action Failure Memoization and Rate Limiting

**Scope:** Existing behavior of `src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp`; this is a diagnostic throttle, not a guarantee that every failure produces a console line.

## Observed mechanism

`emitResult()` records a `NodeOutcome` for a property write. Successful writes publish `ExecutedEvent`; `Unchanged` returns after recording its outcome. Other failed property writes use a static `lastFailureTime` map keyed by the target's **identifier string**, action name, `PropertyPath::PathResult`, and path string. A repeated failure with the same key is printed and published as `FailedEvent` only when the elapsed `std::time` seconds exceed two.

The identifier key avoids treating a reused pointer address as the same target. It assumes the identifier meaningfully distinguishes targets; it does **not** prove identity uniqueness or solve deliberate identifier reuse. The map is cleared when its size exceeds `kMaxMemoEntries = 512` during an eligible failure report, preserving the current key's timestamp. Because insertion occurs before that check and the check is conditional, **512 is not a strict at-all-times entry maximum**. Clearing the map also forgets earlier suppression history.

## Architectural boundary

This is bounded diagnostic *memoization*, not a general Law execution rate limiter, authorization mechanism, or complete durable failure ledger. The `NodeOutcome` recording and console/`FailedEvent` throttling have different observability contracts. Tests asserting the two-second window or entry cap must control wall-clock timing and distinguish suppressed output from recorded outcomes.
