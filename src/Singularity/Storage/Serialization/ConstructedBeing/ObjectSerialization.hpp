#pragma once

#include "json.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <string>

// Registered Object properties already represented canonically by the Object,
// Material, or Matter codecs must not also become a second authority inside
// registeredProperties.
bool objectRegisteredPropertyNeedsEnvelope(const std::string& propertyName);

// Semantic Object record.  Geometry-heavy state is hydrated by the matter
// channel; this declaration remains compatible with the legacy ADL surface.
void to_json(nlohmann::json& j, const Object& obj);
void from_json(const nlohmann::json& j, Object& obj);
