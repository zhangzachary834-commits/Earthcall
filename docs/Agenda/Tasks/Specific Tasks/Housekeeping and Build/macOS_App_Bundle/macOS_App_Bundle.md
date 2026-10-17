# Earthcall.app: an installed macOS app, not only an executable repo

**Origin (Zach, 2026-10-12):** "promote Earthcall into a real macOS app, not just an executable repo." Zach's choices: the app is for him for now (a local `/Applications/Earthcall.app`); the first run gets only a bare foundation (his Ourverse navigator is its own task); and only the MCP server is bundled.

**The Sun's qualification, relayed by Zach:** keep the bundle and build structured so that Developer ID signing, notarization and DMG distribution can be added later without architectural changes. Earthcall is shared through GitHub, so there are no Apple Developer Program requirements for now. Apple signing is future release infrastructure only. It is not part of Earthcall's ontology, identity model or ownership.

## What landed

| Piece | Where |
|---|---|
| Installed vs. checkout: `isBundled`, `resourceRoot` (`Contents/Resources` or the checkout), `logDirectory` (`~/Library/Logs/Earthcall` or `logs`), and `checkoutRoot` (the former `findRepoRoot`) | `src/Singularity/Core/AppPaths.{hpp,cpp}` |
| When bundled, `Engine::init`: stdout/stderr → `~/Library/Logs/Earthcall/earthcall.log`, no checkout fallback, cwd = data home, `resource://` mounted on the VFS, ImGui layout in `<data home>/ui/imgui.ini`. Unbundled behaviour is unchanged. | `src/Singularity/Core/Engine.cpp` |
| Logger categories and `dom_mirror.js` resolve through `AppPaths` | `Logger.cpp`, `DomMirrorBridge.cpp` |
| First run of the installed app: copies `Resources/Foundation` (never over existing files), marks the home, runs `git init` and commits | `SaveSystem::chooseDataRoot(…, foundation)` |
| MCP server finds its signer at `Contents/MacOS/earthcall_first_mover` when bundled | `earthcall-mcp-server.js` |
| Bundle assembly and signing; plist; hardened-runtime entitlements (mic only); placeholder icon | `scripts/macos/{package_app.sh, Info.plist.in, Earthcall.entitlements, make_placeholder_icon.py}` |
| Build, install to `/Applications` (keeping one `Earthcall.previous.app`), carry the checkout's window layout over once | `scripts/macos/install_app.sh`, `Install Earthcall App.command`, CMake target `earthcall_app` (not in ALL) |
| Optional free local certificate, so macOS keeps mic, Screen Recording and Accessibility grants across rebuilds | `scripts/macos/make_local_signing_identity.sh` (Zach runs it; it writes his keychain) |
| Witnesses | `tests/singularity/app_paths_test.cpp` (10), `data_home_test.cpp` (now 19) |

**Verified 2026-10-12:**
- `codesign --verify --deep --strict` passes, with the hardened runtime and the audio-input entitlement.
- `otool -L` shows system libraries only. The bundle is 65 MB (23 MB of that is the MCP server's node closure).
- The bundled MCP server resolves its SDK.
- An `open -n --env EARTHCALL_DATA=<empty scratch>` launch, which starts from `/` with no terminal, logged `installed app`. It created and git-initialised the home, and booted the engine's default Zones (Sanctum, Temple, Cavern, Forge, Home, Ourverse Gathering) with **no Foundation needed**.
- A quit Apple Event exits cleanly.
- `install_app.sh` installed `/Applications/Earthcall.app`, signed ad-hoc, because no local certificate exists yet.

## Pitfalls for the next agent

- **Port 8080:** the app's WebSocket server is on 8080. A second instance, such as a test launch, takes over Zach's MCP clients. Test the bundle against a scratch `EARTHCALL_DATA` and quit it promptly.
- **Ad-hoc signing:** each rebuild is a new identity to TCC. Expect mic, Screen Recording and Accessibility prompts again until the local certificate exists.
- **Dev workflow:** the `earthcall_webgpu` target, `build/` and `Run Earthcall.command` are deliberately unchanged. Don't turn the dev target into a `MACOSX_BUNDLE`; tests and scripts use its path.
- `/Applications/Earthcall` (no `.app`) is an older Finder alias of Zach's from 2026-09-28. Left untouched.

## Future release path (not built; no architectural change needed)
1. Set `EARTHCALL_SIGN_IDENTITY="Developer ID Application: …"` and `EARTHCALL_SIGN_TIMESTAMP=1` for `package_app.sh`.
2. `xcrun notarytool submit … --wait` on a zip of `build_release/Earthcall.app`, then `xcrun stapler staple`.
3. A DMG from the stapled app, with `hdiutil` or create-dmg.
4. Sparkle for updates. A universal binary (needs an x86_64 `libwgpu_native.a`).

## Open
- [ ] Zach: run `scripts/macos/make_local_signing_identity.sh` once, then `Install Earthcall App.command`.
- [ ] Real app icon: Zach's artwork at `scripts/macos/AppIcon.png` (1024²). A generated placeholder is used until then.
- [ ] Keys in the macOS Keychain (`Identity::KeyStore` → SecItem) instead of `~/.earthcall/identity` files.
- [ ] Native About panel, window-frame restoration, `~/Library/Caches/Earthcall` for regenerable data.
- [ ] Bundle the Python backend, if it is ever needed outside development.
- [ ] Foundation content: see the [Personal Ourverse Navigator](../../Zones%20and%20Ourverse/Personal_Ourverse_Navigator/Personal_Ourverse_Navigator.md).

*Claude Code / Claude Opus 5.5 / session `16d04786-8407-4a70-a57c-fa5c11094c91` / 2026-10-12T13:10-07:00. The scope decisions are Zach's and the release qualification is the Sun's, both via Zach. The AppPaths split, the first-run git seeding, the stable-local-certificate approach to TCC and the closure-copy MCP packaging are my design.*
