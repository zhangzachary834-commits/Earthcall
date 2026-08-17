# wasm platform fix notes — 2026-08-10

Fixes for AUDIT_2026-08-10.md §1.4, §2.6, §2.7, and part of §4, plus two adjacent bugs
(§Renderer null deref, §Game::shutdown skipped) named in the same work order. Scope was
`src/Identity/**`, `src/Singularity/Foreign/WebBindings.cpp`, `src/Singularity/Network/
WebSocketClient_wasm.cpp`, `src/Singularity/Storage/SaveSystem.{cpp,hpp}`, `src/Singularity/Storage/CloudStorage.cpp`,
`src/Singularity/Screen/Renderer.cpp`, `src/Singularity/Core/Engine.cpp`,
`src/Singularity/Screen/GL/GluCompat.cpp`, `.gitignore`. No build was run (five agents share one
build tree); everything below was verified by reading, not by compiling.

## What a later agent must know about the wasm platform's real capabilities

- **No OpenSSL, ever, on wasm.** `CMakeLists.txt` guards `find_package(OpenSSL)` and every
  `OpenSSL::SSL`/`OpenSSL::Crypto` link with `if (NOT EMSCRIPTEN)`. There is no partial
  crypto on this platform — none of Ed25519 sign, Ed25519 verify, or a CSPRNG exists in the
  wasm binary. `KeyStore.cpp` already reflects this correctly (`store()` honestly returns
  `false` on wasm); `Identity/KeyPair.cpp`, `SingularId.cpp`, and `Claim.cpp` did not, until
  now.
- **Minting an id, issuing a claim, generating or importing a key, exporting a seed, and
  signing all now throw on wasm**, with a message naming the platform limitation, exactly
  matching the native branches' own refusal to hand out a weak/guessable result. This is
  intentional and is the fix, not a regression to route around. A wasm build genuinely
  cannot author new identities or new signed claims. It was never able to — it just used to
  lie about it.
- **Verifying an *existing* signature also throws on wasm** (`PublicKey::verify()`), for the
  same reason: there is no OpenSSL to do the Ed25519 math with. Anything that needs to tell
  "no crypto on this platform" apart from "checked, and it's invalid" must call the new
  `Identity::cryptoAvailable()` (declared in `KeyPair.hpp`) *before* calling `verify()`,
  not catch the throw. `FirstMoverRegister::evaluate()` and `PersonMigration::
  verifyOwnership()` both do this now; any new caller of `Claim::verify()` on data that
  might be evaluated on wasm needs the same guard, or it will crash uncaught the first time
  it runs there.
- **`PublicKey::fromId()` is real on every platform**, including wasm — it's pure byte
  copying, no OpenSSL involved. It was needlessly stubbed to always-invalid before; that's
  fixed as a byproduct, though it has no visible effect since `verify()` (the only thing
  that uses the resulting key) still throws on wasm.
- **`WebSocketClient` and the `--bind` embind exports now actually compile into the wasm
  binary** — they did not before (both were dead code behind the `EMSCRIPTEN` typo). Neither
  currently has a live caller in the engine (`Person::requestAIAction`, `WebSocketClient`'s
  only caller, is itself uncalled), so wasm-ld will very likely still garbage-collect both at
  `-O3`. If a later agent wires either of them up, they now have working implementations,
  not silent no-ops.
- **wasm saves are NOT durable.** MEMFS is in-memory only; there is no IDBFS mount, no
  `FS.syncfs`, no `--preload-file` anywhere in the tree, still. `writeSaveData()` on wasm
  still writes the file (readable back for the rest of that browser tab's session — this is
  why the return value/behavior was left otherwise unchanged) but now prints, on every save,
  an unambiguous warning that it will be lost on reload or tab close. **Nothing was
  implemented to make wasm saves durable.** See "Fix 3" below for why, and what real
  persistence would require.
- **Cloud sync has no wasm implementation at all** (no HTTP client is linked for wasm) and
  now says so once per session instead of once per save.
