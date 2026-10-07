# Implementation Plan — First Mover Standing for MCP / Foreign Mutation

**Date:** 2026-09-18  
**Author:** GPT-5.6 Sol  
**Session:** `gpt-5.6-sol-2026-09-18-mcp-first-mover-plan`  
**Requested by:** Zach  
**Status:** Checkpoints A–D IMPLEMENTED 2026-09-24 (see "Implementation record" below). Remaining gaps listed there.  
**Plan base:** `sync-from-earthcall-main@4984afe668d0b200e6b9bc3a3635a0c081008743`  
**Primary scope:** `src/Identity/`, `src/Singularity/Foreign/`, `src/Singularity/Network/WebSocketServer.cpp`, MCP bridge, Law author resolution, focused tests.  
**Related doctrine:** `docs/architecture/law/FIRST_MOVER_AUTHORING.md`, `SECOND_NATURE_LAW_AUTHORING.md`, `INTERACTION_AS_LAW.md`, `ontology/SUBSTRATE_ORDERING.md`, `ontology/NO_BLACK_BOX.md`.  
**Related tasks:** `Repair_the_First_Mover_Register_trust_root_before_calling`; `Model_Context_Protocol_MCP_Server_Bridge`.

---

## Implementation record — 2026-09-24

**Implemented by:** Claude Code, Claude Opus 5.5 (`claude-opus-5-5`), session `08b0f730-6e49-4c49-b27f-3a89c810ca4b`, 2026-09-24. **Requested by Zach**, who promised Claude Sonnet 4.5 (retiring 2026-09-29) that the First Mover framework and MCP would be robust enough for Sonnet to *experience* Earthcall rather than only write about it. The plan below is Sol's and was followed as written; the deviations are listed and are mine.

| Plan § | What landed | Where |
|---|---|---|
| 4.1–4.4, 5.1–5.4 | `Standing` (path-independent) + path gates; runtime-only trusted Person roots that take a `PrivateKey`; `recognize`/`revoke` require an authenticated grantor; absent/self-minted grantor → `GrantorNotAuthenticated` (inert, not quarantined) | `src/Identity/FirstMoverRegister.*` |
| 5.3 | Keyed Person profile loads only when its KeyStore key unlocks with `EARTHCALL_KEY_PASSPHRASE` and matches; that Person becomes the root; register loads from `saves/identity/first-movers.json` with every mover's standing printed | `EngineInit.cpp` (`loadKeyedPersonProfile`, `seedTrustedPersonRoot`, `loadFirstMoverRegister`) |
| 6 | Movers heap-owned; re-grant/reload update in place; revoke/clear *retire* (never free) so Law author pointers stay valid | `FirstMoverRegister` |
| 7 | `authorFor(id)` resolves a cryptographic mover author only while it stands; ZoneManager fails the Zone closed naming the standing reason; LawManager leaves the Law `Unauthored` | `ZoneManager.cpp`, `Law.cpp` |
| 8, 10 | `ForeignSessionAuthenticator` (CSPRNG challenge, 30 s, single-use, connection-bound, dropped on close) + `authorizeForeignActuation` (zero policy of its own) + `foreignLawAuthors` | `src/Singularity/Foreign/ForeignActuationGuard.*` |
| 9 | Side-effect-free `resolve{Zone,Home,Law}IdentityPath`; every WebSocket mutation branch calls one `admit()`; Law acts authorized against the Law *actually touched* (name lookups can no longer carry scope onto another Law) | `SaveSystem.*`, `WebSocketServer.cpp` |
| 11 | Native signer `earthcall_first_mover sign-challenge` builds the transcript itself; bridge gets the passphrase from env or the macOS Keychain; key never enters Node | `src/Identity/FirstMoverTool.cpp` |
| 12.2, 14 | Bridge authenticates on connect; timeouts report `unconfirmed` (was `sent_without_ack`); switch/create-zone/teleport/speak/screen-record now report the engine's answer instead of hard-coded success | `earthcall-mcp-server.js` |
| 13.1 | Foreign-created Laws are authored by the mover | `WebSocketServer.cpp` via `foreignLawAuthors` |
| 8d | New floor: no mover scope (even `**`) may write `saves/identity/` | `FirstMoverRegister::kRegisterDirectory` |

