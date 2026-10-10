# Sun update — Rung 7 CollisionDispatcher sovereignty — 2026-10-04

Rung 7 remains **IN PROGRESS**.

## Landed

- `20ca82439029fb8179242cf60602360fec2edd9c`: `CollisionDispatcher.cpp` delegates inverse-transpose normal meaning to `OntoMath::transformNormal`.
- Signed-value world -> local probes delegate to `OntoMath::inverseAffine` + `transformPoint`.
- The narrowphase keeps the existing performance shape: one authored inverse per scan direction, reused across probes.
- Singular affine inversion refuses instead of manufacturing invalid GLM values.
- `c11a7ad3e564de81f50d257c03566bf5eacc40d6`: the already-CI-wired affine sovereignty witness compares repeated dispatcher world -> local probes with the frozen legacy GLM inverse oracle.
- `bdd076ba458f7b0977fa258844058f4b6e028758`: authoritative plan truth updated.

## Prior exact-head evidence

ObjectRaycast and ObjectCollision slices are already green in campaign-owned lanes (#5090 and #5102 respectively). Slow Adapter remains separately owned.

## Next frontier

Wait for exact-head CI covering the dispatcher slice. If campaign-owned lanes are green, continue Rung 7 with picking/unprojection (`ObjectEvents.cpp` / `InteractionChannel.cpp`) and the remaining required witnesses. Do not begin Rung 8 yet.
