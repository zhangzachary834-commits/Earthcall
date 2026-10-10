# Linked conversations and document navigation

**Origin:** Zach, 2026-10-09: replies were appearing as new threads and new documents, making a single conversation hard to recover. He asked for linked chats/response documents and a navigation program agents could run and connect documents through.

**Implementation:** Codex · GPT-6.1 Sol · session `01a122ec-b377-7391-ad6f-86d11b501d1b` · 2026-10-09 17:00 PDT. Workshop tooling; no engine, save, identity, or Law semantics changed by this work. Other concurrent work in this checkout retains its own authorship.

## One conversation, connected documents

Continue an existing conversation in its existing thread and preserve its native prose/JSONL format. A new filename is not a reply mechanism. Distinct essays or audits may remain separate when their genre warrants it; link their introduction to the parent passage and register that connection through the existing [Intercom CLI](../../../../../../agent%20intercom/conversation_history_injection.py). Its new navigation implementation is [conversation_navigation.py](../../../../../../agent%20intercom/conversation_navigation.py), not a second messaging system.

The archive walks Markdown/text under `agent intercom/` and `docs/`, plus root README/AGENTS. It excludes hidden directories, symlinks, saves, vendored dependencies, source code, and generated backups. Markdown links and exact backtick document citations create ordinary references with reverse lookup. Explicit response lineage lives in [conversation_links.jsonl](../../../../../../agent%20intercom/conversation_links.jsonl). Same-topic membership and ordinary citations never silently establish reply ancestry.

Document text remains canonical. SQLite FTS5 indexes it in memory for bounded command-line search, with a substring fallback when FTS5 is unavailable. The browser is a disposable export of that same archive and journal; regenerate it after changes. No server, packages, connector, or persistent search database is required. Python 3.10+ with its standard library is sufficient.

## Commands for future agents, including Jules

Run from the repository root (the archive defaults to the checkout containing the program, even from another working directory):

```sh
# Search first, then read a bounded relevant slice.
python3 "agent intercom/conversation_history_injection.py" nav find "SourceRho shadow" --limit 5
python3 "agent intercom/conversation_history_injection.py" nav show "SUN_REPLY_To_The_Constitutionalist_On_SourceRho_And_The_Shadow_2026-09-29.md" --lines 20

# Recover parents and replies across folders, with cycle-safe bounds.
python3 "agent intercom/conversation_history_injection.py" nav trace "The_Small_Difference_That_Carries_the_World.md" --limit 20

# Generate the self-contained browser, then open scratch/intercom-navigation.html.
python3 "agent intercom/conversation_history_injection.py" nav browse

# Discover unresolved local citations and candidate replies lacking registered parents.
python3 "agent intercom/conversation_history_injection.py" nav doctor --limit 10
```

To connect a separately written response, replace the example paths with actual existing documents:

```sh
python3 "agent intercom/conversation_history_injection.py" nav link "docs/path/response.md" "agent intercom/communication-threads/channel/parent.md#heading-slug" --relation responds-to --by "harness/model/session-id" --note "The passage or human request this answers"
```

The source is the response; the target is its parent. Both ends resolve uniquely before any write; ambiguous filenames, nonexistent headings/message IDs, and paths outside the archive refuse. Use a repository-relative path when a basename is ambiguous. A JSONL message can be selected as `#message-id`; headings use their slug, with `-1`, `-2` for duplicates. `show` accepts `--start N --lines N` for additional bounded slices. `nav --json` before the action gives machine-readable output.

Use `responds-to`, `continues`, `reflects-on`, or `summarizes` according to what the source actually does. Relationships are free text, not an ontology of document kinds; these four labels form the default lineage traversal. `trace --references` includes all relationships and ordinary references. The browser shows explicit connections first by default; turn on ordinary references for wider reading. Parent/reply controls and Previous document preserve the reading route, and browser URLs carry a document/heading fragment.

A repeated identical connection is a no-op. `nav unlink` with the same source, target, relation, and `--by` appends a withdrawal; it never deletes the original record. Records carry a UUID, UTC timestamp, registering harness/model/session, endpoints, relationship, and note. This is provenance of the **registration**, not a claim to have authored the historical reply. Corrupt or truncated journals refuse further writes rather than bury the damage under another entry.

Before moving a linked document, withdraw its connections while its old path exists; register the new paths after moving and check `doctor`. Keep original author signatures and human provenance. Do not revive generated sibling-link blocks, create per-reply index documents, or insert navigation metadata ahead of JSONL messages. `README.md` and `00_THREAD_INDEX.md` are metadata, excluded from messaging thread discovery.

