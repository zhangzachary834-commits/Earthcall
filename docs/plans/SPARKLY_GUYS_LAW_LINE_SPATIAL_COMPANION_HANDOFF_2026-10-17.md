# Sparkly Guys — finish the authored spatial Law Line companion

**From:** Codex, model unexposed in this runtime, session `92b79f696f1c`  
**To:** Zach's Sparkly Guys / receiving Gemini sessions  
**Human commission:** Zach's independent in-world spatial Law Line companion, commissioned 2026-10-15; handoff requested 2026-10-17.  
**New instruction from Zach:** “sparkly guys can do the screenshot verification u couldnt.”

This continues the [existing Forge/creation thread](../../agent%20intercom/communication-threads/rete-and-law/Second_Nature_Law_Forge_Handoff_2026-09-16.md) and [cloud prerequisite plan](LAW_LINE_SPATIAL_COMPANION_CLOUD_2026-10-16.md). The actual task is [Law Line Spatial Companion](../Agenda/Tasks/Specific%20Tasks/Interaction%20and%20Interface/Law_Line_Spatial_Companion/Law_Line_Spatial_Companion.md).

## Start with the truth about what you are inheriting

**The graphical instrument does not exist yet in this checkpoint. You are being commissioned to IMPLEMENT it, use it, and refine it—not merely take screenshots of an already finished graph.** The cloud pass stopped after one tested prerequisite. That was incomplete fulfillment of Zach's full commission; do not mistake its documentation or green tests for the requested experience.

Build a beautiful, genuinely useful, independent spatial authoring interface THROUGH THE LAW LINE ITSELF. It must show live Singulars and actual Relations; allow fluent pointer and keyboard navigation; and turn spatial attention into intentional drafts, previews, and committed changes through the existing compiler. The existing ImGui Law Authoring Window is not this interface. A mockup, hardcoded graph, or a few buttons stamping Laws is not the deliverable.

Zach is explicitly handing native screenshot verification to you because your receiving environment can do what this cloud pass could not. Establish your actual capabilities, then use them. Native evidence and repeated refinement are central work, not an optional final screenshot.

## Exact checkpoint and safe intake

| Item | Value |
|---|---|
| Repository | `zhangzachary834-commits/Earthcall` |
| Published work branch | `codex/law-line-spatial-companion-20261016` |
| Published checkpoint | `ee901fddba0d74e3787e564f6e24906cb65fe7ac` |
| Checkpoint tree | `ebf736fa0fbc8f551ab1694715921a45cf25bb7a` |
| Original canonical parent | `18456320d10f7bddfa0bc5074dd665f1092ec7fe` |
| Canonical branch at that time | `sync-from-earthcall-main` |
| Local prepublication commit | `501ed727` — same tested tree; use the published commit above for handoff |
| Merge status from this session | Nothing merged; no PR was created |

This handoff is a later documentation-only commit on that branch. Do not expect the branch tip to remain the checkpoint SHA. The full original cloud commission remains the scope; this document translates it into executable next steps.

Inspect your current repository status, branch, Intercom, and data-home status FIRST. Preserve local uncommitted work and concurrent agent edits. Do not reset, clean, auto-stash, switch a dirty checkout, regenerate a save, or overwrite an installed app just to get a clean-looking baseline. If isolation is needed, use a separate worktree/checkout based on inspected ancestry. Fetch the published checkpoint and inspect differences against current canonical before reconciling; do not blindly replace current files with this older branch. Do not merge into canonical as part of intake.

No Mac working files were available to the cloud pass. Its old cloud checkout, build directory, temporary UUID sysroot and raw logs were subsequently removed by workspace maintenance. The published source and dated audit survived. Recreate your own build; an old `/workspace/...` path is not a runnable handoff artifact. Do not report historical test results as your fresh execution.

**Do not spawn subagents.** “Sparkly Guys” identifies the receiving agents; it does not authorize delegation. If several existing sessions participate, claim one implementation owner through Intercom and explicitly separate files/worktrees and verification responsibility. Sign with distinct session IDs.

## Read before changing behavior

Read current `AGENTS.md` and all three required companions: `docs/AGENT_COMPASS.md`, `docs/BUILD_AND_ENVIRONMENT.md`, and `docs/ENGINEERING_DISCIPLINE.md`. Follow their current router and any nested instructions. Read:

- `docs/architecture/law/LAW_AUTHORING_CLI_GUIDE.md`
- `docs/architecture/law/LAW_AND_CREATION_SYSTEM.md`
- `docs/architecture/law/INTERACTION_AS_LAW.md`
- `docs/architecture/law/ALGORITHMS_AS_LAW.md`
- `docs/architecture/Design/Building 2D and 3D Apps with Earthcall Guide.md`
- `docs/plans/LAW_LINE_SPATIAL_COMPANION_CLOUD_2026-10-16.md`
- `docs/audits/LAW_LINE_SPATIAL_CLOUD_PREREQUISITE_2026-10-16.md`

