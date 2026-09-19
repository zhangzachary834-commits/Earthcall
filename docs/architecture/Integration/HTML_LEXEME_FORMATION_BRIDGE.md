# HTML as Lexeme Formation — Bidirectional DOM Integration

**Status:** Architecture specified; implementation not yet claimed.  
**Date:** 2026-09-15  
**Origin and telos:** Zachary Zhang supplied the governing idea in conversation on 2026-09-15: Earthcall should integrate websites so that HTML can be modeled natively as **Lexeme Formations**, while Earthcall retains a live local socket back into the page so Person-authored changes can modify that page's HTML/DOM on the Person's side. Zach also supplied the immediate motivating example: ordinary web search should be capable of leaving the flat browser rectangle and being re-manifested as an authorable Earthcall world rather than remaining a hardcoded page. This document formalizes that human direction against the repository's existing Foreign/Web, Language, Relation/Formation, Law, and interaction architecture. GPT-5.6 Sol organized the contract, identity model, convergence protocol, safety boundary, lifecycle rules, and verification requirements below.

**Read beside:**
- [`INTEGRATION_FRAMEWORK.md`](INTEGRATION_FRAMEWORK.md) — the whole external-application integration thesis and the Foreign modality boundary.
- [`SYNCING_BACK.md`](SYNCING_BACK.md) — the general Act-side requirement that Earthcall changes be translated back into the foreign application.
- [`../law/INTERACTION_AS_LAW.md`](../law/INTERACTION_AS_LAW.md) — why interaction is sensed by channels and decided/acted by authored Law rather than UI callbacks.
- [`../ontology/NEW_KIND_FRAMEWORK.md`](../ontology/NEW_KIND_FRAMEWORK.md) — why DOM nouns do not become Earthcall C++ kinds.
- [`../ontology/NO_BLACK_BOX.md`](../ontology/NO_BLACK_BOX.md) — why state that authored Law must govern cannot remain hidden in an adapter.
- [`../Design/Building 2D and 3D Apps with Earthcall Guide.md`](../Design/Building%202D%20and%203D%20Apps%20with%20Earthcall%20Guide.md) — how foreign meaning may later be re-manifested as authored 2D/3D world structure.
- [`../Design/LEXEME_RELATION_FORMATION_SERIALIZATION.md`](../Design/LEXEME_RELATION_FORMATION_SERIALIZATION.md) — persistence constraints for Lexemes, Relations, and Formations.

---

## 0. The thesis

A website integrated into Earthcall is **not an iframe that Earthcall decorates** and is not a new Earthcall UI subsystem. The browser remains the foreign machine that interprets HTML, CSS, JavaScript, layout, accessibility, network state, and browser security. Earthcall senses the live document that machine currently exposes, translates that foreign symbolic structure into its own existing ontology, lets Persons author Laws over the resulting beings, and may then actuate bounded changes back into the local document.

The core loop is:

```text
live browser document
        │
        │ Sense: snapshot + ordered DOM mutation edges
        ▼
Singularity/Foreign/Web
        │
        │ translate foreign structure; define no in-world kinds
        ▼
Lexemes + Relations + Formations in Earthcall
        │
        │ Person-authored Law, Relations, Categories, OntoMath
        ▼
Earthcall-native meaning and manifestation
        │
        │ Act: structured DOM mutation intent
        ▼
Singularity/Foreign/Web
        │
        ▼
local browser document changes
        │
        └────────────── sensed back as truth ──────────────┘
```

The **browser document is sensed foreign state**. The **Earthcall graph is the legible native mirror and the place where authored meaning may be added**. The **Act bridge is an actuator, not an alternate source of truth**: Earthcall requests a local DOM mutation; the browser applies or refuses it; the resulting DOM is sensed back into Earthcall.

This is the web-specific specialization of the existing Integration framework's Sense/Act model.

---

## 1. What this is — and what it refuses

### 1.1 HTML/DOM is a foreign symbolic language

