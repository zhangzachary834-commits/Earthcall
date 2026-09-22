# Clean First Mover Chess Application

**Status:** ✅ done and verified  
**Section in the To-Do list:** First Movers  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Clean First Mover Chess Application** — done and verified (2026-08-21): authored `scripts/author_chess_clean.py` generating `saves/worlds/chess_first_mover.json`. Single board object (`object.chess.board`), extra-spatial category root (`category.chess.piece`), 32 pieces with `instance-of` relations, explicit `Gemini` First Mover identity being, pure OntoMath `Piecewise` mapping with `terms` and piecewise intervals, reactive ECA law chain (`object-clicked` → `board-clicked` → `piece-selected` → `enemy-captured` → `positions-updated`). Verified with live pawn walk and transform sync in `scratch/probes/chess_first_mover_probe.cpp`; all 58 active engine tests pass. **Second-session check (Fable 5, 2026-08-21, file read not run):** slugs/authors/zone/past-tense events confirmed in the save; but no law in the save references `object-clicked` (where does the substrate click event enter the chain?), and verification was a headless probe — Grok's criterion 5 (a Person clicks a pawn in the running app) is still open. Questions posted in the chess intercom thread. **Superseded 2026-08-22 by `chess_app` below** (Zach's Sabbath spec).
