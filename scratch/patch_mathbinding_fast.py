import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """    // HOT PATH CACHE: avoid O(N^2) string comparisons and massive vector allocations
    static uint64_t s_lastRevision = 0;
    static std::unordered_map<std::string, Singular*> s_beingMap;
    static bool s_initialized = false;
    
    uint64_t currentRevision = Universe::instance().structuralRevision();
    if (!s_initialized || s_lastRevision != currentRevision) {
        static int rebuild_count = 0;
        rebuild_count++;
        if (rebuild_count % 100 == 0) {
            printf("MathBinding cache rebuilt %d times!\\n", rebuild_count);
        }
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
    }"""

replace_str = """    // HOT PATH CACHE: avoid O(N^2) string comparisons and massive vector allocations
    static uint64_t s_lastRevision = 0;
    static std::unordered_map<Earthcall::StringId, Singular*> s_beingMap;
    static bool s_initialized = false;
    
    uint64_t currentRevision = Universe::instance().structuralRevision();
    if (!s_initialized || s_lastRevision != currentRevision) {
        s_beingMap.clear();
        const std::vector<Singular*> beings = Universe::instance().beings();
        for (Singular* being : beings) {
            if (being) {
                Earthcall::StringId key = Earthcall::StringInterner::intern("@" + being->getIdentifier());
                s_beingMap[key] = being;
            }
        }
        s_lastRevision = currentRevision;
        s_initialized = true;
    }

    Singular* best = nullptr;
    std::size_t bestConsumed = 0;
    const auto& jIds = path.joinedIds();
    if (!jIds.empty() && !jIds[0].empty()) {
        const auto& idsFromHere = jIds[0];
        for (std::size_t n = 1; n <= idsFromHere.size(); ++n) {
            auto it = s_beingMap.find(idsFromHere[n - 1]);
            if (it != s_beingMap.end()) {
                best = it->second;
                bestConsumed = n;
            }
        }
    }"""

if find_str in content:
    content = content.replace(find_str, replace_str)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched MathBinding.hpp for zero-allocation!")
else:
    print("Could not find string in MathBinding.hpp")
