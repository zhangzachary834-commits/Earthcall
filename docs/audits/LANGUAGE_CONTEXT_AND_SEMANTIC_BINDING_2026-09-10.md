# The Language Branch: Preserve What a Person Meant

**Author:** OpenCode (GPT-6 Astra; `openai/gpt-6-astra`)  
**Session:** `language-depth-20260910-115436` (local research label; harness UUID unavailable)  
**Date / evidence timestamp:** 2026-09-10, 12:00:26 PDT  
**Inspected revision:** `115b8e8b`, with pre-existing working edits to `src/terminal_entry.cpp`  
**Status:** research complete; recommendations proposed, not implemented.  
**Task:** [Contextual Language and Semantic Binding](../Agenda/Tasks/Specific%20Tasks/Contextual_Language_and_Semantic_Binding/Contextual_Language_and_Semantic_Binding.md)

## 1. Why this branch

Zach requested a deep dive into **one** branch and large-scale suggestions that do not repeat other models' proposals. This pass follows `Singularity/Language` and its immediate Lexeme/Relation boundaries.

**Finding:** the largest language opportunity is preserving distinctions between an expression, a particular use of it, its possible interpretations, and the effect a Person gives that use. Current ingestion collapses several of these distinctions before Law can govern them.

The result is a substantial opening: a Person could inspect and change how words bind to their world, compose vocabularies between Zones, and edit structured intentions through language while retaining the exact relationships underneath.

### Human origination

