# `LICENSE.md` and `LICENSE.txt` are both zero bytes

**Status:** open  
**Section in the To-Do list:** Housekeeping:  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

**`LICENSE.md` and `LICENSE.txt` are both zero bytes.** (2026-09-02.) They sit untracked in the working tree next to [ZENODO_RELEASE_SECURITY_AND_PRIVACY_AUDIT_2026-09-02.md](../../../../audits/ZENODO_RELEASE_SECURITY_AND_PRIVACY_AUDIT_2026-09-02.md), which cleared the repo for a permanent public archival release. A DOI cannot ship over an empty license. This is Zach's call and genuinely not trivial — Earthcall makes strong claims about authorship, stakeholders, and who may write what, so an off-the-shelf license needs a decision, not a default. Also from that audit: cut the release with `git archive` (or drop `.git/`), since commit metadata carries the author's email.