- **`currentRenderer()` throws instead of crashing** when called before a renderer is
  installed. On wasm there is no lazy default (unlike native's `OpenGLRenderer`), so any
  draw call that races `Engine::init`, or that runs after a failed `Engine::init`, hits this.
- **`Game::shutdown()` now runs on wasm loop-exit**, matching native. Confirmed by reading
  `entry.cpp`: on wasm, `Engine::run()` never returns to it (`emscripten_set_main_loop_arg`
  with `simulate_infinite_loop=1` unwinds the stack), and `Engine::shutdown()` itself was
  already called correctly from the loop-exit path — only the `Game::shutdown()` call was
  missing.

## Fix-by-fix detail

### Fix 1 — Identity/crypto stubs (CRITICAL, §1.4)

Files: `SingularId.cpp`, `Claim.cpp`, `KeyPair.cpp`/`.hpp`, `FirstMoverRegister.cpp`,
`PersonMigration.cpp`.

**What used to silently succeed and now fails loudly, on wasm only:**

| Call | Before | After |
|---|---|---|
| `SingularId::mintOpaque()` | returned the same constant id (`0,1,2,…`) every time | throws |
| `Claim::issue()`'s nonce | `0,1,…,15`, deterministic | throws (also unreachable in practice — see below) |
| `PrivateKey::sign()` | returned `{}` (well-formed empty signature) | throws |
| `PrivateKey::rawSeed()` | returned all-zero bytes (well-formed-looking seed) | throws |
| `PublicKey::verify()` | returned `false` unconditionally ("checked, bad") | throws ("did not check") |

`PrivateKey::generate()`/`fromRawSeed()` already threw before this work; nothing changed
there except that the rest of the surface is now consistent with them.

**Practical consequence:** since `generate()`/`fromRawSeed()` already threw, no valid
`PrivateKey` (`_pkey != nullptr`) could ever exist on wasm even before this fix — so
`Claim::issue()`'s nonce branch was already dead code (the function throws earlier, at
`signingKey.isValid()`, before reaching the nonce). It's fixed anyway for defense in depth
and because the audit named it explicitly.

**`FirstMoverRegister`'s outcome now distinguishes the two failure shapes**, per the work
order. A new `Gate::CryptoUnavailable` value (local to `FirstMoverRegister.cpp`, not a
serialized/append-only enum) is returned when `Identity::cryptoAvailable()` is false,
*before* `grant.verify()` is ever called — this is checked as a plain boolean, not by
catching the throw, so it doesn't depend on wasm exception-catching support one way or the
other. `explain()` now says: *"refused: no cryptographic verification is available on this
platform (wasm build has no CSPRNG/OpenSSL); this grant's validity cannot be determined
here, so the write is refused rather than assumed"* — as opposed to the old, false
*"refused: grant signature does not verify (quarantined)"*. `isQuarantined()` deliberately
does **not** count `CryptoUnavailable` as quarantine: quarantine means "this looks tampered
with," a judgment about the data; here there is no judgment to make. `mayWrite()` still
refuses either way (fail-closed).

`PersonMigration::verifyOwnership()` got the same treatment directly (it calls
`Claim::verify()` without going through `FirstMoverRegister`): on wasm it now reports every
zone with an `ownerClaim` as broken, with a comment explaining why, instead of crashing on
the first one.

**What was explicitly NOT done:** `PersonMigration::migrateSave()` itself is not guarded and
will still throw uncaught if run on wasm (via `IdentityLedger::resolveOrMint() ->
PrivateKey::generate()`, which already threw before this work). This is unchanged
pre-existing behavior, not a regression from this fix, and it's currently unreachable
in practice: `migrateSave()` is only called from `MigrateIdentitiesTool.cpp` (a standalone
CLI tool, excluded from `SRC_FILES` at `CMakeLists.txt:107` and built only under
`if(NOT EMSCRIPTEN)`) and from tests. If a later agent wires save-load-time migration into
the live wasm engine, it will need an explicit `cryptoAvailable()` guard around that call
site, or the first legacy save loaded in a browser will crash the session.

**Is Web Crypto (`crypto.getRandomValues` via an emscripten binding) the right long-term
fix?** For the CSPRNG half (`mintOpaque()`, nonces), yes — `crypto.getRandomValues` is a
real CSPRNG and browsers have had it for over a decade; that part is a reasonably
contained addition (an `EM_JS` or embind shim filling a byte buffer). For Ed25519
sign/verify, Web Crypto's `SubtleCrypto` did not support Ed25519 in all browsers until
relatively recently, and even where it does, `SubtleCrypto` is **promise-based / async by
design**, which does not drop cleanly into these synchronous `sign()`/`verify()` call
signatures — it would need either Asyncify (already enabled for this target,
`-s ASYNCIFY=1`, which helps) or a redesign of `PrivateKey`/`PublicKey` to an async
interface, which is a bigger change than "swap the RNG." Recommend splitting this: land
Web Crypto RNG for minting/nonces first (smaller, high-value, fixes the collision problem
directly), leave sign/verify throwing (i.e., "cannot author, can read/display only") until
there's an explicit decision about the async-signature redesign. Do not build a half
version of either — a `sign()` that sometimes works asynchronously and sometimes throws
synchronously is exactly the "plausible-looking fake" this whole fix pass was about
removing.

