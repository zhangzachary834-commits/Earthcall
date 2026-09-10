#pragma once

#include "json.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

// Semantic Object record.  Geometry-heavy state is hydrated by the matter
// channel; this declaration remains compatible with the legacy ADL surface.
void to_json(nlohmann::json& j, const Object& obj);
void from_json(const nlohmann::json& j, Object& obj);
