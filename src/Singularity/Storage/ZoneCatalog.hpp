#pragma once

#include "Identity/SingularId.hpp"
#include "json.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace Singularity::Storage {

struct ZoneCatalogRow {
    enum class Kind { Zone, Home };

    Kind kind = Kind::Zone;
    Identity::SingularId singularId;
    std::string slug;
    std::string displayName;
    std::string headGeneration;
    std::string manifestHash;
    std::string rootPath;
    std::string availability;
};

struct ZoneCatalogIssue {
    std::string code;
    std::string path;
    std::string detail;
};

struct ZoneCatalogResult {
    bool valid = false;
    std::vector<ZoneCatalogRow> rows;
    std::vector<ZoneCatalogIssue> issues;
};

// Storage mechanism only. Reading and previewing never create directories,
// touch mtimes, hydrate a Zone, or consult live managers.
class ZoneCatalog {
public:
    explicit ZoneCatalog(std::filesystem::path saveRoot);

    std::filesystem::path path() const;
    ZoneCatalogResult load() const;
    ZoneCatalogResult rebuildPreview() const;

    // Explicit publication boundary used by migration/Save Zone only. The
    // caller must supply an already validated result; ordinary boot never
    // invokes this method.
    bool publish(const ZoneCatalogResult& catalog, std::string& error) const;

    static constexpr const char* kFormat = "earthcall-zone-catalog-v1";

private:
    std::filesystem::path _saveRoot;
};

} // namespace Singularity::Storage
