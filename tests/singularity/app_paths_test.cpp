// Installed Earthcall.app vs. source checkout: the layout test for
// AppPaths::bundleContentsFor, and the unbundled defaults every other test and
// the .command launchers rely on staying exactly as they were.
#include "Singularity/Core/AppPaths.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace {
int g_failures = 0;
void check(bool condition, const std::string& desc) {
    std::printf("  %s: %s\n", condition ? "ok" : "FAILED", desc.c_str());
    if (!condition) ++g_failures;
    std::fflush(stdout);
}
} // namespace

int main() {
    std::printf("Running app_paths_test...\n");

    check(AppPaths::bundleContentsFor("/Applications/Earthcall.app/Contents/MacOS/Earthcall") ==
              fs::path("/Applications/Earthcall.app/Contents"),
          "an executable in X.app/Contents/MacOS is bundled");
    check(AppPaths::bundleContentsFor(
              "/Applications/Earthcall.app/Contents/MacOS/earthcall_first_mover") ==
              fs::path("/Applications/Earthcall.app/Contents"),
          "the signer helper beside it is bundled too");
    check(AppPaths::bundleContentsFor("/Users/z/Earthcall/build/earthcall_webgpu").empty(),
          "a checkout build is not bundled");
    check(AppPaths::bundleContentsFor("/tmp/Contents/MacOS/x").empty(),
          "Contents/MacOS without a .app parent is not bundled");
    check(AppPaths::bundleContentsFor("/tmp/Earthcall.app/MacOS/x").empty(),
          "MacOS without Contents is not bundled");

    // This test binary runs from build/, so it must see the unbundled world.
    check(!AppPaths::isBundled(), "the test binary is not bundled");
    check(AppPaths::logDirectory() == fs::path("logs"), "unbundled logs stay cwd-relative");
    const fs::path checkout = AppPaths::checkoutRoot();
    check(!checkout.empty() && fs::exists(checkout / "AGENTS.md"),
          "the checkout is found from build/");
    check(AppPaths::resourceRoot() == checkout, "unbundled resources are the checkout");
    check(fs::exists(AppPaths::resourceRoot() / "src/Singularity/Foreign/Web/dom_mirror.js"),
          "dom_mirror.js resolves from the checkout");

    if (g_failures) {
        std::printf("app_paths_test: %d FAILED\n", g_failures);
        return 1;
    }
    std::printf("app_paths_test: all passed\n");
    return 0;
}
