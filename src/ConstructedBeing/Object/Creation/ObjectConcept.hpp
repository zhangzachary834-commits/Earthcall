#pragma once

#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Object/Creation/PropertyMapping.hpp"
#include "Relation/RelationManager.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "json.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

// The word for the thing (LAW_AND_CREATION_SYSTEM.md §7a): an extra-spatial
// Object storing the CONCEPT of a set of objects for later instantiation —
// the abstraction gesture made durable.
//
// Captured from a selection; instantiable anywhere, any number of times; each
// instantiation is an independent being (SdfNode deep-copy semantics), derived
// from its sources through PropertyMappings, with provenance recorded by
// identifier (abstracted-from / authored-by / generated-from).
class ObjectConcept : public Object {
public:
    struct MemberTemplate {
        // WHAT the member was. A source set is a set of BEINGS, and beings
        // come in kinds — a concept captured from a Person and one captured
        // from a cube are not the same word. Serialized as an int and shared
        // with ConditionNode's vocabulary (APPEND-ONLY, never renumbered), so
        // a concept and a law's `ForAny Object` name kinds the same way.
        ConditionNode::BeingKind beingKind = ConditionNode::BeingKind::Object;

        // False when the source had no spatial body at all (a Relation, a
        // Formation, a Person's non-embodied surface): the geometry fields
        // below are then meaningless rather than accidentally a unit cube.
        bool hasGeometry = true;

        Object::ShapeKind kind = Object::ShapeKind::Cube;
        Object::ShapeParams params;
        bool hasField = false;
        geom::SdfNode field;            // deep-copies: the template is its own being
        float fieldExtent = 1.0f;
        bool hasPatch = false;
        geom::BezierPatch patch;
        glm::mat4 relativeTransform{1.0f};   // member pose relative to set centroid

        // What the member WAS, not merely how it was shaped. A template used
        // to remember geometry and pose and NOTHING else, so a captured set
        // of a red clay sphere and a blue stone sphere reinstantiated as two
        // identical grey ones: colour, material, mass, and every authored
        // dynamic property were dropped at the moment of abstraction. Worse,
        // mappings only run when a live source set is supplied, so a concept
        // instantiated on its own (which is what Spawn does whenever the
        // event's subject is not an Object) reproduced nothing at all.
        //
        // Keyed by property path, replayed onto the newborn BEFORE the
        // mappings so a derivation can still override what was remembered.
        // Only legible VALUES are kept — never pointers to other beings,
        // which are identity rather than value, and whose structure the
        // RelationTemplates carry instead.
        std::map<std::string, PropertyValue> captured;

        nlohmann::json toJson() const;
        static MemberTemplate fromJson(const nlohmann::json& j);
    };

    // A relation BETWEEN members, remembered by member INDEX so it can be
    // reborn between the corresponding newborns: capture "pillar-2 is
    // attached to beam-0" once, and every instantiation gets its own
    // attachment. Captured from the world's relation graph wherever both
    // endpoints are inside the source set.
    struct RelationTemplate {
        int aIndex = 0;
        int bIndex = 0;
        std::string type;
        bool directed = false;
        float weight = 1.0f;

        nlohmann::json toJson() const;
        static RelationTemplate fromJson(const nlohmann::json& j);
    };

    explicit ObjectConcept(const std::string& name = "Concept");

    // A concept is a being, and a being has ONE identifier. There is no
    // second "concept id": `getIdentifier()` is the Object's, exactly as for
    // every other being, so `setObjectID`/`setConceptId` and the registry
    // agree by construction.
    //
    // This class used to override `getIdentifier()` to return a private
    // generated "concept-<N>", while `setObjectID("concept-sound-emitter")`
    // wrote the OTHER field. ConceptRegistry keys on `getIdentifier()`, so
    // every concept an author named was filed under its generated id and
    // `Spawn("concept-sound-emitter")` could never resolve. Law text addresses
    // beings by NAME; a being law text names must carry a stable slug.
    //
    // `setConceptId` is that authored, stable form. The generated
    // "concept-<N>" assigned at construction is only the fallback for a
    // concept nobody named.
    void setConceptId(const std::string& stableId);
    std::string conceptId() const { return getIdentifier(); }

    const std::string& name() const { return _name; }
    void setName(const std::string& name) { _name = name.empty() ? "Concept" : name; }

