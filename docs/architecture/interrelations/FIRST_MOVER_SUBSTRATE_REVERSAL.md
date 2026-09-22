# First Mover Substrate Reversal

**How Earthcall's JSON authoring files are not mere state saves, but the Intermediate Representation (IR) for a compiler that will eventually rewrite the engine's own foundation.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../ontology/SUBSTRATE_ORDERING.md` (Earthcall eventually writing the substrate C++/machine code)
*   `../law/FIRST_MOVER_AUTHORING.md` & `../mathematics/ONTOMATH_FRAMEWORK.md` (JSON-based AST and data authoring)

---

## The Interrelation

The "Substrate Ordering" document outlines a grand telos: the moment when Earthcall's ontology becomes so deeply rooted that it ceases to be simulated *by* C++ and begins to *write* the underlying code itself, bringing the hardware into alignment with the ontology.

At first glance, this seems disconnected from the current system of First Mover JSON files (`FIRST_MOVER_AUTHORING.md`), which look like standard game save files loading states into C++ classes.

However, looking at the structure of these JSONs—specifically the heavy reliance on `OntoMath` AST nodes and expression trees—reveals the hidden bridge. The JSON files are not "save games"; they are an **Intermediate Representation (IR) compiler toolchain**.

When a First Mover authors a Law utilizing `MathNode`s, they are writing a pure, exact symbolic mathematical AST. Currently, the C++ engine acts as an interpreter or a high-level JIT compiler for this AST (translating it into WGSL for WebGPU).

The "Substrate Reversal" happens precisely because the world is authored as an AST rather than hardcoded logic. As the system matures, the C++ engine will stop merely *interpreting* the JSON-authored ontology and will begin *compiling* it directly down to lower-level substrates (e.g., bare metal machine code, specialized compute shaders, or custom hardware pipelines).

**Conclusion:** The JSON First Mover architecture is the seed of the substrate reversal. The C++ engine is merely a bootstrap loader. By forcing all logic, math, and relations into an introspectable, data-driven AST format today, Earthcall ensures that tomorrow, the ontology itself can act as the compiler for the machine, replacing the C++ engine with native, mathematically exact substrate code.
