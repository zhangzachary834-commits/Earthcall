#include "Form/Object/Creation/ObjectConcept.hpp"

#include "Form/Object/Geometry/SdfJson.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <ctime>
#include <limits>

namespace {

nlohmann::json mat4ToJson(const glm::mat4& m) {
    nlohmann::json arr = nlohmann::json::array();
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) arr.push_back(m[c][r]);
    return arr;
}

glm::mat4 mat4FromJson(const nlohmann::json& j) {
    glm::mat4 m(1.0f);
    if (j.is_array() && j.size() == 16) {
        int i = 0;
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r) m[c][r] = j[i++].get<float>();
    }
    return m;
}

} // namespace

// ---------------------------------------------------------------------------
// PropertyMapping
// ---------------------------------------------------------------------------

nlohmann::json PropertyMapping::toJson() const {
    nlohmann::json j{
        {"source", source.toString()},
        {"transform", transform.toJson()},
        {"target", target.toString()},
        {"agg", static_cast<int>(agg)}
    };
    if (!bindings.empty()) j["bindings"] = mathBindingsToJson(bindings);
    if (hasExact) j["exact"] = exact.toJson();
    return j;
}

PropertyMapping PropertyMapping::fromJson(const nlohmann::json& j) {
    PropertyMapping m;
    m.source = PropertyPath::parse(j.value("source", std::string()));
    if (j.contains("bindings")) m.bindings = mathBindingsFromJson(j["bindings"]);
    if (j.contains("transform")) m.transform = CurveModel::fromJson(j["transform"]);
    m.target = PropertyPath::parse(j.value("target", std::string()));
    m.agg = static_cast<Aggregate>(j.value("agg", 0));
    if (j.contains("exact")) {
        m.hasExact = true;
        m.exact = OntoMath::Piecewise::fromJson(j["exact"]);
    }
    return m;
}

// ---------------------------------------------------------------------------
// ObjectConcept::MemberTemplate
// ---------------------------------------------------------------------------

nlohmann::json ObjectConcept::MemberTemplate::toJson() const {
    nlohmann::json j{
        {"kind", static_cast<int>(kind)},
        {"params", {params.r, params.ry, params.rz, params.halfH, params.majorR,
                    params.minorR, params.paraboloidA, params.ovoidAsym, params.fillet}},
        {"relativeTransform", mat4ToJson(relativeTransform)}
    };
    if (hasField) {
        j["field"] = geom::sdfToJson(field);
        j["fieldExtent"] = fieldExtent;
    }
    return j;
}

ObjectConcept::MemberTemplate ObjectConcept::MemberTemplate::fromJson(const nlohmann::json& j) {
    MemberTemplate m;
    m.kind = static_cast<Object::ShapeKind>(j.value("kind", 0));
    if (j.contains("params") && j["params"].is_array() && j["params"].size() == 9) {
        const auto& p = j["params"];
        m.params.r = p[0].get<float>();
        m.params.ry = p[1].get<float>();
        m.params.rz = p[2].get<float>();
        m.params.halfH = p[3].get<float>();
        m.params.majorR = p[4].get<float>();
        m.params.minorR = p[5].get<float>();
        m.params.paraboloidA = p[6].get<float>();
        m.params.ovoidAsym = p[7].get<float>();
        m.params.fillet = p[8].get<float>();
    }
    if (j.contains("field")) {
        m.hasField = true;
        m.field = geom::sdfFromJson(j["field"]);
        m.fieldExtent = j.value("fieldExtent", 1.0f);
    }
    if (j.contains("relativeTransform")) {
        m.relativeTransform = mat4FromJson(j["relativeTransform"]);
    }
    return m;
}

// ---------------------------------------------------------------------------
// ObjectConcept::RelationTemplate
// ---------------------------------------------------------------------------

nlohmann::json ObjectConcept::RelationTemplate::toJson() const {
    return nlohmann::json{{"a", aIndex},
                          {"b", bIndex},
                          {"type", type},
                          {"directed", directed},
                          {"weight", weight}};
}

