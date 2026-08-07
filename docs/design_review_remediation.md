# Design Review & Remediation Report

**Date:** 2026-08-03
**Reviewer:** Claude (Opus 5), reviewing the previous passes' changes
**Branch:** `sync-from-earthcall-main`
**Scope:** the working-tree and recent-commit changes as of 2026-08-03 — the Rete
incremental-evaluation rewrite, the pair-quantifier removal, the OntoMath field
work, the raw-WebSocket engine transport, and build provisioning. First-party
code only; vendored `third_party/` and `local_deps/` trees were not reviewed for
correctness, only for how they are provisioned.

This is a *design* review, companion to `security_audit_remediation.md`. It
records what earlier passes left behind, what has been fixed, and what is
knowingly still open. The distinction matters: **§1–§3 are fixed and verified;
§4 onward is found-and-not-yet-fixed.** Statuses are in the table below;
trust the table over any prose that has drifted.

**Amended 2026-08-06** (§3 closed; the repo was flattened from `sight-cpp/` to
the root in between, and the two `docs/` trees merged into one).

A note on the starting point: at review time the full test suite passed — 30/30
under `ctest`. None of the findings below were visible as a test failure. They
are latent, and several fail *silently*, which is what made them worth writing
down rather than leaving to be discovered from a bug report.

---

## Table of findings

| § | Finding | Severity | Status |
|---|---|---|---|
| 1 | Rete: pruning optimization frozen into permanent state — late binding never fires | High | **Fixed** |
| 2 | Retired pair quantifiers still reachable from the UI; unchecked enum cast destroys saved law text | High | **Fixed** |
| 3 | `evaluate()` is a no-op whose name and signature promise recomputation | Low | **Fixed** (deleted) |
| 4 | Python engine server rejects every connection; missing dependency; unauthenticated when host-bound | High | Open |
| 5 | OntoMath fields: unreachable feature, no serialization, shader integration inconsistent | Medium | Open (documented) |
| 6 | `WebSocketClient` desktop implementation is a silent no-op with no way to detect it | Medium | Open |
| 7 | `EventEntity` / `Event::Custom` wired to nothing; ownership model conflicts with `ReteFact` | Low | Open |
| 8 | Build provisioning: three competing glfw stories, fresh clone cannot configure | Medium | Open |
| 9 | Vendored binaries and runtime output tracked in git | Medium | Open |

---

## 1. Rete: an optimization that was safe per-frame became permanent (FIXED)

### What earlier passes did

`ReteNetwork` was converted from rebuild-every-frame to incremental: `assertFact`
now maintains the node memories and queues activations as facts arrive, and
`evaluate()` became a no-op returning the agenda.

The rewrite carried over one line from the old `evaluate()` — the pruning skip:

```cpp
if (!boundToLaw && !feedsBeta) continue;   // don't predicate-test a node nothing reads
```

### The defect

That skip was correct when `evaluate()` rescanned `_facts` from scratch every
frame: a node pruned this frame was rebuilt the next. Under incremental
propagation the skip is **irreversible** — nothing backfilled a node's memory
when it later became read. A law that started listening after a fact was
asserted stayed deaf to that fact forever, while `facts()` kept reporting it as
live. No error, no log, no test failure.

Verified with a direct probe against `ReteNetwork` (before the fix):

```
bind-then-assert : agenda=1  facts=1     (fires — what every existing test does)
assert-then-bind : agenda=0  facts=1     (never fires; fact still reported live)
rebind-after-fact: agenda=0  facts=1     (never fires)
```

Every existing test binds before asserting, which is why 30/30 passed.

Affected paths:

- `LawManager::bindTrigger` called while facts are still live.
- `unbindLaw` → `dropUnboundAlphaNodes` deletes the node; the next
  `internTypeAlpha` mints a fresh one with empty memory. **Every trigger rebind
  cycle** had this hole, and `loadFromJson` rebinds every restored law's
  triggers.
- `addBetaNode` had no backfill at all, so a join was permanently blind to any
  fact predating it.
- **Laws created by laws mid-`tick()`** — which `kMaxChainRounds` exists
  specifically to support — never saw facts asserted earlier in the same round.
  Previously they fired on the next round.

### Remediation

Backfill, hung off the observation that **a node's memory is a pure function of
`(predicate, _facts)`** — so rebuilding it by rescanning the fact list is always
correct, idempotent, and order-preserving. The pruning skip is kept (the
predicate cost it avoids is real); what changed is that every transition into
"being read" now refills first:

