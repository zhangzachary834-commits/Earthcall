#include "Singularity/Storage/Serialization/ZonesOfEarth/OurverseSerialization.hpp"

#include <iostream>

namespace {

nlohmann::json formationToJson(const Formation& formation) {
    nlohmann::json j = formation.toJson();
    // Formation::toJson is also used for derived Zone relations and stays
    // minimal. The Ourverse root needs its stable identity and semantic tag.
    j["identifier"] = formation.getIdentifier();
    j["relationTypeTag"] = formation.getRelationTypeTag();
    return j;
}

bool loadFormation(Formation& target,
                   const nlohmann::json& json,
                   const OurverseMemberResolver& resolve,
                   const std::string& fallbackIdentifier,
                   const std::string& fallbackTag) {
    if (!json.is_object()) return false;
    auto loaded = Formation::fromJson(json, resolve);
    if (!loaded) return false;
    target = *loaded;
    const std::string identifier = json.value("identifier", fallbackIdentifier);
    if (!identifier.empty()) target.setIdentifier(identifier);
    const std::string tag = json.value("relationTypeTag", fallbackTag);
    target.setRelationTypeTag(tag);
    return true;
}

} // namespace

nlohmann::json ourverseToJson(const Ourverse& ourverse) {
    nlohmann::json j;
    j["identifier"] = ourverse.getIdentifier();
    j["gatheringZone"] = ourverse.propGatheringZone();
    j["joys"] = formationToJson(ourverse.joys());
    j["filaments"] = formationToJson(ourverse.filaments());
    j["metalaws"] = formationToJson(ourverse.getLaws());
    j["convenesToward"] = ourverse.convenesToward();
    return j;
}

bool ourverseFromJson(Ourverse& ourverse,
                      const nlohmann::json& json,
                      const OurverseZoneResolver& resolveZone,
                      const OurverseMemberResolver& resolveMember) {
    if (!json.is_object() || !resolveZone || !resolveMember) {
        std::cerr << "Ourverse load refused: root or resolver is missing.\n";
        return false;
    }

    bool loadedAny = false;
    if (json.contains("gatheringZone") && json["gatheringZone"].is_string()) {
        const std::string id = json["gatheringZone"].get<std::string>();
        if (!id.empty()) {
            auto zone = resolveZone(id);
            if (!zone) {
                std::cerr << "Ourverse load: gathering Zone '" << id
                          << "' is not present; the root remains unbound.\n";
            } else {
                ourverse.setPrimaryGatheringZone(std::move(zone));
                loadedAny = ourverse.gatheringZone() != nullptr;
            }
        }
    }

    if (json.contains("joys")) {
        loadedAny = loadFormation(ourverse.joys(), json["joys"], resolveMember,
                                  "ourverse.joys", Formation::kJoyHierarchyTag)
                  || loadedAny;
    }
    if (json.contains("filaments")) {
        loadedAny = loadFormation(ourverse.filaments(), json["filaments"], resolveMember,
                                  "ourverse.filaments", "filaments")
                  || loadedAny;
    }
    if (json.contains("metalaws")) {
        loadedAny = loadFormation(ourverse.getLaws(), json["metalaws"], resolveMember,
                                  "ourverse.metalaws", "")
                  || loadedAny;
    }
    if (json.contains("convenesToward") && json["convenesToward"].is_string()) {
        ourverse.loadConvenesToward(json["convenesToward"].get<std::string>());
        loadedAny = true;
    }
    return loadedAny;
}
