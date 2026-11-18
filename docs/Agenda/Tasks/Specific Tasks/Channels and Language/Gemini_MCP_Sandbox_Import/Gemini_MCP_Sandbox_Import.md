# Gemini MCP sandbox import

Zach asked to bring Gemini's work from the Earthcall application-support folder
into his checkout without copying each file by hand. The
[local import command](../../../../../../README_GEMINI_MCP.md#bring-geminis-changes-into-your-checkout)
uses the broker's creation manifest to find the session delta. It prepares a
regular-file Git patch and preflights the batch in the originating checkout.
The Person applies the patch locally. The remote MCP retains its disposable workspace.

The live `horizon-resurrection` preview passed Git's apply check for three Intercom
files and `scripts/horizon_resurrection.py`. The `mcp-write-ping-test` preview
passed for `SANDBOX_PING_TEST.md`. The import tests exercise the real CLI through
temporary Git checkouts and cover preservation of dirty state, binary transfer,
deletions, scoped apply, retries, and conflicts. The live sessions were previewed.
Their selection for apply is in the
[Person Verification List](../../../For%20Zach/Person%20Verification%20List.md#gemini-mcp-sandbox-import).

*Codex · GPT-6 · session `01a1e178-dec7-7060-b866-efbe0035660d` · 2026-11-15 15:16 PST.*

## Mac companion

Zach asked for a Finder-launchable app with a GitHub Desktop handoff.
`scripts/macos/build_sandbox_import_app.sh` builds the SwiftUI companion against
the originating checkout. It lists sessions, shows the preflighted diff, and
applies only the exact patch the Person reviewed. After applying, it opens the
same checkout in GitHub Desktop through the bundled launcher.

Verified the installed app from `~/Applications` on macOS 26.6. The live Horizon
preview showed all four files, and the desktop handoff opened Earthcall on
`sun/gemini-sandbox-lab-v1`. An isolated CLI test verified that an update after
preview blocks apply and that the refreshed preview imports. The live apply
remains a Person choice.

*Codex · GPT-6 · session `01a1e178-dec7-7060-b866-efbe0035660d` · 2026-11-15 15:45 PST.*

## Repeated appends

Zach confirmed the Mac import worked, then reported that Horizon's second pass
hit a baseline conflict on the All-Hands broadcast. The checkout's 72,548 bytes
were an exact prefix of the sandbox file. The importer now accepts an extension
of an existing baseline file when the checkout is at least as long as the
baseline and every checkout byte is present at the beginning of the sandbox
file. Divergent edits still stop the batch.

The live preview for `horizon-resurrection` shows 29 added broadcast lines
alongside three new Tetra-Craft files. The regression test runs two imports
from one session, checks that the second diff contains only the new text,
and verifies that separate checkout and sandbox replies still conflict.

*Codex · GPT-6 · session `01a1e178-dec7-7060-b866-efbe0035660d` · 2026-11-15 22:01 PST.*

## Checkout transplants and shared baselines

Zach asked whether existing sandboxes receive checkout updates, then commissioned
“merge transplants from the main checkout's diffs over like Git does.” The Mac
app now selects **Checkout → Sandbox** or **Sandbox → Checkout**. The local
`sync-preview` and `sync-apply` commands share Git's `merge-file --diff3`
with ordinary imports once a session has retained baselines or checkpoints.

Independent text edits merge. Overlapping text edits show conflict markers,
and add/add, edit/delete and binary conflicts show explicit file choices.
The Person can keep the destination file or take the source file, then review
the new diff before applying. A matching digest binds the source, destination,
baseline and choices to that preview. Source snapshots include uncommitted files.
Renames are handled through additions and deletions. Symlink changes and
colliding filename changes require manual reconciliation.

New sessions retain exact creation bytes outside the Docker-mounted workspace.
Older sessions bootstrap checkpoints from an unchanged side or a shared prefix
of an append-only thread. A missing common ancestor becomes a visible conflict.
After a transplant, checkpoints retain the source version so the destination's
independent edits stay available for the reverse transfer.

Before/after content objects and a transaction journal preserve recovery state
for an interrupted write. A retry verifies each file against those snapshots,
resumes the unfinished batch, then advances the checkpoints. Live owners lock
out concurrent transplants. An exited owner's lock can be recovered.

The focused suite exercises the real CLI through temporary checkouts and
session creation. It covers independent edits across two upstream passes and
a reverse import, overlapping edits and whole-file choices, binary bytes,
deletions, executable bits, preview races, legacy sessions, and an injected
mid-batch write failure followed by recovery. All 31 Gemini MCP checks passed
with `TMPDIR=/private/tmp npm run mcp:gemini:test`. The installed app was opened
and its **Checkout → Sandbox** selector, per-file choices and blocked apply button
were verified against Horizon's real conflicts. The live Horizon preview produces
93 clean file diffs and three conflicts in the broadcast and Tetra-Craft saves.
The sandbox's live merge decisions remain with Zach.

*Codex · GPT-6 · session `01a1e178-dec7-7060-b866-efbe0035660d` · 2026-11-17 13:15 PST.*
