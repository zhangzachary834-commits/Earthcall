#include "Singularity/Core/AppPaths.hpp"

#include <climits>
#include <cstdlib>
#include <vector>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace AppPaths {

namespace fs = std::filesystem;

fs::path executablePath() {
#ifdef __APPLE__
    char buf[PATH_MAX];
    uint32_t sz = sizeof(buf);
    if (_NSGetExecutablePath(buf, &sz) == 0) {
        std::error_code ec;
        auto exe = fs::weakly_canonical(fs::path(buf), ec);
        if (!ec) return exe;
    }
#endif
    return {};
}

fs::path bundleContentsFor(const fs::path& exe) {
    const fs::path macos = exe.parent_path();
    const fs::path contents = macos.parent_path();
    const fs::path app = contents.parent_path();
    if (macos.filename() == "MacOS" && contents.filename() == "Contents" &&
        app.extension() == ".app") {
        return contents;
    }
    return {};
}

bool isBundled() {
    static const bool bundled = !bundleContentsFor(executablePath()).empty();
    return bundled;
}

fs::path checkoutRoot() {
    std::error_code ec;
    std::vector<fs::path> seeds;
    if (auto cwd = fs::current_path(ec); !ec) seeds.push_back(cwd);
    if (auto exe = executablePath(); !exe.empty()) seeds.push_back(exe.parent_path());
    for (auto& dir : seeds) {
        for (int i = 0; i < 8 && !dir.empty() && dir != dir.root_path(); ++i) {
            if (fs::exists(dir / "AGENTS.md", ec) && fs::is_directory(dir / "saves", ec)) {
                return dir;
            }
            dir = dir.parent_path();
        }
    }
    return {};
}

fs::path resourceRoot() {
    if (const fs::path contents = bundleContentsFor(executablePath()); !contents.empty()) {
        return contents / "Resources";
    }
    return checkoutRoot();
}

fs::path logDirectory() {
    if (isBundled()) {
        if (const char* home = std::getenv("HOME"); home && *home) {
            return fs::path(home) / "Library" / "Logs" / "Earthcall";
        }
    }
    return fs::path("logs");
}

} // namespace AppPaths
