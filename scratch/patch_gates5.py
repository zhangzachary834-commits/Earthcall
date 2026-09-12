import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.hpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("std::vector<Condition> _compiledGates;", "std::vector<ConditionPredicate> _compiledGates;")
content = content.replace("const std::vector<Condition>& compiledGates() const { return _compiledGates; }", "const std::vector<ConditionPredicate>& compiledGates() const { return _compiledGates; }")

with open(file_path, "w") as f:
    f.write(content)
print("Patched Law.hpp with ConditionPredicate!")
