#include "Singularity/Storage/Serialization/ZonesOfEarth/OurverseSerialization.hpp"

nlohmann::json ourverseToJson(const Ourverse& ourverse) {
    nlohmann::json j;
    j["identifier"] = ourverse.getIdentifier();
    j["gatheringZone"] = ourverse.propGatheringZone();
    j["joys"] = ourverse.joys().toJson();
    j["filaments"] = ourverse.filaments().toJson();
    j["metalaws"] = ourverse.getLaws().toJson();
    j["convenesToward"] = ourverse.convenesToward();
    return j;
}