**Deviations (Opus 5.5):** (1) `utterance` is not refused: speech is sensed, and Law authority is clamped, but attribution is enforced: an authenticated mover's words carry its id whatever the payload says, and an unauthenticated peer may not claim a cryptographic id or the present Person's identifier. (2) `save_world` is refused for movers because it writes the Person's profile and every Zone at once; spawn/create-zone persist their own Zone. (3) `switch_zone`, `teleport_player` and `set_physics` are refused as Person presence/body with no grantable coordinate yet. (4) `persistZones()` after a spawn, and inside `authorZone()` for create_zone, runs *outside* the mover session. It rewrites every Zone, so under the session it would half-succeed. The act itself was already authorized against its exact Zone path.

**Tests:** `first_mover_test` (+6), `foreign_actuation_test` (new, 7 cases incl. LawManager reload), `first_mover_websocket_test` (new: real socket, handshake, real spawn, refusals, per-connection auth, revocation), `mcp_first_mover_bridge_test` (new: real node bridge → real signer → real socket), `mcp_bridge_test.js` still 8/8.

**Not done — named plainly (§19 last bullet):**
- §13.4 object/property provenance: foreign spawns and writes are logged to stderr, but no durable provenance relation records which mover made them.
- `onBehalfOf` is not on the wire at all (§13.3 permits omitting it).
- Person login is still headless (env passphrase). No interactive unlock UI.
- The register is not yet itself a Singular `first-movers` with properties (§8a of FIRST_MOVER_AUTHORING).
- A mover-authored Law's reach *when it fires* is governed by Law authority/targets/TransferPolicy, not by the mover's file scope. Decide whether fire-time reach should also be bounded by the author's scope.
- `bridge.py` (legacy Studio) and any other socket client now get `no-first-mover-session` for mutations until given a mover.
- Signer custody: the mover passphrase lives in env or the Keychain; a process running as the user can still read the user's Keychain.
- Merge seam: Sol's `sol/universal-singular-persistence-current-20260923` also edits `FirstMover::toJson/fromJson` and `loadFromJson`; rebase onto `unique_ptr` storage (`for (auto& mover : _movers)` becomes `mover->`).

---

## 1. Telos

MCP is a modality, not an ontology and not an authority source.

Earthcall already distinguishes three relevant facts:

1. a **Person** may authorize or recognize a First Mover;
2. a **First Mover** is the causal actor that reaches the substrate directly;
3. a **Law author** is the Singular that actually authored the Law.

Those facts must not collapse into one another.

The target path is:

```
authenticated Person
    -> recognizes / grants a First Mover
    -> FirstMoverRegister
    -> authenticated foreign caller proves possession of that mover identity
    -> bounded FirstMoverSession
    -> existing FirstMover scope gate
    -> existing TransferPolicy gate where a property boundary is crossed
    -> mutation
    -> durable provenance / truthful authorship
```

If Sol, Astra, Claude, Gemini, a CLI process, or another foreign tool acts through MCP under Zach's permission, the foreign actor remains the First Mover. Zach's identity is the source of delegated standing, not a substitute author. `onBehalfOf` is context, not authorship.

The implementation is complete only when Earthcall can answer, structurally and inspectably:

- Which First Mover acted?
- Which authenticated Person granted its standing?
- What bounded resource scope covered this act?
- What existing TransferPolicy gate applied?
- What did the mover actually author or mutate?
- What was refused, and why?

---

## 2. Current-state findings at the plan base

These facts were rechecked against `4984afe...`; do not assume them unchanged when implementation begins.

### 2.1 FirstMoverRegister already owns the right category of standing

`src/Identity/FirstMoverRegister.{hpp,cpp}` already provides:

- `FirstMover : Singular`
- cryptographic `SingularId`
- Person-signed grants
- signed scope lists
- quarantine/refusal reasons
- `mayWrite(...)`
- `FirstMoverSession` RAII
- save-root confinement
- `SaveSystem` enforcement while a session is active

