#include "ConstructedBeing/Object/Creation/ObjectConcept.hpp"

#include "ConstructedBeing/Object/Geometry/SdfJson.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
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
        {"beingKind", static_cast<int>(beingKind)},
        {"hasGeometry", hasGeometry},
        {"params", {params.r, params.ry, params.rz, params.halfH, params.majorR,
                    params.minorR, params.paraboloidA, params.ovoidAsym, params.fillet}},
        {"relativeTransform", mat4ToJson(relativeTransform)}
    };
    if (hasField) {
        j["field"] = geom::sdfToJson(field);
        j["fieldExtent"] = fieldExtent;
    }
    if (!captured.empty()) {
        nlohmann::json state = nlohmann::json::object();
        for (const auto& entry : captured) {
            state[entry.first] = propertyValueToJson(entry.second);
        }
        j["captured"] = state;
    }
    return j;
}

ObjectConcept::MemberTemplate ObjectConcept::MemberTemplate::fromJson(const nlohmann::json& j) {
    MemberTemplate m;
    m.kind = static_cast<Object::ShapeKind>(j.value("kind", 0));
    // Concepts written before members carried a kind were all Objects with
    // bodies — the old default, stated rather than assumed.
    m.beingKind = static_cast<ConditionNode::BeingKind>(
        j.value("beingKind", static_cast<int>(ConditionNode::BeingKind::Object)));
    m.hasGeometry = j.value("hasGeometry", true);
    if (j.contains("captured") && j["captured"].is_object()) {
        for (auto it = j["captured"].begin(); it != j["captured"].end(); ++it) {
            m.captured[it.key()] = propertyValueFromJson(it.value());
        }
    }
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
    // The generated id is the FALLBACK identity — what a concept nobody named
    // is filed under. setConceptId replaces it with the authored slug.
    setObjectID("concept-" + std::to_string(g_nextConceptId.fetch_add(1)));
    setPhysicalObject(0);   // extra-spatial: the word for the thing, not the thing
}

void ObjectConcept::setConceptId(const std::string& stableId) {
    if (stableId.empty()) return;   // a being is never left without a name
    setObjectID(stableId);          // ONE identity: getIdentifier() reads this
    claimConceptIdAtLeast(stableId);
}

namespace {

// The paths a MemberTemplate already owns outright. Remembering them a second
// time in the property snapshot would let a stale copy of the source's WORLD
// pose fight the placement the author asked for, and a stale copy of the shape
// fight the geometry recipe.
bool templateOwnsPath(const std::string& path) {
    static const char* kOwned[] = {"position", "rotation", "transform", "center"};
    for (const char* owned : kOwned) {
        if (path == owned) return true;
    }
    return path.rfind("shape", 0) == 0;   // "shape", "shape.r", "shape.majorR", …
}

// A concept remembers VALUES. A property whose value is another being is
// identity, not value: copying the pointer would make every newborn point at
// the original's neighbours, and copying the identifier would make the save
// file claim a relationship nothing formed. Structure travels through
// RelationTemplates instead.
bool isValueLike(const PropertyValue& v) {
    return !std::holds_alternative<std::monostate>(v) &&
           !std::holds_alternative<Singular*>(v) &&
           !std::holds_alternative<Object*>(v) &&
           !std::holds_alternative<Relation*>(v) &&
           !std::holds_alternative<Formation*>(v);
}

// Which kind of being this is, asked most-specific-first: a Zone and a Law are
// both Objects, and answering "Object" for either would lose exactly the
// distinction the template is being taught to keep.
ConditionNode::BeingKind kindOf(const Singular& being) {
    using K = ConditionNode::BeingKind;
    static const K kOrder[] = {K::Person, K::Law,       K::World,     K::Zone,
                               K::Lexeme, K::Relation,  K::Formation, K::Object};
    for (K candidate : kOrder) {
        if (ConditionNode::matchesKind(being, candidate)) return candidate;
    }
    return K::AnyBeing;
}

// Everything this being carries that may honestly be taken: its registered
// property surface plus its authored dynamic vocabulary, minus what the
// template already owns, minus what is identity rather than value, minus
// whatever the Singularity gate currently closes.
//
// Gated at CAPTURE as well as at replay. A concept must never REMEMBER what
// it may not take: a snapshot taken through a closed gate would sit in the
// save file as a durable copy of governed state, and closing the gate
// afterwards could not undo it.
std::map<std::string, PropertyValue> captureState(Singular& source) {
    std::map<std::string, PropertyValue> state;
    const auto admit = [&](const std::string& path, const PropertyValue& v) {
        if (templateOwnsPath(path) || !isValueLike(v)) return;
        if (!TransferPolicy::instance().canTransfer(PropertyPath::parse(path))) return;
        state[path] = v;
    };
    for (Property* property : source.listProperties()) {
        if (property) admit(property->name(), property->value());
    }
    for (const auto& authored : source.dynamicProperties()) {
        admit(authored.first, authored.second);
    }
    return state;
}

} // namespace

