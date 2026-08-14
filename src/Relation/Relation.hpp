#pragma once
#include <string>
#include "json.hpp" // nlohmann::json single-header
#include "ConstructedBeing/Singular/Singular.hpp"
#include <vector>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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
    struct AttachmentData {
        bool enabled = false;
        glm::mat4 localOffset = glm::mat4(1.0f); // child relative to parent
        glm::vec3 parentAnchor = glm::vec3(0.0f);
        glm::vec3 childAnchor = glm::vec3(0.0f);
        bool inheritTranslation = true;
        bool inheritRotation = true;
        bool inheritScale = true;

        nlohmann::json toJson() const;
        static AttachmentData fromJson(const nlohmann::json& j);
    };

    // ---------------------------------------------------------------------
    // Constructors
    // ---------------------------------------------------------------------
    Relation() = default;
    Relation(const std::string& type,
             const std::string& a,
             const std::string& b,
             bool directed = false,
              float initialWeight = -1.0f);

    // Convenience constructor for working directly with "singular" objects
    Relation(const std::string& type,
             const Singular& aEntity,
             const Singular& bEntity,
             bool directed = false,
              float initialWeight = -1.0f);

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
    bool isAttachment() const { return type == "attachment" || attachment.enabled; }

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

    // Developer mode flag for fallback auditing
    static bool s_developerMode;
    float getWeight() const;
    void setWeight(const float& w);
    
    bool directed = false;
    

    // Timeline of interaction events that influenced this relation
    std::vector<RelationEvent> events;
    AttachmentData attachment;

    void addEvent(const RelationEvent& e) { events.push_back(e); }

    // Refusal 6 getters/setters
    bool getAttachmentEnabled() const { return attachment.enabled; }
    void setAttachmentEnabled(const bool& v) { attachment.enabled = v; }
    glm::mat4 getAttachmentLocalOffset() const { return attachment.localOffset; }
    void setAttachmentLocalOffset(const glm::mat4& v) { attachment.localOffset = v; }
    glm::vec3 getAttachmentParentAnchor() const { return attachment.parentAnchor; }
    void setAttachmentParentAnchor(const glm::vec3& v) { attachment.parentAnchor = v; }
    glm::vec3 getAttachmentChildAnchor() const { return attachment.childAnchor; }
    void setAttachmentChildAnchor(const glm::vec3& v) { attachment.childAnchor = v; }
    bool getAttachmentInheritTranslation() const { return attachment.inheritTranslation; }
    void setAttachmentInheritTranslation(const bool& v) { attachment.inheritTranslation = v; }
    bool getAttachmentInheritRotation() const { return attachment.inheritRotation; }
    void setAttachmentInheritRotation(const bool& v) { attachment.inheritRotation = v; }
    bool getAttachmentInheritScale() const { return attachment.inheritScale; }
    void setAttachmentInheritScale(const bool& v) { attachment.inheritScale = v; }

    std::shared_ptr<PropertyList> getEventsList() const;
    void setEventsList(const std::shared_ptr<PropertyList>& list);

private:
    // A Relation is a legible Singular like any being: its semantic tag,
    // strength, direction, and endpoints address through PropertyPath —
    // so conditions can ask "@rel-id.weight > 0.5" and metalaw-shaped laws
    // can govern relations. Endpoints are read-only: a relation's identity
    // IS its endpoints (getIdentifier derives from them).
    void buildProperties() override;
    std::string propEntityA() const { return entityA; }
    std::string propEntityB() const { return entityB; }
};
