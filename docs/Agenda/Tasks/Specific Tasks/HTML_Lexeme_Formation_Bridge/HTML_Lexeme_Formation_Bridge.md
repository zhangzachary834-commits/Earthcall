# HTML Lexeme Formation Bridge — Implementation Plan

**Status:** In Progress (Foundations Rungs 0–2 implemented and verified)  
**Section:** Modalities · integration · web  
**Date:** 2026-09-15 (Updated 2026-09-16)  
**Architecture:** [`docs/architecture/Integration/HTML_LEXEME_FORMATION_BRIDGE.md`](../../../../architecture/Integration/HTML_LEXEME_FORMATION_BRIDGE.md)  
**Human origin:** Zachary Zhang specified that Earthcall should model website HTML as Lexeme Formations and retain a live local socket back into the page so Person-authored Earthcall changes can modify that page's DOM. The implementation below preserves that requirement while using the existing `RealWebView`, Language/Lexeme identity, Relations/Formations, EventBus/Law, and Foreign/Web boundaries instead of adding a parallel browser ontology.

---

## Completion contract

This task is complete only when one real page running in Earthcall's `WKWebView` can:

1. establish a new document session;
2. mirror a bounded live DOM into distinct Lexeme occurrences, Relations, and Formations;
3. keep the mirror current using ordered `MutationObserver` deltas rather than whole-tree polling;
4. address duplicate same-spelled nodes by exact identity rather than selectors/spelling;
5. accept a structured Person-authored Earthcall DOM Act targeting one exact mirrored node;
6. apply that Act locally in the WebView;
7. sense the resulting DOM mutation back with operation provenance so no echo loop occurs;
8. survive page-script counter-mutations by converging to the browser's actual state;
9. retire the mirror atomically on navigation;
10. expose enough native structure that an authored Law can re-manifest selected DOM-derived meaning as ordinary Earthcall Objects without any site-specific C++ type.

The first shipped witness does **not** need Google, a 3D search river, browser extensions, remote server writes, full CSS layout reconstruction, closed-shadow inspection, or universal cross-origin frame traversal. Those belong to later rungs.

---

## Existing substrate to reuse

Do not start this as a greenfield browser project.

### Foreign/Web already has a real browser actuator

`src/Singularity/Foreign/Web/RealWebView.{hpp,cpp}` already provides the macOS `WKWebView`, URL navigation, JavaScript execution, a `WKScriptMessageHandler`, CSS injection, DOM helpers (`modifyElement`, `addElement`, `removeElement`, `setElementText`, `setElementHTML`), and Earthcall↔page message plumbing.

The new work should extend that surface or place a narrow protocol object beside it. Do not create a second browser implementation.

### Language already supports duplicate spelling with exact identity

`LanguageSystem::intern(symbol, stableId)` permits multiple same-spelled Lexemes while preserving exact identity through `findById` / `findAllBySymbol`. This is precisely what DOM occurrences need.

### Formation already accepts arbitrary Singular membership and Relations

No `DomNode` Earthcall kind is needed. The bridge can construct rooted Formations from occurrence Lexemes and structural Relations.

### The common Law/Event system already provides Decide/Act

Do not add DOM-specific callback behavior above the channel. Sensed transitions should be ordinary ECA events / dirty property or Relation changes, and Person-authored Laws should decide what to do.

---

# Rung 0 — freeze the protocol contract in fixtures before WebKit work

**Goal:** make the translation and identity rules executable without requiring a desktop WebKit session.

### Add a protocol fixture vocabulary under tests

Recommended files (names may be adjusted without changing the architecture):

```text
tests/singularity/foreign_web_dom_protocol_test.cpp
tests/fixtures/foreign_web/dom_snapshot_basic.json
tests/fixtures/foreign_web/dom_delta_basic.json
```

If a dedicated protocol type is useful, keep it beneath the existing modality:

```text
src/Singularity/Foreign/Web/DomMirrorProtocol.hpp
src/Singularity/Foreign/Web/DomMirrorProtocol.cpp
```

A protocol struct here is admissible because it describes the foreign wire contract; it is not an Earthcall in-world kind.

### Define versioned envelopes

Snapshot:

```text
protocolVersion
pageSessionId
sequenceBase
url
rootNodeToken
nodes[]
```

Delta:

