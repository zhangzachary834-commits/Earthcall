#pragma once

#include "json.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include "Person/Body/Body.hpp"

// Body is a Person constitutive vessel.  BodyPart remains a Formation of
// visual Objects; it is not promoted to a generic world Object by persistence.
nlohmann::json bodyPartToJson(const BodyPart& part);
void bodyPartFromJson(const nlohmann::json& j, BodyPart& part);
nlohmann::json bodyToJson(const Body& body);
void bodyFromJson(const nlohmann::json& j, Body& body);