HTML's tags, attributes, text, ordering, and nesting are symbolic facts supplied by a foreign runtime. Earthcall already has a being whose purpose is a linguistic-symbolic unit: `Lexeme`. Therefore the integration should project DOM occurrences into Lexemes and gather them through Relations and Formations rather than minting a parallel DOM ontology.

The intended result is not:

```cpp
class HtmlElement;
class Div;
class Span;
class Anchor;
enum class HtmlTag;
```

Nor is it:

```text
src/UI/
src/BrowserOntology/
src/Html/
```

Those would make the adapter define what a foreign symbol **is** inside Earthcall. The adapter may know how WebKit represents a node because that is substrate/protocol knowledge; it may not turn `div`, `button`, `search-result`, or any other web-domain noun into privileged Earthcall ontology.

### 1.2 The live DOM, not raw response HTML, is the immediate truth

A browser does not merely display the original HTML response. It parses, normalizes, repairs, executes JavaScript, mutates form state, creates shadow trees, and may replace most of the original structure after load. Therefore the first integration target is the **live DOM**.

Raw source HTML may later be captured as a foreign byte/document artifact when provenance requires it, but it must not be confused with the live document Earthcall is currently sensing.

### 1.3 A website is not its current manifestation

The DOM mirror does not imply that the external site's current visual arrangement is the form Earthcall must preserve. Once the foreign facts are legible as beings and Relations, a Person may author another manifestation: ordinary cards, a 2D map, a three-dimensional river, a library, a constellation, or something not anticipated by the engine.

A Google-specific `SearchResult` class, `RiverLayout` class, or hardcoded result renderer would therefore miss the point. Search-result semantics and river manifestation belong in authored Categories, Relations, Formations, Materials, OntoMath, and Laws built above this bridge.

---

## 2. The Sense / Decide / Act seam

The web bridge obeys the same cut used throughout Earthcall:

```text
SENSE
    Which document exists?
    Which exact DOM node exists?
    What is its tag/text/attribute structure?
    Which node is its parent/sibling?
    What mutation occurred, and in what order?
        → Singularity/Foreign/Web, permanently

DECIDE
    Is this node meaningful to this Person?
    Is it a search result, title, image, control, citation, or noise?
    Should it be hidden, transformed, related, or re-manifested?
        → authored Law / Category / Relation / Formation

ACT
    Request that this exact foreign node change text/attribute/position in
    the DOM, be removed, or receive an inserted sibling/child.
        → authored Law expresses the intent;
          Singularity/Foreign/Web performs the irreducible DOM operation
```

The channel may say **"node 137's `class` attribute became `result selected`"**. It must not say **"node 137 is a search result, therefore turn it into a river stone."** The latter is human meaning and belongs above the substrate.

---

## 3. Identity: the most important rule

### 3.1 Spelling is not identity

A page may contain ten thousand `<div>` nodes. The string `"div"` describes a symbol; it does not individuate an occurrence. A CSS selector is also not identity: selectors are predicates over a mutable tree and can match zero, one, or many nodes at different moments.

This bridge therefore follows the same distinction Earthcall has been enforcing elsewhere between **what a being is called** and **which being it is**.

Every live DOM occurrence receives an opaque, exact, session-scoped identity:

```text
page-session.<session-id>.node.<token>
```

The visible symbol remains separate:

```text
Lexeme id:      page-session.a91f.node.137
Lexeme symbol:  div
```

Two `div` nodes share spelling and do not share identity.

### 3.2 Browser-side node identity

For the embedded `WKWebView` path, injected JavaScript should maintain:

```javascript
WeakMap<Node, DomNodeToken>
```

The `WeakMap` is the authoritative association between a live JavaScript `Node` object and the opaque token used over the bridge. By default the bridge should **not** write `data-earthcall-id` or any other identifying attribute into the website's DOM merely to remember identity; integration metadata belongs beside the foreign document, not inside it.

When a node object disappears, its token is never silently rebound to a different node. A newly created DOM `Node` receives a new token even if its tag, text, CSS selector, location, and attributes are identical to a node that was just removed.

### 3.3 Document-session identity

A top-level document load has its own opaque `PageSessionId`. Full navigation retires the previous session and all of its node identities. The same URL loaded twice does not imply the same document identity.