ObjectConcept::RelationTemplate ObjectConcept::RelationTemplate::fromJson(
    const nlohmann::json& j) {
    RelationTemplate t;
    t.aIndex = j.value("a", 0);
    t.bIndex = j.value("b", 0);
    t.type = j.value("type", std::string());
    t.directed = j.value("directed", false);
    t.weight = j.value("weight", 1.0f);
    return t;
}

// ---------------------------------------------------------------------------
// ObjectConcept
// ---------------------------------------------------------------------------

ObjectConcept::ObjectConcept(const std::string& name)
    : _name(name.empty() ? "Concept" : name) {
    initializeConceptIdentity();
}

namespace {
std::atomic<unsigned long long> g_nextConceptId{1};

// A restored concept id advances the fresh counter — same collision rule as
// laws and objects: fresh ids must stay fresh after loads.
void claimConceptIdAtLeast(const std::string& id) {
    const std::string prefix = "concept-";
    if (id.rfind(prefix, 0) != 0) return;
    const unsigned long long n =
        std::strtoull(id.c_str() + prefix.size(), nullptr, 10);
    unsigned long long current = g_nextConceptId.load();
    while (n + 1 > current && !g_nextConceptId.compare_exchange_weak(current, n + 1)) {
    }
}
} // namespace

void ObjectConcept::initializeConceptIdentity() {
    _conceptId = "concept-" + std::to_string(g_nextConceptId.fetch_add(1));
    setObjectID(_conceptId);
    setPhysicalObject(0);   // extra-spatial: the word for the thing, not the thing
}

std::shared_ptr<ObjectConcept> ObjectConcept::captureFrom(
    const std::vector<Object*>& sourceSet,
    const std::string& name,
    Singular* author) {
    auto concept = std::make_shared<ObjectConcept>(name);

    glm::vec3 centroid(0.0f);
    int counted = 0;
    for (const auto* source : sourceSet) {
        if (!source) continue;
        centroid += source->getPosition();
        ++counted;
    }
    if (counted > 0) centroid /= static_cast<float>(counted);
    const glm::mat4 toCentroid = glm::translate(glm::mat4(1.0f), -centroid);

    for (auto* source : sourceSet) {
        if (!source) continue;
        MemberTemplate member;
        member.kind = source->getShapeKind();
        member.params = source->getShapeParams();
        member.hasField = source->hasField();
        if (member.hasField) {
            member.field = source->getFieldData();      // deep copy — its own being
            member.fieldExtent = source->getFieldExtent();
        }
        member.relativeTransform = toCentroid * source->getTransform();
        concept->_members.push_back(std::move(member));

        concept->_provenance.add(std::make_shared<Relation>(
            "abstracted-from", *concept, *source, true, 1.0f));
    }
    if (author) {
        concept->_provenance.add(std::make_shared<Relation>(
            "authored-by", *concept, *author, true, 1.0f));
    }

    // Inter-member relations: wherever the world's graph relates two members
    // of the source set, remember the edge BY INDEX — the concept carries
    // the set's structure, not just its members.
    std::vector<std::string> memberIds;
    for (auto* source : sourceSet) {
        memberIds.push_back(source ? source->getIdentifier() : std::string());
    }
    const auto indexOf = [&](const std::string& id) -> int {
        for (std::size_t i = 0; i < memberIds.size(); ++i) {
            if (!memberIds[i].empty() && memberIds[i] == id) return static_cast<int>(i);
        }
        return -1;
    };
    for (const Relation* rel : Universe::instance().relations()) {
        if (!rel) continue;
        const int a = indexOf(rel->entityA);
        const int b = indexOf(rel->entityB);
        if (a < 0 || b < 0) continue;
        RelationTemplate t;
        t.aIndex = a;
        t.bIndex = b;
        t.type = rel->type;
        t.directed = rel->directed;
        t.weight = rel->getWeight();
        concept->_relationTemplates.push_back(std::move(t));
    }
    return concept;
}

