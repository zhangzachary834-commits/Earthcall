#pragma once
#include <filesystem>

// Where this process sits on disk: inside an installed Earthcall.app, or run
// from a source checkout. Substrate only — it names no being. The Person's
// saves are SaveSystem::chooseDataRoot's concern; this answers the rest: the
// read-only resources shipped with the program, and where its logs go.
//
// Bundled layout (scripts/macos/package_app.sh builds it):
//   Earthcall.app/Contents/MacOS/Earthcall               the engine
//   Earthcall.app/Contents/MacOS/earthcall_first_mover   the signer helper
//   Earthcall.app/Contents/Resources/                    resourceRoot()
// Logs then go to ~/Library/Logs/Earthcall (Console.app shows them).
//
// Unbundled, behaviour is what it was before the app existed: resources are
// the checkout and logs are "logs" under the cwd.
namespace AppPaths {

std::filesystem::path executablePath();

// `Contents/` when `exe` is `<X>.app/Contents/MacOS/<name>`, else empty.
// Pure, so tests can hand it any layout.
std::filesystem::path bundleContentsFor(const std::filesystem::path& exe);

bool isBundled();

// Walk up from the cwd and the executable to a directory holding AGENTS.md
// and saves/. Empty when there is none (always, for an installed app).
std::filesystem::path checkoutRoot();

// Contents/Resources when bundled, else the checkout root (empty if none).
std::filesystem::path resourceRoot();

// ~/Library/Logs/Earthcall when bundled, else "logs" (cwd-relative).
std::filesystem::path logDirectory();

} // namespace AppPaths
