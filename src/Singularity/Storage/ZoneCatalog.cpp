#include "Singularity/Storage/ZoneCatalog.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <set>
#include <system_error>

namespace Singularity::Storage {
namespace {

std::string kindName(ZoneCatalogRow::Kind kind) {
    return kind == ZoneCatalogRow::Kind::Home ? "home" : "zone";
}

void issue(ZoneCatalogResult& out, std::string code,
           const std::filesystem::path& path, std::string detail) {
    out.issues.push_back({std::move(code), path.string(), std::move(detail)});
}

bool parseRow(const nlohmann::json& json, const std::filesystem::path& source,
              ZoneCatalogRow::Kind kind, ZoneCatalogRow& row,
              ZoneCatalogResult& out) {
    if (!json.is_object()) {
        issue(out, "malformed-root", source, "root must be a JSON object");
        return false;
    }
    const std::string idText = json.value("singularId", std::string{});
    const Identity::SingularId id = Identity::SingularId::parse(idText);
    if (!id.isValid() || id.kind() != Identity::SingularId::Kind::Opaque ||
        id.toString() != idText) {
        issue(out, "invalid-singular-id", source,
              "Zone/Home requires one canonical opaque singularId");
        return false;
    }
    const std::string slug = json.value("slug",
        json.value("identifier", json.value("name", std::string{})));
    if (slug.empty()) {
        issue(out, "missing-slug", source, "identifier/slug is empty");
        return false;
    }
    row.kind = kind;
    row.singularId = id;
    row.slug = slug;
    row.displayName = json.value("displayName", json.value("name", slug));
    row.headGeneration = json.value("headGeneration", std::string{});
    row.manifestHash = json.value("manifestHash", std::string{});
    row.rootPath = source.lexically_normal().string();
    row.availability = json.value("availability", std::string("available"));
    return true;
}

void validateUniqueness(ZoneCatalogResult& out) {
    std::set<std::string> ids;
    std::set<std::string> slugs;
    for (const ZoneCatalogRow& row : out.rows) {
        const std::string id = row.singularId.toString();
        if (!ids.insert(id).second) {
            issue(out, "duplicate-singular-id", row.rootPath, id);
        }
        if (!slugs.insert(row.slug).second) {
            issue(out, "duplicate-slug", row.rootPath, row.slug);
        }
    }
    out.valid = out.issues.empty();
}

bool readJson(const std::filesystem::path& path, nlohmann::json& json) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    try {
        input >> json;
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

ZoneCatalog::ZoneCatalog(std::filesystem::path saveRoot)
    : _saveRoot(std::move(saveRoot)) {}

std::filesystem::path ZoneCatalog::path() const {
    return _saveRoot / "catalog" / "zones-v1.json";
}

ZoneCatalogResult ZoneCatalog::load() const {
    ZoneCatalogResult out;
    nlohmann::json document;
    if (!readJson(path(), document)) {
        issue(out, "missing-or-malformed-catalog", path(),
              "transactional activation requires a valid catalog");
        return out;
    }
    if (!document.is_object() || document.value("format", std::string{}) != kFormat ||
        !document.contains("rows") || !document["rows"].is_array()) {
        issue(out, "unsupported-catalog", path(), "format/rows contract not satisfied");
        return out;
    }
    for (const auto& item : document["rows"]) {
        const std::string kindText = item.value("kind", std::string{});
        ZoneCatalogRow::Kind kind;
        if (kindText == "zone") kind = ZoneCatalogRow::Kind::Zone;
        else if (kindText == "home") kind = ZoneCatalogRow::Kind::Home;
        else {
            issue(out, "invalid-kind", path(), kindText);
            continue;
        }
        ZoneCatalogRow row;
        if (parseRow(item, path(), kind, row, out)) {
            row.headGeneration = item.value("headGeneration", std::string{});
            row.manifestHash = item.value("manifestHash", std::string{});
            row.rootPath = item.value("rootPath", std::string{});
            row.availability = item.value("availability", std::string("available"));
            out.rows.push_back(std::move(row));
        }
    }
    validateUniqueness(out);
    return out;
}

ZoneCatalogResult ZoneCatalog::rebuildPreview() const {
    ZoneCatalogResult out;
    const auto scan = [&](const char* parent, const char* filename,
                          ZoneCatalogRow::Kind kind) {
        const std::filesystem::path directory = _saveRoot / parent;
        std::error_code ec;
        if (!std::filesystem::exists(directory, ec)) return;
        std::vector<std::filesystem::path> sources;
        for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
            if (ec) break;
            const auto candidate = entry.path() / filename;
            if (entry.is_directory() && std::filesystem::is_regular_file(candidate, ec)) {
                sources.push_back(candidate);
            }
        }
        std::sort(sources.begin(), sources.end());
        for (const auto& source : sources) {
            nlohmann::json json;
            if (!readJson(source, json)) {
                issue(out, "malformed-root", source, "could not parse JSON");
                continue;
            }
            ZoneCatalogRow row;
            if (parseRow(json, source, kind, row, out)) {
                if (row.headGeneration.empty()) row.availability = "legacy-needs-migration";
                out.rows.push_back(std::move(row));
            }
        }
    };
    scan("zones", "zone.json", ZoneCatalogRow::Kind::Zone);
    scan("homes", "home.json", ZoneCatalogRow::Kind::Home);
    validateUniqueness(out);
    return out;
}

bool ZoneCatalog::publish(const ZoneCatalogResult& catalog, std::string& error) const {
    if (!catalog.valid || !catalog.issues.empty()) {
        error = "REFUSED: catalog publication requires a validated, issue-free preview";
        return false;
    }
    nlohmann::json rows = nlohmann::json::array();
    for (const ZoneCatalogRow& row : catalog.rows) {
        rows.push_back({{"kind", kindName(row.kind)},
                        {"singularId", row.singularId.toString()},
                        {"slug", row.slug}, {"displayName", row.displayName},
                        {"headGeneration", row.headGeneration},
                        {"manifestHash", row.manifestHash},
                        {"rootPath", row.rootPath},
                        {"formatVersion", 1},
                        {"availability", row.availability}});
    }
    const nlohmann::json document{{"format", kFormat}, {"rows", std::move(rows)}};
    const std::filesystem::path finalPath = path();
    std::error_code ec;
    std::filesystem::create_directories(finalPath.parent_path(), ec);
    if (ec) { error = ec.message(); return false; }
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path temporary = finalPath.string() + ".tmp-" + std::to_string(nonce);
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        output << document.dump(2);
        output.flush();
        if (!output) {
            error = "failed to flush catalog staging file";
            std::filesystem::remove(temporary, ec);
            return false;
        }
    }
    std::filesystem::rename(temporary, finalPath, ec);
    if (ec) {
        error = ec.message();
        std::filesystem::remove(temporary, ec);
        return false;
    }
    return true;
}

} // namespace Singularity::Storage
