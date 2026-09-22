# AST Interpretation for Legibility

**How interpreting Abstract Syntax Trees at runtime—rather than compiling them—preserves the "No Black Box" doctrine across both discrete law and continuous math.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../law/PROPHETIC_RETE.md` (The B-Time Rete as an ahead-of-time abstract interpreter)
*   `../Singularity/NATIVE_GPU_ONTOMATH.md` (Native GPU OntoMath as a WGSL AST Virtual Machine)
*   `../ontology/NO_BLACK_BOX.md` (The refusal of hidden, un-introspectable state)

---

## The Interrelation

In traditional game engines or simulation environments, performance-critical logic is compiled. Whether it's compiling C++ to native machine code or translating node-based visual scripting into GLSL/WGSL shaders, the compilation step inherently creates a black box. Once compiled, the engine (and by extension, the user or AI First Mover) can no longer inspect *why* a piece of code behaves the way it does, nor can it dynamically reason about its side effects without executing it.

Earthcall fundamentally rejects this through the "No Black Box" refusal. To maintain performance without sacrificing legibility, both the discrete logic system (Laws) and the continuous spatial system (OntoMath) rely on a shared architectural pattern: **runtime AST interpretation**.

### Discrete Law: The Prophetic Rete
Instead of compiling Laws into C++ or opaque scripts, Laws are authored as data (ConditionModels) that form an AST. The Prophetic Rete acts as an interpreter over this data structure. Because the Law remains data, the Rete can perform ahead-of-time static analysis. It can read the AST to know exactly which properties a Law will touch before it fires, enabling deterministic reversibility and safe multi-author conflict resolution. If Laws were compiled, this prophetic introspection would be impossible.

### Continuous Math: Native GPU OntoMath
Similarly, translating the mathematical definitions of beings into WGSL source code and handing it to the graphics driver for compilation creates an unacceptable barrier. Dynamic structural changes (spawning or deleting objects) force costly driver recompilations, breaking the fluidity of the simulation.
As detailed in `NATIVE_GPU_ONTOMATH.md`, the solution is to flatten the OntoMath AST into a byte-code stream and evaluate it dynamically using a Virtual Machine running within a single, static WGSL shader.

### The Unifying Principle
In both systems, replacing compilation with interpretation transforms impenetrable code back into legible, addressable data. By keeping the AST alive and accessible in memory (or VRAM), Earthcall ensures that every behavior and mathematical shape in the world remains exposed to governance, introspection, and dynamic modification, fully satisfying the ontological requirement that nothing in the world may hide from its inhabitants.
