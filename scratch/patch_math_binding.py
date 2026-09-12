import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """    // So root resolution does what PropertyPath::resolve already does one level
    // down: LONGEST dotted-name match first, most specific wins. A being named
    // "material.clay" beats one named "material", and the segments it consumed
    // are not offered to the property lookup.
    const std::vector<Singular*> beings = Universe::instance().beings();
    std::string candidate = path.segments[0].substr(1);
    Singular* best = nullptr;
    std::size_t bestConsumed = 0;
    for (std::size_t n = 1; n <= path.segments.size(); ++n) {
        if (n > 1) candidate += "." + path.segments[n - 1];
        for (Singular* being : beings) {
            if (being && being->getIdentifier() == candidate) {
                best = being;
                bestConsumed = n;
                break;
            }
        }
    }
    if (!best) return nullptr;   // the named being is not in the world: no value"""

replace_str = """    // So root resolution does what PropertyPath::resolve already does one level
    // down: LONGEST dotted-name match first, most specific wins. A being named
    // "material.clay" beats one named "material", and the segments it consumed
    // are not offered to the property lookup.
    
    // HOT PATH CACHE: avoid O(N^2) string comparisons and massive vector allocations
    static uint64_t s_lastRevision = 0;
    static std::unordered_map<std::string, Singular*> s_beingMap;
    static bool s_initialized = false;
    
    uint64_t currentRevision = Universe::instance().structuralRevision();
    if (!s_initialized || s_lastRevision != currentRevision) {
        s_beingMap.clear();
        const std::vector<Singular*> beings = Universe::instance().beings();
        for (Singular* being : beings) {
            if (being) {
                s_beingMap[being->getIdentifier()] = being;
            }
        }
        s_lastRevision = currentRevision;
        s_initialized = true;
    }

    std::string candidate = path.segments[0].substr(1);
    Singular* best = nullptr;
    std::size_t bestConsumed = 0;
    for (std::size_t n = 1; n <= path.segments.size(); ++n) {
        if (n > 1) candidate += "." + path.segments[n - 1];
        auto it = s_beingMap.find(candidate);
        if (it != s_beingMap.end()) {
            best = it->second;
            bestConsumed = n;
        }
    }
    
    if (!best) return nullptr;   // the named being is not in the world: no value"""

if find_str not in content:
    print("Could not find string to replace!")
    sys.exit(1)

content = content.replace(find_str, replace_str)

with open(file_path, "w") as f:
    f.write(content)
print("Patched MathBinding.hpp")
