import re

with open("src/Singularity/Storage/SaveSystem.cpp", "r") as f:
    content = f.read()

# Add include for BinarySerializer if missing
if "BinarySerializer.hpp" not in content:
    content = content.replace('#include "Singularity/Storage/SaveSystem.hpp"', 
                              '#include "Singularity/Storage/SaveSystem.hpp"\n#include "Singularity/Storage/Serialization/BinarySerializer.hpp"')

# Replace write instances of .ecform with .ecsave
# makeFilename(..., ".ecform") -> ".ecsave"
content = content.replace('makeFilename(customLabel, type, ".ecform")', 'makeFilename(customLabel, type, ".ecsave")')

# "zone.ecform" -> "zone.ecsave" for WRITES, but wait! If we change writes, we must still check BOTH on reads.
# It's safer to just replace all literal "zone.ecform" strings with "zone.ecsave" where they are written, 
# but readZoneIdentity already falls back: 
# auto zoneFile = entry.path() / "zone.ecform";
# if (!std::filesystem::exists(zoneFile, ec)) zoneFile = entry.path() / "zone.ecsave";
# Actually, wait, let's fix the specific readSaveData logic first.

old_read_logic = """    // Check magic bytes or extension to determine if it's msgpack
    if (actualPath.length() > 7 && actualPath.substr(actualPath.length() - 7) == ".ecform") {
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        try {
            nlohmann::json wrapper = nlohmann::json::from_msgpack(bytes);
            if (wrapper.contains("MigrationRoot") && wrapper["MigrationRoot"].is_string()) {
                nlohmann::json root = nlohmann::json::parse(wrapper["MigrationRoot"].get<std::string>());
                return Earthcall::Storage::MigrationFramework::migrateLegacySave(root);
            }
            return Earthcall::Storage::MigrationFramework::migrateLegacySave(wrapper);
        } catch (...) {
            try {
                nlohmann::json j = nlohmann::json::parse(bytes);
                return Earthcall::Storage::MigrationFramework::migrateLegacySave(j);
            } catch (const std::exception& e) {
                std::cerr << "[SaveSystem] Malformed msgpack ecform (and not valid JSON either) in: " << filepath << "\\n";
                return nlohmann::json();
            }
        }
    } else if (actualPath.length() > 7 && actualPath.substr(actualPath.length() - 7) == ".ecsave") {"""

new_read_logic = """    // Check magic bytes or extension to determine if it's msgpack or ecform
    if (actualPath.length() > 7 && (actualPath.substr(actualPath.length() - 7) == ".ecform" || actualPath.substr(actualPath.length() - 7) == ".ecsave")) {
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        
        // Check for true EcformGraph binary magic bytes ("ECFM")
        if (bytes.size() >= 4 && bytes[0] == 0x45 && bytes[1] == 0x43 && bytes[2] == 0x46 && bytes[3] == 0x4D) {
            Earthcall::Storage::EcformGraph graph;
            if (Earthcall::Storage::BinarySerializer::readBinary(actualPath, graph)) {
                return Earthcall::Storage::MigrationFramework::migrateGraphToJson(graph);
            } else {
                std::cerr << "[SaveSystem] Failed to parse true EcformGraph binary: " << filepath << "\\n";
                return nlohmann::json();
            }
        }
        
        try {
            nlohmann::json wrapper = nlohmann::json::from_msgpack(bytes);
            if (wrapper.contains("MigrationRoot") && wrapper["MigrationRoot"].is_string()) {
                nlohmann::json root = nlohmann::json::parse(wrapper["MigrationRoot"].get<std::string>());
                return Earthcall::Storage::MigrationFramework::migrateLegacySave(root);
            }
            return Earthcall::Storage::MigrationFramework::migrateLegacySave(wrapper);
        } catch (...) {
            try {
                // Check if it's compressed .ecsave payload
                std::vector<uint8_t> decompressed = decompressData(bytes);
                return nlohmann::json::from_msgpack(decompressed);
            } catch (...) {
                try {
                    nlohmann::json j = nlohmann::json::parse(bytes);
                    return Earthcall::Storage::MigrationFramework::migrateLegacySave(j);
                } catch (const std::exception& e) {
                    std::cerr << "[SaveSystem] Malformed msgpack/ecform (and not valid JSON either) in: " << filepath << "\\n";
                    return nlohmann::json();
                }
            }
        }
    } else if (actualPath.length() > 7 && actualPath.substr(actualPath.length() - 7) == ".invalid_extension") {"""

content = content.replace(old_read_logic, new_read_logic)

# Replace write operations to use .ecsave instead of .ecform for MSGPACK
content = content.replace('filename.length() > 7 && filename.substr(filename.length() - 7) == ".ecform"', 
                          'filename.length() > 7 && filename.substr(filename.length() - 7) == ".ecsave"')

# We should also replace the string occurrences in write functions, but carefully.
content = content.replace('"zone.ecform"', '"zone.ecsave"')
content = content.replace('"home.ecform"', '"home.ecsave"')

# But wait, readZoneIdentity needs to check both!
# Let's see how readZoneIdentity works. It might just use what is passed, or it looks for specific files.
# Let's write the patched content out for now and examine it.
with open("src/Singularity/Storage/SaveSystem.cpp", "w") as f:
    f.write(content)
print("Patched.")
