#pragma once

#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "json.hpp"

#include <functional>
#include <string>

class Singular;

namespace Singularity::Storage {

// Shared semantic property envelope for every concrete Singular persistence
// root. Concrete codecs still own irreducible structure/identity fields;
// authored state is preserved and registered state falls back here only when
// the concrete root has no stronger canonical representation.
using RegisteredPropertyPersistenceFilter =
    std::function<bool(const std::string& propertyName)>;

void writeSingularProperties(
    nlohmann::json& j,
    const Singular& being,
    const RegisteredPropertyPersistenceFilter& includeRegistered = {});

bool readSingularProperties(
    const nlohmann::json& j,
    Singular& being,
    const PropertyReferenceResolver& resolve = {},
    const RegisteredPropertyPersistenceFilter& includeRegistered = {});

bool resolveDeferredSingularProperties(
    Singular& being,
    const PropertyReferenceResolver& resolve);

} // namespace Singularity::Storage