- [Zach's manifesto](../core/Earthcall%20Ourverse%20Manifesto/EarthcallOurverse.md), lines 668–708: explicit Singular–Relation–Formation structure; a steerable symbolic ecosystem; words participating in the same Formations as other beings; models proposing structured Law candidates under human authorship.
- [Zach's response to Fable](../core/Earthcall%20Ourverse%20Manifesto/Discussion%20on%20Earthcall.md), line 43: meanings belong to Formations of stakeholders; a frozen dictionary can itself become Babel; language must remain amendable.
- [Zach's Terminal vision](../Zones%20of%20Actualization/Earthcall%20Terminal.md): terminal language is Formations of Lexemes and symbol Properties, bootstrapped by command roots.

Those ends and the stakeholder-owned-meaning principle are Zach's. The context envelope, scoped binding editions, packed interpretation representation, and partial bidirectional editing contracts developed below are this session's engineering extensions. Their component techniques have established intellectual origins (§8).

## 2. Prior-proposal check

The following were checked before choosing recommendations:

| Prior work | Already present in the conversation or tree | Extension in this pass |
|---|---|---|
| [Logos plan](../plans/logos_architecture_plan.md) | Lexemes as beings, WebSocket ingress, multi-medium manifestation, AI-generated Laws, multiplayer graph updates | Preserve context and distinguish alternative readings before graph effects |
| [Semantic network vision](../architecture/migration/SEMANTIC_NETWORK_VISION.md) and [audit](semantic_network_audit_report.md) | Decay, reinforcement, transitive inference, semantic-to-physics Laws | Represent the scope and force of a statement independently of an aggregate edge weight |
| [Fable's feedback and Zach's response](../core/Earthcall%20Ourverse%20Manifesto/Discussion%20on%20Earthcall.md) | Meaning-as-use, provenance, performatives, mutable stakeholder-governed meanings, language as a self-hosting hinge | Concrete scoped linking, explicit binding editions, and controlled-language round-trip contracts |
| [Existing Relation work](../Agenda/Tasks/Specific%20Tasks/PARTIAL_2026_08_23/PARTIAL_2026_08_23.md), `Utterance.hpp`, `relation_retry_lexeme_test` | Utterance/type distinction, `occurrence-of`, Lexeme-typed Relations | Repeated-position identity, live-ingress wiring, quotation/claim structure, and type identity through deduplication |
| [Sufficiency thesis](../Reflections%20on%20Earthcall's%20Progression/Reflections%20on%20the%20Substrate/The_Sufficiency_Thesis.md) | Model a conversation or another non-spatial domain | A specific language contract and falsifiable exercise, rather than another recommendation to model conversation |
| [Second-Nature specification](../plans/SECOND_NATURE_LAW_AND_ZONE_FEATURES_SPECIFICATION.md) | Intent Lexemes selecting Law templates | A shared, editable text/graph projection with identity-preserving updates |
| Recent analyses, Agenda, and relevant intercom searches | Rete optimization, persistence repair, provenance, semantic-decay incident | These remain dependencies and context; they are not presented as fresh recommendations |

The novelty claim is relative to this inspected repository corpus, not every private conversation held with another model. In particular, **introducing Utterance, stakeholder dictionaries, natural-language Law synthesis, or multi-medium text is not claimed as new**.

## 3. What the live path actually does

### Ingress and consumption

1. [`WebSocketServer.cpp:309–326`](../../src/Singularity/Network/WebSocketServer.cpp#L309) publishes `Core::Event::Utterance` with payload, source-client text, and target identifier; the source string is transport metadata, not proof of Person identity.
2. [`EventBus.hpp:198–202`](../../src/Singularity/Core/EventBus.hpp#L198) carries those three strings. It carries no Zone or Moment.
3. [`LanguageSystem.cpp:202–207`](../../src/Singularity/Language/LanguageSystem.cpp#L202) queues them, silently returning above 1,000 queued inputs or 1,024 payload bytes.
4. [`LanguageSystem.cpp:128–195`](../../src/Singularity/Language/LanguageSystem.cpp#L128) consumes the queue using **the active Zone at consumption**. It does not read `u.sourceClient`.
5. Successfully parsed input admits/reinforces Relations. Only the unparsed fallback considers `targetSingularId` and creates a `speaks` Relation.

The app calls this path from [`Engine.cpp:333–366`](../../src/Singularity/Core/Engine.cpp#L333), after `update` and before `LawManager::tick`. The working terminal's `utter`/`speak` branch also publishes that event and calls `LanguageSystem::tick` (`terminal_entry.cpp:798–819`). That terminal file is concurrent work, so this is a source observation rather than an execution claim about its current UI.

### Lexical identity and parsing

- [`LanguageSystem.cpp:40–95`](../../src/Singularity/Language/LanguageSystem.cpp#L40): stable IDs can distinguish equal-spelled Lexemes, but `_symbolIndex[symbol]` holds one pointer and `intern` replaces it. `resolve` has no context argument. This is **not** a claim that all equal spellings are necessarily one stored being.
- [`Lexeme.cpp:70–74`](../../src/ConstructedBeing/Singular/Lexeme/Lexeme.cpp#L70): `symbol` is writable through a registered Property; the LanguageSystem index has no matching rename update.
- [`SyntacticParser.cpp:11–32`](../../src/Singularity/Language/SyntacticParser.cpp#L11): tokenization discards byte-classified punctuation and lowercases bytes. No source-span alignment survives this function.
- [`SyntacticParser.cpp:35–114`](../../src/Singularity/Language/SyntacticParser.cpp#L35): the first `is_pos` and `resolves_to` matches decide; the control flow knows English-like noun/verb/preposition/determiner categories and subject-first order. The final Relation is constructed from a **string** canonical type.
- [`Relation.hpp:166–170`](../../src/Relation/Relation.hpp#L166) and [`RelationManager.cpp:119–144`](../../src/Relation/RelationManager.cpp#L119): Relation identifier and equivalence use the type spelling, even when `_typeLexeme` is populated.

This is sufficient machinery for the tested seed sentence. It does not yet implement a language-independent interpretation contract.

### Occurrence, storage, and creation boundaries

- [`Utterance.hpp`](../../src/Singularity/Language/Utterance.hpp) already distinguishes a tokening from a Lexeme type. Its only instantiation found outside its implementation is the focused test; the production LanguageSystem does not use it.
- [`Utterance.cpp:26–40`](../../src/Singularity/Language/Utterance.cpp#L26) creates one `occurrence-of` edge per vector entry. Two mentions of the same Lexeme produce the same endpoint/type identity; the edges do not distinguish positions.
- [`ZoneSerialization.cpp:182–192`](../../src/Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.cpp#L182) writes Lexemes as `id` and `symbol`. [`FormationSerialization.cpp:19–29`](../../src/Singularity/Storage/Serialization/Relation/FormationSerialization.cpp#L19) restores those fields. The existing persistence-root work is a prerequisite for retaining richer linguistic authorship and annotations.
- [`Zone.cpp:265–284`](../../src/ZonesOfEarth/Zone/Zone.cpp#L265) preserves Lexemes in its member sweep but removes non-object/non-Lexeme extras unless separately supplied. Merely constructing the existing Utterance class would not make it a durable live participant.
- [`ObjectConcept.cpp:463–464`](../../src/ConstructedBeing/Singular/Object/Creation/ObjectConcept.cpp#L463) refuses generic Lexeme creation: its birth belongs to Language. A complete authored language pipeline therefore cannot honestly be described as ready to assemble entirely through today's generic Create path.

Current Law condition/action trees already describe execution without natural-language lookup. The proposed semantic linker below concerns richer linguistic bindings and future language-to-Law authoring; it is not a claim that editing a word currently recompiles every Law.

## 4. Executed evidence

Built `logos_modality_test` and `relation_retry_lexeme_test` against the configured core. Both passed when run in an isolated temporary working directory.

The scratch executable [language_meaning_probe.cpp](../../scratch/probes/language_meaning_probe.cpp), linked against that same core using [run_language_meaning_probe.py](../../scratch/probes/run_language_meaning_probe.py), reproduced **10 current-behavior observations**:

| # | Observation reproduced |
|---|---|
| 1 | Two stable `bank` identities coexist; bare lookup selects the last interned one |
| 2 | A registered `symbol` write to `lender` leaves the old `bank` key and no `lender` key |
| 3 | Different type Lexemes both spelling `owns` merge into one Relation for the same endpoints |
| 4 | `Arthur owns sword` and `"Arthur owns sword"` produce identical parsed Relation identities |
| 5 | The parser's returned Relation has no type-Lexeme pointer |
| 6 | The same sentence from two source strings becomes one edge, weight `0.5 → 0.7` |
| 7 | Successfully parsed targeted input produces neither `speaks` nor `occurrence-of` |
| 8 | Input queued in A enters B when the Zone switches before consumption |
| 9 | Zone Lexeme JSON omits a nondefault conceptual weight and an authored annotation |
| 10 | The detached occurrence helper gives repeated references identical Relation IDs |

These are observation assertions, not a suite declaring the behavior desirable. They should change when the contract changes. The existing tests establish useful component behavior; they do not test these distinctions.

Reproduction, after the documented project configuration:

```sh
cmake --build build --target logos_modality_test relation_retry_lexeme_test -j4
python3 scratch/probes/run_language_meaning_probe.py --scratch-root /path/to/existing/temp/root
```

This run used the harness-approved temporary root. The runner reads the Makefile generator's actual compilation database and linker recipe, compiles outside `tests/`, and runs all three executables in a disposable directory. No real saved world is loaded. The JSON observation calls the codec in memory; it is not a cold-load round-trip claim. No GPU, live WebSocket session, full boot, or human interaction was tested.

**Fixture attribution:** the executable creates synthetic `probe.*` Lexemes, Relations, two Zones, and one detached occurrence solely in process memory. They have no recorded Person authors and are not injected into a save; the diagnostic itself is conducted at Zach's request. No source-client label in the fixture represents a real Person.

## 5. Four large-scale recommendations

### A. Context-preserving language acts

**Capability:** a Person can quote, question, suppose, correct, or assert something while the world preserves which of those they did.

An utterance should enter as a situated occurrence with links to its source, intended participants/target, Zone context, source text, and Moment. Capture the relevant context at the input boundary; a later active-Zone change cannot substitute for it. Keep transport-client identity separate from the actual Person/First Mover source.

Represent a candidate proposition as authored structure, for example an extra-visual Object with Relations to its subject, predicate Lexeme, object, and source occurrence. Relations such as “quoted within,” “asserted by,” or “proposed interpretation of” are authored vocabulary. Their names are illustrative, not new fixed enums or C++ fields.

Crucially, representing the candidate `Arthur owns sword` need not add the operational `owns(Arthur,sword)` edge. An authored Law can give an identified occurrence or claim operational force. Another can leave it as a quotation, testimony, a question, or a hypothetical. Competing claims can coexist without inventing a global machine verdict about truth.

This also distinguishes the first and second occurrence of a repeated word. A span/occurrence has its own identity; its link to a Lexeme type may be shared. Counting evidence, trusting a source, remembering repetition, and accepting a claim then become separate authored operations rather than the same `+0.2` weight write.

**Why it is large:** language gains the structure required for discussion, annotation, planning, narrative, and commands within one substrate. Persons can talk *about* an action without accidentally representing the talk as that action's ordinary world relation.

**First discriminating exercise:** the same three-word proposition, asserted and quoted, has shared referenced beings but distinct occurrence/context structure; only the occurrence selected by an authored interpretation Law produces the operational edge. A Zone switch and a second source cannot rewrite that attribution.

The existing Utterance work anticipated part of this. The new contribution is the live context/force contract and repeated-span identity, not another Utterance class. Its eventual representation must pass Refusal 1; the mere presence of today's class is not permission to add further domain classes.

### B. A semantic linker with scoped, edition-specific bindings

**Capability:** two communities can use the same spelling differently, share selected meanings explicitly, and evolve definitions without accidental rebinding.

A spelling is an index key that can return **candidate Lexemes**, not the final identity of a meaning. Resolve candidates through an authored vocabulary Formation, its explicit imports and mappings, and the context of the occurrence. Scope precedence and conflict handling come from authored structure; “nearest,” “first,” and “last loaded” are not substitute policies.

Bind the selected interpretation to stable identities and an identified edition of the definitions it used. Carry that binding into the resulting graph or Law. An author can choose a pinned binding, an explicitly live binding, or an authored migration when a vocabulary changes. These are authored policies, not a universal frozen dictionary.

For example, an installation and a drawing app may both use “open.” Importing the drawing app must not retarget the installation's previously bound command. An authored adapter can deliberately map or partially translate the meanings, naming the contexts in which the mapping holds. This extends Zach's stakeholder-owned language into a workable composition mechanism.

The current same-spelling type collapse is the first mechanical obstacle: identity must survive constructor → Relation ID/equivalence → matching → serialization/hydration. Switching only `_symbolIndex` to a multimap would leave the measured Relation collapse intact. Existing string-tag Laws and saves need an explicit compatibility mapping; identity migrations touch real human data.

**Why it is large:** Zones can exchange vocabularies and language-authored behaviors as composable artifacts. A vocabulary update can show which bindings it affects, and Persons can share meaning intentionally rather than merely share labels.

**First discriminating exercise:** load two equal-spelled definitions in opposite orders; scoped resolution and already-bound effects remain identical. Change a definition and show the actual consumers whose bindings would change. A content hash can detect a changed definition, but cannot prove semantic equivalence.

This uses ideas from lexical scoping, explicit module imports, hygienic binding, and dependency tracking. The specific Earthcall extension is a binding as authored relational structure whose evolution follows its stakeholder Formation.

### C. Packed, inspectable interpretations instead of a single eager parse

**Capability:** a Person can inspect what Earthcall is unsure about and correct the precise referent, attachment, or grammatical reading.

Preserve the raw source and revision-specific spans, then produce candidate structures from authored grammar and lexicon data. A bounded generalized chart parser with a **shared packed parse forest** is a strong baseline for controlled grammars that admit ambiguity: alternatives share their common structure rather than duplicate whole graphs. Earley/SPPF has a mature reference implementation in Lark (§8); no new NLP algorithm needs inventing.

For “move that beside the bank,” the candidates may differ in which selected being “that” names, which “bank” is meant, or what “beside” binds. The input occurrence can refer to the pointer/selection context captured when it was authored. An authored Law ranks or selects the candidate under the relevant vocabulary; a model can propose candidates with its own provenance.

Two boundaries matter:

1. A probability score is not semantic authority. Hypotheses remain hypotheses until an authored process uses them.
2. A bounded or model-generated candidate set is not exhaustive merely because only one item remains. Store whether exploration completed, which grammar it covered, and whether a budget truncated it.

Shared packed structure avoids explicit exponential tree enumeration; it does not make unrestricted language cheap. General Earley parsing has cubic worst-case input-length cost, and arbitrary semantic grounding adds another search problem. Candidate count, source length, chart work, and scheduling budgets must be law-visible and Person-authorable. Use across-tick work where appropriate. Cyclic/empty grammar derivations need an explicit bounded handling contract compatible with Formation constraints.

Grammar productions, lexical mappings, ranking, and commit behavior should be authored data/Laws. A foreign parser used as a First Mover prototype sits behind the existing Language/Foreign channel boundary. Its result must be projected into legible structures, and a later native migration must follow the existing six rungs rather than re-carve an English grammar into C++.

Use Unicode-aware segmentation with versioned source alignment. UAX #29 supplies a standards baseline; it explicitly requires tailoring for many writing systems. It is not a universal natural-language tokenizer. Keep original spelling/punctuation intact even when a normalized search view is useful.

**Why it is large:** grammar and interpretation become a medium Persons can author, teach, inspect, and combine. Earthcall can support controlled command dialects, multilingual mappings, structured debate, and domain notation without a new hardcoded parser for each domain.

**First discriminating exercise:** retain two readings of a small ambiguous command, select one by authored context, and prove that exhausting a search budget reports an incomplete result rather than a falsely unique interpretation.

### D. Bidirectional semantic editing

**Capability:** the sentence and the graph are two editable views of the same bound intention.

Begin with a deliberately constrained authored language. For example:

> Move this lantern beside that bench over three seconds.

Selecting “this lantern” highlights its exact bound being; selecting “three seconds” reveals the duration expression it names. Editing the interval through the graph updates that phrase. Editing only that phrase preserves the existing object identities, authorship links, and unaffected expression nodes.

The relevant established technique is the **bidirectional lens**: updating a view uses both the edited view and the original source, retaining information the view does not display. The original source and alignment are essential; a new LLM paraphrase is not an identity-preserving update protocol.

For a supported projection `view` and update operation `put`, require contracts such as:

- `put(graph, view(graph))` leaves the graph's semantic identities unchanged.
- A supported text edit appears in the next view after the graph update.
- Untouched graph structure retains identity and provenance.
- An unsupported or ambiguous edit becomes an explicit unresolved interpretation.

Natural language and graphs are many-to-many, so these are **partial, grammar-specific lenses**, not a claim of universal perfect translation. Keep a Person's historical wording distinct from a generated current gloss. A later correction can reference an earlier occurrence without silently rewriting what the Person originally said.

The authoring patterns themselves are Laws/Formations under `INTERACTION_AS_LAW`; operating-system text composition and glyph output are modality mechanisms. Existing IME/text-entry and Law-synthesis tasks are dependencies, not new recommendations in this pass.

**Why it is large:** the same foundation yields an executable notebook, inspectable command surface, semantic annotation tool, and controlled-language Law authoring surface. The Person can move between words and exact structure without repeatedly translating everything by hand.

**First discriminating exercise:** edit one numerical phrase through text, then through the graph; all unrelated node IDs and bindings remain stable. A quoted historical occurrence remains word-for-word intact while a newly generated gloss reflects the new graph.

## 6. A build order with one coherent demonstration

1. **Preserve context and identity at ingestion.** Establish a durable occurrence/target/source contract and scoped lexical lookup; expose refusal and truncation outcomes. Reconcile equal-spelled type identity across consumers before adding more language breadth.
2. **Author one small interpretation dialect.** Exercise statements, quotations, and an ambiguous referent using candidate graph structure and an authored choice of operational force. Use first-mover fixtures while the existing general creation gaps are being addressed.
3. **Give that dialect two scoped vocabulary editions.** Import both, change load order, change one edition, and demonstrate intentional binding/migration.
4. **Add the small bidirectional surface.** Let a Person alter a bound target or interval through either words or graph. Expand grammar coverage only after the source/identity contracts survive.

The demonstration: two contexts interpret “open” differently; a quotation of the command remains a quotation; pointing identifies which being the Person meant; revising one community's vocabulary identifies affected bindings; changing one supported phrase updates one intention without rebuilding the rest.

This is an acceptance target, not an implemented Zone. No demo save was authored in this pass.

## 7. Scale, dependencies, and decisions

**Scale should be measured against distinct dimensions:** number of Lexemes, number of source occurrences, number of genuinely ambiguous readings, number of vocabulary imports, and number of edited bindings. Report memory, chart expansion, candidate counts, and edit/lookup latency independently. There are no speedup measurements in this report.

Keep dense text in source buffers with addressable spans; do not require one heap-allocated Singular for every byte. Meaningful occurrences can be authored beings, with shared Lexeme types and shared derived parse structure. `NO_BLACK_BOX.md` §3a's distinction between dense substrate and selective Property elevation is a useful precedent, subject to a language-specific contract.

The present 1,000-Lexeme eviction in `resolve` is not a language-residency design: it removes a being from Zones, and `intern` does not use that cap. Larger vocabularies need resource policy that distinguishes derived indexes from authored identity. This is a source finding, not a measured million-word scaling result or a request simply to raise the constant.

Dependencies already tracked elsewhere include universal Singular/Law creation, relation revision/change-feed coverage, complete per-Zone persistence roots, native text entry, and Second-Person standing. This program must compose with them rather than build another store, permission system, or inference engine beside them.

The authorial questions are specific: what authored contexts give an occurrence operational force; how overlapping vocabulary imports are resolved; which consumers follow an evolving definition; and how lexical resources are retained. Zach's framework assigns those choices to Persons and their stakeholder Formations. The recommendations describe mechanisms that make those choices possible.

## 8. Technical lineage and evidence boundary

External references consulted 2026-09-10:

- [Unicode UAX #29, revision 47](https://www.unicode.org/reports/tr29/tr29-47.html): grapheme/word/sentence segmentation, source-boundary concerns, and necessary locale tailoring.
- [Lark: Parsers](https://lark-parser.readthedocs.io/en/latest/parsers.html) and [Working with the SPPF](https://lark-parser.readthedocs.io/en/latest/forest.html): generalized Earley parsing, packed ambiguity, complexity, and explicit forest traversal. These are references for a prototype, not installed Earthcall dependencies.
- [Boomerang / Harmony](https://www.seas.upenn.edu/~harmony/): Foster, Pierce, Greenberg, and collaborators' work on well-behaved bidirectional transformations, including original-source-preserving view updates.

The executed witnesses in §4 establish current local behavior only. The proposed parser, linking semantics, and editing laws need their own differential and end-to-end acceptance work. Broader semantic understanding, complete natural-language coverage, and a satisfying human experience are not established by these probes.

**Integrity check:** production code was not altered by the research. The build companion now records the narrower claims made by the two existing Language-related tests. The Agenda task indexes all four proposed workstreams and the diagnostic defects, so this report does not silently declare an implementation finished.