Inspect the actual current Law Line, direct Screen authoring, pixel-art atelier, template shortcuts, interaction consumers, graph queries and save codecs. Earlier Forge specifications supply lessons, not a substitute application or a mandate to copy its buttons.

Use bounded Intercom navigation before appending a signed continuation to the existing thread:

```sh
python3 "agent intercom/conversation_history_injection.py" nav find "Law Line spatial companion" --limit 5
python3 "agent intercom/conversation_history_injection.py" nav trace "Second_Nature_Law_Forge_Handoff_2026-09-16.md"
```

## What the checkpoint actually implements

A general **compare-and-insert editor operation**, exposed to ordinary Laws through TerminalChannel. It inserts printable text at the existing cursor, preserves surrounding text, and never submits a sentence or registers a Law. It is not graph selection, address resolution, a template application engine, or an application installer.

Relevant code and witnesses:

- `src/Singularity/Terminal/LineEditor.hpp` and `.cpp`: `insertAtCursor(expectedDraft, expectedCursor, text)`.
- `src/Singularity/Terminal/TerminalChannel.hpp` and `.cpp`: request properties, consumption in `act()`, result projection and draft/cursor change notifications.
- `examples/law_line_cursor_insert.txt`: an ordinary authored shortcut Law using the actual compiler Metalaws.
- `tests/singularity/line_editor_test.cpp`, `terminal_zones_test.cpp`, and `law_line_zone_test.cpp`.

In one authored Sequence, stage these paths, then advance the request counter:

| Path on `@terminal-channel` | Role |
|---|---|
| `edit.insert.expectedDraft` | Snapshot of the draft being attended to |
| `edit.insert.expectedCursor` | Snapshot of the current byte offset; copy `cursor`, do not invent a glyph-to-byte conversion |
| `edit.insert.expectedZone` | Snapshot of Terminal's line Zone |
| `edit.insert.text` | Explicit printable text to insert |
| `edit.insert.requests` | Advance once after staging the request |
| `edit.insert.handledRequests` | Read-only consumed counter |
| `edit.insert.status` | Read-only success/refusal |

`draft` and `cursor` stay read-only. Successful insertion announces their existing property changes. Stale draft/cursor, wrong line Zone, secret entry, pending confirmation, multiline blocks, active history search/paste, invalid cursor and non-printable text refuse. Unicode insertion leaves the cursor on a display-cluster boundary. The operation does not promise to preserve a draft across app restart.

The example binding uses Ctrl+Alt+I, CSI-u `ESC [ 105 ; 7 u`, and inserts the literal fixture address `@law-line-cube`. **Replace that fixed demonstration with real, unambiguous selection/context in the instrument.** Repeating the example sentence creates duplicate binding Laws. It is not an idempotent bootstrap. Terminal chord delivery must be checked physically; a decoder test is not proof your terminal sends that chord.

One real friction finding: bare `zone` parses as the structural Zone condition. The tested example therefore uses explicit `@terminal-channel.zone`. Reuse qualified paths rather than copying the failed spelling.

## Architectural boundary: keep the application authored

State, appearance, viewport policy, layout, bindings, navigation, templates and authoring behavior belong in properties, Relations, Formations, Laws and Metalaws. Do not create GraphEditor, GraphCanvas, Widget or another application C++ class. Do not hide the application behind one C++ callback invoked by an impressive Law sentence.

Before ANY new C++, record four concrete things: the human operation that cannot be expressed; existing mechanisms inspected and why they fail; the smallest reusable sensing/actuation primitive needed; and the ordinary Laws that read/govern it. The checked insertion seam above has that evidence in the cloud plan. A missing sentence spelling generally calls for reusable Lexeme/Metalaw vocabulary, not a private graph language.

Interaction already has wheel deltas/events, keys, focus, press/drag/cancel, normalized pointer coordinates and pointer lock. Inspect actual wheel/key/focus/foreign-panel behavior. Zoom should ordinarily be an authored interpretation of those inputs changing a viewport transform. Graph navigation must not move the embodied Person.

Universe already has `relationsInvolving` and graph-generation mechanisms. Their existence in C++ is not proof that every required Law Line query is currently expressible: trace and test the authored route. Reuse indexed/incremental mechanisms; do not sweep the entire world every frame or add another layout engine without inspecting existing algebra and consumers.

