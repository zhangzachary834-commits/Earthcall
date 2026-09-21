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
// and its relation-kind identity. Endpoints are Singular pointers, not
// name-strings: a string is either an authored property or a hardcoded one
// (Lexeme::symbol is the linguistic case). JSON still writes identifiers —
// that is serialization of identity, not the ontology.
//
// Relation kinds follow the same rule. A Lexeme-grounded Relation stores the
// Lexeme's stable Singular identifier in `type`; its human-readable spelling
// is available through typeLabel(). This deliberately lets two independently
// authored Relation kinds share the same spelling without becoming the same
// semantic relation. Legacy saves that only contain a type label remain
// readable, but that label is compatibility identity until the Relation is
// explicitly grounded in a kind-being.
//
// How to turn a saved identifier back into a being. Relation holds NON-OWNING
// pointers, so deserialization cannot invent endpoints.
using RelationEndpointResolver = std::function<Singular*(const std::string& identifier)>;

class Relation : public Singular {
public:
    struct AttachmentData {
        bool enabled = false;

        // WHAT IS THISSSSSSS?!?!?!??! - Zach
        glm::mat4 localOffset = glm::mat4(1.0f); // child relative to parent
        glm::vec3 parentAnchor = glm::vec3(0.0f);
        glm::vec3 childAnchor = glm::vec3(0.0f);
        bool inheritTranslation = true;
        bool inheritRotation = true;
        bool inheritScale = true;

        nlohmann::json toJson() const;
        static AttachmentData fromJson(const nlohmann::json& j);
    };

    // Kernel operations that an AUTHORED relation-kind being may choose as
    // constitutive semantic substance. The opcode is machinery, not a domain
    // kind: Persons still author which Relation kind carries it.
    // APPEND-ONLY if persisted as authored numeric data.
    enum class ConstitutiveOpcode {
        None = 0,
        CppInheritance = 1
    };

    enum class ConstitutiveStatus {
        NotApplicable,
        Holds,
        Violated,
        Invalid
    };

    static constexpr const char* kConstitutiveOpcodeProperty = "relation.constitutiveOpcode";
    static constexpr const char* kCppBeingKindProperty = "cpp.beingKind";

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

    // Lexeme-typed Relation constructors. The Lexeme is the semantic kind
    // being; `type` stores its stable identifier, not its surface spelling.
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
    void forgetTypeLexeme(const Singularity::Language::Lexeme* lexeme);
    bool hasGroundedType() const { return _typeLexeme != nullptr; }
    std::string typeLabel() const;

    // Evaluate an authored constitutive opcode, if the grounded Relation-kind
    // carries one. CppInheritance reuses ConditionNode::matchesKind — the
    // engine's existing dynamic_cast-based ontology checker. Endpoint B acts
    // as an authored type descriptor by carrying `cpp.beingKind`.
    ConstitutiveStatus evaluateConstitutive() const;

    // ---------------------------------------------------------------------
    // Endpoints — the beings this relation holds, not their names.
    // ---------------------------------------------------------------------
    // Every Endpoint counts the pointer it holds in one process-wide register,
    // so Relation::mayBeEndpoint can answer "no relation anywhere holds this
    // being" in O(1). RelationManager::forgetBeingEverywhere runs on EVERY
    // Singular destructor — every transient ECA::Event is a Moment is a
    // Singular — and used to walk every relation in every live manager (each
    // Law's formations own one). Measured 2026-09-15 in the chess world: 39.5 µs
    // of a 42 µs Moment lifetime, 92% of a Scope::Everyone event sweep's
    // per-candidate cost. Same shape as ReteNetwork::_factParticipants.
    //
    // A SUPERSET on purpose: it counts endpoints of relations no manager owns
    // too, so it can only answer "maybe" too often, never "no" wrongly. The
    // counting lives in these special members and bind/forget, which are the
    // only writes to `ptr` — keep it that way (Jules: if you add a path that
    // sets `ptr`, go through bind()).
    struct Endpoint {
        Singular* ptr = nullptr;
        std::string savedId;
        mutable std::string cachedId;

        Endpoint() = default;
        Endpoint(const Endpoint& o) : ptr(o.ptr), savedId(o.savedId), cachedId(o.cachedId) {
            retainEndpoint(ptr);
        }
        Endpoint& operator=(const Endpoint& o) {
            if (this != &o) {
                retainEndpoint(o.ptr);
                releaseEndpoint(ptr);
                ptr = o.ptr;
                savedId = o.savedId;
                cachedId = o.cachedId;
            }
            return *this;
        }
        ~Endpoint() { releaseEndpoint(ptr); }

        void bind(Singular* s) {
            retainEndpoint(s);
            releaseEndpoint(ptr);
            ptr = s;
            if (ptr) {
                savedId.clear();
                cachedId = ptr->getIdentifier();
            }
        }

        void forget(const Singular* s) {
            if (ptr && ptr == s) {
                if (savedId.empty()) savedId = cachedId;
                releaseEndpoint(ptr);
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

    // False only when no Endpoint of any live Relation holds this pointer.
    // Pointer-compared; never dereferenced (callers may be mid-destruction).
    static bool mayBeEndpoint(const Singular* being);
    static void retainEndpoint(const Singular* being);
    static void releaseEndpoint(const Singular* being);

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

    // Singular interface. `type` is already semantic identity for a grounded
    // Relation, so two kind-beings with one spelling produce distinct Relation
    // identities instead of colliding on the label.
    std::string getIdentifier() const override { return aId() + "-" + type + "-" + bId(); }

    // Canonical relation-kind identity. For Lexeme-grounded Relations this is
    // the Lexeme's unique/stable Singular id. For legacy string-only Relations
    // it remains the historical label until migration grounds the Relation in
    // a kind-being. Do not parse semantic meaning from this string; resolve the
    // kind-being and its properties when semantics matter.
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
    friend Relation relationFromJson(const nlohmann::json& json,
                                     const RelationEndpointResolver& resolve);

    Endpoint _endpointA;
    Endpoint _endpointB;
    Singularity::Language::Lexeme* _typeLexeme = nullptr;

    void buildProperties() override;
    std::string propEntityA() const { return aId(); }
    std::string propEntityB() const { return bId(); }
};
