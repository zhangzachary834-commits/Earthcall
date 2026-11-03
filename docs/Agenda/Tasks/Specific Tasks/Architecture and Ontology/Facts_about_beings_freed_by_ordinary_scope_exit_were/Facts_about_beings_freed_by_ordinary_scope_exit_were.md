# Facts about beings freed by ordinary scope exit were dangling

**Status:** ✅ done and verified  
**Section in the To-Do list:** R&D:  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Facts about beings freed by ordinary scope exit were dangling** — found 2026-09-01 (the fix above made it reachable, and `rete_compile_test` segfaulted). `ReteFact` holds RAW participant pointers and outlives the round that asserted it; `ReteNetwork::retractFactsAbout` exists for exactly this but was only called from the law-driven unmaking path, so a stack-local `Object`, or a `Law` and the provenance `Relation`s it owns, left facts pointing at reclaimed memory. Added `Singular::setBeingReleasedCallback` / `notifyBeingReleased`, called from `~Singular` (which runs after every derived destructor — the listener gets a pointer and nothing else; no virtual call, `getIdentifier()` least of all). `LawManager` installs it, uses the returned orphaned-subject ids to forget `_seededSubjects`, and now has a **destructor** that puts the static `Singular` hooks back if it is the one that installed them — without which a block-scoped `LawManager` dies before the beings declared above it and their destructors call into freed memory, which is the shape of every test in this tree.