SPA history changes (`pushState`, client-side routing) may remain inside one document session when the JavaScript `Document` object remains the same; their URL/history transitions are sensed facts inside that session. A true document replacement starts another session.

A stale packet naming an old session is refused, not guessed into the new page.

### 3.4 Selectors are queries, never durable references

CSS selectors, XPath expressions, DOM paths, element IDs, accessible names, text matches, and semantic predicates are useful **resolution strategies**. They are never the Earthcall identity of a live node.

A Law may ask a query to discover candidates. Once the bridge is acting on a live occurrence, the actuation target is the exact session + node token.

---

## 4. Native representation: Lexeme Formations

### 4.1 Occurrence Lexemes

The initial bridge should represent each live DOM occurrence as a distinct Lexeme with a stable identifier for the life of the page session. Its `symbol` expresses the foreign lexical fact:

```text
element node:  symbol = "div", "a", "img", "input", ...
text node:     symbol = the sensed text occurrence, or "#text" with text as an authored property
comment node:  optional, normally omitted unless requested
```

The exact choice for large text payloads is an implementation concern, but identity must remain occurrence-specific. `LanguageSystem::intern(symbol, stableId)` already permits separate same-spelled Lexemes under distinct stable IDs, and exact-ID lookup is the identity path; the single-symbol index is explicitly only a convenience/default binding.

The DOM bridge must therefore **never use `findBySymbol("div")` to recover a particular DOM occurrence**.

### 4.2 One node, one rooted Formation

A DOM node may be represented as a rooted Formation whose root is the occurrence Lexeme. Attribute-name/value occurrences and other directly constitutive symbolic facts may join that Formation through Relations.

Illustrative structure:

```text
Formation: page-session.a91f.node.137.formation
root:
    Lexeme page-session.a91f.node.137
      symbol = "a"

members:
    Lexeme page-session.a91f.attr.137.4       symbol = "href"
    Lexeme page-session.a91f.attrval.137.4    symbol = "https://example.com/"

Relations:
    node.137    --dom-has-attribute--> attr.137.4
    attr.137.4  --dom-has-value-----> attrval.137.4
```

The Relation type strings shown here are authored vocabulary, not enum values. They may later be gathered into Category/Formation structure of their own; they must not be hardcoded as new `RelationKind` values.

### 4.3 Document Formation

The whole live document is a larger Formation containing the node occurrence Lexemes and the structural Relations among them. At minimum it preserves:

```text
parent / child membership
sibling order
attribute attachment and value
text-node placement
frame/shadow boundaries when visible to the browser bridge
```

Possible initial Relation vocabulary:

```text
dom-child-of
dom-next-sibling
dom-has-attribute
dom-has-value
dom-shadow-root-of
dom-frame-boundary-of
```

The vocabulary is descriptive. The exact set should remain small and justified by facts the browser can actually sense.

### 4.4 Do not duplicate one truth in two representations

If an attribute's authoritative Earthcall mirror is a Lexeme + Relation structure, do not also maintain an unrelated `dom.attributes.class` property whose value can diverge from it. Derived convenience views may exist if they are visibly derived and cannot disagree silently.

High-frequency browser state that is not naturally an HTML attribute—such as an input's live `value`, selection, scroll offset, or focus state—may later be exposed as registered/derived foreign-channel readings or authored properties, but each addition must name its source of truth and its synchronization contract.

### 4.5 Foreign facts are not automatically Person authorship

A sensed DOM structure came from a foreign system. Projecting it into Earthcall does not forge the claim that Zach or another Person authored the site's original HTML. Provenance must distinguish:

- foreign source / sensed occurrence;
- Person-authored interpretation or Category membership;
- Person-authored local mutation request;
- optional Person-authored capture into a durable Earthcall world.

Nothing enters the durable authored world under a fabricated human author.

---

## 5. Initial snapshot and incremental mutation protocol

### 5.1 Snapshot once, deltas thereafter

