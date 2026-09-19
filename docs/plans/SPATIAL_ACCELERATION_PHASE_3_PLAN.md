# Phase 3: Authored Spatial Acceleration & Dynamic Regional Partitioning

## The Goal
To shatter the 16ms barrier and push the rendering phase to support blistering framerates (300-1000+ FPS). Candidate A optimized the **cost** of a single step. Phase 3 optimizes the **quantity** of steps by allowing rays to leapfrog through empty space using a regional Interval Bounding hierarchy.

Crucially, because Earthcall is a universal design host, this bounding logic will **not** rely on a hardcoded C++ global proof. It will evaluate bounds dynamically on a per-region basis, with an architecture designed to eventually accept bounds and rules authored directly via Laws in the Rete network.

## Proposed Architecture

### 1. Dynamic Regional Partitioning
Instead of evaluating `df/dy > 0` globally, the engine will partition the world into a 2D grid (e.g., a Quadtree or a uniform grid of `Zone` sectors).
For each cell `[x_min, x_max] × [z_min, z_max]`, the engine will evaluate two properties:
1. **Regional Monotonicity**: Does `df/dy > 0` hold strictly within *this specific cell*?
2. **Height Extents**: If monotonic, what are the exact $y_{\min}$ and $y_{\max}$ bounds where $f(x,y,z) = 0$?

This allows a world to have caves and overhangs in Sector A (which will render carefully), while Sector B remains a strict heightmap and renders instantly.

### 2. Synthesizing Bounds via `OntoMath`
Earthcall already supports `MathNode::evalRange` (Interval Arithmetic) and `ScalarForm::derivative`.
- To prove monotonicity for a cell, we evaluate the interval bounds of `df/dy = node.derivative("y")` over the cell's 3D bounding box. If `range.lo > 0`, the region is strictly monotonic.
- To find $y_{\max}$, we evaluate the interval bounds of the authored field.

### 3. Rete / Law Integration (The Future-Proofing Seam)
To fulfill the requirement that spatial bounds can be an authored Rete/Formation question:
- The bounds for a cell will not be computed silently inside the GPU renderer.
- Instead, the bounding grid will be formalized as a persistent `Property` or `Relation` (e.g., `spatial.bounds`) on the `Zone` or `Physics` being.
- A First-Mover or Law can author a rule that explicitly sets these bounds for a region.
- The C++ interval arithmetic will act as the "Default Physics Law" (the fallback that runs when no Person has authored a stronger or more specific constraint).

### 4. GPU Shader Integration
The bounding grid will be uploaded to the GPU as a SSBO (Structured Buffer) or a 2D Texture (Min/Max Mipmap).
In `SdfWgsl.cpp`'s `sdfSampleStep` loop:
- The ray will project its coordinates into the 2D grid.
- If the ray is traveling above the cell's $y_{\max}$, it will analytically advance `t` directly to the intersection with the $y_{\max}$ plane (or the cell boundary), entirely skipping Perlin evaluation.

## Open Questions for Zach
1. **Grid Resolution**: Should we start with a fixed uniform grid (e.g., 64x64 cells per zone) for simplicity, or immediately build a quadtree?
2. **GPU Upload Strategy**: A 2D texture (Heightmap Min/Max) is extremely fast for WebGPU to sample during raymarching. Are you comfortable with baking the interval bounds into a dynamic texture, or would you prefer a buffer array?
3. **Law Authored Bounds**: For this immediate iteration, should I focus purely on the C++ Interval arithmetic backend to get the FPS up, and just leave the "seam" open for Rete? Or do you want me to write the actual `Law` structure for bounds right now?

## Verification
- Measure the GPU step count (should drop from ~192 to <10 for grazing horizon rays in monotonic regions).
- Verify the FPS jumps to the 300+ range.