### Fix 2 — `EMSCRIPTEN` vs `__EMSCRIPTEN__` (HIGH, §2.6)

Files: `Integration/WebBindings.cpp`, `Singularity/Network/WebSocketClient_wasm.cpp`,
`Rendering/GL/GluCompat.cpp`.

Both `WebBindings.cpp` and `WebSocketClient_wasm.cpp` guarded on `#ifdef EMSCRIPTEN`
(a build-system environment variable, never a preprocessor macro emcc defines) instead of
`#ifdef __EMSCRIPTEN__`. Both compiled to empty translation units on every platform,
always. Fixed to `__EMSCRIPTEN__` in both.

**This is genuinely-never-compiled code, and it did not build once the guard was fixed:**

- `WebSocketClient_wasm.cpp`'s `Impl` had no `isConnected()` at all (the header declares
  `bool isConnected() const`; only the native desktop-mock TU implemented it) — this would
  have been an undefined-symbol link error in the wasm build. Added, backed by a real
  `connected` flag set in `Impl::onOpen`/`onError`/`onClose` and cleared in `disconnect()`.
- `WebSocketClient_wasm.cpp`'s `send()` was declared `void`; the header (and the native
  desktop-mock implementation) declare it `bool`. This is a hard conflicting-return-type
  compile error, not a warning. Fixed to return `bool` (`false` if there's no open socket,
  `true` otherwise — the underlying `emscripten_websocket_send_utf8_text()` call's own
  result code is not currently checked, matching the pre-existing level of rigor in this
  file; a later agent could tighten this).
- `WebBindings.cpp` was checked against `Core::Event::Utterance`'s actual fields
  (`payload`, `sourceClient`) and `EventBus::publish()`'s signature — both matched, no
  changes needed there beyond the guard.

Neither file currently has a live caller (`Person::requestAIAction`, `WebSocketClient`'s
only caller, is itself uncalled), so `wasm-ld` at `-O3` will most likely still garbage-collect
both — but they now at least *can* link and are no longer silently absent.

`GluCompat.cpp:3` had the same `EMSCRIPTEN`/`__EMSCRIPTEN__` typo guarding an
`__APPLE__ && !defined(EMSCRIPTEN)` `#include` choice. Fixed to `__EMSCRIPTEN__`, though
this one was latent either way: `__APPLE__` is never defined for the
`wasm32-unknown-emscripten` target regardless of the build host, so the `#else` branch
(`<GL/gl.h>`) was already being taken correctly in the wasm build before this fix too.
Fixed anyway per the work order, and so it isn't the next person's landmine.

**Not fixed, reported instead (not in my file ownership):** `OpenGLRenderer.cpp:6` has the
identical `#if defined(__APPLE__) && !defined(EMSCRIPTEN)` typo. Also latent for the same
`__APPLE__`-is-never-defined-on-wasm reason, and doubly so because
`CMakeLists.txt:270` excludes `src/Rendering/GL/OpenGLRenderer.cpp` from the wasm source
list entirely — this file never compiles for wasm regardless. Still worth the same one-word
fix (`EMSCRIPTEN` → `__EMSCRIPTEN__`) for consistency; flagging for whoever owns that file.

### Fix 3 — wasm saves are not durable (HIGH, §2.7)

Files: `Util/SaveSystem.cpp`, `Util/CloudStorage.cpp`.

**Chose: honest reporting, not IDBFS.** Proper persistence (mount IDBFS at the save root,
`FS.syncfs(false, cb)` after every write, and hold the "save complete" signal until that
callback fires — `syncfs` is asynchronous) touches files outside this pass's ownership
(the IDBFS mount has to happen once at startup, most naturally in `entry.cpp` or
`Engine::init`, and the completion signal needs to reach whatever UI reports "saved," which
lives in `Singularity/Core/GameSaveLoad.cpp` / `ZonesOfEarth/ZoneManager.cpp`, neither of
which is mine). It cannot be verified without building and running in a browser, which this
pass explicitly cannot do. A half-wired IDBFS mount that isn't confirmed to actually sync
would be exactly the "plausible-looking fake" this whole audit is about — worse than an
honest "not saved," because it *looks* fixed. So: made it honest instead.

- `writeSaveData()` (both overloads) still writes to MEMFS — the write genuinely succeeds
  and the file is readable back for the rest of that browser tab's session, so nothing about
  the return contract changed (both still return the filename on success; nothing calling
  this expects anything else, and changing it would ripple into files outside this pass's
  ownership). **On wasm, every successful write now also prints, to stderr, every single
  time:** `"<filename> exists only in this browser tab's in-memory filesystem; it will be
  LOST on reload or tab close. No persistent storage (IDBFS or equivalent) is wired up for
  this build yet -- this is not a real save."` This fires on *every* save, deliberately,
  because it's the actual outcome of the specific save just requested, not a fixed fact about
  the build the Person can be told once and forget.
