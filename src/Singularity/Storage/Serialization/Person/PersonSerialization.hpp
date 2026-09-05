#pragma once

#include "json.hpp"

class Person;

// Person-root profile codec. The payload intentionally remains the existing
// PersonDatabase shape so profile files are not migrated by this extraction.
nlohmann::json personToJson(const Person& person);
void personFromJson(const nlohmann::json& json, Person& person);