```text
protocolVersion
pageSessionId
sequence
originOperationId?  // optional
records[]
```

Act:

```text
protocolVersion
pageSessionId
operationId
kind
targetNodeToken
payload
```

Do not put raw arbitrary JavaScript in the Act envelope.

### Required Rung 0 tests

- reject empty/malformed session IDs;
- reject duplicate node tokens in one snapshot;
- reject missing parent references except the root;
- reject cycles in the incoming DOM parent relation;
- reject out-of-range/bounded payload counts and sizes;
- preserve quotes/backticks/newlines/Unicode as data;
- reject unknown protocol versions loudly;
- prove duplicate tag strings do not collapse protocol identity.

**Exit criterion:** protocol parser/validator tests pass headlessly before any real WebView injection is written.

---

# Rung 1 — browser-side identity and bounded initial snapshot

**Goal:** make one `WKWebView` document speak the protocol truthfully.

### Extend the existing WebKit path

Prefer one injected script resource, e.g.:

```text
src/Singularity/Foreign/Web/dom_mirror.js
```

and a thin C++ bridge beside `RealWebView`, e.g.:

```text
src/Singularity/Foreign/Web/DomMirrorBridge.hpp
src/Singularity/Foreign/Web/DomMirrorBridge.cpp
```

The exact filenames are not ontology; keep them under `Foreign/Web`.

### JavaScript responsibilities

On document readiness:

1. mint one random/opaque `pageSessionId` for the current `Document`;
2. maintain `WeakMap<Node, token>`;
3. assign monotonically unique or cryptographically opaque node tokens within that session;
4. walk the admitted DOM iteratively, not with unbounded recursion;
5. serialize tag/text occurrence, attributes, parent token, and sibling/order facts;
6. enforce maximum nodes, attribute count, text bytes, and total message bytes;
7. send a snapshot through the existing `WKScriptMessageHandler` path.

Do **not** write `data-earthcall-id` into the page by default.

### Snapshot scope

Start with the top-level `Document` and ordinary accessible light DOM. Represent shadow/frame boundaries explicitly but do not attempt to bypass browser access rules.

For a page larger than the configured admission budget:

- send a truthful bounded snapshot;
- mark deferred roots/counts in the protocol;
- provide an explicit later expansion request;
- never silently pretend the omitted subtree is absent.

### Required Rung 1 tests

Use a deterministic local HTML fixture with:

- duplicate `<div>` and `<span>` nodes;
- nested nodes;
- repeated identical text;
- attributes with hostile-looking string content;
- one iframe boundary where practical;
- open shadow root when available.

Verify exact unique tokens and stable tokens across repeated snapshots of the same still-live DOM node.

**Exit criterion:** `WKWebView` sends a valid bounded snapshot whose exact-node identity survives repeated reads while the document remains live.

---

# Rung 2 — translate snapshot into Lexeme Formations

**Goal:** admit the browser snapshot into Earthcall without adding a DOM class hierarchy.

### Translator

Add a translator beneath the foreign modality or reuse a suitable existing Foreign projection surface. It should:

1. validate the complete snapshot before mutating the live Earthcall mirror;
2. mint/intern one **occurrence Lexeme per DOM node token** using an exact stable ID derived from page session + node token;
3. preserve the visible tag/text symbol separately from identity;
4. create occurrence Lexemes for attributes/values as needed by the chosen representation;
5. create the minimal structural Relation vocabulary described in the architecture document;
6. gather each node into its rooted node Formation;
7. gather the whole live document into a document Formation;
8. register/provide those beings to the active foreign-mirror Zone/Universe surface only after the snapshot is coherent.

### Transaction rule

A malformed snapshot must not leave half a page in the world. Build/validate detached state first, then publish the coherent mirror.

### Identity rule

Never resolve a DOM occurrence through `LanguageSystem::findBySymbol`. Exact session-scoped IDs only.

### Ownership/lifetime rule

Because `Formation` stores non-owning pointers, retirement must remove/release occurrence Lexemes from every live Formation before their owning shared pointers are released. Reuse the same lifetime discipline already documented in `LanguageSystem::detachFromAllZones` / Formation release paths rather than leaving dangling members.

### Required Rung 2 headless witness

Given the JSON fixture:

- two `div` occurrences have different exact Lexeme IDs;
- their `symbol` values may both equal `div`;
- parent-child and sibling order resolve through Relations;
- attributes belong to the correct occurrence;
- the document Formation has the expected membership/topology;
- translating the same coherent snapshot twice is idempotent or replaces the old mirror transactionally without duplicate live beings;
- one invalid record causes no partial publication.

**Exit criterion:** a headless fixture reconstructs a truthful native graph without any WebKit requirement.

---

# Rung 3 — incremental `MutationObserver` deltas

**Goal:** stop treating the page as a snapshot and make the mirror live.

### Browser observer

Install one `MutationObserver` after initial snapshot convergence. Observe at least:

```text
childList
subtree
attributes
characterData
```

Batch one observer delivery into one ordered delta envelope.

Translate raw MutationRecords into a small edge vocabulary:

```text
insert
remove
move/reorder (or a canonical remove+insert pair if that preserves truth)
attribute set/change
attribute remove
text change
```

### Sequence discipline

Maintain a monotonically increasing sequence per page session.

Earthcall accepts:

```text
expected next sequence
```

and refuses / requests resync on:

```text
gap
unknown session
malformed reference
impossible parent
out-of-budget batch
```

Do not continue by guessing.

### Native application

Delta application should be transactionally scoped per batch where possible. Relation/membership changes must not temporarily expose a dangling node graph to Law evaluation halfway through a logically atomic browser mutation batch.

### Publish useful edges

After the mirror is coherent, publish ordinary Earthcall events for meaningful sensed transitions, with the exact occurrence Lexeme / Formation as subject where applicable. Names should remain descriptive, e.g. `foreign-node-inserted`, rather than encoding site semantics such as `search-result-added`.

### Required Rung 3 tests

- insert one of two identical nodes and prove only the new identity appears;
- remove one and prove the other duplicate remains addressable;
- reorder siblings and preserve exact identities;
- change one attribute and touch no unrelated node;
- change one text node and preserve parent identity;
- drop sequence N and deliver N+1: resync requested, N+1 not guessed into place;
- remove a node while it participates in Formations: no dangling pointer on the next lookup/tick.

**Exit criterion:** a dynamic local fixture can mutate repeatedly while the Earthcall graph stays structurally coherent.

---

# Rung 4 — structured Earthcall → DOM Act path

**Goal:** give Person-authored Earthcall behavior a lawful actuator back into the exact local page.

### Do not expose arbitrary JS as the normal Law surface

Even though `RealWebView::executeJavaScript` exists, the new Law-facing bridge should accept a bounded structured command set. Start with:

```text
setText
setAttribute
removeAttribute
removeNode
insertElement
insertText
moveNode
```

`setStyleProperty` may be useful as a convenience later, but it should compile to a structured mutation rather than become a miniature CSS ontology.

### Targeting

Every command names:

```text
pageSessionId + exact node token
```

not selector/text/tag.

### Serialization safety

Never concatenate unescaped Person/page strings into JavaScript source. Pass structured payloads through a JSON/object channel into a fixed injected dispatcher. Treat every string as data.

### Refusal behavior

The browser bridge returns explicit status for:

```text
unknown session
unknown/retired node
invalid operation
browser exception
policy/security refusal
budget refusal
```

No successful-looking mirror mutation is allowed before browser confirmation.

### Required Rung 4 witness

From Earthcall, target the second of two identical `<div>` nodes by exact ID and set only its text. The first must remain unchanged.

**Exit criterion:** exact-node local actuation works without selectors and without arbitrary Law-authored JavaScript.

---

# Rung 5 — operation provenance and echo-free convergence

**Goal:** close the loop without recursion or lost page reactions.

### Operation IDs

Every Earthcall Act gets a unique `operationId` within its page session.

Browser dispatcher flow:

```text
receive Act(operationId)
→ validate exact node
→ apply synchronous structured DOM mutation
→ observer.takeRecords()
→ serialize resulting records with originOperationId = operationId
→ send confirmation delta
```

The ordinary observer remains installed. Later asynchronous page reactions are emitted normally without that operation ID.

### Earthcall semantics

An origin-tagged confirmation delta:

- updates/converges the mirror;
- completes the pending Act;
- may publish an audit event;
- does **not** cause the originating Law intent to be emitted again merely because its own consequence was sensed.

An untagged later page mutation is fresh foreign Sense input and may legitimately activate Laws.

### Required Rung 5 witness