The bridge begins a document session with a bounded initial snapshot. After convergence, it does **not** repeatedly serialize the whole DOM every frame. The injected JavaScript installs a `MutationObserver` and emits ordered mutation batches.

This matches Earthcall's event doctrine: transitions are edges; standing state is state.

### 5.2 Snapshot envelope

The exact transport encoding may be JSON first and replaced later without changing the ontology. A minimal envelope is:

```text
protocolVersion
pageSessionId
sequenceBase
url
rootNodeToken
nodes[]
```

Each node record contains only sensed facts required to reconstruct the mirror:

```text
nodeToken
nodeType
symbol/tag or text payload
parentToken
sibling/order information
attributes[]
namespace when relevant
```

The bridge must validate all lengths, counts, and recursion/iteration bounds before admitting the payload. A hostile or pathological page is foreign input.

### 5.3 Mutation edges

After the snapshot, `MutationObserver` batches become ordered edges such as:

```text
node-inserted
node-removed
node-reparented / reordered
attribute-changed
attribute-removed
text-changed
```

A mutation packet includes:

```text
pageSessionId
sequence
originOperationId?   // present when this edge confirms an Earthcall Act
records[]
```

Earthcall applies packets only in sequence. A gap, duplicate outside the replay window, malformed reference, or unknown session causes a bounded resynchronization request instead of a best-effort guess.

### 5.4 `MutationObserver` is the edge source, not a polling loop

The browser already knows exactly when the DOM changes. Polling the full tree would waste CPU, lose causal boundaries, and make rapid remove/insert/reorder sequences ambiguous. The observer should batch the browser's own mutation records once per delivery turn and send the smallest truthful delta that preserves ordering.

---

## 6. Actuation: Earthcall may modify the local page

### 6.1 Structured acts, not arbitrary JavaScript Laws

`RealWebView` already has `executeJavaScript`, DOM modification helpers, and a JavaScript message channel. Those are useful substrate capabilities. They must not become a Law action equivalent to `eval(<Person-authored-string>)`.

The native Law vocabulary should issue **structured DOM intents** such as:

```text
set text of exact node
set attribute on exact node
remove attribute from exact node
insert a new text/element occurrence at an exact parent/position
remove exact node
move exact node under exact parent/position
```

The Foreign/Web adapter converts those structured acts into JavaScript/WebKit calls. This keeps code execution beneath the channel while keeping human intention legible above it.

### 6.2 Local manifestation is the initial authority boundary

Rung 1 modifies only the page instance owned by Earthcall's local `WKWebView`. Changing a node locally does **not** claim to have changed the remote website's database, server, source repository, or another user's page.

A later provider/API integration may lawfully send remote mutations, but that is a different actuator with its own authority and provenance.

### 6.3 Act intent is not sensed truth

When Earthcall asks the page to change, the mirror should not simply mutate itself optimistically and declare success. The correct cycle is:

```text
Law authors intent
→ Foreign/Web applies command
→ browser DOM becomes whatever the browser actually permits
→ resulting mutation records return through Sense
→ Earthcall mirror converges to the sensed result
```

This matters because page JavaScript may reject, normalize, immediately overwrite, or react to a local mutation.

---

## 7. Echo prevention and convergence

A bidirectional bridge without provenance becomes an infinite loop:

```text
Earthcall write
→ DOM mutation
→ observer reports mutation
→ Earthcall interprets it as new external intent
→ writes again
→ ...
```

Every Earthcall-side Act therefore receives an `originOperationId`.

For embedded WebKit, the preferred first implementation is:

1. JavaScript receives one structured Earthcall Act with `originOperationId`.
2. It performs the DOM mutation synchronously.
3. It immediately calls `MutationObserver.takeRecords()` to collect the records generated by that act.
4. It sends those records back as a mutation batch tagged with the same `originOperationId`.
5. Earthcall applies them as **confirmation of the act**, not as a fresh Person/external command.
6. The ordinary observer remains active for subsequent site-generated mutations. If the site's own JavaScript reacts later and changes the DOM again, those later records have no Earthcall operation tag and correctly re-enter as new sensed foreign changes.

