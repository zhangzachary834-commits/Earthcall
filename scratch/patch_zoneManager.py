import re

with open('src/ZonesOfEarth/ZoneManager.cpp', 'r') as f:
    content = f.read()

# Target 1: saveState
target1 = """    if (!atomicWriteFile(p, j.dump(-1))) {
        std::cerr << "[ZoneManager] saveState: failed to commit " << p << "\\n";
        return;
    }"""

replacement1 = """    std::vector<uint8_t> outBytes;
    if (p.extension() == ".ecform") {
        nlohmann::json wrapper = nlohmann::json::object();
        wrapper["MigrationRoot"] = j.dump(-1);
        outBytes = nlohmann::json::to_msgpack(wrapper);
    } else {
        std::string txt = j.dump(-1);
        outBytes.assign(txt.begin(), txt.end());
    }
    if (!atomicWriteFile(p, outBytes)) {
        std::cerr << "[ZoneManager] saveState: failed to commit " << p << "\\n";
        return;
    }"""

content = content.replace(target1, replacement1)

# Target 2: legacy migration
target2 = """                    } else if (atomicWriteFile(formPath, j.dump(-1))) {
                        cleanupPredecessorMatter(oldMatterPath);"""

replacement2 = """                    } else {
                        nlohmann::json wrapper = nlohmann::json::object();
                        wrapper["MigrationRoot"] = j.dump(-1);
                        std::vector<uint8_t> outBytes = nlohmann::json::to_msgpack(wrapper);
                        if (atomicWriteFile(formPath, outBytes)) {
                            cleanupPredecessorMatter(oldMatterPath);"""

content = content.replace(target2, replacement2)

with open('src/ZonesOfEarth/ZoneManager.cpp', 'w') as f:
    f.write(content)

print("Patched ZoneManager.cpp")
