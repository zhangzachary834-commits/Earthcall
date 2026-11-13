#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {
bool expect(bool condition, const char* witness) {
    if (!condition) std::cerr << "property_path_test: " << witness << "\n";
    return condition;
}
}

int main() {
    const PropertyPath path = PropertyPath::parse("shape.color.r");
    if (!expect(path.segments == std::vector<std::string>({"shape", "color", "r"}),
                "dotted segments")) return 1;
    if (!expect(path.toString() == "shape.color.r", "round-trip spelling")) return 1;
    if (!expect(!path.empty(), "nonempty path")) return 1;

    // The resolver's longest-flat-name lookup depends on every joined run,
    // not merely the top-level segments.
    const std::vector<std::vector<std::string>> expected = {
        {"shape", "shape.color", "shape.color.r"},
        {"color", "color.r"},
        {"r"}
    };
    const auto& ids = path.joinedIds();
    if (!expect(ids.size() == expected.size(), "joined row count")) return 1;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (!expect(ids[i].size() == expected[i].size(), "joined column count")) return 1;
        for (std::size_t j = 0; j < expected[i].size(); ++j) {
            if (!expect(ids[i][j].isValid(), "joined StringId is valid")) return 1;
            if (!expect(Earthcall::StringInterner::resolve(ids[i][j]) == expected[i][j],
                        "joined StringId spells intended subpath")) return 1;
        }
    }
    if (!expect(path.fullId() == ids[0].back(), "fullId matches longest run")) return 1;
    if (!expect(PropertyPath::parse("shape.color.r").fullId() == path.fullId(),
                "reparsing preserves interned identity")) return 1;

    const PropertyPath single = PropertyPath::parse("velocity");
    if (!expect(single.segments == std::vector<std::string>({"velocity"}),
                "single-segment path")) return 1;
    if (!expect(single.joinedIds().size() == 1 && single.joinedIds()[0].size() == 1,
                "single-segment joined run")) return 1;

    const PropertyPath empty = PropertyPath::parse("");
    if (!expect(empty.empty() && empty.toString().empty(), "empty path")) return 1;
    if (!expect(empty.joinedIds().empty() && !empty.fullId().isValid(),
                "empty path has no interned address")) return 1;
    return 0;
}