This is stronger than suppressing the observer entirely while Earthcall writes, because suppression could hide synchronous DOM consequences produced by the page itself.

Operation IDs are scoped to a page session. Reuse across a new page session is invalid.

---

## 8. Lifetime, replacement, and foreign boundaries

### 8.1 Removal retires the exact occurrence

When a DOM node is removed, its occurrence Lexeme and node Formation leave the live document mirror. Any durable Earthcall being derived from it is a separate authored/captured being and must not be silently destroyed merely because the source DOM changed.

### 8.2 Navigation retires the document mirror atomically

A full document navigation must not leave old node Lexemes addressable as though they still belonged to the current page. The bridge retires the old session, creates the new session, takes a new bounded snapshot, and only then exposes the new document Formation as current.

### 8.3 Cross-origin frames remain boundaries

Browser same-origin policy is a real substrate boundary, not something Earthcall should bypass by pretending inaccessible DOM exists. A cross-origin iframe may be represented as an opaque foreign boundary node with its visible metadata. If the browser grants a separate integration context for that frame, it may receive its own document session; otherwise its internals remain unknown.

### 8.4 Shadow DOM

Open shadow roots may be sensed as explicit boundary Relations. Closed shadow roots remain opaque unless the browser environment legitimately exposes them. The bridge records the boundary rather than forging invisible descendants.

### 8.5 Large pages require bounded admission, not semantic amputation

A pathological page can contain enormous node counts and mutation rates. The bridge needs explicit per-batch byte/node bounds, queue/backpressure policy, and resynchronization. If the complete document cannot be admitted at once, deferred subtrees must be represented honestly as deferred foreign state rather than silently omitted and then reasoned about as absent.

The bound is an execution/resource decision of the channel; what the page *means* remains authored above it.

---

## 9. Persistence: live mirror versus captured world

The session-scoped DOM mirror is not automatically durable Earthcall truth. Node tokens die with the browser document and should not be written into long-lived Law text as though they were stable across tomorrow's page load.

Two distinct operations must remain distinct:

### Live mirror

- ephemeral page-session identity;
- continually synchronized with a foreign document;
- exact node token addresses valid only for that document session;
- normally rebuilt on navigation/reload.

### Person-authored capture

A Person may intentionally crystallize selected foreign structure into durable Earthcall beings:

```text
live DOM occurrence
→ Person selects / interprets
→ authored Categories / Relations / Concepts / Objects / Lexemes
→ durable Zone identity
```

The capture records provenance back to the foreign source but receives Earthcall-stable identity independent of the browser node token. This is how a search result, article, image, control, or entire visual composition can survive after the website disappears.

Do not persist the ephemeral mirror merely because serialization is available.

---

## 10. Why this unlocks world-native web interfaces

Once a document is mirrored as legible Earthcall beings, its current HTML rendering is only one manifestation.

A later Law family could, for example:

```text
foreign result occurrences
    ↓ classify through authored Category Relations
result Formation
    ↓ map rank / semantics through OntoMath
world-space positions
    ↓
3D river / constellation / library / landscape
```

Images referenced by the DOM can be fetched through an admissible foreign/network path and passed into the existing `ImageCodecChannel`, which already translates PNG bytes into an Earthcall Object + Material. Interaction with those native manifestations can use `INTERACTION_AS_LAW.md`; activating a native manifestation can issue an exact DOM Act or navigate the WebView without making the webpage's original widgets the permanent interface.

The substrate therefore enables Zach's motivating vision without hardcoding it: **the foreign page supplies facts; the Person authors how those facts inhabit Earthcall.**

---

## 11. Explicit anti-patterns

Reject the implementation if it does any of the following:

