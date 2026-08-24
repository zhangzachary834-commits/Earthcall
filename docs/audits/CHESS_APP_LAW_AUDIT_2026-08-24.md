# Audit — Chess App Law — 2026-08-24

Scope: The `chess_app` implementation, including the `saves/worlds/chess_app.json` world seed, the `scratch/probes/chess_app_probe.cpp` integration test, and the 35 authored laws governing chess mechanics. 

Method: Code inspection of the C++ test probe and manual JSON parsing of the authored laws (extracting condition/action models and event triggers) to evaluate adherence to Earthcall Non-negotiables (`AGENTS.md`) and computational principles (`ALGORITHMS_AS_LAW.md`).

---

## Verdict

The chess app is an outstanding structural success that flawlessly respects the Earthcall architecture, but contains a severe domain bug caused by a known limitation in the law expression system.

**The structural success:** The implementation requires zero new C++ domain nouns. `grok-4.6` correctly modeled itself as a First Mover, piece coordinates and game states are purely authored properties, and complex board interactions are fully handled by the production system's event cascades. This is exactly how an algorithm becomes a law.

**The domain bug:** Sliding pieces (Queens, Rooks, Bishops) **do not give check from a distance**. They only give check if they are directly adjacent to the King. The app is playable, but it is not chess. 

---

## §1 — The Architectural Successes

### 1.1 CRITICAL — Event cascade settles within the tick bounds
`ALGORITHMS_AS_LAW.md` mandates that iterative cascades must settle within `kMaxChainRounds = 8`. The chess application achieves a complex state-machine loop that flawlessly resolves in one tick:
1. `object-clicked` (native) maps to `square-clicked`
2. Move logic (`law-chess-pawn-w-step`, etc.) fires, updates positions, and publishes `move-committed`
3. King tracking and `check-reset` fire, publishing `king-probed`
4. `check-scanned` triggers multiple evaluation laws (`law-chess-check-knight`, etc.) which increment the `@state.chess.checkers` counter.
5. In the *following round of the same tick*, `law-chess-revert` or `law-chess-commit` process the `check-evaluated` signal and read the final `checkers` tally. 

This correctly relies on the engine's guarantee: *"facts asserted DURING [a round] survive into the next round"*. 

### 1.2 HIGH — Exact algebra used for spatial predicates
Instead of relying on loops or external C++ helpers, the implementation leans into the exact algebra (`Singularity::OntoMath::Piecewise`).
- **Board mapping:** `law-chess-click` maps `pointerWorld.x` to the 8x8 grid using an 8-piece step function on intervals `[-4, 4]`. 
- **Knight moves:** `law-chess-check-knight` asserts an L-jump by perfectly mapping the distance constraint to a piecewise bound: `(dx - kx)^2 + (dy - ky)^2 == 5.0`.

---

## §2 — The Sliding Check Bug

### 2.1 CRITICAL — Sliding pieces only check adjacently
`law-chess-check-adjacent-slide` restricts its condition to `1.0 <= (dx - kx)^2 + (dy - ky)^2 <= 2.0`. There are no other laws evaluating check for sliding pieces. If a black Queen is on `a1` and the white King is on `h8`, the app evaluates `inCheck = false`.

### 2.2 Why it happened: The Quantifier Limitation
The omission of distant check is not a simple oversight; it stems from a fundamental limitation in the `ConditionNode` language. 

To evaluate if a move is blocked, the move laws (e.g., `law-chess-bishop`) use a `ForAny` over pieces to check if any piece's coordinates lie between the start and end points. This works because the start (`@state.chess.selectedX`) and end (`@state.chess.targetX`) are globally bound properties.

Evaluating *check* from an unknown distance requires finding an enemy sliding piece in line of sight, and then verifying no other piece blocks the path. In first-order logic, this requires two bounded variables:
`ForAny(Enemy, Not(ForAny(Blocker, between(Enemy, Blocker, King))))`

Earthcall's condition language has an implicit single subject (`""`). Inside the nested `ForAny`, the subject becomes the `Blocker`, and there is no way to refer to the `Enemy`'s coordinates because it is not globally bound. `grok-4.6` encountered this inexpressibility ceiling and fell back to the only check it could express without a secondary subject: adjacency.

## §3 — Summary of Minor Findings
- **Castling and En Passant** are not implemented. (Acceptable for an initial application footprint).
- **First Mover Identity:** The identity `grok-4.6` is brilliantly registered with `kind: "first-mover"` and `onBehalfOf: "Zach"`, perfectly complying with Refusal #5 (`Person` means Human). 

---

## §4 — What "Fixed" Looks Like

Fixing the sliding check bug without altering the C++ engine requires splitting the raycast into multiple ticks or rounds, using the world as the scratchpad:
1. **Mark targets:** On `king-probed`, a `Scope::Everyone` sweep applies to all enemy sliders in line-of-sight of the King, tagging them with a property `isCheckCandidate = true` and publishing `slider-eval-started`.
2. **Sequential evaluation:** The cascade must queue these candidates, evaluate `ForAny` blockages using the candidate's coordinates (now copied to `@state.chess.evalX`), and increment `checkers` if unblocked, before moving to the next candidate. 
3. This pushes the evaluation out of Kind III (within a tick cascade) and into Kind I (Fold/Sweep) combined with state-machine continuation, which is the prescribed `ALGORITHMS_AS_LAW.md` approach for unbounded arbitrary traversals.
