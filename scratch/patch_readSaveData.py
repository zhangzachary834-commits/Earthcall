import re

with open('src/Singularity/Storage/SaveSystem.cpp', 'r') as f:
    content = f.read()

target = """    // Check magic bytes or extension to determine if it's msgpack
    if (actualPath.length() > 7 && actualPath.substr(actualPath.length() - 7) == ".ecsave") {"""

replacement = """    // Check magic bytes or extension to determine if it's msgpack
    if (actualPath.length() > 7 && actualPath.substr(actualPath.length() - 7) == ".ecform") {
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        try {
            nlohmann::json loaded = nlohmann::json::from_msgpack(bytes);
            nlohmann::json legacy;
            if (loaded.contains("MigrationRoot") && loaded["MigrationRoot"].is_string()) {
                legacy = nlohmann::json::parse(loaded["MigrationRoot"].get<std::string>());
            } else {
                legacy = loaded;
            }
            return Earthcall::Storage::MigrationFramework::migrateLegacySave(legacy);
        } catch (const std::exception& e) {
            std::cerr << "[SaveSystem] Malformed ecform msgpack in: " << filepath << " : " << e.what() << "\\n";
            // If it fails to parse as msgpack, try parsing as JSON (in case a test wrote plain JSON to .ecform)
            in.clear();
            in.seekg(0, std::ios::beg);
            nlohmann::json j;
            try {
                in >> j;
                return Earthcall::Storage::MigrationFramework::migrateLegacySave(j);
            } catch (...) {
                return nlohmann::json();
            }
        }
    } else if (actualPath.length() > 7 && actualPath.substr(actualPath.length() - 7) == ".ecsave") {"""

new_content = content.replace(target, replacement)

with open('src/Singularity/Storage/SaveSystem.cpp', 'w') as f:
    f.write(new_content)

print("Patched SaveSystem.cpp")
