#pragma once

#include "json.hpp"

#include <string>

class Person;
class Ourverse;

// A session is Storage's orchestration record, not a new ontology kind.  It
// names concrete Singular roots while each root's own codec owns its payload.
inline constexpr const char* kSemanticRootsKey = "semanticRoots";
inline constexpr const char* kSemanticRootsFormat = "earthcall.semantic-roots";
inline constexpr int kSemanticRootsVersion = 1;

enum class SemanticRootsReadResult {
    Absent,
    Applied,
    Malformed,
};

// Writes current semantic roots once. Zone arrays remain mirrored at the
// legacy session keys during the transition because Zone identity snapshots
// and existing First-Mover tools still address them there.
void writeSemanticRoots(nlohmann::json& session,
                        nlohmann::json zones,
                        nlohmann::json zoneRefs,
                        const Person* person,
                        const Ourverse* ourverse);

// Materializes a root envelope into the legacy in-memory view used by the
// current load pipeline. Semantic roots take precedence when both are present;
// a malformed advertised root is refused rather than silently falling back.
SemanticRootsReadResult materializeSemanticRoots(nlohmann::json& session,
                                                  std::string* error = nullptr);
