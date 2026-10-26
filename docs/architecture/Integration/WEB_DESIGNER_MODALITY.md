# Earthcall Web Designer Modality

## Reconciliation status — 2026-10-26

This document was reconciled against the Golden-Seal implementation and current default on 2026-10-26. Earlier round language that treated a First Mover, importer, initializer, or nearby Person pointer as authorship is superseded. **Authorship is Person-exclusive human stewardship; efficient causation and delegation are separate provenance.** The current reconciliation is tracked in [WEB_DESIGNER_RECONCILIATION_2026-10-26.md](../../audits/WEB_DESIGNER_RECONCILIATION_2026-10-26.md). Fresh current-head CI is required before this reconciliation is called verified.

## 1. Architectural Telos & Ontology

> **"C++ May Know How A Browser Works. C++ Must Not Decide What The Person Means."**

The Earthcall Web Designer Modality establishes website authoring as a genuine **Earthcall Law-authoring domain**, preserving Earthcall's Person-centered ontology, the Seven Refusals, and the minimum-maximum principle:

1. **Refusal 1 (No new C++ classes for domain nouns)**: There is no `class WebPage`, `class Button`, `class Heading`, `class Card`, or `class NavigationBar`. In-world beings are standard `Singular` (`Object`, `Formation`, `Relation`) entities living in the canonical world (`Universe::instance()`) and carrying authored properties.
2. **Refusal 2 (Subsystems placed in Singularity)**: The web modality channel lives under `Singularity/Foreign/Web/Designer/` as a channel to foreign software/DOM environments.
3. **Refusal 3 & Open Semantic Facets (Retiring WebConcept as source of truth)**: C++ enums do not define the universe of meanings a Person may author. `WebConcept` is demoted to an ephemeral compiled adapter view. Open strings (`roleName`, `variantName`, `flowName`, `alignName`, `justifyName`, `spacingName`, `scaleName`, `weightName`, `toneName`, `shapeName`, `elevationName`) allow Persons to introduce completely new semantic concepts (e.g. `sanctuary`, `ethereal`) entirely through authored data without modifying any C++ enum or table.
4. **Refusal 5 (Person means Human)**: Authorship is Person-exclusive human stewardship. Authored state carries first-class `Relation("authored-by", *being, *person, true, 1.0f)` provenance. A First Mover, importer, compiler, initializer, loader, or injector may create/instantiate/inject/compile/realize state but does not become its author by efficient causation. `syncRegister()` manufactures no semantic authorship; shipped vocabulary is ordinary authored Earthcall data and restores its recorded stable Person identity.
5. **Refusal 6 (No Black Box)**: Every property is accessible and governable via standard Earthcall `PropertyPath`. State is universalized above `web.*` (`role`, `level`, `layout.flow`, `surface.variant`, `prominence`, `subordinate`), with `web.*` strictly reserved for medium-specific browser parameters.
6. **Refusal 7 (No hardcoded semantic constitution in C++ methods)**: Authored meaning, composition, design tokens, and behavioral intent live in ordinary Earthcall state. C++ may implement irreducible browser mechanics, sanitization, and lowering, but must not decide what the Person means.
7. **Storage & Channel Invariants**: Realization and storage are strictly orthogonal. Direct raw filesystem I/O (`std::ofstream`, `create_directories`) is prohibited; all persistence routes through `VirtualFileSystem` (`memory://`) and `FileChannel` (`file.path`, `file.content`, `file.atomicWrite`).

HTML, CSS, and DOM are not Earthcall's ontology—they are foreign realization targets of an in-world semantic model.

---

## 2. Canonical Earthcall Language Precedent

The Web Designer Modality uses the canonical Earthcall language precedent rather than a private semantic dictionary. `syncRegister()` is mechanism-only; the shipped vocabulary is loaded as ordinary authored Laws, Lexemes, and Relations from Earthcall data. `compilePhrase()` scans character spans against those live Lexemes and their `denotes` Relations; unmatched or genuinely ambiguous meaning is not invented by C++:

```
Characters / Authored Input
       │  "a large centered heading", "a rounded primary button"
       ▼
Canonical Lexemes (Singularity::Language::Lexeme)
       │  First-class lexical tokens carrying spelling and semantic identity
       ▼
First-Class "denotes" Relations (Relation)
       │  Relation("denotes", *lexeme, *law, true, 1.0f)
       ▼
Person-Authored Laws & Metalaws (Law)
       │  ConditionNode & ActionNode defining semantic transformations:
       │  ActionNode::set("role", PropertyValue("heading"))
       │  ActionNode::set("scale", PropertyValue("large"))
       │  ActionNode::set("layout.align", PropertyValue("center"))
       ▼
In-World Singular State & Relational Topology
       │  Objects in Universe::instance() with universal dynamic properties
       │  Topological DAG:
       │    Relation("authored-by", *being, *author)
       │    Relation("contains", *parent, *child)
       │    Relation("precedes", *childA, *childB)
       ▼
Thin Web Realization (WebRealizer)
       │  Translates semantic in-world state into:
       │    - HTML5 semantic landmarks (<nav>, <section>, <h1>, <button>, custom components)
       │    - CSS custom property design tokens derived from authored root theme properties
       │    - Clean DOM client action dispatchers (zero intrusive toast widgets)
       ▼
Browser / WebView Realization Target
```

