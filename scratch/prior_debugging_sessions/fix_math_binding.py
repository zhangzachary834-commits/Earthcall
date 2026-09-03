import re

file_path = "src/ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
with open(file_path, "r") as f:
    content = f.read()

# Change resolveLawRoot signature and logic to return a std::pair<Singular*, std::size_t>
# Or just std::tuple. Let's change the signature to:
# inline Singular* resolveLawRoot(Singular& subject, const PropertyPath& path, std::size_t& startIndex)
content = content.replace(
    "inline Singular* resolveLawRoot(Singular& subject, const PropertyPath& path,\n                                PropertyPath& remainder) {",
    "inline Singular* resolveLawRoot(Singular& subject, const PropertyPath& path,\n                                std::size_t& startIndex) {"
)
content = content.replace(
    "remainder = path;",
    "startIndex = 0;"
)
content = content.replace(
    "remainder.segments.assign(path.segments.begin() + 2, path.segments.end());",
    "startIndex = 2;"
)

# Fix lawGetValue to use startIndex
content = content.replace(
    "PropertyPath remainder;\n    Singular* root = resolveLawRoot(subject, path, remainder);\n    return root && (remainder.getValue(*root, out) == PropertyPath::PathResult::Ok);",
    "std::size_t startIndex = 0;\n    Singular* root = resolveLawRoot(subject, path, startIndex);\n    return root && (path.getValue(*root, out, startIndex) == PropertyPath::PathResult::Ok);"
)

# Fix lawSetValue to use startIndex
content = content.replace(
    "PropertyPath remainder;\n    Singular* root = resolveLawRoot(subject, path, remainder);\n    if (!root) return PropertyPath::PathResult::NoSuchProperty;\n    return remainder.setValue(*root, v);",
    "std::size_t startIndex = 0;\n    Singular* root = resolveLawRoot(subject, path, startIndex);\n    if (!root) return PropertyPath::PathResult::NoSuchProperty;\n    return path.setValue(*root, v, startIndex);"
)

# Update worldReadings to use Earthcall::StringId instead of std::string
# using WorldReading = std::function<bool(Singular& subject, PropertyValue& out)>;
# 
# inline std::map<std::string, WorldReading>& worldReadings() {
#     static std::map<std::string, WorldReading> readings;
#     return readings;
# }
content = content.replace(
    "std::map<std::string, WorldReading>",
    "std::unordered_map<Earthcall::StringId, WorldReading>"
)

# Update registerWorldReading
content = content.replace(
    "inline void registerWorldReading(const std::string& dottedName, WorldReading reading) {\n    if (dottedName.rfind(\"@world.\", 0) != 0) return;   // the referent is reserved\n    if (!reading) {\n        worldReadings().erase(dottedName);\n        return;\n    }\n    worldReadings()[dottedName] = std::move(reading);\n}",
    "inline void registerWorldReading(const std::string& dottedName, WorldReading reading) {\n    if (dottedName.rfind(\"@world.\", 0) != 0) return;   // the referent is reserved\n    Earthcall::StringId id = Earthcall::StringInterner::intern(dottedName);\n    if (!reading) {\n        worldReadings().erase(id);\n        return;\n    }\n    worldReadings()[id] = std::move(reading);\n}"
)

# Update isWorldReadingPath check
# Since we intern all combinations in PropertyPath, path._joinedIds[0].back() is the full path StringId!
# But wait, `path.segments` might not match. The old code was:
# if (isWorldReadingPath(path)) {
#     const auto& readings = worldReadings();
#     if (readings.empty()) return false;
#     const auto found = readings.find(path.toString());
#     if (found == readings.end() || !found->second) return false;
#     return found->second(subject, out);
# }
content = content.replace(
    "if (isWorldReadingPath(path)) {\n        const auto& readings = worldReadings();\n        if (readings.empty()) return false;          // no channel answers \"@world.*\"\n        const auto found = readings.find(path.toString());\n        if (found == readings.end() || !found->second) return false;\n        return found->second(subject, out);\n    }",
    "if (isWorldReadingPath(path)) {\n        const auto& readings = worldReadings();\n        if (readings.empty()) return false;          // no channel answers \"@world.*\"\n        const auto found = readings.find(path.fullId());\n        if (found == readings.end() || !found->second) return false;\n        return found->second(subject, out);\n    }"
)

content = content.replace(
    "if (isWorldReadingPath(path) && worldReadings().count(path.toString())) {",
    "if (isWorldReadingPath(path) && worldReadings().count(path.fullId())) {"
)

with open(file_path, "w") as f:
    f.write(content)
