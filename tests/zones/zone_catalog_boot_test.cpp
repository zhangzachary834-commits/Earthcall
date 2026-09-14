#include "Identity/SingularId.hpp"
#include "Singularity/Storage/ZoneCatalog.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

[[noreturn]] void fail(const char* expression, const char* file, int line) {
    std::cerr << "CHECK failed: " << expression << " at " << file << ':' << line << '\n';
    std::exit(EXIT_FAILURE);
}
#define CHECK(expr) do { if (!(expr)) fail(#expr, __FILE__, __LINE__); } while (false)

void writeRoot(const std::filesystem::path& root, const std::string& parent,
               const std::string& slug, const Identity::SingularId& id) {
    const auto directory = root / parent / slug;
    std::filesystem::create_directories(directory);
    std::ofstream output(directory / (parent == "homes" ? "home.json" : "zone.json"));
    output << nlohmann::json{{"singularId", id.toString()},
                             {"identifier", slug}, {"name", "Display " + slug}}.dump(2);
}

} // namespace

int main() {
    const auto root = std::filesystem::temp_directory_path() /
                      ("earthcall_zone_catalog_" + Identity::SingularId::mintOpaque().abbreviated());
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    for (int i = 0; i < 25; ++i) {
        writeRoot(root, "zones", "zone-" + std::to_string(i),
                  Identity::SingularId::mintOpaque());
    }
    for (int i = 0; i < 2; ++i) {
        writeRoot(root, "homes", "home-" + std::to_string(i),
                  Identity::SingularId::mintOpaque());
    }

    Singularity::Storage::ZoneCatalog catalog(root);
    CHECK(!std::filesystem::exists(catalog.path()));
    const auto preview = catalog.rebuildPreview();
    CHECK(preview.valid);
    CHECK(preview.rows.size() == 27);
    CHECK(!std::filesystem::exists(catalog.path()));

    // Preview is observably read-only: source write times do not move and no
    // catalog directory appears until explicit publication.
    const auto source = root / "zones" / "zone-0" / "zone.json";
    const auto before = std::filesystem::last_write_time(source);
    CHECK(catalog.rebuildPreview().valid);
    CHECK(std::filesystem::last_write_time(source) == before);
    CHECK(!std::filesystem::exists(root / "catalog"));

    std::string error;
    CHECK(catalog.publish(preview, error));
    const auto loaded = catalog.load();
    CHECK(loaded.valid);
    CHECK(loaded.rows.size() == 27);

    // One duplicate alias invalidates the complete preview. Publication must
    // refuse it and leave the already-good catalog untouched.
    writeRoot(root, "zones", "duplicate-folder", Identity::SingularId::mintOpaque());
    {
        auto p = root / "zones" / "duplicate-folder" / "zone.json";
        nlohmann::json json;
        std::ifstream input(p); input >> json;
        json["identifier"] = "zone-0";
        std::ofstream output(p); output << json.dump(2);
    }
    const auto collision = catalog.rebuildPreview();
    CHECK(!collision.valid);
    CHECK(!catalog.publish(collision, error));
    CHECK(catalog.load().valid);

    // An explicit malformed identity is not treated as a legacy omission.
    {
        auto p = root / "homes" / "home-0" / "home.json";
        nlohmann::json json;
        std::ifstream input(p); input >> json;
        json["singularId"] = "not-an-id";
        std::ofstream output(p); output << json.dump(2);
    }
    const auto malformed = catalog.rebuildPreview();
    CHECK(!malformed.valid);
    CHECK(!malformed.issues.empty());

    std::filesystem::remove_all(root);
    std::cout << "zone_catalog_boot_test: all checks passed\n";
    return EXIT_SUCCESS;
}
