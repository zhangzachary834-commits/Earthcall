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
