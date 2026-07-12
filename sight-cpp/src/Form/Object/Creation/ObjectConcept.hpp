#pragma once

#include "Form/Object/Object.hpp"
#include "Form/Object/Creation/PropertyMapping.hpp"
#include "Relation/RelationManager.hpp"
#include "json.hpp"

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
        Object::ShapeKind kind = Object::ShapeKind::Cube;
        Object::ShapeParams params;
        bool hasField = false;
        geom::SdfNode field;            // deep-copies: the template is its own being
        float fieldExtent = 1.0f;
        glm::mat4 relativeTransform{1.0f};   // member pose relative to set centroid

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

    std::string getIdentifier() const override { return _conceptId; }
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

    // Birth: new independent beings at `placement`. When `sources` is given,
    // the mappings run (PerMember pairs newborn i with source i mod count;
    // aggregates fold the whole set). Mappings write through PropertyPath, so
    // full setter side-effects (collision update, etc.) run on the newborns.
    std::vector<std::unique_ptr<Object>> instantiate(
        const glm::mat4& placement = glm::mat4(1.0f),
        const std::vector<Object*>* sources = nullptr);

    nlohmann::json toJson() const;
    static std::shared_ptr<ObjectConcept> fromJson(const nlohmann::json& j);

private:
    void initializeConceptIdentity();

    std::string _conceptId;
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
    Formation _formation{Form::ShapeType::Cube, glm::vec3(1.0f)};
};
