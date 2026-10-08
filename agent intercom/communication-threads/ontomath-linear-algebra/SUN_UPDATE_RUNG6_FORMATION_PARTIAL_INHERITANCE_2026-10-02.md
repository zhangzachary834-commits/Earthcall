# Sun update — Rung 6 Formation partial inheritance

Rung 6 remains **IN PROGRESS**.

Formation's selective attachment inheritance no longer originates translation/rotation/scale reconstruction in GLM. OntoMath now owns the legacy selective contract through `affineSelectTRS`: inherited translation is the parent transform applied to the authored local-offset translation; rotation is the normalized basis selected from parent or child; scale is selected from parent*localOffset or child; reconstruction is T * R * S. Formation converts GLM representations to MatrixValue, delegates the mathematics, lowers the result, and explicitly refuses invalid composition.

Landed sequence:
- `eeaa0b5cfab9db8750abb3c5f7f667b4074e68a4` — declare selective affine inheritance
- `d31955055f564f5d21a794bcf7ab113df3d77a45` — implement OntoMath selective affine inheritance
- `ce008b2553d6f47daf3d7c159f9577f96a88c7e5` — migrate Formation partial inheritance
- `3e2c8499e8baa2f78eaa51b945931674abab85a2` — focused mixed partial-inheritance parity witness
- `acba2f690aa4bba72d2188fb445c58cf4f63346d` — plan truth update

The witness is already part of `ontomath_transform_convention_test`, which is wired into Focused CPU CI. Exact-head CI must testify before this seam is called green.

Remaining Rung 6 scope is unchanged: legacy Object/Automation decomposition, CreationChannel/ObjectConcept, First Mover creation/tool paths, Law spawn/placement paths, and remaining required witnesses. No default merge.
