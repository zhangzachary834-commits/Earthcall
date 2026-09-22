import re

with open("src/ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp", "r") as f:
    content = f.read()

new_func = """void resolveSemanticTokenSlowPath(Singular* root, PropertyValue& out);

inline bool lawGetValue(Singular& subject, const PropertyPath& path, PropertyValue& out) {
    if (isTimePath(path)) return lawGetTime(path, out);
    if (isWorldReadingPath(path)) {
        const auto& readings = worldReadings();
        if (readings.empty()) return false;          // no channel answers "@world.*"
        const auto found = readings.find(path.fullId());
        if (found == readings.end() || !found->second) return false;
        return found->second(subject, out);
    }
    std::size_t startIndex = 0;
    Singular* root = resolveLawRoot(subject, path, startIndex);
    bool ok = root && (path.getValue(*root, out, startIndex) == PropertyPath::PathResult::Ok);
    if (ok && out.index() == 15) {
        resolveSemanticTokenSlowPath(root, out);
    }
    return ok;
}"""

content = re.sub(
    r'inline bool lawGetValue\(Singular& subject, const PropertyPath& path, PropertyValue& out\) \{.*?return root && \(path\.getValue\(\*root, out, startIndex\) == PropertyPath::PathResult::Ok\);\n\}',
    new_func,
    content,
    flags=re.DOTALL
)

with open("src/ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp", "w") as f:
    f.write(content)
