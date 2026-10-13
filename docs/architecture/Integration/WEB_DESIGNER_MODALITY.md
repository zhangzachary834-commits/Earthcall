# Earthcall Web Designer Modality

## 1. Architectural Telos & Ontology

> **"C++ May Know How A Browser Works. C++ Must Not Decide What The Person Means."**

The Earthcall Web Designer Modality establishes website authoring as a genuine **Earthcall Law-authoring domain**, preserving Earthcall's Person-centered ontology, the Seven Refusals, and the minimum-maximum principle:

1. **Refusal 1 (No new C++ classes for domain nouns)**: There is no `class WebPage`, `class Button`, `class Heading`, `class Card`, or `class NavigationBar`. In-world beings are standard `Singular` (`Object`, `Formation`, `Relation`) entities living in the canonical world (`Universe::instance()`) and carrying authored properties.
2. **Refusal 2 (Subsystems placed in Singularity)**: The web modality channel lives under `Singularity/Foreign/Web/Designer/` as a channel to foreign software/DOM environments.
3. **Refusal 3 & Open Semantic Facets (Retiring WebConcept as source of truth)**: C++ enums do not define the universe of meanings a Person may author. `WebConcept` is demoted to an ephemeral compiled adapter view. Open strings (`roleName`, `variantName`, `flowName`, `alignName`, `justifyName`, `spacingName`, `scaleName`, `weightName`, `toneName`, `shapeName`, `elevationName`) allow Persons to introduce completely new semantic concepts (e.g. `sanctuary`, `ethereal`) entirely through authored data without modifying any C++ enum or table.
4. **Refusal 5 (Person means Human)**: Persons author elements; elements carry first-class `Relation("authored-by", *being, *person, true, 1.0f)` provenance edges. Modality default seeds are authored by a designated First Mover authority (`s_seededAuthor`).
5. **Refusal 6 (No Black Box)**: Every property is accessible and governable via standard Earthcall `PropertyPath`. State is universalized above `web.*` (`role`, `level`, `layout.flow`, `surface.variant`, `prominence`, `subordinate`), with `web.*` strictly reserved for medium-specific browser parameters.
6. **Refusal 7 (No hardcoded behavior in C++ methods)**: Behavior, presentation, and composition emerge entirely from Person-authored Laws and Metalaws reacting to events and mutating in-world state.
7. **Storage & Channel Invariants**: Realization and storage are strictly orthogonal. Direct raw filesystem I/O (`std::ofstream`, `create_directories`) is prohibited; all persistence routes through `VirtualFileSystem` (`memory://`) and `FileChannel` (`file.path`, `file.content`, `file.atomicWrite`).

HTML, CSS, and DOM are not Earthcall's ontology—they are foreign realization targets of an in-world semantic model.

---

## 2. Canonical Earthcall Language Precedent

The Web Designer Modality uses the canonical Earthcall language precedent rather than a private tokenizer switch or static opcode dictionary:

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
       │    Relation("web-contains", *parent, *child)
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
| Property Path | Purpose | Canonical Values / Open Extensibility |
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
1. **Containment**: Parent-child relationship is established by `Relation("web-contains", *parent, *child, true, 1.0f)`.
2. **Sibling Precedence DAG**: Sibling ordering is established by `Relation("precedes", *first, *second, true, 1.0f)`. When realizing children of a parent, `WebRealizer::findChildren` constructs an in-memory DAG and executes **Kahn's topological sort**, guaranteeing that the DOM sequence faithfully realizes the authored relational topology.
3. **Authorship Provenance**: Elements carry `Relation("authored-by", *being, *author, true, 1.0f)` connecting every element to the Person or First Mover who authored it.

---

## 5. Thin Web Realizer & Authored Theme Tokens

`WebRealizer` does not dictate design choices or color palettes:
- **Authored Theme Tokens**: The root/page element can carry authored theme properties (`theme.primaryColor`, `theme.backgroundColor`, `theme.textColor`, `theme.radius`, `theme.fontFamily`). `WebRealizer::generateStyleSheet` reads these properties from the root `Singular` and emits them into `:root` CSS custom properties (`--web-primary`, `--web-bg`, etc.).
- **Minimal Browser Fallbacks**: If no theme properties are authored, clean neutral default tokens provide fallbacks.
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

---

## 8. Verification & Extensibility Exit Test

The modality is validated by 8 comprehensive automated tests in `tests/singularity/web_designer_modality_test.cpp`:
1. **Lexeme & Relation In-World Resolution**: Validates that natural language tokens resolve through first-class `Singularity::Language::Lexeme` instances connected to `Law` objects via `"denotes"` `Relation` edges.
2. **Law & Lexeme Universal Composition**: Validates that orthogonal semantic properties (`role`, `scale`, `layout.align`, `surface.variant`, `prominence`) compose cleanly onto a target `Singular`.
3. **Relational Ordering & Provenance DAG**: Validates that sibling ordering is derived from `precedes` relations via topological sort and that `authored-by` provenance relations are preserved.
4. **Thin WebRealizer & Authored Theme Tokens**: Validates that root theme properties (`theme.primaryColor`, etc.) are emitted into CSS `:root` variables and verifies the absence of hardcoded toast UI.
5. **Extensibility Exit Test (Zero C++ Enum Modification)**: Validates that a Person can author a brand-new semantic concept (`role = "sanctuary"`, `surface.variant = "ethereal"`) purely through data, and `WebRealizer` realizes it into `<div class="web-sanctuary ... web-surface--ethereal ...">` without changing any C++ enum or table.
6. **Orthogonal Storage via VFS & FileChannel**: Validates atomic writes via `FileChannel` and memory storage via `VirtualFileSystem` (`memory://`).
7. **Law-Driven Event Reactivity**: Validates end-to-end DOM event dispatching to `EventBus`, reactive `Law` execution, property mutation (`surface.variant`), and re-realization.
8. **End-to-End Frontend Website Prototype Generation**: Authors an 8-section living website prototype and exports it through `FileChannel`.

### Artifacts & Visual Witness
- **Frontend Prototype**: `web_ui/designer_prototype.html` (22 KB) and `saves/web_designer/prototype.html`.
- **Governed VFS File**: `saves/web_designer/test_governed_vfs.html`.
- **Visual Witness**: `web_ui/designer_prototype.png` (291 KB, 1280x2400 rendered screenshot captured via headless Google Chrome).