- The `Failed to sync ... to cloud` spam (§2.7's related finding) is now `[SaveSystem] Cloud
  sync unavailable in this build; saves stay local to this session only` **once per process**
  (a `static std::atomic<bool>` guard), because cloud sync is a fixed, known-unavailable fact
  about this build on *every* platform (there's no cloud server wired up natively either —
  `CloudStorage.cpp`'s native path talks to `https://localhost:8080` by default, which
  nothing runs), not new information each time. `CloudStorage.cpp`'s three wasm stubs
  (upload/download/fetch-metadata) got the matching single-shot warning at the source too.
- **A real IDBFS fix, when someone builds it, needs:** an `EM_ASM`/embind call to
  `FS.mkdir('/saves'); FS.mount(IDBFS, {}, '/saves');` plus `FS.syncfs(true, cb)` once at
  startup (populate from IndexedDB) — most naturally in `Engine::init` right where the
  save root is otherwise set up — and an `FS.syncfs(false, cb)` after every write, with the
  "save complete" UI signal (wherever `GameSaveLoad.cpp`/`ZoneManager.cpp` report it) held
  until that callback fires. `-s ASYNCIFY=1` is already on for this target, which is what
  makes waiting for that callback from otherwise-synchronous C++ code tractable without a
  full callback-based rewrite of the save path.

### Fix 4 — `currentRenderer()` null deref

File: `Rendering/Renderer.cpp`. `return *g_current;` is now guarded: if no renderer has been
installed (no lazy default exists on wasm, and native's lazy `OpenGLRenderer` default would
already have run above), it throws a `std::runtime_error` naming the two ways this happens
(a draw call before `Engine::init`'s `setCurrentRenderer()`, or a failed `Engine::init`)
instead of dereferencing null with no diagnostic.

### Fix 5 — `Game::shutdown()` skipped on wasm

File: `Singularity/Core/Engine.cpp`. `emscripten_main_loop()`'s loop-exit branch now calls
`ctx->game->shutdown()` immediately before `ctx->engine->shutdown()`, mirroring
`Engine::run()`'s native `#else` arm. Verified against `entry.cpp`: on wasm,
`engine.run(game)` never returns to `entry.cpp`'s own `engine.shutdown()` call at line 21
(`emscripten_set_main_loop_arg(..., simulate_infinite_loop=1)` unwinds the stack), so this
was the only place `Game::shutdown()` could run on wasm, and it wasn't.

### Fix 6 — `.gitignore` hygiene

Added `build-wasm/` and `node_modules/` to `.gitignore`, in the existing file's style
(comment explaining why, matching the `build/`/`build-asan/` precedent).

**Paths needing `git rm --cached` (not run — orchestrator's call per instructions):**
- `build-wasm/` — 81 tracked files, including `build-wasm/earthcall.wasm`,
  `build-wasm/earthcall.js`, the whole `build-wasm/CMakeFiles/` and
  `build-wasm/_deps/vhacd-subbuild/` trees, and `build-wasm/CMakeCache.txt` (the one pinned
  to a specific machine's emsdk path that breaks fresh-clone configure).
  **Exception: `build-wasm/index.html` should NOT simply be untracked-and-left** — see below.
- `node_modules/`, wherever it appears — currently only
  `scratch/experiments/puppeteer_test/node_modules/` (2,221 files).

**Not acted on, per instructions — recommendation only:** `build-wasm/index.html` is
currently the *only* page that boots the wasm build (it's the only tracked file setting
`Module.canvas` to the `#earthcall-canvas` selector `WebGpuContext_wasm.cpp:18` hardcodes),
yet it lives inside what is now a gitignored build-output directory — so once `build-wasm/`
is untracked, the *next* `rm -rf build-wasm` (a completely reasonable thing to do to a build
directory) permanently deletes the only working entry point for the wasm build, again.
Recommend moving it to a tracked, non-build-output location before or as part of the
untracking — most naturally `web_ui/`, alongside the existing (currently non-functional)
`web_ui/index.html`, either replacing that file or as `web_ui/wasm.html`. Whichever page
ends up there needs to actually `<script src="earthcall.js">` (the build output, still
produced into `build-wasm/` and still gitignored) — `web_ui/index.html` currently has the
canvas element but never loads `earthcall.js`, which is a separate, pre-existing gap from
the one this note is about.
