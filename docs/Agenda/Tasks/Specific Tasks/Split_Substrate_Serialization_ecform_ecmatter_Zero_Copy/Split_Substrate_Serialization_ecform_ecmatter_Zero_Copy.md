# Split Substrate Serialization (`.ecform` + `.ecmatter`) & Zero-Copy FlatBuffers (2026-09-01)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Near-term priorities (2026-08-14 — from architecture review):  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Split Substrate Serialization (`.ecform` + `.ecmatter`) & Zero-Copy FlatBuffers (2026-09-01)** — done and verified (2026-09-01): Implemented split substrate serialization decoupling semantic text laws/identities/attributes (`.ecform` / lean `.json`) from physical binary geometry (`.ecmatter` via zero-copy FlatBuffers). Purged Base64 textures and obsolete `BinaryPack`; expanded `Earthcall.fbs` schema for Polyhedron, BezierPatch, SmoothSurfaceData, FaceTextures, and Transforms; unified `ZoneManager` save/load with two-step hydration (Step 1 Semantic Skeleton -> Step 2 Physical Matter injection); added automatic transparent legacy JSON splitter on load; added `tests/singularity/substrate_split_test.cpp` guarding roundtrip, semantic purity, and transparent migration. Verified: 81/83 ctest pass (only known `smooth_tessellation_cache_test` pre-existing failure and machine load-sensitive `frame_lag_test`). See `docs/plans/SPLIT_SUBSTRATE_SERIALIZATION_PLAN_2026-09-01.md`.
