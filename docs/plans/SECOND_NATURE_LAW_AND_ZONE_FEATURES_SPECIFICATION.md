# Second-Nature Law & Zone Features Specification

**Authorial Attribution:**
- **Human Origin & Foundational Vision:** Zach (Earthcall creator). Derived directly from Zach's core vision documents: `Second-Nature Law Authoring.md` (making Law creation as second-nature as keyboard/mouse via MetaLaws), `Universal Artistic-Math-Simulation Environment.md` (unlimited resolution procedural surface drawing without black-box pixel caps), `PROPHETIC_RETE.md` (pre-execution law analysis), and `To-do list.md` (Closed-Form Undo System & FaceTexture procedural painting).
- **AI Agent Synthesis & Architecture Extension:** Jules (AI Agent, session 2026-09-03). Formulating concrete, executable feature specifications leveraging existing C++ channels, OntoMath AST structures, and Rete engine capabilities without violating the Seven Refusals.

---

## Executive Summary

This specification defines four major engine and interface features that elevate Earthcall's Person experience. Rather than adding complex hardcoded C++ UI widgets or domain-specific classes, all four features are realized as **First Movers, MetaLaws, OntoMath AST transformations, and InteractionChannel event streams**.

---

## 1. Second-Nature MetaLaw Authoring Engine

### 1.1 Intent & Origin
In `Second-Nature Law Authoring.md`, Zach wrote:
> *"Law Authoring window is very expressive but very tedious for human authors... Laws must be as easy to author as keyboard and mouse. Ordinarily, law creation has to be so intuitive and human-fitted that it's second-nature... The ultimate computer should not merely execute laws authored by humans. It should become a medium in which humans can author laws as naturally as they presently manipulate matter, symbols, images, and language."*

### 1.2 Architecture & Mechanics
The **Second-Nature MetaLaw Authoring Engine** replaces manual JSON/ImGui node composition with **Lexeme-driven Set-to-Set Law Synthesis**:

1. **Lexeme Selection & Gestural Intent**:
   The Person selects target Singulars in-world and speaks or clicks a `Lexeme` representing intent (e.g., `@lexeme.gravitate`, `@lexeme.harmonize`, `@lexeme.orbit`, `@lexeme.repel`).
2. **MetaLaw Evaluation (`ActionNode::Kind::Synthesize`)**:
   A First Mover MetaLaw (`meta-law-synthesizer`) intercepts the `lexeme-applied` event and target `Formation`. It evaluates the template `LawConcept` associated with the chosen Lexeme.
3. **Automatic Rete Graph Generation**:
   The MetaLaw creates new `ConditionNode`s and `ActionNode`s, binds stable subject identifiers (`@subject`, `@target`), and registers the synthesized `Law` with `LawManager`.
4. **Instant In-World Binding**:
   The synthesized Law fires immediately within the Rete engine, updating target properties (such as `@target.position` or `@target.color`) in response to world conditions.

---

## 2. Chrono-Symmetry Actuator ($Cmd+Z / Cmd+Y$ Closed-Form Undo)

### 2.1 Intent & Origin
In `To-do list.md`, Zach noted:
> *"Implement Closed-Form Undo System: The Z and Y keys were unbound because a key that does nothing is worse than a key that is not there. The mathematics framework takes reversibility seriously (`ONTOMATH_FRAMEWORK.md` §6), but the hand cannot reach it. Implement the actual undo state integration so the surface can run backward."*

### 2.2 Architecture & Mechanics
The **Chrono-Symmetry Actuator** implements closed-form temporal reversibility directly into the `InteractionChannel`:

1. **Event Capture in `InteractionChannel`**:
   The `InteractionChannel` listens for `key-pressed` events corresponding to $Cmd+Z$ (Undo) and $Cmd+Y$ (Redo). When detected, it emits a past-tense edge (`person-undo-requested`, `person-redo-requested`).
