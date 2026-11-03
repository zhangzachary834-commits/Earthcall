import re

with open('src/ZonesOfEarth/ZoneManager.cpp', 'r') as f:
    content = f.read()

target = """                    } else {
                        nlohmann::json wrapper = nlohmann::json::object();
                        wrapper["MigrationRoot"] = j.dump(-1);
                        std::vector<uint8_t> outBytes = nlohmann::json::to_msgpack(wrapper);
                        if (atomicWriteFile(formPath, outBytes)) {
                            cleanupPredecessorMatter(oldMatterPath);
                        logIo("Migrated legacy save '" + filename +
                              "' to split substrate (.ecform + generation-coupled .ecmatter).");
                    } else {"""

replacement = """                    } else {
                        nlohmann::json wrapper = nlohmann::json::object();
                        wrapper["MigrationRoot"] = j.dump(-1);
                        std::vector<uint8_t> outBytes = nlohmann::json::to_msgpack(wrapper);
                        if (atomicWriteFile(formPath, outBytes)) {
                            cleanupPredecessorMatter(oldMatterPath);
                            logIo("Migrated legacy save '" + filename +
                                  "' to split substrate (.ecform + generation-coupled .ecmatter).");
                        } else {"""

content = content.replace(target, replacement)

# Add one more closing brace after the error block
target_closing = """                        std::cerr << "[ZoneManager] Legacy migration root write FAILED for "
                                  << filename << "\\n";
                    }
                }
            }"""
replacement_closing = """                        std::cerr << "[ZoneManager] Legacy migration root write FAILED for "
                                  << filename << "\\n";
                        }
                    }
                }
            }"""
content = content.replace(target_closing, replacement_closing)


with open('src/ZonesOfEarth/ZoneManager.cpp', 'w') as f:
    f.write(content)

print("Patched ZoneManager.cpp again")