---

## 3. Universal Properties vs Medium-Specific State

To ensure authored semantic meaning survives realization into desktop UI, mobile UI, 2D/3D scenes, or print without being locked to the browser, properties are generalized above `web.*`:

### Universal Semantic Properties
| Property Path | Purpose | Example authored values / open extensibility |
|---|---|---|
| `role` | Semantic role of the element | `page`, `section`, `navigation`, `heading`, `paragraph`, `action`, `container`, `list`, `item`, `media`, `quote`, `divider`, plus **arbitrary open roles** (e.g. `sanctuary`) |
| `text` | Display text / content | Plain string |
| `level` | Hierarchy depth (e.g. heading level) | Integer (1 to 6) |
| `layout.flow` | Arrangement flow | `vertical`, `horizontal`, `grid` |
| `layout.align` | Cross-axis alignment / text align | `start`, `center`, `end`, `stretch` |
| `layout.justify` | Main-axis distribution | `start`, `center`, `end`, `space-between`, `space-around` |
| `layout.spacing` | Spacing / gap scale | `none`, `tight`, `normal`, `relaxed`, `generous` |
| `scale` | Content / typographic scale | `small`, `normal`, `large`, `huge` |
| `typography.weight` | Font weight | `light`, `normal`, `medium`, `bold` |
| `typography.tone` | Semantic color tone | `default`, `muted`, `prominent`, `accent` |
| `surface.variant` | Surface styling / material intent | `default`, `primary`, `secondary`, `surface`, `card`, `hero`, `subtle`, `outline`, `ghost`, plus **arbitrary open variants** (e.g. `ethereal`) |
| `surface.shape` | Shape / corner treatment | `sharp`, `rounded`, `pill`, `circle` |
| `surface.elevation` | Visual elevation / depth | `flat`, `subtle`, `raised`, `floating` |
| `subordinate` | De-emphasized or supplementary role | Boolean (`true`/`false`) |
| `prominence` | Visual dominance level | Integer (0 to 10) |

### Medium-Specific Browser Properties (`web.*`)
Only properties genuinely unique to the web runtime carry the `web.*` prefix:
- `web.href`: Target hyperlink URI.
- `web.action.event`: Custom EventBus event name dispatched when clicked.
- `web.media.src`: Media resource URL (image/video).
- `web.media.alt`: Alternative text for accessibility.
- `web.element.tag`: Explicit HTML tag override (when exact tag selection is required).

---

## 4. Hierarchy, Precedence & Provenance via Relational Topology

Websites in Earthcall do not rely on flattened synthetic arrays or arbitrary `web.order` floating-point numbers:
1. **Containment**: Parent-child relationship is established by `Relation("contains", *parent, *child, true, 1.0f)`.
2. **Sibling Precedence DAG**: Sibling ordering is established by `Relation("precedes", *first, *second, true, 1.0f)`. When realizing children of a parent, `WebRealizer::findChildren` constructs an in-memory DAG and executes **Kahn's topological sort**, guaranteeing that the DOM sequence faithfully realizes the authored relational topology.
3. **Authorship Provenance**: Authored elements carry `Relation("authored-by", *being, *author, true, 1.0f)` whose author endpoint is a real `Person`. First Movers and tools may carry separate creation/injection/delegation provenance; they are not substituted into `authored-by`.

---

## 5. Thin Web Realizer & Authored Theme Tokens

`WebRealizer` does not dictate design choices or color palettes:
- **Authored Theme Tokens**: The root/page element can carry authored theme properties (`theme.primaryColor`, `theme.backgroundColor`, `theme.textColor`, `theme.radius`, `theme.fontFamily`). `WebRealizer::generateStyleSheet` reads these properties from the root `Singular` and emits them into `:root` CSS custom properties (`--web-primary`, `--web-bg`, etc.).
- **No opinionated theme backfill**: Missing authored aesthetic tokens remain unset rather than receiving C++ design opinions. The realizer retains only foreign-medium mechanics and narrow accessibility/legibility behavior.
- **Substrate Cleanliness**: Substrate-level UI injections (such as hardcoded toast notifications) are strictly prohibited. Interactivity is signaled cleanly via semantic DOM event listeners that dispatch back to Earthcall's `EventBus`.

---

## 6. Governed Storage: VirtualFileSystem & FileChannel