std::vector<std::unique_ptr<Object>> ObjectConcept::instantiate(
    const glm::mat4& placement,
    const std::vector<Object*>* sources) {
    std::vector<std::unique_ptr<Object>> newborns;
    newborns.reserve(_members.size());

    for (std::size_t i = 0; i < _members.size(); ++i) {
        const MemberTemplate& member = _members[i];
        auto newborn = std::make_unique<Object>();
        if (member.hasField) {
            newborn->setFieldShape(member.field, member.fieldExtent);
        } else {
            newborn->setShape(member.kind, member.params);
        }
        newborn->setTransform(placement * member.relativeTransform);
        newborn->updateCollisionZone(newborn->getTransform());   // like every creation tool

        // Derivation: carry structure across through the mappings. Each
        // transfer passes the Singularity gate first — an object's
        // properties are accessible to set-to-set only where the
        // TransferPolicy (itself law-governable) allows.
        if (sources && !sources->empty()) {
            for (const auto& mapping : _mappings) {
                if (!TransferPolicy::instance().canTransfer(mapping.source)) continue;
                std::map<std::string, double> evalVars;
                bool allValid = true;
                const Singular* guardSubject = nullptr;

                if (mapping.bindings.empty() && !mapping.hasExact && !mapping.source.empty()) {
                    // Legacy single source - try direct transfer first if non-numeric
                    if (mapping.agg == PropertyMapping::Aggregate::PerMember) {
                        Object* src = (*sources)[i % sources->size()];
                        PropertyValue v;
                        if (src && mapping.source.getValue(*src, v) == PropertyPath::PathResult::Ok) {
                            double x = 0.0;
                            if (propertyValueToNumber(v, x)) {
                                auto y = mapping.transform.evaluate(x);
                                mapping.target.setValue(*newborn, PropertyValue(y));
                            } else {
                                // Non-numeric (vec3, string, etc) direct transfer
                                mapping.target.setValue(*newborn, v);
                            }
                        }
                    } else {
                        // Aggregations must be numeric
                        double acc = (mapping.agg == PropertyMapping::Aggregate::Max)
                                         ? -std::numeric_limits<double>::infinity()
                                         : 0.0;
                        int count = 0;
                        for (auto* src : *sources) {
                            PropertyValue v;
                            double xi = 0.0;
                            if (src && mapping.source.getValue(*src, v) == PropertyPath::PathResult::Ok &&
                                propertyValueToNumber(v, xi)) {
                                ++count;
                                if (mapping.agg == PropertyMapping::Aggregate::Max) {
                                    acc = std::max(acc, xi);
                                } else {
                                    acc += xi;
                                }
                            }
                        }
                        if (count > 0) {
                            double val = (mapping.agg == PropertyMapping::Aggregate::Mean)
                                    ? acc / count : acc;
                            auto y = mapping.transform.evaluate(val);
                            mapping.target.setValue(*newborn, PropertyValue(y));
                        }
                    }
                } else {
                    // Multivariate or exact math mode
                    auto bindings = mapping.bindings;
                    if (bindings.empty() && !mapping.source.empty()) {
                        bindings["x"] = mapping.source;
                    }

                    for (const auto& bindingPair : bindings) {
                        const std::string& varName = bindingPair.first;
                        const PropertyPath& path = bindingPair.second;
                        
                        if (!TransferPolicy::instance().canTransfer(path)) {
                            allValid = false;
                            break;
                        }
                        double val = 0.0;
                        bool have = false;

                        if (mapping.agg == PropertyMapping::Aggregate::PerMember) {
                            Object* src = (*sources)[i % sources->size()];
                            PropertyValue v;
                            have = src && path.getValue(*src, v) == PropertyPath::PathResult::Ok &&
                                   propertyValueToNumber(v, val);
                            if (!guardSubject) guardSubject = src;
                        } else {
                            double acc = (mapping.agg == PropertyMapping::Aggregate::Max)
                                             ? -std::numeric_limits<double>::infinity()
                                             : 0.0;
                            int count = 0;
                            for (auto* src : *sources) {
                                PropertyValue v;
                                double xi = 0.0;
                                if (src && path.getValue(*src, v) == PropertyPath::PathResult::Ok &&
                                    propertyValueToNumber(v, xi)) {
                                    ++count;
                                    if (mapping.agg == PropertyMapping::Aggregate::Max) {
                                        acc = std::max(acc, xi);
                                    } else {
                                        acc += xi;
                                    }
                                }
                            }
                            if (count > 0) {
                                have = true;
                                val = (mapping.agg == PropertyMapping::Aggregate::Mean)
                                        ? acc / count
                                        : acc;
                            }
                        }

                        if (have) {
                            evalVars[varName] = val;
                        } else {
                            allValid = false;
                            break;
                        }
                    }

                    if (allValid && !bindings.empty()) {
                        // Exact math when authored; undefined transfers nothing.
                        double x = evalVars.count("x") ? evalVars["x"] : (evalVars.empty() ? 0.0 : evalVars.begin()->second);
                        const auto y = mapping.apply(x, evalVars, guardSubject);
                        if (y) mapping.target.setValue(*newborn, PropertyValue(*y));
                    }
                }
            }
        }

        // Provenance by identifier (Relations store names, not pointers —
        // safe across the newborn's move into its container).
        _provenance.add(std::make_shared<Relation>(
            "generated-from", *newborn, *this, true, 1.0f));
        newborns.push_back(std::move(newborn));
    }

    // The set's STRUCTURE is reborn too: every captured inter-member
    // relation becomes a fresh relation between the corresponding newborns,
    // registered into the world's graph through the Universe. Each
    // registration publishes "relation-formed" like any other.
    if (!_relationTemplates.empty() && Universe::instance().hasRelationRegistrar()) {
        for (const auto& t : _relationTemplates) {
            if (t.aIndex < 0 || t.bIndex < 0 ||
                t.aIndex >= static_cast<int>(newborns.size()) ||
                t.bIndex >= static_cast<int>(newborns.size())) {
                continue;
            }
            Universe::instance().addRelation(std::make_shared<Relation>(
                t.type, *newborns[static_cast<std::size_t>(t.aIndex)],
                *newborns[static_cast<std::size_t>(t.bIndex)], t.directed, t.weight));
        }
    }

    // A birth can wake laws: the echo announces WHICH concept just
    // manifested (subject: the concept — the newborns' provenance names it).
    if (!newborns.empty()) {
        ECA::Event echo;
        echo.type = "concept-instantiated";
        echo.subject = this;
        echo.timestamp = std::time(nullptr);
        Core::EventBus::instance().publish(echo);
    }
    return newborns;
}

