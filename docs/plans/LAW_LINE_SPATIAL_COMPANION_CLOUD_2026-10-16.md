# Law Line spatial companion: cloud execution and continuation

This implements the first prerequisite of [Zach's full commission](../Agenda/Tasks/Specific%20Tasks/Interaction%20and%20Interface/Law_Line_Spatial_Companion/Law_Line_Spatial_Companion.md), not its requested final instrument. Continue the existing [Forge conversation](../../agent%20intercom/communication-threads/rete-and-law/Second_Nature_Law_Forge_Handoff_2026-09-16.md), whose lesson is precisely that a few working buttons are not a usable instrument.

## Cloud checkout and concrete limits

Remote Git read succeeded at `18456320d10f7bddfa0bc5074dd665f1092ec7fe`, default branch `sync-from-earthcall-main`. Work branch: `codex/law-line-spatial-companion-20261016`. This is a fresh cloud checkout; no Mac working tree or uncommitted work is assumed present. No subagents were used.

Checkout moved to `/workspace/earthcall-cloud-92b79f696f1c` because CMake's unanchored `scratch/.*` exclusion matches every source in an absolute `/workspace/scratch/...` path, producing undefined references instead of compiling core sources. Do not change source filtering just to get this task through; the move is sufficient. CMake/Ninja were installed in the cloud user environment. Vendored GLFW 3.4 supports a null backend; the default fetched 3.3.8 attempts X11 despite the newer configuration switches. System libssl is Linux-compatible; checked-in Mac static libraries are not. UUID headers were downloaded and extracted to a temporary sysroot, without changing repository dependencies.

This environment exposes no `/dev/dri`, native mouse/keyboard automation, or desktop window. Native physical input and GPU captures remain unavailable evidence, not passing tests or synthetic substitutes. Apt installation also failed against system cache permissions; extracting individual development packages is available. A software display would not establish native Metal parity.

## Demonstrated C++ substrate gap: editor insertion

**Human operation:** insert an explicitly chosen address or property path into the existing draft, preserving everything before and after the cursor, without registering a Law.

**Mechanisms inspected:** Terminal `draft`/`cursor` are read-only; completion open requests read stored templates and replace the matching prefix; LineEditor `prefill` replaces the entire buffer; ordinary key insertion is private and device driven. Completion therefore cannot truthfully serve as exact, non-destructive insertion at an arbitrary retained cursor.

**Smallest primitive:** compare-and-insert text into the existing LineEditor. No graph, layout, selection policy, template meaning, or target resolution lives in C++.

**Law governance:** stage `edit.insert.expectedDraft`, `.expectedCursor`, `.expectedZone`, and `.text`, then advance `.requests` in one Sequence. `TerminalChannel::act` consumes the request after Law decisions. `.handledRequests` and `.status` expose the result. Insertions never call `speak`, submit, or adopt a Law. The draft and cursor notify their normal consumers after success. This transient transport state is not a promised persistent application view.

Example: `examples/law_line_cursor_insert.txt`. Paste it once in an isolated LawLine fixture. Ctrl+Alt+I (CSI-u `ESC [ 105 ; 7 u`) inserts the explicit example address into the current draft. The binding is itself an ordinary Law compiled by the existing Metalaws. Terminals may intercept the chord; transport must be configured before calling it a physical success. Repeating this sentence creates another binding Law; this example is not an idempotent application installer. Delete the exact binding Law after experimenting, using the existing confirmation route.

## Inspected mechanisms for the next increment

- Interaction already senses wheel/key/focus/press cancellation, normalized pointer coordinates and pointer lock. Wheel deltas drain per frame; key repeats do not create fresh press edges. Do not add a Zoom engine command. Authored viewport transforms interpret these inputs.
- Universe already offers `relationsInvolving` and graph-generation metadata. Relation exposes `entityA`, `entityB`, `directed`, and grounded `type`. Do not introduce a graph manager or classify categories by a new enum.
- The atelier demonstrates authored direct Screen fields and capture guards. Its single composed field is not automatically a multi-application compositor. Text2D is a legacy manifestation with known limitations; rich fields do not imply legible native text.
- Stored templates use authored shelf paths and compiler Metalaws. Prefix completion is not semantic matching or exact address insertion.
- Algorithm iteration belongs in authored Fold/Everyone/cascade/drive patterns. Dynamic graph traversal must be proven through actual available grammar rather than hidden in an application C++ loop.
- Save roots follow `SaveSystem::rootPath()` and `earthcall_data_home.py`. `saves/` is frozen fixtures. No save data or author identity was changed in this checkpoint.

## Proposed instrument and acceptance order

A proposed working name is **Wayfarer**: a stable neighborhood map with a clearly marked current subject, a visible return path, and a quiet sentence workbench. This is Codex's proposal, not an artistic choice attributed to Zach. Positions belong to visual representatives; the represented being's position is untouched. Membership links remain graph links rather than forced tree ancestry.

First implement and actually walk: open → resolve one real being → show/inspect one actual Relation → follow and return → explicitly insert context into draft → preview → commit through the current compiler → observe the intended subject. Then broaden keyboard fluency, live updates, persistence, scale and aesthetic refinement. Do not spend the next pass adding decorative features before this journey exists.

Close/reopen and install/update must be deliberate and idempotent. The authored source must use the submitting Person, never a baked-in `Zach`/DID. Do not claim persistence from registry reach or Terminal acknowledgement. Use isolated fixtures and the applicable codecs, then save/restart the production consumer when available.

## Verification register

Build and test outcomes are recorded in the checkpoint audit when available. Test mechanisms: isolated LineEditor insertion/refusal; real Terminal act boundary; real Law Line example compilation through seeded authored Metalaws and a sensed shortcut. These are injected input witnesses, not physical keyboard, pointer hit-testing or a complete authored graph.

All commissioned journeys A–F remain open until the graphical instrument exists. No roughly-one-minute onboarding or one-to-three-gesture claim is made.

Signed: Codex / model unexposed by runtime / session `92b79f696f1c` / 2026-10-16 UTC.

### Reproducible Linux CPU configuration

The temporary UUID header came from `apt-get download uuid-dev` and `dpkg-deb -x` into `/tmp/earthcall-sysroot`. On this cloud host the configuration is:

```sh
/root/.local/bin/cmake -S . -B build-cloud -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_MAKE_PROGRAM=/root/.local/bin/ninja \
  -DFETCHCONTENT_SOURCE_DIR_GLFW="$PWD/local_deps/glfw-3.4" \
  -DCMAKE_CXX_FLAGS=-I/tmp/earthcall-sysroot/usr/include \
  -DOPENSSL_ROOT_DIR=/usr -DOPENSSL_INCLUDE_DIR=/usr/include \
  -DOPENSSL_CRYPTO_LIBRARY=/usr/lib/x86_64-linux-gnu/libcrypto.so \
  -DOPENSSL_SSL_LIBRARY=/usr/lib/x86_64-linux-gnu/libssl.so \
  -DUUID_LIB=/usr/lib/x86_64-linux-gnu/libuuid.so.1
/root/.local/bin/cmake --build build-cloud --target \
  line_editor_test law_line_zone_test terminal_zones_test -j4
/root/.local/bin/ctest --test-dir build-cloud --output-on-failure \
  -R '^(line_editor_test|terminal_zones_test|law_line_zone_test)$'
```

For a shorter compiler/template witness (including insertion), run:

```sh
EARTHCALL_TEMPLATE_TEST_ONLY=1 python3 tests/singularity/run_law_line_zone_test.py \
  ./build-cloud/law_line_zone_test
```

The first configure of a reused GLFW 3.3 cache may also need `-U GLFW_USE_WAYLAND`; a fresh build directory avoids it. Dependency sources were reused from the initial cloud configure via `FETCHCONTENT_SOURCE_DIR_*` overrides to avoid redownloading; those overrides are optional for a fresh networked build. Reproduce on the same checkout, not against an unknown Mac working tree.

[Observed prerequisite evidence and limitations](../audits/LAW_LINE_SPATIAL_CLOUD_PREREQUISITE_2026-10-16.md).