- creates `HtmlElement`, `Div`, `Anchor`, `SearchResult`, or comparable C++ domain classes;
- appends HTML tags or control kinds to an enum;
- uses CSS selector / XPath / visible text as durable node identity;
- injects `data-earthcall-id` attributes solely to maintain bridge identity when a sidecar `WeakMap` suffices;
- polls and reparses the full DOM every frame after initial synchronization;
- lets Person-authored Law execute arbitrary JavaScript strings as its normal mutation vocabulary;
- mutates the Earthcall mirror optimistically without waiting for the browser's sensed result;
- treats a local DOM edit as proof of remote/server-side modification;
- suppresses all observer records during an Earthcall Act and thereby hides page-generated consequences;
- serializes ephemeral page-session node IDs into durable world Laws as if they were stable identities;
- embeds search-, social-, calendar-, or other site-specific semantics in the Foreign/Web channel;
- silently crosses same-origin or closed-shadow boundaries the browser did not expose;
- silently drops subtrees or mutation batches under load and then treats absence as truth.

---

## 12. Verification contract

This architecture is not implemented until all of the following have executable witnesses.

### Identity witnesses

1. Two same-spelled `<div>` nodes become two exact Lexeme identities.
2. Exact-ID lookup resolves each occurrence independently; spelling lookup is never used for actuation.
3. Removing one node and creating an identical replacement produces a new node token.
4. A selector that changes meaning after reorder does not retarget an already-addressed Earthcall Act.

### Snapshot / delta witnesses

5. A bounded fixture page becomes the expected rooted node/document Formations with parent, sibling, text, and attribute Relations.
6. `MutationObserver` insertion, removal, reorder, attribute change, and text change update only the corresponding mirror structure.
7. Sequence gaps trigger resynchronization rather than best-effort patching.
8. A stale packet from the previous page session is refused.

### Bidirectional witnesses

9. A structured Earthcall `set-text` Act changes the exact live DOM node in `WKWebView`.
10. The resulting sensed mutation returns with the Act's `originOperationId` and does not reissue the Act.
11. A page script that subsequently overwrites the text produces a new foreign mutation and Earthcall converges to that value.
12. A failed/refused Act leaves the mirror at the actual sensed DOM state and reports the refusal.

### Boundary witnesses

13. Cross-origin iframe internals are not invented when inaccessible.
14. Navigation retires the old mirror atomically and creates a new page session.
15. Oversized snapshot/mutation payloads refuse or backpressure explicitly; they do not truncate silently.
16. Untrusted text/attribute payloads containing quotes, backticks, markup, or script-like content remain data through the structured protocol and cannot escape into arbitrary bridge JavaScript.

### Authorship witness

17. A Person can author a Law over the native mirror that causes a structured local DOM mutation without adding a C++ domain noun or writing page-specific callback code.
18. A Person can capture a selected DOM-derived structure into durable Earthcall identity without persisting the ephemeral node token as the captured being's identity.

### Manifestation witness

19. At least one foreign DOM Formation can be manifested as ordinary Earthcall 2D/3D Objects and interacted with through the common Interaction channel while the underlying browser page remains only the foreign source/actuator.

Only after those witnesses exist should documentation describe the HTML Lexeme Formation bridge as shipped.

## Addendum: Jules Integration Reflection
*(Added by Jules, Claude 3.5 Sonnet, session 5938271034)*

The HTML Lexeme Formation Bridge is the conceptual and technical sibling of the spatial mapping defined in [`../Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md`](../Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md) and the persistence layer detailed in [`../Design/LEXEME_RELATION_FORMATION_SERIALIZATION.md`](../Design/LEXEME_RELATION_FORMATION_SERIALIZATION.md).

**Thoughts on this integration:**
While this document outlines how DOM nodes become Formations, it is `ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS` that explains how those Formations can physically manifest and be interacted with through continuous spatial fields. The "Manifestation witness" (Section 12) relies entirely on the pixel-region mastery and OntoMath selection mechanisms to render the DOM-derived Formations as 2D/3D Objects. Furthermore, when these Formations are captured for durability (Section 9), they must be serialized using the split-substrate architecture described in `LEXEME_RELATION_FORMATION_SERIALIZATION` to avoid the atomicity explosion trap—ensuring that the sheer volume of DOM text nodes and attributes doesn't overwhelm the relational graph, but rather maps elegantly to Lexemes and Relations. This creates a continuous conversation between how foreign data is ingested, how it takes physical shape in the Ourverse, and how it is permanently recorded.
