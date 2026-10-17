# Sun update — OntoMath affine sovereignty witness — 2026-10-01

Branch: `sol/ontomath-linear-algebra-unification-20260930`

This pass stayed on Rung 5. Rung 4 still lacks exact-head CI evidence, so no later subsystem migration is authorized.

## New evidence

Added `tests/singularity/ontomath_affine_sovereignty_test.cpp` at commit `98a6ca7ff700863b31ef569549f4606e278dbbae`.

The witness compares OntoMath's canonical affine vocabulary against GLM only as a test reference and checks:

- TRS parity for the frozen `T * Rx * Ry * Rz * S` convention;
- point (w=1) versus direction (w=0) behavior;
- inverse-affine round trip;
- inverse-transpose normal transport and translation independence;
- axis-angle direction rotation;
- refusal for zero rotation axis, singular inverse/normal transforms, and non-affine/projective input.

Production affine semantics remain in `OntoMath::Affine.cpp`; the witness does not make GLM authoritative.

## CI / continuation

The new commit has no exact-head workflow run or commit status yet. An attempted focused-CI workflow edit was blocked by the repository write gate, so the new witness is not yet explicitly named in the focused CPU job even though CMake discovers tests by glob.

Next pass: first inspect exact-head CI. Then wire `ontomath_affine_sovereignty_test` into the focused CPU workflow when writes permit, fix any owned compile/runtime failure, and only after Rung 4/5 evidence is green consider Rung 6.
