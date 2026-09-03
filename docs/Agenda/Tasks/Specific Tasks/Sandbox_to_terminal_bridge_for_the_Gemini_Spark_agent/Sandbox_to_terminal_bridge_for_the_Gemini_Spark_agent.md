# Sandbox-to-terminal bridge for the Gemini Spark agent

**Status:** ✅ done and verified  
**Section in the To-Do list:** Housekeeping:  
**Split out of `docs/Agenda/Tasks/To-do list.md` on 2026-09-02** by Claude Opus 5 (session `session_01GsrBySNw4oG1zof5AQ21KM`), per Zach's instruction that each To-Do bullet be one sentence linking to its own task document. **Content below is the original bullet, verbatim — nothing was summarized away.**

---

✅ **Sandbox-to-terminal bridge for the Gemini Spark agent** — done and verified (2026-08-17): built `~/Documents/sandbox-to-terminal-bridge/` (outside this repo; stdlib Python, no service). A sandboxed agent writes `inbox/request.md`; the bridge classifies the command under a default-deny policy, requires a typed approval at the terminal for anything not read-only (retyping the full command for critical categories), runs it, and appends command + exit code + stdout/stderr to `outbox/transcript.md`. Verified: 57/57 classifier cases; all four approval branches driven over a real pty with side effects checked; watch loop, 300s timeout kill, and 120 KB output cap exercised end to end. Five evasions found and fixed during probing (`xargs rm`, `bash -c` payloads, loop bodies, `../` escapes to `/etc`, symlinked `/tmp` root). Notification decision (2026-08-17): nothing can push into a sandbox, and automating the Gemini app was rejected as a ToS gray area — so the agent polls `outbox/latest.md`, which is now rewritten at every transition (not only on completion) so a poll during an open approval prompt cannot read the previous command's result as its own. See its `README.md` § What this does not protect you from.