Direct filesystem calls (`std::ofstream`, `std::filesystem::create_directories`) are completely absent from the Web Designer implementation.

Persistence is handled uniformly through Earthcall's storage abstractions:
- **In-Memory Storage**: Writing to `memory://...` URIs routes through `VirtualFileSystem::instance().writeMemoryFile()`.
- **Physical Filesystem Storage**: Writing to physical disk paths resolves via `VirtualFileSystem::resolve()` and executes through `Singularity::Storage::FileChannel`:
  ```cpp
  Singularity::Storage::FileChannel channel;
  channel.setProperty("file.path", resolvedPath);
  channel.setProperty("file.content", html);
  channel.setProperty("file.atomicWrite", true);
  ```
  This ensures sandboxing, atomic temp staging, and telemetry logging are uniformly respected across all exports.

---

## 7. Dynamic Reactivity: DOM Actions to Earthcall Laws

User interactions in the browser cross into Earthcall without black-box JavaScript:
1. Interactive elements carry `data-action-event="<eventName>"` and `data-being-id="<beingId>"`.
2. The lightweight browser bridge listens for DOM clicks and forwards them to `WebDesignerChannel::handleWebAction`.
3. `handleWebAction` publishes the event to `Core::EventBus`.
4. Any Person-authored reactive `Law` listening for the event fires and mutates the in-world `Singular` properties.
5. Upon re-realization, the mutated properties update the HTML/DOM output.

This loop holds only while the page is hosted inside Earthcall's own WebKit view (`window.webkit.messageHandlers.earthcall`). An exported page opened in an ordinary browser keeps its structure and styling but its actions only log; see §9.

---

## 8. Verification & Extensibility Exit Test

The reconciled focused witness is `tests/singularity/web_designer_modality_test.cpp`. Its current source invokes **55 test functions** covering the original modality path plus the later Sun/Sparkly review gates: Person-only authorship, stable DID resolution and duplicate-display-name refusal, canonical authored vocabulary loading, no Power-of-Attorney inference, graph-authoritative redefinition and ambiguity refusal, first-class supersession/currentness, single-sink Relation admission, raw-CSS fail-closed capability governance, theme-definition versus application provenance, Zone custody, Ghost-Door/Maintenance-Hatch lifetime refusal, and zero opinionated theme fallback defaults.

Evidence must be tied to the exact head that ran. The Oct-21 sealed branch reported 55/55 local focused witnesses; the Oct-26 reconciliation does **not** inherit that verdict automatically. Fresh CI/build evidence is recorded separately.

### Artifacts
- **Frontend Prototype**: `web_ui/designer_prototype.html` and `saves/web_designer/prototype.html`.
- **Authored vocabulary/theme fixtures**: `saves/fixtures/web_vocabulary_seed.json` and `saves/fixtures/web_themes.json`.
- **Reconciliation record**: [WEB_DESIGNER_RECONCILIATION_2026-10-26.md](../../audits/WEB_DESIGNER_RECONCILIATION_2026-10-26.md).

---

## 9. Realization Mode: Compiled Today, Live Runtime Later

*Added 2026-10-18 from Zach's direction (Claude Code / Claude Opus 5.5, session `c1339adf-4919-4ca2-bbde-3d0f0eb32a19`).*

Earthcall is a C++ application, and no Python or web reimplementation of the ontology exists yet. The Web Designer is therefore a **compiler**: Earthcall evaluates Lexemes and Laws at authoring time and emits HTML/CSS that runs on the web **without** Earthcall. People who visit a realized site are not running Earthcall and are not running Laws.

The path from compiler to live runtime has three stages, each useful on its own:

| Stage | Where Laws run | What the page can do |
|---|---|---|
| 1. Compiled static (today) | Earthcall, at authoring time | Authored structure, theme, and layout; behavior lives in whatever foreign app serves the page |
| 2. Compiled live | Earthcall at authoring time, plus a compiled subset in page script | Governed `web.data.source` binding to allowed endpoints; filters, sorts, and condition→set Laws realized as JS |
| 3. Web Law runtime (future) | An engine modeled on the ontology, in the browser or a sidecar | Persons author live by speaking to the page |

Invariants for stage 2:
- Compiled script is a **realization** of canonical `ConditionNode`/`ActionNode` Laws, never a second web-specific Law language or serialization.
- `web.data.source` is a network capability governed through authority (origin allowlist), on the same footing as the round-5 raw-CSS capability, not a caller-set bool.
- Realized behavior carries its Law's identity into the page so provenance survives realization.

Plan, first real use (TeacherOps "Waiting Board"), and open questions: [Compiled Live Pages task](../../Agenda/Tasks/Specific%20Tasks/Channels%20and%20Language/Web_Designer_Compiled_Live_Pages/Web_Designer_Compiled_Live_Pages.md).
