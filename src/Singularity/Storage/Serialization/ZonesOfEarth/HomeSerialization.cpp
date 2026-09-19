#include "Singularity/Storage/Serialization/ZonesOfEarth/HomeSerialization.hpp"

void homeToJson(nlohmann::json& j, const Home& home) {
    j["being"] = "home";
    j["primary"] = home.isPrimaryHome();
    j["entryRequiresWill"] = home.entryRequiresWill();
    j["cannotForceStay"] = home.cannotForceStay();
    j["stakes"] = home.stakeIds();
    j["inhabitants"] = home.inhabitantIds();
}

void homeFromJson(const nlohmann::json& j, Home& home) {
    if (j.value("primary", false)) home.markPrimaryHome();
    if (j.contains("stakes") && j["stakes"].is_array()) {
        std::vector<std::string> ids;
        for (const auto& s : j["stakes"]) {
            if (s.is_string()) ids.push_back(s.get<std::string>());
        }
        home.loadStakeIds(std::move(ids));
    }
    if (j.contains("inhabitants") && j["inhabitants"].is_array()) {
        std::vector<std::string> ids;
        for (const auto& s : j["inhabitants"]) {
            if (s.is_string()) ids.push_back(s.get<std::string>());
        }
        home.loadInhabitantIds(std::move(ids));
    }
}