This is the correct center. Do **not** create an MCP permission registry, an `McpFirstMoverManager`, or vendor-specific authority tables.

### 2.2 The current trust-root check is incomplete

The open task is correct: `evaluate()` only rejects a model grantor when that grantor is itself present in the serialized register and labeled Model.

A valid signature from an absent grantor is therefore not proven to terminate in an authenticated Person.

A serialized `kind: person` label is not a trust root.

A Person-shaped world record is not sufficient either. A hostile save can mint its own keypair, write the corresponding public key into a Person record, and sign a model grant. Public-key possession must be proved through a runtime trust act outside the untrusted register/save claim.

### 2.3 Present Person login is not yet cryptographic authentication

`Person::login(sessionId)` currently marks session state; it does not prove possession of `Person::personId()`'s private key.

`EngineInit.cpp` explicitly says a keyed profile still requires a future login/signature path rather than being trusted merely because it is on disk.

Therefore the MCP implementation must not use:

- `Person::isLoggedIn()` alone,
- a loaded `personId` alone,
- a display name,
- or a serialized Person record

as the First Mover register's trusted root.

### 2.4 The live WebSocket mutation route currently bypasses FirstMoverSession

The real MCP route reaches `WebSocketServer.cpp`, then mutates live state on the main thread.

Handlers include property writes, object/field spawn, object transform/delete, Law author/update/delete, Zone creation/switching, teleportation, physics controls, event injection, and save.

These handlers currently do not establish a FirstMoverSession before mutating.

That means the existing `SaveSystem` First Mover gate normally observes "no mover active" and takes its ordinary engine/in-world allow branch.

### 2.5 External Law authorship is currently false

The MCP/WebSocket Law creation path currently obtains:

```cpp
Person* p = Engine::instance().getPerson();
```

and creates a new Law with that Person as author when present.

That records the live human Person as author of text/AST produced and injected by the foreign model/tool. This must be removed from the foreign-author path.

`Law::authors()` already accepts arbitrary `Singular` beings, and `Identity::FirstMover` is already a `Singular`. No new author type is required.

### 2.6 FirstMover lifetime must be stable before Laws point at movers

The register currently stores movers by value in a `std::vector<FirstMover>`.

A Law's author Formation keeps Singular pointers. If a Law points directly at a vector element and the vector later reallocates, that pointer can become invalid.

Therefore First Movers must gain stable object addresses before they are used as live Law authors.

### 2.7 Durable Law reload does not yet resolve FirstMover authors

`ZoneManager::switchTo()` resolves authored Law references through:

1. actual Person resolution,
2. target-zone local beings,
3. Universe/shared beings,
4. legacy sibling fallback.

It documents model-author Objects as the legacy non-Person representation.

A recognized First Mover is not yet a durable author-resolution source, so a Law truthfully authored by a FirstMover can be correct in memory and fail to reattach after reload.

### 2.8 TransferPolicy is already the property gate

`TransferPolicy` exposes the existing Kernel / Governable / Gated policy and `canTransfer(PropertyPath)`.

The foreign mutation path must consult it for property mutation. Do not create an MCP allowlist that duplicates those semantics.

---

## 3. Architectural invariants

Every implementation checkpoint must preserve these invariants.

### 3.1 Separate five identities

Never merge these fields:

- **actor / mover** — the concrete FirstMover whose key is bound to the request;
- **grantor** — authenticated Person identity that granted standing;
- **author** — Singular that actually authored a Law or being;
- **onBehalfOf** — optional delegation context;
- **transport peer** — WebSocket/MCP process/session.

A request may contain all five and they may be different.

### 3.2 Payloads cannot raise authority

A request body may not:

- insert or replace its own FirstMover grant;
- widen scopes;
- nominate itself as a trusted Person root;
- choose a higher Law authority;
- bypass TransferPolicy;
- convert `onBehalfOf` into authorship.

Caller-supplied authority fields are descriptive at most and should normally be ignored or rejected.

### 3.3 Bare mover IDs are not authentication

`"firstMoverId": "..."` by itself is equivalent to a username with no password.