- `refillAlphaMemory` / `refillBetaMemory` — private, rescan `_facts`.
- `bindLawToAlpha` — refills if nothing read the node yet, then queues the live
  facts as the new law's backlog. **Only the newly bound law is queued**, so
  laws already on the node do not fire twice.
- `addBetaNode` — creating the beta is what makes its alphas read, so it refills
  both, then builds the join over what is already there.
- `bindLawToBeta` — queues whatever the join already holds.
- `alphaIsRead` / `alphaFeedsAnyBeta` — the skip condition, named once so
  propagation and backfill cannot drift apart. This was the actual root cause
  class: the same predicate expressed twice in two places, one of which was
  later made permanent.

Binding is therefore now **retroactive**, and setup order does not matter.

Two supporting fixes the backfill required:

- **Join token asymmetry.** The left branch passed `ReteToken{{f}, {}}` with
  empty bindings while the right branch populated `bindings["left"]`. A join
  closure reading `bindings` behaved differently depending on which side the new
  fact arrived on. Both now go through shared `alphaToken` / `joinSeed` /
  `joinedToken` builders — required so a backfilled memory is byte-identical to
  an incrementally built one, and it removes the arrival-order dependence.
- **`LawManager::bindTrigger` sets `_dirty`** when the bind actually queued
  something. Without it, a law bound mid-tick queues a backlog that waits for
  the next unrelated event — a one-tick stall rather than a loss, but still only
  half a fix. Guarded on a non-empty agenda so an ordinary bind on a quiet
  network does not buy a wasted round.

Also fixed in passing:

- **Dangling reference.** `const ReteFact& f = _facts.back()` was held across
  `alpha.predicate(f)` and `beta.join(...)` — arbitrary caller closures. Any one
  of them causing another fact to be asserted reallocates `_facts` and dangles
  the reference mid-loop, and `EventBus` explicitly documents re-entrant publish
  ("laws chain by firing events from their actions"). Propagation now reads the
  local copy.
- Restored the `static_cast<std::ptrdiff_t>` dropped from the `_facts.erase`
  iterator arithmetic.

### Verification

Probe results, before → after:

```
assert-then-bind  : 0 → 1
rebind-after-fact : 0 → 1
late-beta         : 0 → 1
no-double-fire    :     law1=1 law2=1   (a late joiner does not re-fire the incumbent)
predicate-honored :     law1=1 not 2    (backfill still filters by predicate)
```

Full build clean; 30/30 `ctest` still passing.

### For future work

The pruning skip and the backfill are now a matched pair. **Do not add a path
that makes an alpha or beta node read without backfilling it** — that
reintroduces this exact class of silent deafness. `alphaIsRead` is the single
definition of "read"; route new logic through it.

---

## 2. Retired pair quantifiers: reachable from the UI, destructive on load (FIXED)

### What earlier passes did

`ConditionNode::Kind::ForAnyPair` (12) and `ForAllPair` (13) were removed —
enumerators, `compile()` cases, `toJson()` cases, factories, UI editor block,
and the tests that covered them. The stated rationale, kept in the header:
*model pairs as Relations in the graph.*

The reasons the removal was right are worth recording, since the architecture
docs had it listed as landed: the pair scan was O(n²) per evaluation and never
got its spatial index, and because the second element of the pair was carried by
*borrowing* the `@event.object` vocabulary, a pair claim could never export its
witness to the action. It could detect a pair but not act on the one it found.

### The defects

**(a) The UI still handed out the removed kinds.** `LawGraphWindow.cpp` still
listed `"for ANY PAIR..."` and `"for ALL PAIRS..."` and still passed a hardcoded
`14` as the combo item count. Selecting either set `node.kind = 12` or `13` — now
out of range for the enum. Downstream: `seedConditionKind` no-op, a blank editor
card, `compile()` falling through to a constant `false` (a permanently
unsatisfiable condition), and `describe()` returning bare `"condition"`.

No compiler warning was possible. Because the enumerators were deleted *along
with* their `case` labels, every `switch` stayed exhaustive, so `-Wswitch` had
nothing to flag. Silent at compile time and silent at runtime.

**(b) Load/save destroyed law text.** `fromJson` did an unchecked
`static_cast<Kind>(j.value("kind", 0))` with no validation and no `default:`
anywhere in the file. Any already-saved world containing kind 12 or 13 loaded
clean into an out-of-range enum, silently never fired — and on re-save,
`toJson()` wrote `kind: 12` while dropping `children`, `beingKindB`, and
`except`, because no case matched. **Merely opening and saving a world
permanently stripped the condition's payload.**

