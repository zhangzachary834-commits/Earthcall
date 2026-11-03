import re

with open("src/Person/PersonDatabase.cpp", "r") as f:
    content = f.read()

replacement = """    if (!std::filesystem::exists(filepath)) {
        return false;
    }
    
    try {
        nlohmann::json j = SaveSystem::readSaveData(filepath);
        outPerson.deserialize(j);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse Person json: " << e.what() << std::endl;
        return false;
    }"""

content = re.sub(r'    if \(!std::filesystem::exists\(filepath\)\) \{\n        return false;\n    \}\n    \n    std::ifstream file\(filepath\);\n    if \(!file\.is_open\(\)\) return false;\n    \n    nlohmann::json j;\n    try \{\n        file >> j;\n        outPerson\.deserialize\(j\);\n        return true;\n    \} catch \(const std::exception& e\) \{\n        std::cerr << "Failed to parse Person json: " << e\.what\(\) << std::endl;\n        return false;\n    \}', replacement, content)

with open("src/Person/PersonDatabase.cpp", "w") as f:
    f.write(content)