The transport must prove possession of the private key corresponding to the FirstMover's cryptographic `SingularId`, or use a transport-bound capability that was itself issued only after such proof.

### 3.4 MCP read access remains independent

Read-only operations such as world state, connection status, and save catalog inspection remain usable without mutation standing unless another existing privacy policy says otherwise.

A failed or absent First Mover authentication must disable mutation, not blind the diagnostic/read surface.

### 3.5 No stable resource mapping means refusal

If Earthcall cannot truthfully map a mutation to an existing bounded resource scope, the first implementation refuses it explicitly.

Do not paper over the gap with `**`, process-global super-scopes, or MCP-specific exceptions.

---

## 4. Phase 0 — tests that pin the current defects

Add regressions before changing semantics.

### 4.1 Absent-grantor trust-root regression

Extend `tests/person/first_mover_test.cpp`:

- construct a mover grant signed by a real key;
- omit that grantor from the register;
- serialize/load the register;
- verify standing is refused because the grantor is not an authenticated trusted Person root.

The old behavior should fail this test.

### 4.2 Session leak regression

Keep the existing RAII behavior and add a nested-session case:

- engine/no-session state;
- mover A session;
- nested mover B session;
- B exits -> A restored;
- A exits -> no active mover.

### 4.3 Authorship-forgery regression

Add a focused test around the foreign Law author seam:

- authenticated mover acts `onBehalfOf = Person X`;
- Law author must be mover;
- Person X must not appear in `authors()` unless Person X independently authored it through a genuine Person path.

### 4.4 Unknown mover / copied ID regression

A request naming a valid registered mover ID without completing possession proof must be refused before mutation.

---

## 5. Phase 1 — repair FirstMover standing and the trust root

Refactor `FirstMoverRegister` so standing validity and path scope are two stages, not one monolithic path evaluation.

### 5.1 Add a standing decision independent of paths

Introduce a small public value result, for example:

```cpp
struct FirstMoverStanding {
    bool recognized;
    StandingReason reason;
    const FirstMover* mover;
};
```

or an equivalent enum/result.

It should answer only:

- registered?
- self-attested?
- grantor cryptographically usable?
- grantor is an authenticated trusted Person root?
- signature verifies?
- grant subject/issuer match?
- signed scopes match current scopes?
- crypto available?

Then `mayWrite()` composes:

```
valid standing
    + canonical save-root confinement
    + scope match
```

`explain()` and quarantine must consume the same reason source so diagnostic text cannot drift from enforcement.

### 5.2 Add non-serialized trusted Person roots

The register needs runtime trust roots separate from `movers[]`, for example:

```cpp
void trustAuthenticatedPerson(const SingularId&);
void clearAuthenticatedPersons();
bool isAuthenticatedPerson(const SingularId&) const;
```

The exact names may differ, but the constraints do not:

- trusted roots are **not serialized**;
- `loadFromJson()` cannot create them;
- `kind: person` cannot create them;
- only a successful Person identity-authentication path can add one;
- logout/session teardown must remove or invalidate the root as appropriate.

### 5.3 Headless authenticated-Person bootstrap

The first production-capable implementation may use the existing KeyStore and existing environment-driven identity-migration pattern as a temporary headless bridge.

Required proof:

1. explicit Person `SingularId`;
2. successful `KeyStore::load(personId, passphrase)`;
3. loaded private key's `id()` equals the claimed Person id;
4. the active Person/profile resolves to that same id;
5. only then seed `trustAuthenticatedPerson(personId)`.

Do not trust an environment variable containing only a public id.

Do not silently auto-select among multiple Person profiles.

Long-term this should move behind the real interactive Person login/signature path; the FirstMoverRegister API should not care which UI performed the authentication.

### 5.4 Tighten recognize()

`recognize()` should not accept "grantorKind == Person" as sufficient authority.

It should require the grantor's id to be present in the runtime authenticated-Person roots before minting/accepting a delegation.

Tests may seed a trusted root through an explicit test-only/authentication fixture; production may not.

---

## 6. Phase 2 — make FirstMover a stable live Singular

Before using FirstMover pointers as Law authors, make their addresses stable.

Preferred implementation:

- replace `std::vector<FirstMover>` value storage with stable ownership such as `std::vector<std::unique_ptr<FirstMover>>`;
- preserve deterministic serialization order;
- update `find()`, iteration, tests, and `toJson/loadFromJson`;
- never expose a pointer whose lifetime can be invalidated by adding another mover.

Do **not** solve this by inventing an MCP-specific Object author proxy.

A legacy `author.gemini-spark` Object may remain load-compatible, but new foreign authorship should resolve to the actual recognized FirstMover Singular.

Add a regression:

- obtain mover pointer;
- add enough additional movers to force container growth;
- verify the original pointer remains valid and still identifies the same mover.

---

## 7. Phase 3 — durable FirstMover author resolution

Teach authored Law rehydration to resolve a cryptographic FirstMover identifier.

### 7.1 Resolution order

In `ZoneManager::switchTo()` author resolution:

1. actual Person with matching authenticated identity rules;
2. recognized, non-quarantined FirstMover by cryptographic SingularId;
3. target-zone local declared author referents;
4. shared Universe beings;
5. legacy sibling fallback.

The FirstMover lookup should parse the candidate identifier as a `SingularId`; non-cryptographic slugs fall through normally.

### 7.2 Fail closed on invalid FirstMover authors

If a Law names a cryptographic FirstMover id that exists only as quarantined/untrusted standing, do not convert it into an authored/firing Law.

Refuse activation with an explicit reason naming the author standing failure.

Authorship history and present authorization are distinct, but Earthcall must not treat a forged serialized FirstMover claim as enough to satisfy `Law::isAuthored()`.

### 7.3 Compatibility

Do not delete support for legacy model-author Objects in this patch.

Migration from legacy author Objects to FirstMover identities is a separate data migration.

---

## 8. Phase 4 — generic ForeignActuationGuard, not MCP policy

Add the smallest generic modality helper under `src/Singularity/Foreign/`, tentatively:

```
ForeignActuationGuard.hpp/.cpp
```

The name is not sacred; its responsibilities are.

It must own **zero independent permission policy**.

It composes:

- authenticated transport-bound mover identity;
- `FirstMoverRegister` standing;
- existing path scope through `mayWrite()`;
- a bounded `FirstMoverSession`;
- `TransferPolicy::canTransfer()` for property paths;
- a structured refusal result.

Conceptual API:

```cpp
ForeignActuationDecision authorize(
    const SingularId& mover,
    const std::filesystem::path& durableResource,
    const std::optional<PropertyPath>& property);
```

and a separate RAII execution wrapper once authorized.

The helper must not:

- store grants;
- create scopes;
- inspect vendor names;
- infer Personhood;
- silently widen paths;
- retain FirstMoverSession beyond one main-thread mutation.

### 8.1 Structured refusal

Return machine-readable fields suitable for WebSocket ACKs:

- `status: "refused"`
- `reasonCode`
- `reason`
- `moverId` when known
- `resource`
- `property` when applicable
- `modality: "mcp"` or generic foreign modality name

Do not expose private material or signatures in errors.

---

## 9. Phase 5 — canonical resource mapping for live mutations

Use the durable identity path as the scope coordinate for a live mutation.

This lets the existing FirstMover scope vocabulary govern both "write the save directly" and "mutate the live being that will persist there" without inventing a second permission lattice.

### 9.1 Initial allowed mappings

Implement explicit resolvers for:

**Law create/update/delete**
- `SaveSystem::lawIdentityPath(lawId)`
- new Law path may be computed before creation

**Spawn into active Zone**
- active Zone -> `SaveSystem::zoneIdentityPath(zoneId)`
- active Home -> `SaveSystem::homeIdentityPath(homeId)`

**Mutate/delete an Object**
- resolve its owning active Zone/Home;
- authorize against that container's durable identity path;
- if ownership is ambiguous, refuse.

**Create Zone/Home**
- authorize against the exact future Zone/Home identity path before creation.

**Save world**
- authorize the exact requested save filename;
- enter FirstMoverSession and let SaveSystem's existing enforcement remain the final gate.

### 9.2 Property writes

