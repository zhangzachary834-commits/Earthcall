import re

with open("src/Singularity/Storage/SaveSystem.cpp", "r") as f:
    content = f.read()

# Fix zoneIdentityExists
content = content.replace('''    auto path = std::filesystem::path(folder) / safe / "zone.ecsave";
    if (std::filesystem::exists(path, ec) && std::filesystem::file_size(path, ec) > 0) return true;
    path = std::filesystem::path(folder) / safe / "zone.json";''', 
'''    auto path = std::filesystem::path(folder) / safe / "zone.ecsave";
    if (std::filesystem::exists(path, ec) && std::filesystem::file_size(path, ec) > 0) return true;
    path = std::filesystem::path(folder) / safe / "zone.ecform";
    if (std::filesystem::exists(path, ec) && std::filesystem::file_size(path, ec) > 0) return true;
    path = std::filesystem::path(folder) / safe / "zone.json";''')

# Fix homeIdentityExists
content = content.replace('''    auto path = std::filesystem::path(folder) / safe / "home.ecsave";
    if (std::filesystem::exists(path, ec) && std::filesystem::file_size(path, ec) > 0) return true;
    path = std::filesystem::path(folder) / safe / "home.json";''',
'''    auto path = std::filesystem::path(folder) / safe / "home.ecsave";
    if (std::filesystem::exists(path, ec) && std::filesystem::file_size(path, ec) > 0) return true;
    path = std::filesystem::path(folder) / safe / "home.ecform";
    if (std::filesystem::exists(path, ec) && std::filesystem::file_size(path, ec) > 0) return true;
    path = std::filesystem::path(folder) / safe / "home.json";''')

# Fix readZoneIdentity
content = content.replace('''    const auto path = (std::filesystem::path(folder) / safe / "zone.ecsave").string();
    return readSaveData(path);''',
'''    auto path = std::filesystem::path(folder) / safe / "zone.ecsave";
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        path = std::filesystem::path(folder) / safe / "zone.ecform";
    }
    return readSaveData(path.string());''')

# Fix readHomeIdentity
content = content.replace('''    const auto path = (std::filesystem::path(folder) / safe / "home.ecsave").string();
    return readSaveData(path);''',
'''    auto path = std::filesystem::path(folder) / safe / "home.ecsave";
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        path = std::filesystem::path(folder) / safe / "home.ecform";
    }
    return readSaveData(path.string());''')

# Fix getZoneIdentityPath
content = content.replace('''std::string getZoneIdentityPath(const std::string& identifier) {
    std::string dir = resolveIdentityFolder(SaveType::ZONE, identifier);
    if (dir.empty()) return "";
    const auto ecformPath = dir / "zone.ecsave";
    std::error_code ec;
    if (std::filesystem::exists(ecformPath, ec) && std::filesystem::file_size(ecformPath, ec) > 0) return ecformPath.string();
    const auto jsonPath = dir / "zone.json";
    if (std::filesystem::exists(jsonPath, ec) && std::filesystem::file_size(jsonPath, ec) > 0) return jsonPath.string();
    return ecformPath.string();
}''',
'''std::string getZoneIdentityPath(const std::string& identifier) {
    std::string dir = resolveIdentityFolder(SaveType::ZONE, identifier);
    if (dir.empty()) return "";
    const auto ecsavePath = dir / "zone.ecsave";
    std::error_code ec;
    if (std::filesystem::exists(ecsavePath, ec) && std::filesystem::file_size(ecsavePath, ec) > 0) return ecsavePath.string();
    const auto ecformPath = dir / "zone.ecform";
    if (std::filesystem::exists(ecformPath, ec) && std::filesystem::file_size(ecformPath, ec) > 0) return ecformPath.string();
    const auto jsonPath = dir / "zone.json";
    if (std::filesystem::exists(jsonPath, ec) && std::filesystem::file_size(jsonPath, ec) > 0) return jsonPath.string();
    return ecsavePath.string();
}''')

# Fix getHomeIdentityPath
content = content.replace('''std::string getHomeIdentityPath(const std::string& identifier) {
    std::string dir = resolveIdentityFolder(SaveType::HOME, identifier);
    if (dir.empty()) return "";
    const auto ecformPath = dir / "home.ecsave";
    std::error_code ec;
    if (std::filesystem::exists(ecformPath, ec) && std::filesystem::file_size(ecformPath, ec) > 0) return ecformPath.string();
    const auto jsonPath = dir / "home.json";
    if (std::filesystem::exists(jsonPath, ec) && std::filesystem::file_size(jsonPath, ec) > 0) return jsonPath.string();
    return ecformPath.string();
}''',
'''std::string getHomeIdentityPath(const std::string& identifier) {
    std::string dir = resolveIdentityFolder(SaveType::HOME, identifier);
    if (dir.empty()) return "";
    const auto ecsavePath = dir / "home.ecsave";
    std::error_code ec;
    if (std::filesystem::exists(ecsavePath, ec) && std::filesystem::file_size(ecsavePath, ec) > 0) return ecsavePath.string();
    const auto ecformPath = dir / "home.ecform";
    if (std::filesystem::exists(ecformPath, ec) && std::filesystem::file_size(ecformPath, ec) > 0) return ecformPath.string();
    const auto jsonPath = dir / "home.json";
    if (std::filesystem::exists(jsonPath, ec) && std::filesystem::file_size(jsonPath, ec) > 0) return jsonPath.string();
    return ecsavePath.string();
}''')

# Fix resolveActiveSaveIdentity
content = content.replace('''        auto zoneFile = entry.path() / "zone.ecsave";
        if (!std::filesystem::exists(zoneFile, ec)) {
            zoneFile = entry.path() / "zone.json";
        }''',
'''        auto zoneFile = entry.path() / "zone.ecsave";
        if (!std::filesystem::exists(zoneFile, ec)) {
            zoneFile = entry.path() / "zone.ecform";
            if (!std::filesystem::exists(zoneFile, ec)) {
                zoneFile = entry.path() / "zone.json";
            }
        }''')

content = content.replace('''        auto homeFile = entry.path() / "home.ecsave";
        if (!std::filesystem::exists(homeFile, ec)) {
            homeFile = entry.path() / "home.json";
        }''',
'''        auto homeFile = entry.path() / "home.ecsave";
        if (!std::filesystem::exists(homeFile, ec)) {
            homeFile = entry.path() / "home.ecform";
            if (!std::filesystem::exists(homeFile, ec)) {
                homeFile = entry.path() / "home.json";
            }
        }''')

with open("src/Singularity/Storage/SaveSystem.cpp", "w") as f:
    f.write(content)

print("Applied read fallback fixes.")