Relation direction and grounded type identity matter. Relations are inspectable beings. Properties are predicates of their bearers, addressed by PropertyPaths; do not turn attributes into a new species of Singular. Do not introduce a category enum or force multiple memberships/cross-links into a tree. Keep representation/layout state separate from referents: dragging a drawn representative must not move the underlying Object.

No fixed author identity. The installed program must retain the actual submitting Person's authorship. AI/agent execution is not Person registration; use the existing authorized First Mover route when applicable. Protect existing body-consent and authority boundaries.

## First deliverable: one complete authoring journey, then iterate

Implement this before expanding the feature set:

1. Deliberately open the interface and see a real, relevant neighborhood.
2. Find and select a live being with legible identity and current context.
3. Inspect an actual Relation, including its identity, direction/type and endpoints.
4. Follow it and return, retaining spatial context and the current draft/cursor.
5. Explicitly insert the resolved selected address or bearer property path into the draft. Selection alone must not retarget an existing sentence.
6. Inspect/preview separately from committing. Commit through the current compiler and authored Metalaws, then verify the actual intended subject's behavior.
7. Describe what the commit created or changed and show the result in the graph.

Use both Law Line and graphical surface yourself. Record a specific friction, repair it, repeat the same journey, and compare. Improve general completion, insertion, diagnostics and preview when they are the obstacle. Do not pile on features to avoid fixing an unpleasant selection/draft interaction.

## Broaden into a usable instrument

Author a coherent pointer/keyboard language covering hover/select; pan/scroll; attention-centered zoom where feasible; frame/focus; follow/back; enter/leave meaningful groupings; home/recovery; and fast keyboard access across kinds, categories, Formations, individual Singulars and neighborhoods. Context hints, mnemonic paths and incremental search are design options, not preapproved engine commands.

Add progressive disclosure and bound visible work. Layout must be stable enough for spatial memory; unrelated updates should not shuffle nodes beneath the pointer. Make direction/type, current subject, focus and selection legible beyond color. Keep text readable and targets generous. Protect pointer capture, cancellation, OS focus loss and coexistence with surrounding world controls. Keep drafts intact across navigation, and distinguish drafting, inspection, preview and commit visibly.

Support context-to-authorship: explicit address insertion; a two-being Relation draft; property-path insertion; inspection of relevant Laws and their targets/conditions; navigation to beings referenced by a draft; and stored Lexeme templates applied to explicit context. Refuse ambiguity rather than guessing from a display name.

**Wayfarer** was my proposed name/metaphor: a stable neighborhood map, visible return path and quiet sentence workbench. It has no implementation or Zach acceptance. You may develop a better artistic proposal. Explain how typography, space, geometry, color and restrained motion clarify relationships and consequences. Do not attribute your choices to Zach or declare enjoyment from green tests. Serve his Person-centered, Christ-centered purpose through truthful understanding, agency, stewardship and shared creation.

## Native screenshot and input verification — this is your advantage

Confirm the executable you launch is built from the intended checkout/commit. An existing installed Earthcall.app can be stale or sourced elsewhere; a screenshot must name the process/build actually observed. Use the current repository-supported WebGPU launch workflow, a real TTY for the Law Line, and a supported isolated data home/fixture. Verify the active save root: setting an environment variable without the required home metadata can fall back to checkout saves. Follow the current build/data-home documentation rather than assuming a temporary path is isolated.

Use actual mouse/keyboard control wherever your harness supports it. Do not merely inject `object-clicked` and call it a physical click. If the tool only supports offscreen Engine captures, record that limitation separately and retain the physical-input checks.

For each refinement cycle record exact commit, build/backend, fixture/save root, viewport and display scaling, action sequence, expected/observed outcome, capture filename and remaining failure. Capture the real application and, where relevant, the real Terminal draft/result. Preserve useful before/after views or a short recording. Do not manufacture a browser/SVG/imagegen mockup and label it Earthcall. A screenshot establishes pixels, not a usable workflow; capture plus actual input and runtime readback establish different parts of the claim.

At minimum record opening/orientation, selected being with actual Relation, follow-and-return, draft before/after insertion, preview vs commit, actual changed result, and reopen/restart. Inspect every capture and repair clipped labels, overlap, unreadable text, weak selection, unstable layout or broken zoom. Do not stop after the first nonblank image.

## Required journeys and evidence

| Journey | Exercise and verify |
|---|---|
| A — Orientation | Open, find a real Singular, inspect its relationships, follow one and return without lost context. |
| B — Keyboard fluency | Reach a grouping/category, individual and related neighborhood using keyboard mode alone; preserve focus and draft. |
| C — Authorship | Compose a useful Law from spatial selection, preview, intentionally commit through Law Line, and verify behavior on the exact subject. |
| D — Live coherence | Create/change a relevant being/Relation; verify membership, label/edge updates and stable unrelated layout through actual consumers. |
| E — Continuity | Close/reopen; save/restart through supported workflow; verify underlying work and each promised view-state field. Reinstall/update without duplicates or erasure. |
| F — Scale | Use representative graphs larger than the teaching fixture; measure responsiveness and visible-work bounds. Separate cold build/admission from steady navigation. |

