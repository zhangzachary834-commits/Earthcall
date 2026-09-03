#pragma once
#include <string>
#include "json.hpp" // nlohmann::json single-header
#include "ConstructedBeing/Singular/Singular.hpp"
#include <vector>

namespace Singularity {
namespace Language {
class Lexeme;
}
}
#include <ctime>
#include <functional>
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

// A Relation is a first-class Singular whose identity IS its two endpoints
// and its type. Endpoints are Singular pointers, not name-strings: a string
// is either an authored property or a hardcoded one (Lexeme::symbol is the
// linguistic case). JSON still writes identifiers — that is serialization of
// the pointer, not the ontology.
//
// How to turn a saved identifier back into a being. Relation holds NON-OWNING
// pointers, so deserialization cannot invent endpoints.
using RelationEndpointResolver = std::function<Singular*(const std::string& identifier)>;

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
             Singular& aBeing,
             Singular& bBeing,
             bool directed = false,
             float initialWeight = -1.0f);

    Relation(const std::string& type,
             const Singular& aBeing,
             const Singular& bBeing,
             bool directed = false,
             float initialWeight = -1.0f);

    // Lexeme-typed Relation constructors
    Relation(Singularity::Language::Lexeme& typeLexeme,
             Singular& aBeing,
             Singular& bBeing,
             bool directed = false,
             float initialWeight = -1.0f);

    Relation(Singularity::Language::Lexeme& typeLexeme,
             const Singular& aBeing,
             const Singular& bBeing,
             bool directed = false,
             float initialWeight = -1.0f);

    Singularity::Language::Lexeme* getTypeLexeme() const { return _typeLexeme; }
    void setTypeLexeme(Singularity::Language::Lexeme* lexeme);

    // ---------------------------------------------------------------------
    // Endpoints — the beings this relation holds, not their names.
    // ---------------------------------------------------------------------
    struct Endpoint {
        Singular* ptr = nullptr;
        std::string savedId;
        mutable std::string cachedId;

        void bind(Singular* s) {
            ptr = s;
            if (ptr) savedId.clear();
        }

        void forget(const Singular* s) {
            if (ptr && ptr == s) {
                if (savedId.empty()) savedId = cachedId;
                ptr = nullptr;
            }
        }

        std::string id() const {
            if (ptr) {
                cachedId = ptr->getIdentifier();
                return cachedId;
            }
            return savedId;
        }

        bool hasValue() const { return ptr != nullptr; }
    };

    Singular* a() const { return _endpointA.ptr; }
    Singular* b() const { return _endpointB.ptr; }
    bool hasEndpoints() const { return _endpointA.hasValue() && _endpointB.hasValue(); }

    std::string aId() const { return _endpointA.id(); }
    std::string bId() const { return _endpointB.id(); }

    void bind(Singular* aBeing, Singular* bBeing) {
        _endpointA.bind(aBeing);
        _endpointB.bind(bBeing);
    }

    void forgetEndpoint(const Singular* being) {
        if (!being) return;
        _endpointA.forget(being);
        _endpointB.forget(being);
    }



    // ---------------------------------------------------------------------
    // Introspection / Queries
    // ---------------------------------------------------------------------
    void describe() const;

    bool involves(const Singular* being) const;
    bool involves(const Singular& being) const;
    bool involves(const std::string& identifier) const; // query by saved id

    bool isBetween(const Singular& aBeing, const Singular& bBeing) const;
    bool isBetween(const std::string& a, const std::string& b) const; // query by saved id

    // ---------------------------------------------------------------------
    // (De)Serialization helpers
    // ---------------------------------------------------------------------
    nlohmann::json toJson() const;
    static Relation fromJson(const nlohmann::json& j,
                             const RelationEndpointResolver& resolve = {});
    bool isAttachment() const { return type == "attachment" || attachment.enabled; }

    // Singular interface
    std::string getIdentifier() const override { return aId() + "-" + type + "-" + bId(); }

    // `type` is the string symbol/tag of the bond (attachment, instance-of, is_pos, …).
    // When grounded in a Lexeme, `_typeLexeme` points to that Lexeme being.
    std::string type;

    static bool s_developerMode;
    float getWeight() const;
    void setWeight(const float& w);

    bool directed = false;

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
    Endpoint _endpointA;
    Endpoint _endpointB;
    Singularity::Language::Lexeme* _typeLexeme = nullptr;

    void buildProperties() override;
    std::string propEntityA() const { return aId(); }
    std::string propEntityB() const { return bId(); }
};
