#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "json.hpp"

namespace Earthcall {
namespace Storage {

// Intermediate graph-based representation of an Earthcall save
// This bridges the legacy monolithic JSON format and the upcoming 
// native Lexeme-Relation binary graph and .eclang text DSL.
struct EcformGraph {
    struct LexemeRecord {
        uint32_t id;
        std::string symbol;
    };

    struct SingularRecord {
        std::string entityId; // Currently Earthcall uses string IDs
        uint32_t kindLexemeId;
        uint32_t ownerLexemeId;
        uint32_t zoneLexemeId;
        uint32_t flags;
    };

    struct RelationRecord {
        std::string relationId;
        std::string fromEntityId;
        std::string toEntityId;
        uint32_t typeLexemeId;
        float weight;
        uint32_t timestamp;
    };
    
    struct FormationRecord {
        std::string rootEntityId;
        std::vector<std::string> memberIds;
    };
    
    struct PropertyVariant {
        uint32_t nameLexemeId;
        uint8_t typeTag; // 0=int, 1=float, 2=string, 3=ptr
        int64_t intVal = 0;
        double floatVal = 0.0;
        uint32_t strLexemeId = 0;
        std::string ptrEntityId;
    };

    struct PropertyStreamRecord {
        std::string entityId;
        std::vector<PropertyVariant> properties;
    };

    std::vector<LexemeRecord> lexemes;
    std::unordered_map<std::string, uint32_t> lexemeMap;

    std::vector<SingularRecord> singulars;
    std::vector<RelationRecord> relations;
    std::vector<FormationRecord> formations;
    std::vector<PropertyStreamRecord> properties;
    
    uint32_t getOrAddLexeme(const std::string& symbol);
};

class MigrationFramework {
public:
    // Convert a legacy JSON save into the intermediate graph representation
    static EcformGraph migrateJsonToGraph(const nlohmann::json& legacyJson);
    
    // Convert an intermediate graph representation back to legacy JSON 
    // (for fallback and to keep the engine running while the new format is wired in)
    static nlohmann::json migrateGraphToJson(const EcformGraph& graph);
    
    // Core hook: intercept a loaded JSON save and ensure it's structurally valid.
    // In the future, this will read .ecform binary or .eclang directly.
    static nlohmann::json migrateLegacySave(const nlohmann::json& inputJson);
};

} // namespace Storage
} // namespace Earthcall