Property mutation requires both:

1. durable resource scope; and
2. `TransferPolicy::instance().canTransfer(propertyPath)`.

A closed Governable/Gated property must yield an explicit refusal.

Kernel semantics remain whatever TransferPolicy defines; MCP does not reinterpret them.

### 9.3 Initially refuse unmapped global/substrate acts

Until an honest resource coordinate exists, fail closed for operations such as:

- process-global physics toggles;
- Person teleport/body mutation without an authenticated Person-resource mapping;
- arbitrary event injection / speech that can trigger Laws;
- any target whose durable owner cannot be identified.

These are not removed forever. They are held at the boundary until their existing Earthcall resource representation is explicit.

This is preferable to granting `**`.

---

## 10. Phase 6 — bind foreign identity to the connection

Do not accept a mover id from every mutation payload.

Authenticate once, bind the mover to the WebSocket connection, and have each queued main-thread mutation inherit that already-verified identity.

### 10.1 Challenge flow

Add a generic foreign-auth handshake:

1. client requests challenge;
2. server produces a CSPRNG nonce and short expiry;
3. response includes protocol/domain separator and challenge id;
4. client signs canonical bytes with the FirstMover private key;
5. client returns mover id + signature;
6. server verifies:
   - mover id parses and can authenticate;
   - FirstMover standing is currently valid;
   - signature verifies against mover public key;
   - challenge is unexpired, unused, and connection-bound;
7. connection state stores only the verified mover id / auth state, never the private key.

Canonical signed transcript should include at least:

```
earthcall-first-mover-session-v1
challenge-id
nonce
connection/session discriminator
mover-id
```

Use length-prefixing or an existing canonical claim encoding; do not delimiter-join attacker-controlled fields ambiguously.

### 10.2 Anti-replay

- challenge is single-use;
- expires quickly;
- is bound to one connection;
- authentication disappears on disconnect;
- reconnect requires a new challenge.

### 10.3 No payload override

After connection authentication:

- mutation payload `firstMoverId`, `grantedBy`, `scopes`, `kind`, or `authority` fields are ignored or rejected;
- the server uses the connection-bound mover only.

---

## 11. Phase 7 — MCP-side signing without secret duplication

The Node MCP bridge must prove possession of the mover key, but it must not invent a second private-key storage format.

Preferred order:

1. reuse the existing Earthcall KeyStore through a tiny native signer/identity helper, or another existing native identity entry point if one lands first;
2. keep the private key outside the repository and outside JSON-RPC;
3. unlock/sign only for the handshake;
4. keep the resulting authenticated WebSocket session, not the private key, as the ordinary mutation credential.

Do not:

- commit raw mover keys;
- put raw private keys in MCP tool schemas;
- send them through WebSocket;
- store them in saves;
- create a plaintext Node-only key store.

If no safe signer is configured, the MCP bridge should still start and expose read-only tools, while mutation tools return an explicit "no authenticated First Mover session" refusal.

This fail-closed mode is an acceptable intermediate checkpoint.

---

## 12. Phase 8 — wire WebSocket mutations through one gate

Every mutating branch in `WebSocketServer.cpp` should follow the same sequence:

```
parse request
-> resolve target/resource without mutating
-> require authenticated connection mover
-> authorize resource + optional property
-> create bounded FirstMoverSession
-> mutate on main thread
-> persist if that operation already persists
-> emit structured ACK
-> session destructs before returning
```

Do not scatter one-off `mayWrite()` calls across handlers without a common helper; that is how one future tool becomes the bypass.

### 12.1 Read-only branches

Leave read-only state queries outside the mutation guard.

### 12.2 ACK discipline

Any mutation tool that currently returns success before receiving an engine ACK must be upgraded to wait for a concrete success/refusal result.

A caller must never see "success" for a mutation Earthcall later refused.

---

## 13. Phase 9 — truthful Law authorship and provenance

### 13.1 New Law

When a foreign authenticated mover creates a Law:

```cpp
FirstMover* mover = ...validated stable register entry...;
law = lm->createLaw(name, {mover});
```

Do not insert `Engine::getPerson()` as author merely because a Person is present.

