#include "Singularity/Storage/Serialization/SessionSemanticRoots.hpp"

#include "Person/Person.hpp"
#include "Singularity/Storage/Serialization/Person/PersonSerialization.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/OurverseSerialization.hpp"
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"

namespace {

SemanticRootsReadResult malformed(std::string* error, const std::string& message) {
    if (error) *error = message;
    return SemanticRootsReadResult::Malformed;
}

} // namespace

void writeSemanticRoots(nlohmann::json& session,
                        nlohmann::json zones,
                        nlohmann::json zoneRefs,
                        const Person* person,
                        const Ourverse* ourverse) {
    nlohmann::json roots;
    roots["format"] = kSemanticRootsFormat;
    roots["version"] = kSemanticRootsVersion;
    roots["zones"] = std::move(zones);
    roots["zoneRefs"] = std::move(zoneRefs);
    if (person) roots["person"] = personToJson(*person);
    if (ourverse) roots["ourverse"] = ourverseToJson(*ourverse);

    // The direct person/ourverse records were an intermediate migration shape.
    // A current session records each root only inside semanticRoots.
    session.erase("person");
    session.erase("ourverse");
    session[kSemanticRootsKey] = std::move(roots);

    // Zone identity is still dual-read by the existing First-Mover surface.
    // These are compatibility projections, not additional Person/Ourverse
    // payloads; their retirement needs the identity-store migration as well.
    const nlohmann::json zonesProjection = session[kSemanticRootsKey]["zones"];
    const nlohmann::json refsProjection = session[kSemanticRootsKey]["zoneRefs"];
    session["zones"] = zonesProjection;
    session["zoneRefs"] = refsProjection;
}

SemanticRootsReadResult materializeSemanticRoots(nlohmann::json& session,
                                                  std::string* error) {
    if (!session.contains(kSemanticRootsKey)) return SemanticRootsReadResult::Absent;

    const auto& roots = session[kSemanticRootsKey];
    if (!roots.is_object()) return malformed(error, "semanticRoots is not an object");
    if (roots.value("format", std::string{}) != kSemanticRootsFormat) {
        return malformed(error, "semanticRoots format is not earthcall.semantic-roots");
    }
    if (roots.value("version", 0) != kSemanticRootsVersion) {
        return malformed(error, "semanticRoots version is unsupported");
    }
    if (!roots.contains("zones") || !roots["zones"].is_array()) {
        return malformed(error, "semanticRoots.zones is missing or not an array");
    }
    if (!roots.contains("zoneRefs") || !roots["zoneRefs"].is_array()) {
        return malformed(error, "semanticRoots.zoneRefs is missing or not an array");
    }
    if (roots.contains("person") && !roots["person"].is_object()) {
        return malformed(error, "semanticRoots.person is not an object");
    }
    if (roots.contains("ourverse") && !roots["ourverse"].is_object()) {
        return malformed(error, "semanticRoots.ourverse is not an object");
    }

    // The rest of ZoneManager is still intentionally ordered around the
    // long-standing top-level view. Materialize only after validation, then
    // let its proven staged hydration bind every cross-root identifier.
    session["zones"] = roots["zones"];
    session["zoneRefs"] = roots["zoneRefs"];
    if (roots.contains("person")) session["person"] = roots["person"];
    else session.erase("person");
    if (roots.contains("ourverse")) session["ourverse"] = roots["ourverse"];
    else session.erase("ourverse");
    return SemanticRootsReadResult::Applied;
}
