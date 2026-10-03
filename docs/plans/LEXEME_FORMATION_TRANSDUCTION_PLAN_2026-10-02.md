# Lexeme / Relation / Formation Transduction

**Status:** Rungs 0–2 + Rung 3a implemented on `sol/lexeme-formation-transduction-20261002`  
**Date:** 2026-10-02  
**Human direction:** Zachary Zhang  
**Implementation:** GPT-5.6 Sol

## Thesis

Earthcall should not own a special `natural-language -> HTML` compiler.

It should own one more general capability:

```text
source symbolic manifestation
        ↓
individuated Lexemes
        ↓
Relations / Formations / denotations
        ↓
authored Law and Metalaw resolve meaning
        ↓
semantic Formation
        ↓
authored manifestation graph
        ↓
target Lexemes / Relations / Formations
        ↓
target channel
```

HTML is the first strong witness because both ends already exist:

1. the Law Line proves that a Lexeme can derive executable meaning through
   `Lexeme --denotes--> Law`, with grammar position and Metalaw resolving
   ambiguity; and
2. the HTML bridge already projects a live DOM into individuated Lexeme
   occurrences, Relations, and Formations.

The missing seam is reusable graph-to-graph transduction.

## Invariant: manifestation is not identity

If Earthcall being `S` is shown as HTML occurrence `H`, then:

```text
S != H

S --manifests-as--> H
```

Destroying or replacing `H` must not silently destroy `S`. One source being
may have many simultaneous manifestations (HTML, 2D, 3D, speech, serialization),
and one target channel may choose among multiple authored representations.

This is why the implementation creates fresh target Lexemes and separate
correspondence Relations rather than mutating/retyping the source Formation.

## Rung 0 — detached graph transduction kernel [IMPLEMENTED]

Files:

```text
src/Singularity/Language/GraphTransduction.hpp
src/Singularity/Language/GraphTransduction.cpp
tests/singularity/graph_transduction_test.cpp
```

The kernel accepts an explicit, already-resolved `Plan` and produces a detached
target Formation.

It guarantees:

- spelling is not identity;
- duplicate target symbols remain separate occurrences;
- many source meanings may contribute to one target occurrence without losing
  provenance;
- the source Formation is never reused as the target;
- target Relation semantics are supplied by authored Relation-kind Lexemes when
  available, not by a new C++ enum;
- every source-derived target occurrence/Relation must have an explicit
  correspondence Relation kind, so provenance cannot silently disappear;
- the whole requested target is validated before publication;
- no `LanguageSystem`, `Universe`, Zone, browser, or channel registration occurs
  inside the mechanism;
- malformed plans publish nothing.

## Rung 1 — the mapping request is itself a Formation [IMPLEMENTED]

Rung 0's hand-built C++ `Plan` is now only the substrate IR.

`GraphTransduction::buildFromTemplate` consumes an ordinary Formation shaped
like this:

```text
semantic.page  --manifests-as--> Lexeme("section")
semantic.title --manifests-as--> Lexeme("h1")

Lexeme("h1") --dom-child-of--> Lexeme("section")
```

The right-hand Lexemes are **target prototypes**, not live DOM occurrences.
Their Relations state the desired target topology.

The transducer:

1. finds source -> prototype mappings by an authored Relation-kind Lexeme
   supplied by the caller;
2. discovers the target prototypes without interpreting their spelling;
3. clones each prototype into a fresh exact target occurrence identity;
4. clones Relations among prototypes, preserving their authored kind,
   direction, and weight;
5. records both source-being -> target-occurrence and source-Relation ->
   target-Relation provenance;
6. returns the same detached Rung-0 result.

There is deliberately no `TransductionRequest` C++ being and no HTML-specific
rule table.

The test constructs the requested semantic -> HTML graph entirely as ordinary
Lexemes, Relations, and one Formation, then produces:

```text
html.authored.formation
    section
      ├── h1
      │    └── "Welcome"
      ├── article A
      └── article B
```

The two `article` occurrences remain different beings.

## Rung 2 — authored Law produces/edits the template Formation [IMPLEMENTED]

No transduction-specific Law verb was added. Instead the existing universal
composition verbs were completed where they were artificially narrower than
the ontology:

- `AddElement` now admits/removes members on a `Formation` as well as an
  Object's element composition;
- `AddRelation`, when the Law acts on a Formation, admits the SAME Relation
  being into that Formation and into the active Zone/world graph;
- `AddRelation` now persists authored direction explicitly;
- relation type text of the form `@<lexeme-id>` means an exact, grounded
  Relation-kind Lexeme. Missing explicit kinds refuse rather than falling back
  to a string label.

The integration witness serializes a Law, restores it, applies it to an empty
request Formation, and has the restored authored Law construct:

```text
semantic.page    --[manifests-as]--> prototype "section"
semantic.heading --[manifests-as]--> prototype "h1"
semantic.card A  --[manifests-as]--> prototype "article"
semantic.card B  --[manifests-as]--> prototype "article"

prototype "h1"      --[dom-child-of]--> prototype "section"
prototype "article" --[dom-child-of]--> prototype "section"
```

Both relation kinds are exact Lexeme-grounded beings and every edge is
directed because the Law authored it that way. `buildFromTemplate` then
transduces that Law-authored graph without knowing any semantic mapping in C++.