nlohmann::json ObjectConcept::toJson() const {
    nlohmann::json membersJson = nlohmann::json::array();
    for (const auto& m : _members) membersJson.push_back(m.toJson());
    nlohmann::json mappingsJson = nlohmann::json::array();
    for (const auto& m : _mappings) mappingsJson.push_back(m.toJson());
    nlohmann::json relationsJson = nlohmann::json::array();
    for (const auto& t : _relationTemplates) relationsJson.push_back(t.toJson());
    return nlohmann::json{
        {"id", _conceptId},
        {"name", _name},
        {"members", membersJson},
        {"mappings", mappingsJson},
        {"relationTemplates", relationsJson},
        {"provenance", _provenance.toJson()}
    };
}

std::shared_ptr<ObjectConcept> ObjectConcept::fromJson(const nlohmann::json& j) {
    auto concept = std::make_shared<ObjectConcept>(j.value("name", std::string("Concept")));
    // Preserve saved identity (fresh-counter collision guarding is the world
    // loader's concern, as with Law::fromJson).
    if (j.contains("id")) {
        concept->_conceptId = j["id"].get<std::string>();
        concept->setObjectID(concept->_conceptId);
        claimConceptIdAtLeast(concept->_conceptId);
    }
    if (j.contains("members")) {
        for (const auto& m : j["members"]) {
            concept->_members.push_back(MemberTemplate::fromJson(m));
        }
    }
    if (j.contains("mappings")) {
        for (const auto& m : j["mappings"]) {
            concept->_mappings.push_back(PropertyMapping::fromJson(m));
        }
    }
    if (j.contains("relationTemplates")) {
        for (const auto& t : j["relationTemplates"]) {
            concept->_relationTemplates.push_back(RelationTemplate::fromJson(t));
        }
    }
    return concept;
}