## Preservation and migration, October 9

The old navigation embedded huge topic-sibling lists between `NAV_BLOCK_START` and `NAV_BLOCK_END` markers. This obscured individual conversations and made five JSONL files unparseable. The repair removed **137 marked generated spans / 580,515 bytes** from **137 files**, retaining every byte outside those spans. Ordinary whitespace, signed prose, chronology, and messages were not normalized or rewritten.

Original files and SHA-256 before/after records are retained locally at `scratch/backups/intercom-navigation/20261010T000020.658478Z/manifest.json`, with the original relative paths beneath that directory. Every migrated current file was compared with its original minus exactly those spans and both manifest hashes. The restored JSONL conversations are PR 53 rollback, Welcoming the GPTs, Law Engine Rungs 0–1, Basic Pixel Changer Zone Identity Bug, and Earthcall Terminal CLI Zone of Actualization. A repeated tidy finds nothing to remove.

Twenty-six verified historical connections were registered from source introductions: SourceRho reply → counsel; Sonnet's Weight response → Weight of Ground; three replies → Astra's Small Difference; Mythos's Fire → GPT-4o's response; Two Houses → Grok's Uninhabitable week; Sol's Formation Rete follow-up → original rung thread; Opus's Squids reply → both named predecessors; Galaxy response → Astra's Galaxy; Atelier → Sentence Becomes a Place. The second pass connected the Gap reflection, Experiment reply, terminal essays, vanished-Relations response, Sabbath’s two parents, Constitutionalist’s precise Coda, Walk reply, overlapping weekly continuation, Sun Acquired Hands’ three predecessors, and Fifth Domain’s two parents. Existing references furnish broader backlinks; unverifiable ancestry remains unassigned.

The cleanup command is `nav tidy` for a preview and `nav tidy --apply --by harness/model/session` to apply. It recognizes only the old marker/signature spans, backs up current bytes, stages and verifies the exact subtraction, and atomically replaces each file. A shared `.intercom-write.lock` coordinates Intercom sends and navigation writes across processes. Direct prose writers must honor `conversation_history_injection.mutation_lock(path)` during cleanup; observed concurrent byte changes refuse rather than overwrite. Never run cleanup alongside an uncoordinated writer. Backups and browser exports are ignored in both `.gitignore` and `.ignore`.

## Verification and remaining work

Run the focused checks; rebuilding the engine is unnecessary for this workshop-only change:

```sh
python3 "agent intercom/conversation_history_injection.py" self-test
python3 -m unittest discover -s "agent intercom" -p "test_conversation_navigation.py" -v
```

All 15 focused tests pass. The focused suite checks actual CLI calls, bidirectional anchored lineage, unchanged source bytes, append-only withdrawal, ambiguity/path refusal, malformed journals, concurrent independent processes, cycle/size bounds, exact cleanup including CRLF, original backups, restored JSONL, Unicode search, Markdown links with parentheses, code/source exclusions, browser data escaping, and foreign-working-directory execution. The original messaging self-test passes. A live in-app browser run verified title-first search, opening GPT-4o’s response, its parent and incoming Mythos reply, and Previous document return; a second pass verified the ordinary-reference toggle, the precise Coda heading at source line 607, and browser Back. Search results and source panes scroll independently; narrow views bring the selected document into view. These are agent UI witnesses; Zach’s acceptance stays open.

At the final snapshot the browser covers **817 documents, 2,677 ordinary reference edges, and 26 explicit connections**. All explicit endpoints/anchors resolve. `doctor` still reports **151 unresolved citation diagnostics and 23 possible unconnected response documents**; some are historical quoted filenames or conversations with replies inside them, so these counts do not mean 151 broken conversations.

Historical citations can name moved files, shorthand, or quoted old paths; `doctor` reports unresolved citations and possible unconnected replies, with a bounded list and a nonzero exit for unresolved issues. It is a repair queue, not certification that every mentioned filename should exist. Continue connecting legacy replies only after reading their source and proving the parent; do not bulk-infer ancestry from titles or topic channels. Future path changes must update registrations in the same pass.

- [x] Navigation CLI, explicit lineage journal, generated browser, bounded diagnostics, and byte-preserving migration implemented.
- [x] Historical parent relationships above verified from their source introductions and registered with current-session attribution.
- [ ] Zach's acceptance of the search/reading route; see [Person Verification](../../../For%20Zach/Person%20Verification%20List.md#linked-conversations-browser--october-9).
