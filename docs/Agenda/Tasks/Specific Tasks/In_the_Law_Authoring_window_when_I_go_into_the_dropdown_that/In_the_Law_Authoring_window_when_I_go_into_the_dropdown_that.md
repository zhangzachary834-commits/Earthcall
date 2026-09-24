# In the Law Authoring window, when I go into the dropdown that says "th

**Status:** partial — searchable referent → Singular → property lens landed 2026-09-07; set/∀ authoring remains open.
**Section in the To-Do list:** Law · Kernel · Governance  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

In the Law Authoring window, when I go into the dropdown that says "the subject of the event/law/the event's other subject", it's not clear whether the dropdown has an option to check more than one property at once and more than one Singular at a time. Same for Act—the dropdown doesn't seem to give me a way to make it act on multiple different Singulars at once. Probably the only way is to manually author for every single new property and Singular you want to check and act on. However that is not sufficient for when you need to check or act on properties of every single Singular of a given kind, since just adding one more Singular doesn't add all of them, and it would become stale if the Singulars go in and out of existence and new ones are added. There's also seems to be no way to check conditions on *all* being at once, and act on *all* given Singulars. 

---

## 2026-09-06 — Singular-first property lens

Zach asked for the Law Author to stop presenting one monolithic field of dotted property strings: first choose a Singular vocabulary, then its registered property, with a navigable graphical authoring surface. The existing `knownPathOptions()` registry remains the sole vocabulary; the two-stage chooser in `src/Singularity/Screen/LawGraphWindow.cpp` is a lens over that live registry, not a second list and not a new category system. The existing “whose property” selector still chooses the actual referent after the property is selected.

This resolves the immediate sift-through-every-path problem. It does not yet resolve the original set/∀ request above, which remains the next substantive authoring capability.

Recorded by Codex, session `01a07a4c-2b09-7f62-b975-3a23084ddeaf`, 2026-09-06 00:00 PDT.

## 2026-09-07 — Person feedback and full palette redesign

Zach's live feedback identified that the first pass still conflated three different things: C++ Singular types, concrete Singular instances, and property-vocabulary headings. “Synthesis Studio” therefore appeared beside “Object” as though both named the same kind of thing, while selecting a concrete Singular in the adjacent control did not actually narrow the property list. Zach also asked for type-ahead discovery at every stage and questioned whether ImGui imposed the limitation.

The first-pass dropdown row has been replaced with one professional palette-style Property Lens. Its visible columns are **Referent**, **Specific Singular or expected runtime C++ type**, and **Property**. Selecting a concrete Singular now enumerates that exact instance's live `listProperties()` registry, including authored dynamic properties, and therefore genuinely narrows the third column. Abstract law/event referents can instead be narrowed by the runtime polymorphic types the Law calculus can test (`Object`, `Person`, `Relation`, `Formation`, `Law`, `Zone`, `Lexeme`, and First Mover Law). World readings and time now have an explicit context route because they are not Singular-owned properties. Both Singular and property columns accept type-ahead text.

The conclusion from this pass is that ImGui was not the constraint: it can render a multi-column command palette adequately. The conceptual model of the first pass was the constraint, exactly as Zach diagnosed.

Recorded by Codex, session `01a07a4c-2b09-7f62-b975-3a23084ddeaf`, 2026-09-07 00:00 PDT.

## 2026-09-07 — Specific Singular becomes the necessary middle step

Zach corrected the second pass: Specific Singular must not share one vertical pane with the type alternatives. Property meaning is borne by an actual Singular, so the stable sequence is **Singular type → specific live Singular → that Singular's registered property**. Runtime reference (Law subject, event subject/object, or a named Singular) is a separate horizontal question about how the resulting property path resolves when the Law runs.

The Property Lens now implements that sequence literally. Runtime type filters the middle column; selecting one live instance exclusively determines the third column through its `listProperties()` registry. Abstract subject/event paths therefore use a selected concrete instance as the vocabulary witness while retaining a dynamic runtime referent. Named-Singular paths encode that selected identity. Time and `@world` remain an explicit context exception rather than pretending that free-floating readings are owned properties.

The chosen path is no longer an opaque string afterward: reference buttons rewrite the qualifier without requiring `@event.subject` syntax, the lens descends through properties that point to nested Singulars, vector-valued properties expose live Whole/x/y/z granularity buttons, and color-shaped vectors adapt those buttons to Whole/r/g/b. This extends Zach's requirement into syntax-aware manipulation while keeping `PropertyPath` itself as the serialized truth.

Recorded by Codex, session `01a07a4c-2b09-7f62-b975-3a23084ddeaf`, 2026-09-07 03:35 PDT.
