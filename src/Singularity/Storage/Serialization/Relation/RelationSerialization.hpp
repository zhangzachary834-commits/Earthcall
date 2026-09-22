#pragma once

#include "Relation/Relation.hpp"

#include <string>

// attachment.* PropertyPaths map into the canonical nested attachment object
// and therefore must never become a second authority in the fallback
// registeredProperties envelope.
bool relationRegisteredPropertyNeedsEnvelope(const std::string& propertyName);

// The persistence vocabulary of a Relation belongs to Storage, not to the
// Formation or Zone that happens to hold the edge.  Endpoints remain stable
// identifiers on disk and are only bound through the caller's resolver.
nlohmann::json relationToJson(const Relation& relation);
Relation relationFromJson(const nlohmann::json& json,
                          const RelationEndpointResolver& resolve = {});
