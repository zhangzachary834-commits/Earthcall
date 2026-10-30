# Sun update — Rung 6 Formation full-inheritance migration — 2026-10-01

Rung 6 remains IN PROGRESS.

Landed production seam:
- `src/Relation/Formation/Formation.cpp`
- Full attachment inheritance no longer originates `parentTransform * localOffset` through GLM.
- Production converts both runtime matrices to OntoMath MatrixValue, invokes `OntoMath::affineCompose`, and lowers the authored result back to the cached glm::mat4 representation.
- Invalid/non-lowerable composition explicitly refuses that attachment update; C++ does not invent a fallback transform.

Focused witness:
- `tests/singularity/ontomath_transform_convention_test.cpp`
- Drives the real Formation relation/attachment path with translation, rotation, non-uniform parent scale, and local offset.
- Pins the result against the frozen legacy `parentTransform * localOffset` oracle.

Commits:
- production: `f5e6e98d8df0af020dc2f57246b4c10ddc12419a`
- witness: `3619711614057bfd3fc335a08861d52ff735cd36`
- plan truth update follows those commits.

Not claimed complete:
- Formation partial-inheritance decomposition/rebuild remains direct GLM semantics.
- Object/Automation decomposition, CreationChannel/ObjectConcept, First Mover paths, and Law spawn/placement remain.
- Exact-head CI must testify before this seam is called green.

No default merge performed.
