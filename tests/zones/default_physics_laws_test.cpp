// Default physics laws are first movers — engine bootstrap, not a second
// register of "authored physics" sitting above the First Mover block.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/Physics/DefaultPhysicsLaws.hpp"
#include "json.hpp"

#include <cstdio>
#include <set>
#include <string>

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++g_failures;
        std::printf("  FAILED: %s\n", what.c_str());
        return;
    }
    std::printf("  ok: %s\n", what.c_str());
}

} // namespace

int main() {
    std::printf("Running default physics first-mover test...\n");

    const auto laws = Physics::createDefaultPhysicsLaws();
    check(!laws.empty(), "the factory seeds at least one law");

    const char* expected[] = {
        "physics-gravity",
        "physics-collision",
        "physics-kinematics",
        "physics-acoustics",
        "physics-acoustics-envelope",
        "physics-acoustics-vibrato",
        "physics-acoustics-occlusion",
        "physics-acoustics-decay",
    };
    check(laws.size() == sizeof(expected) / sizeof(expected[0]),
          "the factory's census is the known bootstrap suite");

    std::set<std::string> ids;
    for (const auto& law : laws) {
        check(law != nullptr, "every seeded law exists");
        if (!law) continue;
        check(law->isFirstMover(),
              std::string(law->name()) + " is a first mover");
        ids.insert(law->getIdentifier());
    }
    for (const char* id : expected) {
        check(ids.count(id) == 1, std::string(id) + " is present and unique");
    }

    LawManager mgr;
    for (const auto& law : laws) mgr.add(law);
    const nlohmann::json saved = mgr.toJson();
    bool serialized = false;
    if (saved.contains("laws")) {
        for (const auto& lj : saved["laws"]) {
            const std::string id = lj.value("id", std::string());
            if (ids.count(id)) serialized = true;
        }
    }
    check(!serialized, "bootstrap physics is not written into the world save");

    std::printf(g_failures == 0 ? "default_physics_laws_test: ALL OK\n"
                                : "default_physics_laws_test: FAILURES\n");
    return g_failures > 0 ? 1 : 0;
}