### Remediation

Deleting the combo entries fixes (a) but not (b), so unknown kinds were given
somewhere safe to land:

- **`Kind::Unsupported = 255`** — the landing place for any kind this build
  cannot read. Not offered as an authoring choice.
- **`fromJson` validates** against a whitelist of live kinds. Deliberately a
  whitelist, not a `< count` range check: 12 and 13 sit *inside* the range and
  must keep failing. Unknown kinds become `Unsupported`, are logged through
  `LawAuditLogger`, and **keep their original JSON** on the node
  (`std::shared_ptr<nlohmann::json> unsupported` — null in the common case).
- **`toJson` returns that JSON verbatim** for an `Unsupported` node. Load/save
  is now lossless for law text this build cannot evaluate.
- **`compile()` returns false and logs on every evaluation**, so a law that went
  quiet after a version change is traceable instead of mysteriously inert.
  `describe()` reads `"unsupported condition (kind 12) - never holds"`.
- **UI**: the two entries are gone and the count is now `IM_ARRAYSIZE(kinds)`, so
  the list cannot drift from the enum again — that drift was the whole bug. An
  `Unsupported` node renders a card explaining that it will not fire, that its
  text is preserved, and what kinds 12/13 were.
- **Kinds 12 and 13 are burned** in an enum comment. `Kind` is append-only;
  reusing them would make an existing saved world load as something else.
- Removed the dead `beingKindB` field — still round-tripped through JSON but
  read by nothing since the quantifiers went.

### Verification

```
retired-kind : kind=255 describe="unsupported condition (kind 12) - never holds"
round-trip   : {"beingKind":1,"beingKindB":1,
                "children":[{"kind":11,"otherId":"@event.object"}],"kind":12}
```

Byte-identical round trip including `children` and `beingKindB`; `compile()`
returns false. 30/30 `ctest` passing.

### For future work

`Unsupported` is now the general mechanism for forward/backward-incompatible
condition JSON, not a one-off for the pair quantifiers. Any future kind removal
should rely on it rather than trusting the cast — and any new `Kind` must be
appended, never assigned 12 or 13.

---

## 3. `evaluate()` no longer evaluates (FIXED — deleted, not renamed)

Independently raised by Gemini in a later pass, which recommended renaming to
`pendingActivations()`. The diagnosis was right; the remedy was not the best
available, for two reasons that only show up on inspection:

- **`agenda()` already existed** and returned the identical
  `const std::vector<ReteActivation>&`. So `evaluate()` was not merely misnamed
  — it was a duplicate of an accessor already carrying the honest name.
  Renaming would have produced a *third* name for one accessor.
- **`evaluateRete()` had zero callers**, and the sole `evaluate()` call site in
  `tick()` discarded the return value — a dead statement immediately above
  `drainAgenda()`.

### Why "keep the name and implement it" was not viable

The other fork considered was to give `evaluate()` real work to do. There is
none. After the §1 backfill the agenda is complete at every instant:
`assertFact` propagates on arrival, the bind paths backfill and queue, the
retract paths purge, and alpha predicates are `bool(const ReteFact&)` — pure
functions of a fact that is immutable once asserted, so no queued result can go
stale. Manufacturing work for an evaluation phase means reinstating the
per-frame full recompute the incremental rewrite removed, at O(nodes × facts)
every frame, to produce an agenda that was already correct.

The one thing a classic Rete does at this point that this network does not is
**conflict resolution** — ordering the pending set by recency/specificity/
salience before firing. That is legitimate future work, but it belongs in
`drainAgenda()`: it orders what is pending, it does not compute what is pending.
Recorded in the header so it lands in the right place.

### Remediation

Deleted `ReteNetwork::evaluate()` (declaration and definition), deleted
`LawManager::evaluateRete()`, and removed the dead call from `tick()`.
`agenda()` peeks, `drainAgenda()` takes and clears — two methods, two distinct
jobs, no third name. Also corrected three comments that still described
`evaluate()` as predicate-testing every node against every fact each frame; that
stale description was load-bearing for the wrong mental model.

Build clean; 35/35 `ctest` passing (the suite grew from 30 with intervening
commits), and the §1 probe re-run unchanged.

**Note for future readers:** this is why the §1 bug was invisible. A name that
promises recomputation tells every caller that setup order does not matter. It
did matter, for months, and the name is what hid it.

---

## 4. Python engine server (OPEN — highest-value remaining item)

