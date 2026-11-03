# Human-Facing Key Singulars with Machine Key Properties

**Status:** open  
**Section in the To-Do list:** Person-facing surface  
**Created:** 2026-09-07 by Zach / Claude  
**References:** `docs/audits/PERSON_INTERFACE_AUDIT_2026-08-18.md` §3.4, `docs/architecture/law/INTERACTION_AS_LAW.md`, `src/Singularity/Input/Keyboard/KeyboardHandler.hpp`

---

## Architectural Intent

Rather than treating low-level hardware keys as `Singular`s in the ontology, machine-level keys are represented as registered **`Properties`** (exposed via `Singularity::Input::InteractionChannel` or input modality channels, such as key states, codes, and modifier levels). 

This allows Persons to author **`Key` `Singular`s** at runtime in a more human-facing, semantic sense (representing authored inputs, chords, and controls), with the underlying machine-level keys and bindings serving as their properties.

## Scope & Concrete Steps

1. **Machine Keys as Properties**: Expose and govern physical/GLFW key states, scan codes, and modifiers as registered property paths on the interaction modality layer (avoiding domain/hardware type clutter in the ontology type system).
2. **Authorable Key Singulars**: Allow Persons to author human-facing `Key` Singulars at runtime (carrying semantic intent, human labels, and bindings that map to machine key properties and Law activation triggers).
3. **Retire `KeyboardHandler` Scaffolding**: Remove the ten dead `setup*Bindings()` preset routines (`KeyboardHandler.hpp:60-73` / `KeyboardHandler.cpp:149-260`) whose empty callbacks and uncalled bindings act as legacy scaffolding.
