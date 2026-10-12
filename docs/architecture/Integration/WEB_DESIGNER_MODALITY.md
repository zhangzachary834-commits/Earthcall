# Earthcall Web Designer Modality

## 1. Architectural Telos & Ontology

The Earthcall Web Designer Modality establishes website authoring as a genuine **Earthcall Law-authoring domain**, preserving Earthcall's Person-centered ontology and the Seven Refusals:

1. **Refusal 1 (No new C++ classes for domain nouns)**: There is no `class WebPage`, `class Button`, `class Heading`, or `class NavigationBar`. In-world beings are standard `Singular` (`Object`, `Formation`, `Relation`) entities carrying authored dynamic properties (`web.*`).
2. **Refusal 2 (Subsystems placed in Singularity)**: The web modality channel lives under `Singularity/Foreign/Web/Designer/` as a channel to foreign software/DOM environments.
3. **Refusal 3 & Enums**: Semantic facets (`SemanticRole`, `LayoutFlow`, `LayoutAlign`, `LayoutJustify`, `SpacingScale`, `ContentScale`, `TypographyWeight`, `TypographyTone`, `SurfaceVariant`, `SurfaceShape`, `SurfaceElevation`) are strictly append-only enums serialized as integers and mapped to stable string identifiers.
4. **Refusal 6 (No Black Box)**: Every property is accessible and governable via standard Earthcall `PropertyPath` (`web.role`, `web.layout.flow`, `web.surface.variant`, etc.).
5. **Refusal 7 (No hardcoded behavior in C++ methods)**: Web behavior, presentation, and composition emerge entirely from Person-authored Laws and Metalaws reacting to EventBus events.

HTML, CSS, and DOM are not Earthcall's ontology—they are concrete realization targets of an in-world semantic model.

---

## 2. The End-to-End Pipeline

The web designer modality operates across five layered stages:

```
Natural Language / Author Input
       │  "a large centered heading", "a rounded primary button"
       ▼
Lexemes & Definitions (WebLexemeDef)
       │  Resolves into stable opcodes: web.concept.scale.large, web.concept.role.action
       ▼
Metalaws (Law instances with ConditionNode & ActionNode)
       │  Evaluates condition on compilation slot and executes action:
       │  ActionNode::set("compilation.output.web.scale.size", "large")
       ▼
In-World Singular State & Relations
       │  Dynamic properties on Objects:
       │    web.role = "heading"
       │    web.scale.size = "large"
       │    web.layout.align = "center"
       │  Hierarchical Containment Relations:
       │    Relation(kContains, parent, child)
       ▼
Web Realization (WebRealizer)
       │  Translates semantic in-world state into:
       │    - HTML5 semantic landmarks (<nav>, <section>, <h1>, <button>)
       │    - CSS custom property design tokens & utility classes
       │    - Interactive DOM client scripts bridging to Earthcall EventBus
       ▼
Browser / WebView Realization Target
```

---

## 3. The Semantic Model (`WebConcept`)

`WebConcept` defines the semantic facets of authored elements without presupposing HTML tags:

| Facet | In-World Path | Semantic Options | HTML/CSS Realization |
|---|---|---|---|
| **SemanticRole** | `web.role` | `Page`, `Section`, `Navigation`, `Heading`, `Paragraph`, `Action`, `Container`, `List`, `Item`, `Media`, `Quote`, `Divider` | `<div>`, `<section>`, `<nav>`, `<h1>`-`<h6>`, `<p>`, `<button>`/`<a>`, `<ul>`, `<li>`, `<img>`, `<blockquote>`, `<hr>` |
| **LayoutFlow** | `web.layout.flow` | `Vertical`, `Horizontal`, `Grid` | `flex-direction: column`, `flex-direction: row; flex-wrap: wrap`, CSS Grid |
| **LayoutAlign** | `web.layout.align` | `Start`, `Center`, `End`, `Stretch` | `align-items`, `text-align`, `justify-content` |
| **LayoutJustify** | `web.layout.justify` | `Start`, `Center`, `End`, `SpaceBetween`, `SpaceAround` | `justify-content` |
| **SpacingScale** | `web.layout.spacing` | `None`, `Tight`, `Normal`, `Relaxed`, `Generous` | CSS `gap: 0`, `0.5rem`, `1rem`, `1.5rem`, `2.5rem` |
| **ContentScale** | `web.scale.size` | `Small`, `Normal`, `Large`, `Huge` | `font-size: 0.875rem`, `1rem`, `2.25rem`, `3.5rem` |
| **TypographyWeight** | `web.typography.weight`| `Light`, `Normal`, `Medium`, `Bold` | `font-weight: 300`, `400`, `500`, `700` |
| **TypographyTone** | `web.typography.tone` | `Default`, `Muted`, `Prominent`, `Accent` | Color tokens (`--web-text-muted`, `--web-primary`, etc.) |
| **SurfaceVariant** | `web.surface.variant` | `Default`, `Primary`, `Secondary`, `Surface`, `Card`, `Hero`, `Subtle`, `Outline`, `Ghost` | Background/border styles, elevation gradients |
| **SurfaceShape** | `web.surface.shape` | `Sharp`, `Rounded`, `Pill`, `Circle` | `border-radius: 0`, `var(--web-radius)`, `9999px`, `50%` |
| **SurfaceElevation** | `web.surface.elevation`| `Flat`, `Subtle`, `Raised`, `Floating` | `box-shadow` depth tokens |