2. **Closed-Form Antiderivative Evaluation**:
   An active First Mover Law (`chrono-symmetry-law`) catches the edge. For every governable property modified by a `Flow` action, it executes `ActionNode::valueSecondsAgo(dt)`.
3. **Reversal Without Log Memory**:
   Because `Flow` rates ($\frac{dx}{dt} = f(t)$) have closed-form antiderivatives $F(t)$, state is restored by evaluating $F(t - \Delta t)$ directly. The substrate does not store megabyte state snapshots or frame logs—the world moves backward smoothly through exact mathematics.

---

## 3. Prophetic-Rete Predictive Holograms (Causal Foresight)

### 3.1 Intent & Origin
Grounded in Zach & Opus 5's `PROPHETIC_RETE.md` and `B-time Rete.md`. Before a law fires or an action alters the world, the Rete network can determine the downstream consequences before they occur.

### 3.2 Architecture & Mechanics
The **Prophetic-Rete Predictive Hologram** provides Persons with visual foresight before committing changes:

1. **Pre-Execution Terminal State Analysis**:
   When a Person hovers over a MetaLaw choice or drags an object toward a trigger threshold, the Rete analyzer simulates the terminal state of the active agenda at $t + \Delta t$.
2. **Ghost SDF Overlay (`ShapeKind::Field`)**:
   The target's predicted transform, scale, color, or relation network is rendered as a semi-transparent cyan/gold "ghost hologram" using a lightweight SDF shell overlaying the current geometry.
3. **Commit or Cancel**:
   Releasing the gesture commits the action and collapses the hologram into physical reality. Moving the pointer away cancels the pre-execution state without altering the world.

---

## 4. Infinite-Resolution OntoMath Shader Brush (Procedural Surface Painting)

### 4.1 Intent & Origin
In `To-do list.md` and `Universal Artistic-Math-Simulation Environment.md`, Zach instructed:
> *"FaceTextures must work, be visible, and handle edge cases... FaceTextures shouldn't be a black box or a hardcoded limiter (e.g. FaceTexture dimensions should not be hardcoded away from Person authoring—we shouldn't be stuck with only having 256x256 resolution textures)."*

### 4.2 Architecture & Mechanics
The **Infinite-Resolution OntoMath Shader Brush** replaces fixed 2D pixel grids with **Procedural Expression Painting**:

1. **Brush Stroke as MathNode AST**:
   When a Person paints on an object surface using the Pottery or Face Brush tool, strokes write terms directly into an OntoMath `ScalarForm` / `FieldNode` AST attached to `@object.faceExpression`.
2. **WGSL Shader Compilation**:
   `SdfWgsl.cpp` transpiles the painted `faceExpression` AST into a native WGSL fragment shader function.
3. **Resolution Independence**:
   Because the texture is an exact mathematical expression $C(u, v) = \text{Noise}(u \cdot 10, v \cdot 10) + \text{Sin}(u)$, zooming in infinitely yields crisp, sub-pixel mathematical details with zero pixelation or memory bloat.

---

## 5. Summary of Property Paths & Modality Interfaces

| Feature | Primary Modality Channel | Governed Property Paths | First Mover Law Slug |
|---|---|---|---|
| **Second-Nature MetaLaw** | `Language` / `Input` | `@law.conditions`, `@law.actions` | `@meta-law-synthesizer` |
| **Chrono-Symmetry Undo** | `Input` / `Time` | `@singular.*` via antiderivative | `@chrono-symmetry-law` |
| **Prophetic Hologram** | `ScreenChannel` | `@screen-channel.ghostOverlay` | `@prophetic-hologram-law` |
| **OntoMath Shader Brush** | `ScreenChannel` / `OntoMath` | `@object.faceExpression` | `@procedural-brush-law` |

---
*Synthesized by Jules (AI Agent), 2026-09-03, in full alignment with Zach's Earthcall Manifesto and AGENTS.md.*
