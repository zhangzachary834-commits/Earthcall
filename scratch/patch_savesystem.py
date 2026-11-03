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

# Now fix create_directories

content = re.sub(
    r'(\bstd::filesystem::create_director(?:y|ies)\([^,]+,\s*ec\);[ \t\n]*)if \(ec\) \{',
    r'\g<1>if (ec && ec.value() != 0 && ec.value() != 17) { /* 17 = EEXIST */',
    content
)

# Also fix the `if (!std::filesystem::create_directories(p, ec))` patterns

content = re.sub(
    r'if \(!std::filesystem::create_directories\(([^,]+),\s*ec\)\) \{',
    r'if (!std::filesystem::create_directories(\1, ec) && !std::filesystem::exists(\1, ec)) {',
    content
)

content = re.sub(
    r'if \(!std::filesystem::create_directory\(([^,]+),\s*ec\)\) \{',
    r'if (!std::filesystem::create_directory(\1, ec) && !std::filesystem::exists(\1, ec)) {',
    content
)

with open('src/Singularity/Storage/SaveSystem.cpp', 'w') as f:
    f.write(content)

print("Patched SaveSystem.cpp cleanly")