// ---------------------------------------------------------------------------
// ConceptRegistry
// ---------------------------------------------------------------------------

ConceptRegistry& ConceptRegistry::instance() {
    static ConceptRegistry registry;
    return registry;
}

void ConceptRegistry::registerCoreConcepts() {
    auto soundEmitter = std::make_shared<ObjectConcept>("sound-emitter");
    soundEmitter->setObjectID("concept-sound-emitter");
    // Empty member template: a sound emitter doesn't need physical geometry to exist,
    // though we might add a tiny invisible shape if it requires bounds. For now, empty shape.
    ObjectConcept::MemberTemplate tmpl;
    tmpl.kind = Object::ShapeKind::Cube;
    tmpl.params.r = 0.01f;
    tmpl.params.ry = 0.01f;
    tmpl.params.rz = 0.01f;
    soundEmitter->members().push_back(tmpl);
    
    // Add default acoustic properties
    soundEmitter->setAttribute("acoustic.isSoundEmitter", "true");
    soundEmitter->setAttribute("acoustic.amplitude", "1.0");
    soundEmitter->setAttribute("acoustic.frequency", "440.0");
    soundEmitter->setAttribute("acoustic.waveType", "sine");
    
    add(soundEmitter);
}


void ConceptRegistry::add(const std::shared_ptr<ObjectConcept>& concept) {
    if (!concept) return;
    const std::string id = concept->getIdentifier();
    auto existing = std::find_if(_concepts.begin(), _concepts.end(),
                                 [&](const std::shared_ptr<ObjectConcept>& candidate) {
                                     return candidate && candidate->getIdentifier() == id;
                                 });
    if (existing != _concepts.end()) return;

    _concepts.push_back(concept);
    _formation.addMember(concept.get());

    ECA::Event echo;
    echo.type = "concept-registered";
    echo.subject = concept.get();
    echo.timestamp = std::time(nullptr);
    Core::EventBus::instance().publish(echo);
}

bool ConceptRegistry::remove(const std::string& conceptId) {
    auto it = std::find_if(_concepts.begin(), _concepts.end(),
                           [&](const std::shared_ptr<ObjectConcept>& concept) {
                               return concept && concept->getIdentifier() == conceptId;
                           });
    if (it == _concepts.end()) return false;
    _formation.removeMember(it->get());
    _concepts.erase(it);
    return true;
}

std::shared_ptr<ObjectConcept> ConceptRegistry::find(const std::string& conceptId) const {
    for (const auto& concept : _concepts) {
        if (concept && concept->getIdentifier() == conceptId) return concept;
    }
    return nullptr;
}

nlohmann::json ConceptRegistry::toJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& concept : _concepts) {
        if (concept) arr.push_back(concept->toJson());
    }
    return nlohmann::json{{"concepts", arr}};
}

void ConceptRegistry::loadFromJson(const nlohmann::json& j) {
    for (const auto& concept : _concepts) {
        if (concept) _formation.removeMember(concept.get());
    }
    _concepts.clear();
    if (!j.contains("concepts")) return;
    for (const auto& cj : j["concepts"]) {
        add(ObjectConcept::fromJson(cj));
    }
}
