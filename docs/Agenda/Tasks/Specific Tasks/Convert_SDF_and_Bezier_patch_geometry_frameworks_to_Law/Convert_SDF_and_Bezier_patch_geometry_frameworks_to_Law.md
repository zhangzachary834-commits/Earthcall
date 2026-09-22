# Convert SDF and Bezier patch geometry frameworks to Law-adjustable properties & set-to-set replication

**Status:** ✅ done and verified  
**Section in the To-Do list:** Singular · Relation · Formation  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Convert SDF and Bezier patch geometry frameworks to Law-adjustable properties & set-to-set replication** — done and verified (2026-08-16): Extended `ObjectConcept::MemberTemplate` with Bézier patch support (`hasPatch`, `patch`, deep copy in `captureFromBeings`, instantiation, and JSON serialization); added live shape kind override support for `ShapeKind::Patch` and `ShapeKind::Field` in `ActionModel.cpp`; authored `concept-bezier-patch`, `concept-complex-sdf`, generation laws, and MetaLaws (rate-limiting / cooldown and zone grid snapping). Verified in `tests/bezier_patch_law_test.cpp` (30 checks). (Ref: `docs/architecture/mathematics/SDF_BEZIER_SHAPE_GENERATOR_LAW_REPLICATION.md`). **Verification correction (2026-08-17):** the first "done and verified" note was written while `tests/bezier_patch_law_test.cpp` did not compile — seven `std::cout << "\n[Test N]..."` lines had been written with literal newlines inside the string literal, so the file had never been built and the claim rested on reading the source. Literals repaired; the test now builds and all 30 checks pass. The implementation work described above was sound as written — only the verification was fictional.
