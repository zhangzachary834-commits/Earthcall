#pragma once

#include "Relation/Relation.hpp"

// The persistence vocabulary of a Relation belongs to Storage, not to the
// Formation or Zone that happens to hold the edge.  Endpoints remain stable
// identifiers on disk and are only bound through the caller's resolver.
nlohmann::json relationToJson(const Relation& relation);
Relation relationFromJson(const nlohmann::json& json,
                          const RelationEndpointResolver& resolve = {});