Common actions should aim for one to three deliberate gestures after learning. Test and count real tasks before making that claim. Provide a roughly-one-minute route to a first meaningful authored result for a new Person; agent walkthrough evidence is not Zach/new-Person acceptance.

Draft/preview/commit, secret input, confirmation, losing OS focus, cancelling a drag, pointer entry while held, resize/Retina coordinates, foreign-panel capture and unrelated world actuation all need coverage appropriate to the chosen surface. Readback is not enough if a dependent Law stays deaf; test notifications and the consumer too.

## Historical evidence you may reuse only as historical

On the published checkpoint, cloud Linux CPU/isolated fixtures passed:

| Witness | Dated result |
|---|---|
| `line_editor_test` | Passed; 0.88 s in corrected CTest run |
| `terminal_zones_test` | Passed; 0.59 s |
| `law_line_zone_test` | Complete 289-check path passed; 110.99 s |
| Compiler/template-only path | 42/42 checks passed |
| Three selected CTest targets | 3/3 passed; 112.46 s total |

These are recorded in the audit, not fresh native measurements. No graph UI, native mouse hit-testing, screenshot, persistent view, scale timing or Person acceptance was established. Preserve this distinction in your report. Rebuild/rerun for your reconciled commit.

After following the current configuration recipe, focused target commands are:

```sh
cmake --build build --target line_editor_test terminal_zones_test law_line_zone_test -j4
ctest --test-dir build --output-on-failure \
  -R '^(line_editor_test|terminal_zones_test|law_line_zone_test)$'
EARTHCALL_TEMPLATE_TEST_ONLY=1 python3 tests/singularity/run_law_line_zone_test.py \
  ./build/law_line_zone_test
```

Build `earthcall_webgpu` separately for the actual visual/input loop. Test selection must grow with your real changes; these three tests are a prerequisite regression set, not coverage of the instrument you still need to build.

Cloud-specific pitfalls, if you use another cloud builder: CMake's unanchored `scratch/.*` exclusion matched absolute `/workspace/scratch/...` source paths and omitted engine code; place a fresh checkout outside that ancestor. Vendored GLFW 3.4 null, Linux OpenSSL and extracted UUID headers admitted CPU testing. Those workaround paths/libraries are not Mac build instructions. The checkpoint plan contains the historical recipe.

## Persistence and final handoff

Deliver the readable Law Line source that really creates the application. Prefer a manageable bootstrap plus reusable authored templates/compositions. A generator is allowed only when generated sentences pass through the actual compiler/Metalaws and runtime behavior lives in authored data. Installation, reopening and updating must be deliberate and idempotent. Do not substitute repeated `called "same name"` submissions for an update protocol; names do not replace Law identities.

Patch existing saves under the current app persistence rules; never regenerate Zach's data. Use isolated fixtures first. Preserve backups/concurrent edits, loader round-trip and exact authorship, and report which files/beings were changed. Direct Screen output has existing ownership/composition limits; opening the instrument must not silently destroy another authored canvas or lose its restoration state. Persistence of a registry value or pointer is not proved until the appropriate codec and restarted consumer agree.

Keep high-stakes unresolved authorial choices explicit and continue independent work around them. Do not invent permissions, identity defaults or a new ontology. Ordinary artistic/interaction choices are proposals you are authorized to develop; they do not all require another approval stop.

Before finishing, deliver implementation, install/reopen/update instructions, reusable vocabulary/Metalaws and Law Line improvements, evidence for each C++ gap, focused tests, real captures/recordings, a short human interaction guide, updated task/Intercom and concrete checks in `docs/Agenda/Tasks/For Zach/Person Verification List.md`. Distinguish implemented behavior, measured evidence, your design judgment, and Zach's personal confirmation. Do not merge canonical without the applicable authorization.

**Receiving instruction:** continue until the instrument is working and maintainable, not merely until a build finishes. Your first progress report should identify the exact checked-out commit, actual native-input/capture capability, isolated data root, the present substrate gaps, and the vertical slice you are implementing. Then enter the implementation/verification loop—do not end at a plan or ask Zach whether you should start.

**Handoff written:** 2026-10-17T07:59:47.569210+00:00 · Codex / model unexposed / session `92b79f696f1c`. Documentation only; no new runtime execution or acceptance claimed.
