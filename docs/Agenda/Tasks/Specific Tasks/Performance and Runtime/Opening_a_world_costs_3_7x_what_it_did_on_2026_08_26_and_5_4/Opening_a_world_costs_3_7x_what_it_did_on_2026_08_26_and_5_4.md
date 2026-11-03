# Opening a world costs 3.7x what it did on 2026-08-26, and ~5.4 s of that is unexplained

**Status:** open  
**Section in the To-Do list:** Performance (opened 2026-08-24 by `tests/singularity/frame_lag_test.cpp`)  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**Opening a world costs 3.7x what it did on 2026-08-26, and ~5.4 s of that is unexplained.** (opened 2026-08-31.) `load.ms` was 1868.93 on 08-26 and measures **6024 / 6440 / 6791 / 6931 ms** over four steady runs now (drift 1.00-1.06), against a 4000 ms aspiration; baseline re-recorded to 6950 with the full derivation in the file's header. Of the increase, ~1200 ms is bought deliberately — the field mesh resolution restored from 160x4x160 to 128x24x128 for Bugs.md #12 — and was A/B'd on this machine at 5.4-5.6 s coarse vs 6.4-6.9 s restored. **The other ~5.4 s landed somewhere between 08-26 and now and nobody has looked.** Two leads: (a) that A/B could only have moved if the *lazy* field tessellation is still being forced somewhere on the hydration path, so the "104 s -> 0.4 ms" claim on item 2 above holds for `setFieldShape` but not for the whole load; (b) an earlier 1375 ms reading was not a fast load at all — boot was throwing (Bugs.md #13) and the load was abandoning partway, so any load timing recorded while that exception was live is worthless. The real answer to the 1.2 s is item 1c, sparse surface-following tessellation, not sampling the terrain less.
