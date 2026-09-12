import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.hpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """    std::vector<std::string> _requiredProperties;   // derived at recompile()"""

replace_str = """    std::vector<std::string> _requiredProperties;   // derived at recompile()
    std::vector<Condition> _compiledGates;"""

find_str2 = """    const std::vector<std::string>& requiredProperties() const { return _requiredProperties; }"""

replace_str2 = """    const std::vector<std::string>& requiredProperties() const { return _requiredProperties; }
    const std::vector<Condition>& compiledGates() const { return _compiledGates; }"""

if find_str in content and find_str2 in content:
    content = content.replace(find_str, replace_str)
    content = content.replace(find_str2, replace_str2)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched Law.hpp with compiledGates!")
else:
    print("Could not find string in Law.hpp!")