New in this pass: `backend-python/sockets/engine_server.py`, a raw WebSocket
server on port 5001 intended to carry high-frequency physics/law state from the
C++ engine, started from `app.py`.

**(a) Every connection fails.** The handler is declared
`async def handler(self, websocket, path)`. `websockets.serve` in 15.x — 15.0.1
is what is installed — invokes the handler with a single argument. The two-arg
form belongs to the removed `websockets.legacy` API. Verified live:

```
TypeError: handler() missing 1 required positional argument: 'path'
NO REPLY: ConnectionClosedError received 1011 (internal error)
```

**(b) The dependency is not declared.** `websockets` is absent from
`requirements.txt`, and `app.py` imports it at module scope — *outside* the
`if __name__ == '__main__'` guard. A fresh install therefore `ImportError`s the
**entire backend**, not just the engine server.

**(c) It never starts under a real server.** `start_engine_server` is called
only inside the `__main__` guard, so under gunicorn/WSGI the import runs and the
server does not.

**(d) Unauthenticated when host-bound.** `app.py` passes the env-configurable
`host` through to the raw server. `HOST=0.0.0.0` — normal in a container —
exposes an unauthenticated, unvalidated port that ingests engine state and
echoes input back.

**(e) Minor:** `connected_clients.remove()` in a `finally` should be `discard`
(`KeyError` if `add` never happened); `connected_clients` is populated but never
used to broadcast; the daemon thread has no shutdown path and `self.loop` is
assigned inside the thread.

**Recommendation:** fix the handler signature, pin `websockets` in
`requirements.txt`, move the import inside the guard or make the engine server
start from the WSGI entry point, and bind the engine port to loopback
unconditionally until a handshake exists.

---

## 5. OntoMath fields (OPEN — documented in `ontomath_fields.md`)

The field work is designed ahead of its wiring. `WebGpuRenderer.cpp` calls
`sdfwgsl::compile(field)` and **never passes the `fieldNode` argument**, so
`fieldEval` always compiles to `return 0.0` and no field reaches the screen. The
per-piece status table now lives in `docs/ontomath_fields.md`.

Because the feature is unreachable, existing SDF rendering is unaffected — with
`fieldEval` returning 0 the new branches are inert. But the volumetric code has
problems to resolve *before* it is wired:

- **Optical depth uses the wrong length.** `step_size = max(raw, eps)` is the
  undamped SDF value, while the ray actually advances by `max(d, eps)` where
  `d = raw/|grad|` when damping is on. Absorption is integrated over a distance
  the ray did not travel — wrong for exactly the implicit-field case damping
  exists for.
- **Bogus depth on volumetric-only pixels.** `if (!hit && transmittance > 0.99)
  discard;` lets a no-surface-hit pixel through, after which `pf = ro + rd*t` is
  a non-surface point. The code still takes `sdfGrad(pf)` as a normal and writes
  `out.depth` from it.
- **Double attenuation.** `mix(base_rgb, field_rgb, 1.0 - transmittance)` —
  `volumetric_scatter` already accumulates `* transmittance`.
- **Opaque alpha for a transparent volume**, in a depth-writing pass.

Design-level gaps: no discriminator on `ScalarField` recording whether Path A or
Path B is active (both configurations coexist; the compiler always picks A);
no `toJson`/`fromJson` on `Field` or `FieldNode`, so a `FieldNode` does not
survive save/load unlike every other `Singular`; and `FieldNode::buildProperties`
registers `PropertyRef`s against `field.get()` while `field` is a replaceable
`shared_ptr` — replacing it (which the header's own note contemplates doing over
the network) leaves three registry entries pointing at freed memory.

Two load-bearing comments were also deleted from `SdfWgsl.cpp` in this pass: the
explanation of *why* dividing by gradient magnitude makes sphere tracing legal on
an iso-surface, and the "WGSL has no forward declarations, so order matters"
note. The new code depends on that ordering rule. This codebase otherwise keeps
exactly these "why" comments; they are worth restoring.

---

## 6. `WebSocketClient` is a mock that cannot be distinguished from a connection (OPEN)

The desktop implementation prints and drops: `connect()` logs, `send()` logs and
discards the payload, and the stored `messageCallback` is never invoked. There
is no `isConnected()` and no return value anywhere in the interface, so a
desktop caller has no way to learn its sync is going nowhere.

The stated rationale — "we avoid compiling heavy networking libraries for now" —
does not hold in this repo: `CMakeLists.txt` already fetches ASIO and
websocketpp, and `WebSocketServer.cpp` already uses them.

