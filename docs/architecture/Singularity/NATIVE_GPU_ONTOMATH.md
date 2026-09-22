# Native GPU OntoMath (The AST Interpreter)

## 1. The Dynamic SDF Recompilation Problem
Earthcall’s `OntoMath` provides a mathematically pure representation of the Zone, modeling Beings as analytic fields (SDFs). Currently, the `@screen-channel` translates this pure math into pixels by recursively generating a string of WGSL source code (`SdfWgsl.cpp`) and passing it to the graphics driver for compilation.

While this is exceptionally fast for parameter changes (moving a Torus merely updates uniform memory), it is catastrophically slow for structural changes. If a Person authors a Law that spawns or deletes an Object, the mathematical topology of the tree changes. The Engine must generate new WGSL text and force the graphics driver to compile a new shader mid-frame. 
Shader compilation is a heavyweight OS operation (10–100+ ms). Spawning objects dynamically therefore results in unacceptable frame hitching, effectively limiting `OntoMath` to static scenes.

## 2. The Frontier: Native GPU Math Powers
To achieve true micro-mastery over the GPU, we must strip the graphics driver of its compiling authority and grant `OntoMath` native execution powers on the graphics hardware. 

This is achieved by building a **GPU AST Interpreter** (a Virtual Machine running inside the fragment shader). Instead of translating math into WGSL text, we translate the `OntoMath` tree into a flat stream of data.

### The Architecture
1. **Bytecode Serialization:** The CPU walks the `OntoMath` AST and flattens it into a linear array of integers and floats. Every mathematical operation becomes an opcode (e.g., `Sphere = 1`, `SmoothUnion = 5`, `Transform = 12`), followed by its parameters and child indices.
2. **The Micro-Mastery Substrate:** This bytecode array is suballocated into the `GpuBufferPool`'s `Storage` arena. Because it is purely data, structural changes to the AST (spawning/deleting Beings) cost nothing but memory bandwidth.
3. **The WGSL Virtual Machine:** We write exactly **one** static WGSL shader. This shader never changes and never recompiles. It contains a `while` loop and a small stack array. For every pixel, it reads the opcodes from the Storage buffer, pushes and pops values from the stack, and evaluates the tree dynamically.

## 3. Ontological Supremacy: The True Global SDF
By decoupling AST complexity from shader compilation, the Engine achieves the ultimate render target: **The Unified Zone Field.**

Rather than rendering objects by rasterizing overlapping invisible bounding boxes (which causes crippling fragment overdraw on high-DPI displays), the entire Zone can be unified into a single `MathNode` tree. 
The WGSL Virtual Machine executes a single, full-screen raymarch pass. It evaluates the unified bytecode payload once per step, computing the exact minimum distance to the closest mathematical surface in the entire universe.

**Results:**
- **Zero Recompilation:** Infinite dynamic complexity; objects can be spawned or destroyed continuously with zero compilation hitching.
- **Zero Overdraw:** The ray terminates immediately upon striking the nearest surface, ignoring all occluded complexity.
- **Universal CSG Blending:** Because all Beings are evaluated natively in the same mathematical pass, a Law can trivially apply `Op::SmoothUnion` across disparate Objects, causing them to physically meld into each other purely as an artifact of the sensory projection.

Native GPU OntoMath is the final evolution of the `@screen-channel`, marrying the absolute invariant purity of the CPU's math engine with the massively parallel execution of the GPU substrate.