Fixture page behavior:

1. Earthcall sets node text to `A`.
2. Confirmation returns tagged with operation ID.
3. No second Earthcall Act is generated from its own confirmation.
4. The page schedules JavaScript that changes the text to `B`.
5. That later untagged mutation reaches Earthcall.
6. Mirror ends at `B`.

**Exit criterion:** 1000 repeated Acts do not create an echo storm, and page reactions remain visible.

---

# Rung 6 — navigation and lifecycle correctness

**Goal:** make live identity impossible to confuse across page/document replacement.

### Full navigation

On a true new `Document`:

1. mark old page session retiring;
2. stop accepting new Acts against it;
3. drain/refuse stale in-flight packets;
4. release old live mirror membership safely;
5. create a new page session;
6. take and validate a new snapshot;
7. atomically publish the new document mirror.

### SPA navigation

When `Document` identity remains but URL/history changes, keep the same page session and sense URL/history as mutable foreign state. Do not mint a new session merely because the address bar changed.

### Stale-message witness

Queue an old-session delta, navigate, then deliver it. It must be refused and must not mutate any new-session Lexeme that happens to have the same tag/text/token number.

**Exit criterion:** zero cross-navigation identity aliasing.

---

# Rung 7 — browser boundaries and resource control

**Goal:** make arbitrary pages safe to sense without treating foreign input as trusted.

### Browser boundaries

- inaccessible cross-origin iframe → opaque boundary occurrence;
- open shadow root → may be represented explicitly;
- closed shadow root → opaque unless legitimately exposed;
- browser/security refusal → recorded as refusal, never bypassed.

### Resource/backpressure rules

Add explicit budgets for:

```text
snapshot bytes
node count
attributes per node
text bytes per node
mutation records per batch
queued bytes / queued batches
maximum act payload
```

When exceeded, choose among:

```text
defer subtree
backpressure
request resnapshot
refuse act
```

Never silently truncate state and then report a complete mirror.

### Stress witness

Generate a local page with a large synthetic DOM and high mutation rate. Verify bounded memory/queue growth and explicit resync/backpressure behavior.

**Exit criterion:** hostile volume produces bounded, legible degradation rather than memory explosion or silent semantic loss.

---

# Rung 8 — Law-facing authorability

**Goal:** prove the bridge is not merely a developer API.

### Minimum authoring surface

Expose sufficient registered channel/mirror properties and events that an ordinary authored Law can:

- recognize a mirrored occurrence by exact identity, Category/Relation membership, or authored semantic relation;
- issue one structured DOM Act;
- react to an external DOM mutation;
- be disabled and thereby stop the behavior.

Do not implement site-specific classification in C++.

### Required authored witness

Build a fixture page with three paragraph nodes. In Earthcall:

1. Person authors a Category/Relation marking one exact occurrence as `category.foreign.demo-target` (or equivalent authored vocabulary).
2. Person authors a Law: on a chosen event, if the subject belongs to that Category, request `setText("changed by Earthcall")`.
3. No page-specific C++ callback is added.
4. Disable the Law and verify the mutation stops.

**Exit criterion:** a Person can govern the page through normal Law/Relation authorship.

---

# Rung 9 — capture live foreign structure into durable Earthcall identity

**Goal:** separate "currently mirrored webpage" from "meaning the Person chose to keep."

### Capture operation

Provide a Person-visible act that takes a selected live DOM-derived Formation and creates durable Earthcall beings with:

- fresh durable Singular IDs;
- selected Lexeme symbols/content;
- selected structural/semantic Relations;
- provenance pointing back to URL + page session + source occurrence metadata;
- explicit Person authorship of the **capture/interpretation**, not false authorship of the website's original source.

Do not copy the page-session node token as the durable identity.

### Required witness

Capture one DOM-derived structure, close/navigate away from the page, and verify the captured being remains while the live mirror is retired.

**Exit criterion:** durable authorship survives independently of the foreign session.

---

# Rung 10 — native manifestation proof

**Goal:** reach the first version of the experience that motivated this work.

Do not build Google-specific code yet. Use a neutral fixture containing repeated content blocks and images/links.

### Projection

Person-authored Law/Formation work should:

1. select a subset of DOM-derived beings through Relations/Categories;
2. create ordinary Earthcall Objects representing those meanings;
3. map ordering/weight/relationship facts into 2D/3D position through authored Law/OntoMath;
4. where an image URL is intentionally admitted and fetched through the foreign/network boundary, feed bytes to `ImageCodecChannel` so the image becomes an Object + Material;
5. preserve a Relation from native manifestation back to the foreign occurrence/captured semantic being;
6. use the common Interaction channel to hover/click the native manifestation;
7. on activation, issue a structured DOM Act or browser navigation through the bridge.

### Minimal visual witness

The first witness may be modest—a row or spatial arc of native result-like Objects—provided the arrangement is authored and not a special C++ renderer.

The later "search results pour through 3D rivers" experience is then a Law/Formation/Material/OntoMath authoring problem rather than another substrate rewrite.

**Exit criterion:** one live webpage can be meaningfully interacted with through a native Earthcall manifestation while remaining synchronized to the underlying local page.

---

## Browser-extension / external-browser continuation (later, not Rung 1)

After the embedded WebKit path is correct, the same protocol may be transported from a browser extension or helper process over Earthcall's existing WebSocket/network substrate:

```text
Chrome / Safari extension
↕ versioned DOM mirror protocol
localhost authenticated WebSocket
↕
Singularity/Foreign + Network
```

Do not begin there. Embedded WebKit gives us one process, an existing message bridge, controlled lifecycle, and a much smaller security surface. The protocol should be transport-neutral so the external-browser path can reuse it later.

The external-browser phase must add authentication, origin scoping, connection identity, replay protection, and explicit Person consent before accepting Acts.

---

## Suggested file boundary

This is guidance, not permission to ignore what the source says when implementation begins.

Likely touched/added files:

```text
src/Singularity/Foreign/Web/RealWebView.hpp
src/Singularity/Foreign/Web/RealWebView.cpp
src/Singularity/Foreign/Web/DomMirrorProtocol.hpp        [new, if useful]
src/Singularity/Foreign/Web/DomMirrorProtocol.cpp        [new, if useful]
src/Singularity/Foreign/Web/DomMirrorBridge.hpp          [new, if useful]
src/Singularity/Foreign/Web/DomMirrorBridge.cpp          [new, if useful]
src/Singularity/Foreign/Web/dom_mirror.js                [new]

possibly:
src/Singularity/Language/LanguageSystem.*                only if a missing exact-identity lifecycle API is proven
src/Relation/Formation/Formation.*                       only if an existing generic invariant is insufficient

new tests:
tests/singularity/foreign_web_dom_protocol_test.cpp
tests/singularity/foreign_web_dom_projection_test.cpp
tests/singularity/foreign_web_dom_delta_test.cpp
macOS/WebKit integration witness target for live round-trip
```

Do not modify generic ontology classes merely to make the first implementation convenient. If a missing generic capability is found, prove it with a focused test and explain why the requirement is not web-specific before changing the core.

---

## Implementation order summary

```text
0 protocol + hostile-input fixtures
1 WKWebView session identity + bounded snapshot
2 snapshot -> Lexeme/Relation/Formation transaction
3 MutationObserver ordered deltas
4 structured exact-node Act
5 originOperationId convergence / echo prevention
6 navigation lifecycle
7 browser boundaries + backpressure
8 Person-authored Law witness
9 durable capture
10 native 2D/3D manifestation witness
11 later: browser extension / localhost WebSocket transport
```

Do not skip directly to the 3D river. If exact identity, delta convergence, and Act provenance are wrong, a beautiful manifestation will merely hide a corrupt bridge.

---

## Definition of done

Mark this task done only after:

- all headless protocol/projection/delta tests are green;
- the macOS WebKit live-roundtrip witness has actually been executed, not merely compiled;
- duplicate same-spelled nodes are addressed correctly by exact identity;
- navigation cannot alias old nodes into a new document;
- 1000 repeated Earthcall Acts produce no echo loop;
- a page-side counter-mutation is sensed and wins as current foreign truth;
- hostile strings cannot escape the structured dispatcher into code;
- resource limits are explicit and tested;
- an authored Law, not a C++ callback, performs one useful DOM modification;
- a selected foreign structure can be captured with durable Earthcall identity;
- one native Earthcall manifestation remains connected to its underlying live foreign occurrence.

Record the exact executable/manual protocol and results when these are run. Until then the architecture is specified, not shipped.