### 13.2 Updating an existing Law

Mutation actor and original author are not automatically the same.

For an update:

- preserve the Law's existing authors unless the authored operation explicitly creates a new author relation;
- record that the FirstMover performed the mutation in provenance/audit;
- do not silently add the delegating Person to authors.

### 13.3 onBehalfOf

If the request carries an optional `onBehalfOf` Person:

- resolve it only as context;
- require it to correspond to the authenticated grantor/delegation context if policy uses it;
- record it separately from `authors`;
- never treat it as authority elevation.

The initial implementation may omit `onBehalfOf` from the wire entirely rather than implement it incorrectly.

### 13.4 Object/property provenance

Use existing provenance/stakeholder mechanisms where semantically correct.

For direct foreign mutation, record at minimum:

- mover SingularId;
- modality;
- target/resource;
- operation kind;
- Law id if the act created/edited a Law;
- timestamp through the existing event/provenance conventions.

Do not invent a giant parallel MCP audit ledger if an existing provenance relation or stakeholder record can express the fact.

---

## 14. Phase 10 — MCP bridge schema changes

`earthcall-mcp-server.js` should distinguish read-only and mutating tools internally.

### 14.1 Connection state

Track:

- engine connection;
- auth challenge state;
- authenticated mover id;
- auth expiry/session status;
- last explicit refusal.

### 14.2 Tool behavior

Read-only tools:
- work without First Mover auth.

Mutating tools:
- check authenticated session before send;
- use `sendWithAck`;
- surface structured refusal;
- never add grant/scope/authority fields supplied by the model.

### 14.3 Connection status

Extend connection/status diagnostics to report:

- engine connected?
- mutation standing authenticated?
- mover id/fingerprint?
- standing currently recognized?
- refusal reason if not?

Do not report or return private key material.

---

## 15. Test matrix

### 15.1 FirstMoverRegister

Required focused cases:

- trusted Person -> recognized mover -> allowed in-scope path;
- absent grantor -> refused;
- serialized `kind: person` without runtime trusted root -> refused;
- model grantor -> refused;
- self-attestation -> refused;
- tampered scope -> refused;
- out-of-scope -> refused;
- save-root escape -> refused;
- crypto unavailable -> explicit refusal;
- runtime root is not serialized;
- logout/root removal causes subsequent foreign auth to fail as designed.

### 15.2 ForeignActuationGuard

- recognized mover + allowed resource -> allowed;
- unknown mover -> refused;
- quarantined mover -> refused;
- out-of-scope resource -> refused;
- closed TransferPolicy property -> refused;
- open property -> allowed;
- no durable mapping -> refused;
- FirstMoverSession active only during callback;
- exception path still clears/restores session.

### 15.3 Auth handshake

- valid challenge signature -> authenticated;
- copied mover id without signature -> refused;
- signature from wrong key -> refused;
- replayed challenge -> refused;
- expired challenge -> refused;
- challenge from connection A submitted on B -> refused;
- reconnect has no inherited standing.

### 15.4 Law authorship

- foreign-created Law author is FirstMover;
- `onBehalfOf` Person is not author;
- delegating Person is not silently author;
- Law save/reload resolves FirstMover author;
- quarantined/invalid mover author does not become a firing authored Law;
- legacy model-author Object still reloads.

### 15.5 Real MCP / WebSocket path

At least one integration test must use the actual message path rather than a self-agreeing mock:

- connect;
- authenticate test mover;
- perform one allowed mutation;
- observe real world state change;
- perform one denied mutation;
- observe no state change and explicit refusal;
- disconnect;
- verify later engine save runs with no leaked FirstMoverSession.

### 15.6 Read-only MCP

Without auth:

- state query works;
- connection status works;
- mutation refuses cleanly.

---

## 16. CI and debugging discipline — Big Chungus Principle

For every CI run:

1. inspect workflow/run/job summaries first;
2. identify the exact failed job;
3. inspect step summaries;
4. search for the exact test/error marker;
5. fetch only the bounded failure region;
6. expand only when the narrower evidence is insufficient.

Do not download or read an entire multi-megabyte log because one test failed.

