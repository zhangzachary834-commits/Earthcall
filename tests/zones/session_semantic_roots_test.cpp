#include "Singularity/Storage/Serialization/SessionSemanticRoots.hpp"

#include <cassert>
#include <cstdio>
#include <string>

int main() {
    nlohmann::json session{
        {kSemanticRootsKey,
         {{"format", kSemanticRootsFormat},
          {"version", kSemanticRootsVersion},
          {"zones", nlohmann::json::array({{{"identifier", "zone.from-root"}}})},
          {"zoneRefs", nlohmann::json::array({{{"identifier", "zone.from-root"}}})},
          {"person", {{"identifier", "person.root"}}}}},
        {"zones", nlohmann::json::array({{{"identifier", "zone.stale-projection"}}})},
        {"person", {{"identifier", "person.stale-projection"}}}
    };

    std::string error;
    assert(materializeSemanticRoots(session, &error) == SemanticRootsReadResult::Applied);
    assert(error.empty());
    assert(session["zones"][0]["identifier"] == "zone.from-root");
    assert(session["person"]["identifier"] == "person.root");

    nlohmann::json malformed{{kSemanticRootsKey,
                              {{"format", kSemanticRootsFormat},
                               {"version", kSemanticRootsVersion},
                               {"zones", nlohmann::json::object()},
                               {"zoneRefs", nlohmann::json::array()}}},
                             {"zones", nlohmann::json::array({{{"identifier", "do-not-use"}}})}};
    assert(materializeSemanticRoots(malformed, &error) == SemanticRootsReadResult::Malformed);
    assert(error == "semanticRoots.zones is missing or not an array");
    assert(malformed["zones"][0]["identifier"] == "do-not-use");

    std::puts("session_semantic_roots_test: ALL OK");
    return 0;
}