---

## 4. Metalaw Compilation & Extensibility

Metalaws are registered in `WebDesignerMetalaws::syncRegister`. Each Metalaw listens for compilation opcodes on a channel target or element and updates the output slots.

### Adding New Semantic Concepts
To add a new semantic concept (e.g. `web.concept.role.badge` or `web.concept.surface.frosted`):
1. **Extend Enum / Facet in `WebConcept.hpp`**: Append the new enum value (maintaining append-only invariant).
2. **Add Lexeme Definition in `WebDesignerMetalaws.cpp`**:
   ```cpp
   {"badge", "web.concept.role.badge", "metalaw.web.role.badge"},
   ```
3. **Register Metalaw in `WebDesignerMetalaws::syncRegister`**:
   ```cpp
   addMetalaw(laws, "metalaw.web.role.badge", "web.concept.role.badge",
              ActionNode::sequence({
                  ActionNode::set("compilation.output.web.role", PropertyValue(std::string("container"))),
                  ActionNode::set("compilation.output.web.surface.shape", PropertyValue(std::string("pill"))),
                  ActionNode::set("compilation.output.web.scale.size", PropertyValue(std::string("small")))
              }), author);
   ```
4. **Realization Mapping in `WebRealizer.cpp`**: If the concept needs specialized CSS or DOM tag mapping, add the corresponding token or tag rule.

---

## 5. Composition & Hierarchy via Relations

Instead of monolithic templates or giant raw HTML strings, websites are formed through **universal set-to-set relations**:
- Elements are created as `Singular` objects authored by a Person.
- Parent-child hierarchy is established with first-class `Relation` beings using `WebRelationType::kContains` (`web-contains`).
- Ordering is represented by `web.order` properties on child beings.
- When realized, `WebRealizer` traverses the directed acyclic graph of containment relations, generating valid, indented, semantic HTML5.

---

## 6. Dynamic Reactivity: Web Actions to Earthcall Laws

User interactions (clicks, toggles) are not handled by black-box JavaScript callbacks:
1. Interactive elements carry `data-action-event` and `data-being-id` attributes.
2. When triggered in the browser, the client script dispatches an event (`web-action` and custom event name).
3. `WebDesignerChannel::handleWebAction` receives the event and publishes it to the Earthcall `Core::EventBus`.
4. Any Person-authored reactive `Law` listening to the event fires and mutates the in-world `Singular` properties via `ActionNode::set`.
5. Upon re-realization, the DOM reflects the mutated state.

---

## 7. What Remains Intentionally Unsupported

- **Arbitrary Unsanitized Raw HTML Injection**: Websites are authored via semantic concepts and Laws. Escapes exist via explicit style/href properties, but raw unverified HTML strings are not admitted as domain nouns.
- **Client-side Framework Runtimes (React/Vue/Angular)**: Earthcall is the runtime; DOM is the realization target. Injecting foreign reactive frameworks would create a duplicate state machine violating Refusal 6.
- **Bespoke Hardcoded Layout Engines**: Layout is driven by CSS Grid and Flexbox via semantic facets.
- **Client-Side Storage as Ground Truth**: The Earthcall world and save system are the single order of truth.

---

## 8. Verification & Prototype Witness

The implementation is verified end-to-end by:
- `tests/singularity/web_designer_modality_test.cpp`:
  - **Test 1**: Lexeme Resolution (stable opcodes).
  - **Test 2**: Metalaw Composition (orthogonal composition and conflict detection).
  - **Test 3**: Law-Authored Hierarchy & Relations (containment DAG and in-world dynamic properties).
  - **Test 4**: HTML / CSS / DOM Realization (HTML5 landmarks and CSS design tokens).
  - **Test 5**: Law-Driven Dynamic Behavior (user action -> EventBus -> reactive Law -> state mutation -> re-realization).
  - **Test 6**: End-to-end multi-section frontend prototype export.
- Prototype artifacts:
  - `web_ui/designer_prototype.html` (21.7 KB)
  - `saves/web_designer/prototype.html` (21.7 KB)
  - `web_ui/designer_prototype.png` (Screenshot showing visual rendering of the complete prototype).