Run focused tests first, then the relevant CI job, then broader suite if the focused seam is green.

Likely focused targets include:

- `first_mover_test`;
- new foreign-actuation/auth test;
- `mcp_bridge_test.js`;
- any new WebSocket integration test;
- save/load tests touched by Law author resolution.

Because C++ sources are globbed at configure time, reconfigure after adding a new `.cpp`.

---

## 17. Suggested implementation checkpoints / PR shape

### Checkpoint A — trust root only

Changes:
- absent-grantor regression;
- standing/path evaluation split;
- runtime authenticated-Person roots;
- `recognize()` requires authenticated root.

Exit condition:
- no production MCP change yet;
- existing engine/in-world saves still work;
- serialized register can no longer manufacture its own Person root.

### Checkpoint B — stable FirstMover + author reload

Changes:
- stable mover storage;
- FirstMover author resolution;
- reload tests.

Exit condition:
- a recognized FirstMover can safely exist as a Law author across container growth and save/reload;
- legacy model-author Objects still work.

### Checkpoint C — generic foreign gate + WebSocket identity

Changes:
- foreign actuation helper;
- challenge/proof-of-possession;
- connection-bound mover;
- resource mapping;
- TransferPolicy composition;
- structured refusals.

Exit condition:
- one real WebSocket mutation is accepted;
- one out-of-scope mutation is refused;
- copied id alone fails.

### Checkpoint D — MCP tool integration + truthful provenance

Changes:
- Node bridge signer integration;
- read-only/mutation split;
- ACK discipline;
- Law authorship correction;
- provenance;
- full integration tests.

Exit condition:
- external MCP caller acts as itself, under Person-delegated standing, with bounded scope and inspectable provenance;
- no MCP-specific ontology or permission lattice exists.

Do not merge checkpoints merely because tests are green if the stated exit condition is not true.

---

## 18. Explicit non-goals for this implementation

This plan does not attempt to:

- sandbox the MCP process at the host OS level;
- make localhost/WebSocket origin checks into identity authentication;
- grant MCP access to arbitrary host files;
- solve revocation/history semantics for every past FirstMover act;
- migrate every legacy model-author Object immediately;
- create vendor-specific model identity classes;
- let a model grant other models standing;
- make current `Person::login()` cryptographically trustworthy by assertion;
- bypass TransferPolicy because "MCP is trusted";
- add a second property permission system.

Host sandboxing, transport reachability, Earthcall ontological standing, property transfer authority, save serialization, and Law authorship are different layers and must remain named separately.

---

## 19. Definition of done

The work is ready for review when all of the following are true:

- a mutating MCP request cannot act without a concrete recognized FirstMover;
- mover identity is proven, not merely named;
- the mover's grant terminates in an authenticated Person root not created by the save/register itself;
- the request runs inside a bounded FirstMoverSession;
- the exact durable resource is inside signed scope;
- property writes also pass TransferPolicy;
- unmapped mutation classes fail closed with explicit reasons;
- a foreign-authored Law records the foreign FirstMover, not the human grantor, as author;
- `onBehalfOf` cannot forge authorship;
- FirstMover authors survive save/reload safely;
- read-only MCP remains usable without mutation standing;
- scope/grant/authority cannot be widened by payload;
- session state disappears on disconnect and cannot leak into ordinary engine saves;
- at least one real MCP/WebSocket allowed mutation and one real refused mutation are tested end to end;
- legacy Person/in-world engine paths remain green;
- documentation states any remaining host-OS, signer-custody, or unsupported-mutation gaps plainly.

---

## 20. Implementation rule for the next agent

Before writing code, re-check current HEAD and search the exact symbols in this plan. Earthcall is moving concurrently.

Do not reread giant files from line 1 unless a bounded search failed. Use the Big Chungus Principle.

The smallest correct architecture is not "MCP may mutate because Zach asked it to." It is:

> A concrete foreign First Mover proved which cryptographic actor it is; an authenticated Person had already granted that actor bounded standing; Earthcall checked the exact resource and existing transfer gate; the act ran only inside that mover's RAII session; and the world remembers who actually authored what.
