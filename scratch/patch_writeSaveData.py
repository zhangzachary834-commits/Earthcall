import re

with open('src/Singularity/Storage/SaveSystem.cpp', 'r') as f:
    content = f.read()

target = """std::string writeSaveData(const nlohmann::json& j, const std::string& customLabel, SaveType type) {
    std::string filename = makeFilename(customLabel, type, ".ecform");
    if (filename.empty()) return "";

    bool success = atomicWriteFile(filename, [&](std::ostream& out) {
        out << j.dump(-1);
        return static_cast<bool>(out);
    });"""

replacement = """std::string writeSaveData(const nlohmann::json& j, const std::string& customLabel, SaveType type) {
    std::string filename = makeFilename(customLabel, type, ".ecform");
    if (filename.empty()) return "";

    bool success = false;
    if (filename.length() > 7 && filename.substr(filename.length() - 7) == ".ecform") {
        nlohmann::json wrapper = nlohmann::json::object();
        wrapper["MigrationRoot"] = j.dump(-1);
        std::vector<uint8_t> outBytes = nlohmann::json::to_msgpack(wrapper);
        success = atomicWriteFile(filename, [&](std::ostream& out) {
            out.write(reinterpret_cast<const char*>(outBytes.data()), outBytes.size());
            return static_cast<bool>(out);
        });
    } else {
        success = atomicWriteFile(filename, [&](std::ostream& out) {
            out << j.dump(-1);
            return static_cast<bool>(out);
        });
    }"""

content = content.replace(target, replacement)

with open('src/Singularity/Storage/SaveSystem.cpp', 'w') as f:
    f.write(content)

print("Patched writeSaveData in SaveSystem.cpp")