This proves the requested manifestation itself can be persisted Person-authored
Earthcall state rather than fixture data.

## Rung 3 — layered Lexeme semantic graph for natural language

### Rung 3a — lexical occurrence + denotation layers [IMPLEMENTED]

The Law Line no longer discards semantic provenance after parsing a spelling.

`Span` now carries the exact chosen `opcode`, `lexemeId`, and `lawId`.
`Parse` retains every structured `Ambiguity` with its byte interval and full
`Word` candidates, whether a Metalaw resolves it or leaves it open.

`LawSentenceGraph::project` exposes this as ordinary Earthcall graph state:

```text
utterance.lexical
    occurrence.0 --lexical-next--> occurrence.1 --> ...

utterance.denotation
    occurrence.0 --occurrence-denotes--> exact authored Lexeme
    occurrence.0 --candidate-denotation--> candidate Lexeme A
    occurrence.0 --candidate-denotation--> candidate Lexeme B
```

Each occurrence is a fresh Lexeme whose exact identity comes from the utterance
identity + occurrence index, never its spelling. It carries inspectable
`language.start/end/role/opcode/lexemeId/lawId` properties.

Open ambiguity is not failure of representation. An unresolved spelling still
becomes an occurrence whose `language.candidates` PropertyList names every
candidate and whose candidate-denotation Relations preserve every live authored
Lexeme possibility. When Metalaw chooses one, the candidate edges remain and an
additional exact chosen-denotation edge records the narrowed reading.

The projection does not reinterpret characters or perform a second parse. It is
a graph manifestation of what the Terminal modality already read.

### Rung 3b — semantic relation / intended-being layers [NEXT]

Generalize the Law Line's current sentence-specific composition into explicit
higher graph layers:

```text
characters
  -> lexical occurrence Formation
  -> denotation graph
  -> semantic Relation graph
  -> intended-being Formation
```

Requirements:

- repeated words/phrases are distinct occurrence Lexemes;
- an occurrence may denote more than one meaning;
- grammar/Formation context removes impossible meanings first;
- remaining ambiguity is exposed to Metalaw rather than guessed;
- phrases, prefixes, suffixes, and symbolic runs remain valid Lexemes;
- semantic composition must preserve provenance back to every occurrence that
  contributed to a composed meaning.

The existing Law Line remains a valid Terminal modality; it should become one
consumer/witness of the more general semantic graph rather than being deleted
in favor of an opaque parser.

## Rung 4 — semantic Formation -> HTML Formation by authored Law

Use the same native vocabulary as the existing DOM mirror:

```text
dom-child-of
dom-next-sibling
dom-has-attribute
dom-has-value
```

A manifestation Law may decide, for example, that one semantic heading becomes
an `h1`, while another world chooses `header` or
`div role="heading"`.

No mapping such as `Heading == h1`, `Formation == section`, or
`Relation == DOM nesting` belongs in C++.

The output is a target Formation of exact HTML occurrence Lexemes, still
detached from the browser.

## Rung 5 — target HTML Formation -> structured DOM Act

Connect the generated HTML Formation to the existing Foreign/Web Act protocol.

The actuator should diff desired target Formation against the current
`DomMirrorTranslator` graph and emit bounded structured operations:

```text
setText
setAttribute
removeAttribute
insertNode
removeNode
moveNode
```

Never make arbitrary JavaScript text the ordinary Law-facing operation.

The browser remains truth: Act -> browser -> MutationObserver -> sensed graph.

## Rung 6 — round-trip convergence

Prove:

```text
semantic source
   -> HTML Formation
   -> DOM Act
   -> browser DOM
   -> DomMirrorTranslator
   -> sensed HTML Formation
```

and verify identity/provenance correspondence rather than mere string equality.

The emitted target and sensed target may differ if the browser normalizes or page
JavaScript reacts. Earthcall must converge to sensed truth without overwriting the
semantic source.

## Rung 7 — language-to-language theorem witness

Once the same machinery handles both semantic composition and manifestation,
demonstrate two translations that do not mention HTML in their core algorithm:

```text
natural language -> semantic Formation -> HTML Formation
Earthcall Formation -> semantic Formation -> HTML Formation
```

Then demonstrate another target (e.g. a 2D authored UI Formation) using the same
source semantic graph.

At that point HTML is evidence of the architecture, not its special case.

## Refusals

This work must refuse these shortcuts:

1. No `HtmlElement`, `Heading`, `Card`, `SearchResult`, or other domain C++
   kind merely to make translation convenient.
2. No source being is silently retyped into a manifestation.
3. No target occurrence is recovered by spelling when exact identity exists.
4. No hidden parser chooses between still-valid meanings; Metalaw gets the open
   ambiguity.
5. No C++ table permanently owns semantic mappings that Persons should author.
6. No target graph is partially published on a failed transduction.
7. No HTML string serializer becomes the semantic source of truth.
8. No browser Act is assumed successful until the resulting DOM is sensed back.

## Immediate next implementation

Build Rung 3b: project the composed ConditionNode / ActionNode / clause
relationships into a semantic Relation Formation, then derive an intended-being
Formation that can feed the same GraphTransduction machinery as Rungs 0–2.

After that, wire TerminalChannel to publish these layers for each spoken
utterance instead of keeping them only as an explicit projection API.