std::shared_ptr<ObjectConcept> ObjectConcept::captureFrom(
    const std::vector<Object*>& sourceSet,
    const std::string& name,
    Singular* author) {
    std::vector<Singular*> beings(sourceSet.begin(), sourceSet.end());
    return captureFromBeings(beings, name, author);
}

std::shared_ptr<ObjectConcept> ObjectConcept::captureFromBeings(
    const std::vector<Singular*>& sourceSet,
    const std::string& name,
    Singular* author) {
    auto concept = std::make_shared<ObjectConcept>(name);

    // The centroid is taken over the members that HAVE a place. A Relation in
    // the set has no position, and averaging in an origin it never occupied
    // would drag the whole concept off-centre.
    glm::vec3 centroid(0.0f);
    int counted = 0;
    for (const auto* source : sourceSet) {
        if (const auto* placed = dynamic_cast<const Object*>(source)) {
            centroid += placed->getPosition();
            ++counted;
        }
    }
    if (counted > 0) centroid /= static_cast<float>(counted);
    const glm::mat4 toCentroid = glm::translate(glm::mat4(1.0f), -centroid);

    for (auto* source : sourceSet) {
        if (!source) continue;
        MemberTemplate member;
        member.beingKind = kindOf(*source);
        member.captured = captureState(*source);

        if (auto* embodied = dynamic_cast<Object*>(source)) {
            member.hasGeometry = true;
            member.kind = embodied->getShapeKind();
            member.params = embodied->getShapeParams();
            member.hasField = embodied->hasField();
            if (member.hasField) {
                member.field = embodied->getFieldData();   // deep copy — its own being
                member.fieldExtent = embodied->getFieldExtent();
            }
            member.relativeTransform = toCentroid * embodied->getTransform();
        } else {
            member.hasGeometry = false;
        }
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
    if (!sources) return instantiate(placement, static_cast<std::vector<Singular*>*>(nullptr));
    std::vector<Singular*> beings(sources->begin(), sources->end());
    return instantiate(placement, &beings);
}

namespace {

// WHAT MAY BE BORN. Every Singular may be a source — reading a property and
// deriving from it is the same act whoever carries it — but birth is not
// symmetrical with reading, and pretending it is would be the loudest kind of
// mistake this file could make.
//
//   Person   — refused, always. A Person is an actual human being; there is no
//              such thing as instantiating one, and a system that would try is
//              already wrong about what it is doing.
//   Zone,
//   Law,
//   World    — refused for now, but for an entirely different reason: they are
//              births whose GOVERNANCE is undecided. A newborn Zone needs an
//              owner and a jurisdiction; a newborn Law needs authors and a
//              trigger. Guessing those is how a creation system quietly starts
//              legislating. They wait for their author to say what they mean.
//   Object   — born.
//
// A member that may not be born is SKIPPED and SAID, never silently downgraded
// into a cube.
bool birthKind(ConditionNode::BeingKind kind, std::string& refusal) {
    switch (kind) {
        case ConditionNode::BeingKind::Person:
            refusal = "a Person is not instantiated";
            return false;
        case ConditionNode::BeingKind::Zone:
            refusal = "a Zone's birth needs an owner and a jurisdiction (undecided)";
            return false;
        case ConditionNode::BeingKind::Law:
            refusal = "a Law's birth needs authors and a trigger (undecided)";
            return false;
        case ConditionNode::BeingKind::World:
            refusal = "a World is not instantiated";
            return false;
        case ConditionNode::BeingKind::Relation:
            refusal = "relations are reborn from RelationTemplates, not member templates";
            return false;
        case ConditionNode::BeingKind::Formation:
            refusal = "a Formation is membership, not a birth";
            return false;
        case ConditionNode::BeingKind::Lexeme:
            refusal = "a Lexeme's birth belongs to the Language channel";
            return false;
        case ConditionNode::BeingKind::AnyBeing:
        case ConditionNode::BeingKind::Object:
            return true;
    }
    return true;
}

} // namespace

std::vector<std::unique_ptr<Object>> ObjectConcept::instantiate(
    const glm::mat4& placement,
    const std::vector<Singular*>* sources) {
    std::vector<std::unique_ptr<Object>> newborns;
    newborns.reserve(_members.size());

    // Members and newborns are no longer index-for-index once a member can be
    // refused, and the RelationTemplates address members BY INDEX — so the
    // mapping between the two is kept explicitly rather than assumed.
    std::vector<int> newbornOfMember(_members.size(), -1);

    for (std::size_t i = 0; i < _members.size(); ++i) {
        const MemberTemplate& member = _members[i];
        std::string refusal;
        if (!birthKind(member.beingKind, refusal)) {
            ECA::Event refused;
            refused.type = "concept-member-refused";
            refused.subject = this;
            refused.timestamp = std::time(nullptr);
            Core::EventBus::instance().publish(refused);
            continue;
        }

        auto newborn = std::make_unique<Object>();
        if (member.hasGeometry) {
            if (member.hasField) {
                newborn->setFieldShape(member.field, member.fieldExtent);
            } else {
                newborn->setShape(member.kind, member.params);
            }
            newborn->setTransform(placement * member.relativeTransform);
            newborn->updateCollisionZone(newborn->getTransform());  // like every creation tool
        } else {
            newborn->setPhysicalObject(0);   // it had no body; it is given none
        }

        // What the member WAS, replayed. Gated again here: a snapshot taken
        // while a gate stood open must not keep flowing after a law closes it,
        // so the moment of TRANSFER is checked, not only the moment of memory.
        for (const auto& remembered : member.captured) {
            const PropertyPath path = PropertyPath::parse(remembered.first);
            if (!TransferPolicy::instance().canTransfer(path)) continue;
            if (path.setValue(*newborn, remembered.second) ==
                PropertyPath::PathResult::NoSuchProperty) {
                // The newborn has no such registered property — then it is
                // authored vocabulary, and authored vocabulary is exactly what
                // dynamic properties are for.
                newborn->setDynamicProperty(remembered.first, remembered.second);
            }
        }

        // The concept's OWN authored state travels to the newborn. A
        // MemberTemplate remembers shape, field and pose; everything else a
        // Person put on the concept ("acoustic.amplitude is 1.0") lives on
        // the concept being itself. Copying it FIRST means the mappings
        // below — and any authored child action — can still override it.
        //
        // Dynamic properties, not attributes, are what law text can read:
        // PropertyPath::resolve consults findProperty(), which falls through
        // to the dynamic table, so a dotted name like "acoustic.amplitude"
        // resolves. `_composition.attributes` is a bare string map no
        // property path can see, and writing acoustic state there is why no
        // law condition could ever read it.
        for (const auto& authored : dynamicProperties()) {
            newborn->setDynamicProperty(authored.first, authored.second);
        }
        for (const auto& attribute : getAttributes()) {
            newborn->setAttribute(attribute.first, attribute.second);
        }

        // Derivation: carry structure across through the mappings. Each
        // transfer passes the Singularity gate first — an object's
        // properties are accessible to set-to-set only where the
        // TransferPolicy (itself law-governable) allows.
        if (sources && !sources->empty()) {
            for (const auto& mapping : _mappings) {
                // WHAT IS TAKEN IS WHAT IS GATED. This used to consult
                // `mapping.source` alone — but a multivariable mapping reads
                // its `bindings` and may leave the legacy single source
                // empty, and `canTransfer({})` is false, so every
                // bindings-authored mapping was refused before it began and
                // the per-binding check further down was unreachable. An
                // absent path is not a closed one; the gate belongs on the
                // paths this mapping actually reads.
                const MathBindings readPaths = mapping.readPaths();
                if (readPaths.empty()) continue;   // reads nothing, transfers nothing
                bool gateOpen = true;
                for (const auto& binding : readPaths) {
                    if (!TransferPolicy::instance().canTransfer(binding.second)) {
                        gateOpen = false;
                        break;
                    }
                }
                if (!gateOpen) continue;

                std::map<std::string, double> evalVars;
                bool allValid = true;
                const Singular* guardSubject = nullptr;

                if (mapping.bindings.empty() && !mapping.hasExact && !mapping.source.empty()) {
                    // Legacy single source - try direct transfer first if non-numeric
                    if (mapping.agg == PropertyMapping::Aggregate::PerMember) {
                        Singular* src = (*sources)[i % sources->size()];
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
                    // Multivariate or exact math mode. The gate above already
                    // cleared every one of these paths.
                    const MathBindings& bindings = readPaths;

                    for (const auto& bindingPair : bindings) {
                        const std::string& varName = bindingPair.first;
                        const PropertyPath& path = bindingPair.second;

                        double val = 0.0;
                        bool have = false;

                        if (mapping.agg == PropertyMapping::Aggregate::PerMember) {
                            Singular* src = (*sources)[i % sources->size()];
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
        newbornOfMember[i] = static_cast<int>(newborns.size());
        newborns.push_back(std::move(newborn));
    }

    // The set's STRUCTURE is reborn too: every captured inter-member
    // relation becomes a fresh relation between the corresponding newborns,
    // registered into the world's graph through the Universe. Each
    // registration publishes "relation-formed" like any other.
    //
    // Addressed through newbornOfMember, not by raw index: a refused member
    // shifts every later newborn, and an edge that silently retargets is an
    // edge between the wrong two beings — worse than a missing one.
    if (!_relationTemplates.empty() && Universe::instance().hasRelationRegistrar()) {
        const auto newbornAt = [&](int memberIndex) -> Object* {
            if (memberIndex < 0 || memberIndex >= static_cast<int>(newbornOfMember.size())) {
                return nullptr;
            }
            const int at = newbornOfMember[static_cast<std::size_t>(memberIndex)];
            return at < 0 ? nullptr : newborns[static_cast<std::size_t>(at)].get();
        };
        for (const auto& t : _relationTemplates) {
            Object* a = newbornAt(t.aIndex);
            Object* b = newbornAt(t.bIndex);
            if (!a || !b) continue;
            Universe::instance().addRelation(
                std::make_shared<Relation>(t.type, *a, *b, t.directed, t.weight));
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
    nlohmann::json authored = nlohmann::json::object();
    for (const auto& entry : dynamicProperties()) {
        authored[entry.first] = propertyValueToJson(entry.second);
    }
    nlohmann::json attrs = nlohmann::json::object();
    for (const auto& entry : getAttributes()) attrs[entry.first] = entry.second;
    return nlohmann::json{
        {"id", getIdentifier()},
        {"name", _name},
        {"authoredProperties", authored},
        {"attributes", attrs},
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
        concept->setConceptId(j["id"].get<std::string>());
    }
    // The concept's own authored state — the properties law text reads off
    // anything this concept produces. A property that vanishes on save was
    // never granted.
    if (j.contains("authoredProperties") && j["authoredProperties"].is_object()) {
        for (auto it = j["authoredProperties"].begin();
             it != j["authoredProperties"].end(); ++it) {
            concept->setDynamicProperty(it.key(), propertyValueFromJson(it.value()));
        }
    }
    if (j.contains("attributes") && j["attributes"].is_object()) {
        for (auto it = j["attributes"].begin(); it != j["attributes"].end(); ++it) {
            if (it.value().is_string()) {
                concept->setAttribute(it.key(), it.value().get<std::string>());
            }
        }
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
    // Ancestry. `toJson` has always written this and `fromJson` never read it
    // back, which made provenance WRITE-ONLY: every abstracted-from,
    // authored-by and generated-from edge died at the next load. The
    // anti-Babel ceilings of §7d are predicates OVER these chains —
    // generation depth walks ancestry, authorship attestation demands the
    // chain terminate in a Person — so a world that forgets them on load
    // cannot enforce either. A being's descent is part of what it is.
    if (j.contains("provenance")) {
        concept->_provenance.loadFromJson(j["provenance"]);
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

// ---------------------------------------------------------------------------
// REMAINING DEBT — read before adding anything here.
//
// This function authors a being in C++. That is refusal #1: domain things and
// their state are authored IN-WORLD as data, by a Person, never carved into
// the type system. `sound-emitter` belongs in a save file with a real author
// recorded against it, not in a translation unit.
//
// It stays for exactly one reason: the engine's own audio path spawns it, and
// there is currently no seed world that guarantees it exists — a half-migrated
// concept (removed here, not yet authored anywhere) would be strictly worse
// than an honest one still hard-coded. See docs/AUDIO_FIX_NOTES.md.
//
// `material.crystal` HAS been removed. It was unused by the audio path, it
// carried hard-coded domain state (`physics.mass 5.0`), and its identifier
// squatted the `material.*` namespace that Material root resolution reserves
// (MathBinding's longest-first match would have let a concept shadow a real
// Material for "@material.crystal.baseColor").
// ---------------------------------------------------------------------------
void ConceptRegistry::registerCoreConcepts() {
    auto soundEmitter = std::make_shared<ObjectConcept>("sound-emitter");
    // The STABLE slug law text names ("spawn concept-sound-emitter").
    soundEmitter->setConceptId("concept-sound-emitter");
    // A sound emitter has no business being seen: a near-degenerate cube keeps
    // it a legal Object with bounds without putting anything visible in-world.
    ObjectConcept::MemberTemplate tmpl;
    tmpl.kind = Object::ShapeKind::Cube;
    tmpl.params.r = 0.01f;
    tmpl.params.ry = 0.01f;
    tmpl.params.rz = 0.01f;
    soundEmitter->members().push_back(tmpl);

    // The emitter's acoustic state, as DYNAMIC PROPERTIES — the authored half
    // of a being's vocabulary, and the only half PropertyPath can resolve.
    // Written as attributes these were invisible to every law condition and
    // every binding.
    //
    // Typed, not stringly: the envelope law writes a double into
    // acoustic.amplitude, and a property whose opening value is the string
    // "1.0" would make the first write a type change rather than an update.
    // isSoundEmitter stays a string because that is what the laws compare
    // against and what AudioSystem reads.
    //
    // Every field a law reads is seeded here, INCLUDING the two the laws used
    // to bind blind: acoustic.baseFrequency (the vibrato law's carrier) and
    // acoustic.lowpassCutoff (the occlusion law's target). An unseeded dotted
    // property cannot be created by a write — PropertyPath::setValue only
    // mints single-segment dynamic properties — so binding one was a
    // guaranteed no-op.
    // The ASPECT marker. `Law::rebuildRequiredProperties` reduces every
    // authored path to its ROOT segment and `Law::couldApplyTo` then asks the
    // being for a property of exactly that name — so a law whose text says
    // `acoustic.amplitude` requires a property called `acoustic`, and without
    // one every emitter is filtered out of that law's sweep before its
    // condition is ever evaluated. Naming the aspect makes the prefilter true
    // instead of merely non-fatal, and costs nothing: PropertyPath::resolve
    // matches longest-first, so `acoustic.amplitude` still resolves to the
    // amplitude and never to this. (The deeper fix — teaching couldApplyTo
    // that a dotted authored property satisfies its own root — is in Law.cpp;
    // see docs/AUDIO_FIX_NOTES.md.)
    soundEmitter->setDynamicProperty("acoustic", PropertyValue(std::string("sound-emitter")));
    soundEmitter->setDynamicProperty("acoustic.isSoundEmitter", PropertyValue(std::string("true")));
    soundEmitter->setDynamicProperty("acoustic.amplitude", PropertyValue(1.0));
    soundEmitter->setDynamicProperty("acoustic.frequency", PropertyValue(440.0));
    soundEmitter->setDynamicProperty("acoustic.baseFrequency", PropertyValue(440.0));
    soundEmitter->setDynamicProperty("acoustic.lowpassCutoff", PropertyValue(22000.0));
    soundEmitter->setDynamicProperty("acoustic.waveType", PropertyValue(std::string("sine")));

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
