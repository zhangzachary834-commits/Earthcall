#pragma once
#include <string>
#include "json.hpp" // nlohmann::json single-header
#include "Singular.hpp"
#include <vector>
#include <ctime>

// Forward declare for JSON definitions later
struct RelationEvent {
    std::time_t timestamp{0};
    std::string description;
    float deltaWeight{0.0f};

    nlohmann::json toJson() const {
        return nlohmann::json{{"timestamp", timestamp}, {"description", description}, {"deltaWeight", deltaWeight}};
    }

    static RelationEvent fromJson(const nlohmann::json& j) {
        RelationEvent e;
        e.timestamp   = j.value("timestamp", 0L);
        e.description = j.value("description", "");
        e.deltaWeight = j.value("deltaWeight", 0.0f);
        return e;
    }
};

// Lightweight representation of a relationship between two named entities.
// The semantics of the relationship are expressed via the free-form `type`
// string (e.g. "friend", "parent", "owns", etc.).
//
// Relationships can be directed (A -> B) or undirected (A <-> B) and may
// optionally carry a numeric `weight` describing the strength/importance of
// the relation.
class Relation : public Singular {
public:
    // ---------------------------------------------------------------------
    // Constructors
    // ---------------------------------------------------------------------
    Relation() = default;
    Relation(const std::string& type,
             const std::string& a,
             const std::string& b,
             bool directed = false,
             float weight = 1.0f);

    // Convenience constructor for working directly with "singular" objects
    Relation(const std::string& type,
             const Singular& aEntity,
             const Singular& bEntity,
             bool directed = false,
             float weight = 1.0f);

    // ---------------------------------------------------------------------
    // Introspection / Queries
    // ---------------------------------------------------------------------
    // Human-readable description to stdout (for quick debugging).
    void describe() const;

    // Returns true if either endpoint matches the supplied entity name.
    bool involves(const std::string& entity) const;
    bool involves(const Singular& entity) const;

    // Returns true if this relation connects the two supplied entities.
    // For undirected relations, order does not matter. For directed
    // relations, the order must match exactly (a == entityA and
    // b == entityB).
    bool isBetween(const std::string& a, const std::string& b) const;
    bool isBetween(const Singular& aEntity, const Singular& bEntity) const;

    // ---------------------------------------------------------------------
    // (De)Serialization helpers
    // ---------------------------------------------------------------------
    nlohmann::json toJson() const;
    static Relation fromJson(const nlohmann::json& j);

    // Singular interface
    std::string getIdentifier() const override { return entityA + "-" + type + "-" + entityB; }

    // ---------------------------------------------------------------------
    // Public data members (simple POD for ease of use). If stronger
    // encapsulation is desired in the future, switch to private with
    // getters/setters.
    // ---------------------------------------------------------------------
    std::string type;     // semantic tag of the relationship
    std::string entityA;  // first endpoint
    std::string entityB;  // second endpoint
    bool directed = false;
    float weight = 1.0f;

    // Timeline of interaction events that influenced this relation
    std::vector<RelationEvent> events;

    void addEvent(const RelationEvent& e) { events.push_back(e); }
};