**Recommendation:** at minimum add `bool isConnected()` and make `send()` report
failure, so the mock cannot be mistaken for a live connection. Better: implement
it against the websocketpp already present.

---

## 7. `EventEntity` / `Event::Custom` are wired to nothing (OPEN)

Nothing publishes or subscribes `Event::Custom`. `EventEntity` is constructed
only by `SynthesisSystem::instantiateClass("Event")`, which itself has no
callers. It never registers with `Universe`, so laws cannot range over it —
contradicting its own header comment ("so it can exist natively in the physics
graph").

One thing to settle *before* it is wired: `Event::Custom` owns via `shared_ptr`
while `ReteFact` holds raw `Singular*`, and `retractFactsAbout` is driven by
`"object-destroyed"`, which is never published for an `EventEntity`. Feeding one
into the Rete path as it stands sets up precisely the dangling read that
`Law.hpp` warns about.

---

## 8. Build provisioning (OPEN)

- **Three competing glfw stories.** `CMakeLists.txt` hints
  `local_deps/glfw_install/lib`; `HEAD` tracks both a `local_deps/glfw-3.4`
  source tree *and* `glfw.zip`; the working tree adds `third_party/glfw`.
- **A fresh clone cannot configure.** `CMakeLists.txt` adds
  `third_party/flatbuffers/include` to the include path, but
  `third_party/flatbuffers` is untracked — 0 files in git — as are
  `third_party/glm` and `third_party/glfw`.

**Recommendation:** pick one provisioning mechanism and delete the other two.

---

## 9. Repository weight and stray artifacts (OPEN)

- `third_party/wgpu/lib/libwgpu_native.a` — **32.8 MB tracked**. The CMake
  comment gives a genuinely good reason to *pin* the wgpu header generation
  (older releases ship a pre-`WGPUStringView` ABI and crash at runtime), but
  pinning does not require committing the archive; a checksummed fetch of that
  exact tag gives the same guarantee.
- `HEAD`'s `local_deps` + `glfw.zip` add **373 MB across 14,043 files**,
  including generated Doxygen HTML and PNGs. In git history this is permanent.
- `sight-cpp/a.out` (a build artifact) and `sight-cpp/test_variant.cpp` (a
  16-line scratch probe) sit loose in the source root, ungitignored.
- `saves/earthcall-io.log` is listed in `.gitignore` but still **tracked**, so it
  shows as modified permanently. `saves/logs/game_save_log.txt` and four
  `.ecsave` files are runtime output committed into the source tree.

---

## Changes that were straight improvements

Noted so they are not re-litigated:

- `ProbabilityForm.cpp` — replacing `arg->index() != 0` with
  `propertyValueToNumber` correctly accepts `float`/`int` rather than only
  `double`.
- `WebSocketServer.cpp` — `get_local_endpoint(ec)` removes a throwing call from
  a thread body. (It then ignores `ec` and calls `.port()` on a possibly-unset
  endpoint; worth tightening.)
- The Rete retraction paths — pruning beta memory and the agenda **by fact id**
  rather than by participant pointer is more correct than what it replaced, and
  `unbindLaw` purging the agenda fixed unbinding being cosmetic.
- `SaveSystem::writeSaveData` unification and the `src/Util/MigrateSaves.cpp`
  exclusion-path correction.

---

## Method

Findings were derived by reading the diff against `HEAD` plus the untracked new
files, then confirmed against the code rather than inferred from it. Two were
verified by execution rather than inspection, because both would otherwise have
been arguable:

- **§1** — a standalone probe linked against `earthcall_core` exercising
  `ReteNetwork` directly through bind/assert orderings, run before and after the
  fix.
- **§4a** — the engine server started in-process and connected to with a real
  `websockets` client.

Claims that a feature is unreachable (§3, §5, §7) are from exhaustive `grep` for
callers across `src` and `tests`. The 30/30 baseline was established by building
and running the full `ctest` suite before any changes, and re-run after; the
2026-08-06 amendment re-established it at 35/35 in the flattened tree.

A note on reviewing review findings: §3 arrived second-hand (from Gemini) and
was re-derived from the code before being acted on. That was worth doing — the
diagnosis held, but the recommended remedy did not survive contact with the
detail that `agenda()` already existed. Take a finding's *diagnosis* and its
*prescription* as separately checkable.

Not covered: the WebGPU renderer beyond the `SdfWgsl` diff, the Lexeme/language
subsystem, and correctness of vendored dependency source.
