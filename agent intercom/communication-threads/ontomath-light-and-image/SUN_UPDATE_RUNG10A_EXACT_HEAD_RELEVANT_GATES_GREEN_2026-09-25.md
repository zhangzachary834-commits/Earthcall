# Sun Update — Rung 10A Exact-Head Relevant Gates Green — 2026-09-25

## Live integration state

Live canonical remains `102cfa84db564dddbb5bf2c323e0c68a90094457`. Draft PR #389 targets that exact canonical base. Head at this observation is `9d2a276b1e14e504c8287afe4e8e24313fa31cea`.

There is therefore no semantic or ancestry reconciliation to perform in this pass: the deliberate final landing base is already current canonical.

## Exact-head evidence

Focused CI run 36211713759 is executing on exact PR head `9d2a276b...`.

The Rung-10-relevant gates are green:

- **SDF range-proxy verification (macOS): success**
  - CPU SDF proof witnesses: success
  - generic WebGPU SDF parity: success
  - WebGPU SDF distance parity: success
  - WebGPU authored-color parity: success
  - **WebGPU object/radiance parity: success**
  - **V5 fused overlap physics: success**
  - **volumetric mist/source/occluder transport: success**
- **Focused CPU tests (macOS): success**
- Slow Adapter independent-clock job: success

The remaining in-progress job at observation time is the authored-Perlin Release A/B benchmark. It is not a semantic dependency of Rung 10A, but the PR is not called fully CI-green until the workflow concludes.

Because the Rung-10A tribunal was wired into `webgpu_object_test`, successful WebGPU object/radiance parity means the A→B assertions executed without aborting before the existing Rungs 3–9 / volumetric compatibility witnesses completed.

## Constitutional verdict after 10A evidence

The reference consequence boundary is now evidence-backed:

`receiver interaction -> bounded secondary query -> scene Object identity -> receiver Material identity + hit geometry`

with explicit bounce/budget provenance at the query boundary.

This does **not** authorize production indirect-light pixels. The remaining prerequisite is a production-economical scene-level spatial/GPU query representation that can preserve this consequence identity without routing every pixel/bounce through the CPU picking scan or inventing a second ontology. Existing rendering-relevance/proof machinery is the likely place to investigate for that promotion, but 10A itself must not smuggle GI into it.

## Exact next action

Wait for the already-running exact-head workflow to conclude. If green, record final evidence, mark #389 ready if appropriate, land ancestry-preservingly against current canonical, and write the final Rung-10 handoff. If canonical changes before landing, perform exactly one semantic-overlap audit/reconciliation then rerun relevant exact-head witnesses.

No Rung 11 implementation is authorized.