    std::vector<MemberTemplate>& members() { return _members; }
    const std::vector<MemberTemplate>& members() const { return _members; }

    std::vector<PropertyMapping>& mappings() { return _mappings; }
    const std::vector<PropertyMapping>& mappings() const { return _mappings; }
    void addMapping(PropertyMapping mapping) { _mappings.push_back(std::move(mapping)); }

    std::vector<RelationTemplate>& relationTemplates() { return _relationTemplates; }
    const std::vector<RelationTemplate>& relationTemplates() const {
        return _relationTemplates;
    }

    RelationManager& provenance() { return _provenance; }
    const RelationManager& provenance() const { return _provenance; }

    // The abstraction gesture: selection in, concept out. Geometry recipes are
    // deep-copied; member poses are recorded relative to the set centroid so
    // the concept is placeable anywhere. A set of one is fine.
    static std::shared_ptr<ObjectConcept> captureFrom(
        const std::vector<Object*>& sourceSet,
        const std::string& name,
        Singular* author = nullptr);

    // The same gesture over ANY beings. The manifesto's layers 4 and 5 —
    // transference between different kinds, and set-to-set involving Persons,
    // Bodies, Zones, Relations and Formations — are a statement about what may
    // be a SOURCE, and every Singular carries a property surface, so every
    // Singular can be one. Kind and property state are captured from all of
    // them; geometry and pose only from those that have a body.
    //
    // What may be BORN is a narrower question, and deliberately so: see
    // `birthKind` in the .cpp. A Person is never a birth.
    static std::shared_ptr<ObjectConcept> captureFromBeings(
        const std::vector<Singular*>& sourceSet,
        const std::string& name,
        Singular* author = nullptr);

    // Birth: new independent beings at `placement`. When `sources` is given,
    // the mappings run (PerMember pairs newborn i with source i mod count;
    // aggregates fold the whole set). Mappings write through PropertyPath, so
    // full setter side-effects (collision update, etc.) run on the newborns.
    //
    // The newborn also inherits the concept's OWN authored state — its
    // dynamic properties and its attributes. A MemberTemplate remembers only
    // shape/field/pose; everything a Person authored ONTO the concept
    // ("acoustic.amplitude is 1.0") lived on the concept being itself and
    // reached no newborn at all, which made every such property unreachable
    // by law text on anything the concept produced.
    std::vector<std::unique_ptr<Object>> instantiate(
        const glm::mat4& placement = glm::mat4(1.0f),
        const std::vector<Singular*>* sources = nullptr);

    // An Object set IS a being set — the upcast is not the author's problem.
    std::vector<std::unique_ptr<Object>> instantiate(
        const glm::mat4& placement,
        const std::vector<Object*>* sources);

    nlohmann::json toJson() const;
    static std::shared_ptr<ObjectConcept> fromJson(const nlohmann::json& j);

private:
    void initializeConceptIdentity();

    std::string _name;
    std::vector<MemberTemplate> _members;
    std::vector<PropertyMapping> _mappings;
    std::vector<RelationTemplate> _relationTemplates;
    RelationManager _provenance;
};

// ---------------------------------------------------------------------------
// ConceptRegistry — mirror of LawManager for concepts: registry + Formation +
// serialization + a "concept-registered" ECA echo. A singleton because Spawn
// actions (compiled closures) must resolve concepts by id at fire time.
// ---------------------------------------------------------------------------
class ConceptRegistry {
public:
    static ConceptRegistry& instance();

    void add(const std::shared_ptr<ObjectConcept>& concept);   // dedupes by id
    bool remove(const std::string& conceptId);
    std::shared_ptr<ObjectConcept> find(const std::string& conceptId) const;
    const std::vector<std::shared_ptr<ObjectConcept>>& getAll() const { return _concepts; }

    void registerCoreConcepts();


    Formation& formation() { return _formation; }
    const Formation& formation() const { return _formation; }

    nlohmann::json toJson() const;
    // Restore a saved registry (replace-all) — Spawn actions resolve
    // concepts by id at fire time, so a loaded world's birth laws work
    // as soon as this runs.
    void loadFromJson(const nlohmann::json& j);

private:
    ConceptRegistry() = default;
    ConceptRegistry(const ConceptRegistry&) = delete;
    ConceptRegistry& operator=(const ConceptRegistry&) = delete;

    std::vector<std::shared_ptr<ObjectConcept>> _concepts;
    Formation _formation;
};
