# 2D objects, `Singular`s, and UI created by 2D system — Shape2D render path & picking

**Status:** ✅ done and verified  
**Section in the To-Do list:** Person-facing surface  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **2D objects, `Singular`s, and UI created by 2D system — Shape2D render path & picking** — **done and verified (2026-08-25)**: Implemented screen-space 2D rendering and picking for `ShapeKind::Shape2D` objects. Added `x2D`, `y2D`, `zOrder2D` fields and accessors on `Object` with full PropertyPath registration (`x2D`, `y2D`, `zOrder2D`, `shape.width2D`, `shape.height2D`); added `Object::draw2DObject` using `Renderer::drawTris2D` / `drawLines2D`; added 2D draw pass in `EngineRender.cpp` inside `begin2D`/`end2D` sorted by `zOrder2D`; added pixel AABB picking in `InteractionChannel::observe` where 2D controls occlude 3D objects; early-outed `Object::raycastFace` for 2D objects; updated save/load serialization in `Serialization.cpp`; guarded by `tests/singularity/shape2d_test.cpp` (17/17 checks pass). Full test suite passes (68/68). Remaining 2D items: `Text2D` with font atlas, and layout laws over `Formation`s. (Ref: [INTERACTION_AS_LAW.md](../../architecture/law/INTERACTION_AS_LAW.md)).
