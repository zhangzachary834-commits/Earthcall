#pragma once

#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "json.hpp"

#include <functional>
#include <string>

class Singular;

namespace Singularity::Storage {

// Shared semantic property envelope for EVERY concrete Singular persistence
// root. Concrete codecs still own irreducible structure/identity fields; this
// layer mechanically preserves:
//   * Person-authored/dynamic properties; and
//   * registered, PropertyValue-legible, semantically writable properties.
//
// Read-only/derived properties are intentionally absent: their source state is
// serialized by the concrete root that derives them.
//
// Identity-valued PropertyValues are preserve-first/bind-later. If the
// resolver cannot name the referenced Singular yet, raw JSON remains attached
// as hydration-only pending state and is emitted unchanged by the next save.
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
