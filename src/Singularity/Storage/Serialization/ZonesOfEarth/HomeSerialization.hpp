#pragma once

#include "json.hpp"
#include "ZonesOfEarth/HomesOfEarth/Home.hpp"

// Home adds dwelling state to Zone; it does not duplicate Zone serialization.
void homeToJson(nlohmann::json& j, const Home& home);
void homeFromJson(const nlohmann::json& j, Home& home);
