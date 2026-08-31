#pragma once
#include <string>
#include "json.hpp" // nlohmann::json single-header
#include "ConstructedBeing/Singular/Singular.hpp"
#include <vector>
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

    // BRUHHHHHHHHH WHO MADE THIS INTO "std::string" BRUHHHHHH ITS SUPPOSED TO BE A LEXEME
    Relation(const std::string& type,
             Singular& aBeing,
             Singular& bBeing,
             bool directed = false,
             float initialWeight = -1.0f);

    // const Singular& is accepted so existing call sites (physics, provenance)
    // keep compiling. The stored pointer is non-owning identity, same as
    // Formation members.
    // BRUHHHHHHHHH WHO MADE THIS INTO "std::string" BRUHHHHHH ITS SUPPOSED TO BE A LEXEME
    Relation(const std::string& type,
             const Singular& aBeing,
             const Singular& bBeing,
             bool directed = false,
             float initialWeight = -1.0f);

    // ---------------------------------------------------------------------
    // Endpoints — the beings this relation holds, not their names.
    // ---------------------------------------------------------------------
    Singular* a() const { return _a; }
    Singular* b() const { return _b; }
    bool hasEndpoints() const { return _a && _b; }

    // Identifier of an endpoint. Law-text and JSON address beings by these
    // strings; the pointer is the relation's actual state. When the being is
    // not in this world (provenance load, a dangling save), the registered
    // identifier property still holds the saved name.
    std::string aId() const { return _a ? _a->getIdentifier() : _savedA; }
    std::string bId() const { return _b ? _b->getIdentifier() : _savedB; }

    void bind(Singular* aBeing, Singular* bBeing) {
        _a = aBeing;
        _b = bBeing;
        if (_a) _savedA.clear();
        if (_b) _savedB.clear();
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

    // `type` is a hardcoded string property: the semantic tag of the bond
    // (attachment, instance-of, is_pos, …). It is not an endpoint.
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
    // Non-owning. The beings live in a Zone / LanguageSystem / Formation.
    // Registered to law as the identifier properties `entityA` / `entityB`
    // (JSON and law-text still speak identifiers). The pointer is the
    // in-memory handle of that same fact — not a second, ungoverned endpoint.
    Singular* _a = nullptr;
    Singular* _b = nullptr;
    // Saved identifier when the being is absent from this world. Same fact
    // as the registered `entityA` / `entityB` properties, not a second
    // endpoint. Bind() clears these.
    std::string _savedA;
    std::string _savedB;

    void buildProperties() override;
    std::string propEntityA() const { return aId(); }
    std::string propEntityB() const { return bId(); }
};
