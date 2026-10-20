# Second Person Consent and Geometry Substrate

**How unbypassable representational consent guards must be evaluated prior to lowering into the Geometry Execution Substrate, ensuring ontological governance before execution erasure.**

## 1. The Erasure of Ontology in the Substrate

The `GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md` establishes a strict downward flow: "Meaning is authored. Mathematics states form. Geometry IR lowers form. Backends execute IR." The execution substrate (WGSL, CPU evaluation, pixels) is purposefully stripped of ontological meaning. A mesh or an SDF primitive does not know if it represents a rock, a UI button, or a Person's Body. It only knows vertices, radii, and blending operations.

This separation prevents the engine from hallucinating semantic behavior based on geometry. However, it creates a critical threshold: once a being's mathematical form is lowered into Geometry IR, its ontological identity—and the governance attached to it—is effectively erased from the perspective of the execution backend.

## 2. The Representational Consent Guard

The `SECOND_PERSON_FRAMEWORK.md` defines unbreakable guards regarding the representation of a Person. Any Object derived from a Person's Body or Voice carries a `derived-from` Relation at the Kernel tier. Crucially, the framework mandates: "Any channel about to render, sound, export, or transmit a representation of a Person resolves the `derived-from` chain and the source Person's authored constraint *in C++, unconditionally*."

This consent check must happen *before* the representation crosses the boundary into the Geometry Execution Substrate.

If the engine lowers a Person's avatar into raw SDF primitives and *then* attempts to verify if another Person has permission to render or transmit those primitives, it will fail. The SDF primitives no longer carry the `derived-from` Relation; they are just math. The consent guard would be bypassed by the very architecture designed to optimize rendering.

## 3. The Chokepoint of Manifestation

The architectural synthesis requires a strict chokepoint prior to manifestation. The workflow must be:

1. **Resolution:** The Interaction or Render channel identifies a Singular intended for manifestation.
2. **Provenance Check:** The channel traces the Singular's Relations. If it encounters a `derived-from` Relation pointing to a Person's Body/Voice, it triggers the consent evaluation.
3. **Consent Evaluation:** The source Person's authored constraints (stakes, covenant, loud refusal) are evaluated in C++.
4. **Lowering (if approved):** Only after consent is explicitly granted does the Singular's OntoMath definition proceed to lowering into Geometry IR.

If consent is denied, the channel refuses loudly, and the mathematical form is never generated or passed to the execution substrate.

## 4. Preventing "Deepfake" Pipelines

By evaluating consent *above* the geometry substrate, the architecture structurally prevents "deepfake" pipelines. A foreign modality (like an AI agent acting through a First Mover) cannot bypass the consent check by scraping the geometry buffers, because unauthorized representations never enter those buffers in the first place.

The Geometry Execution Substrate remains fast, mathematically pure, and devoid of domain nouns, precisely because the heavy lifting of Second-Person governance is resolved in the semantic layer before the math is ever compiled.