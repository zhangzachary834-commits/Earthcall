#pragma once

#include "json.hpp"
#include "Form/Object/Object.hpp"
#include "ZonesOfEarth/World/World.hpp"
#include <glm/gtc/type_ptr.hpp>

// Free functions enabling nlohmann::json (ADL) serialization

void to_json(nlohmann::json& j, const Object& obj);
void from_json(const nlohmann::json& j, Object& obj);

void to_json(nlohmann::json& j, const World& world);
void from_json(const nlohmann::json& j, World& world); 