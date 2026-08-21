#include "ActionModel.hpp"

#include "ConstructedBeing/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"

#include <ctime>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>
#include <iostream>
#include <tuple>
#include <map>

// ---------------------------------------------------------------------------
// The ambient trace. A Law arms one around its action loop; every compiled
// node reports into it. Nothing here is thread-safe because law application
// is single-threaded (the tick runs on the main thread, and the EventBus
// dispatches on the publishing thread by design — see LawManager's note).
// ---------------------------------------------------------------------------
namespace {
ActionNode::Trace* g_activeTrace = nullptr;
}

ActionNode::TraceScope::TraceScope() {
    _saved = g_activeTrace;
    g_activeTrace = &_trace;
}

ActionNode::TraceScope::~TraceScope() { g_activeTrace = _saved; }

ActionNode::Trace* ActionNode::activeTrace() { return g_activeTrace; }

void ActionNode::record(NodeOutcome outcome) {
    if (g_activeTrace) g_activeTrace->nodes.push_back(std::move(outcome));
}

const char* ActionNode::reasonName(PropertyPath::PathResult reason) {
    switch (reason) {
        case PropertyPath::PathResult::Ok: return "Ok";
        case PropertyPath::PathResult::NoSuchProperty: return "No Such Property";
        case PropertyPath::PathResult::TypeMismatch: return "Type Mismatch";
        case PropertyPath::PathResult::ReadOnly: return "Read Only";
        case PropertyPath::PathResult::BadComponent: return "Bad Component";
    }
    return "Unknown";
}

namespace {
    // A node that addresses a property reports what its write did.
    void emitResult(Singular& target, const std::string& actionName, PropertyPath::PathResult res, const PropertyPath& path) {
        const bool wrote = res == PropertyPath::PathResult::Ok;
        ActionNode::record(ActionNode::NodeOutcome{actionName, path.toString(), wrote, res, {}});

        if (wrote) {
            Core::EventBus::instance().publish(ActionNode::ExecutedEvent{actionName, &target, std::time(nullptr)});
            return;
        }

        // Console de-duplication. Keyed by IDENTIFIER, never by address: a
        // freed being's slot gets recycled, and a pointer-keyed memo would
        // hand a newborn the corpse's timestamp and swallow its first real
        // error. Bounded, because a law failing on ten thousand subjects
        // must not grow a ten-thousand-entry memo that outlives them.
        static std::map<std::tuple<std::string, std::string, PropertyPath::PathResult, std::string>, std::time_t> lastFailureTime;
        constexpr std::size_t kMaxMemoEntries = 512;
        auto key = std::make_tuple(target.getIdentifier(), actionName, res, path.toString());
        std::time_t now = std::time(nullptr);
        if (now - lastFailureTime[key] > 2) {
            if (lastFailureTime.size() > kMaxMemoEntries) {
                const std::time_t saved = lastFailureTime[key];
                lastFailureTime.clear();
                lastFailureTime[key] = saved;
            }
            lastFailureTime[key] = now;
            std::cerr << "[Law Error] " << actionName << " failed on " << target.getIdentifier()
                      << " for path " << path.toString() << " ("
                      << ActionNode::reasonName(res) << ")" << std::endl;
            Core::EventBus::instance().publish(ActionNode::FailedEvent{actionName, &target, res, now});
        }
    }

    // A node whose effect is not a property write (Publish, Create, Spawn,
    // Destroy, the composition family) reports the same way: what it was,
    // whether it landed, and — when it didn't — why, in words, because no
    // PathResult describes "there was no Zone to be born into".
    void emitEffect(const std::string& actionName, bool landed, const std::string& note = {}) {
        ActionNode::record(ActionNode::NodeOutcome{
            actionName, {}, landed, PropertyPath::PathResult::Ok, landed ? std::string() : note});
    }
}

const char* ActionNode::kindName(Kind k) {
    switch (k) {
        case Kind::Set: return "Set";
        case Kind::Add: return "Add";
        case Kind::Scale: return "Scale";
        case Kind::Lerp: return "Lerp";
        case Kind::Drive: return "Drive";
        case Kind::Sequence: return "Sequence";
        case Kind::Parallel: return "Parallel";
        case Kind::Spawn: return "Spawn";
        case Kind::Map: return "Map";
        case Kind::Flow: return "Flow";
        case Kind::Publish: return "Publish";
        case Kind::Create: return "Create";
        case Kind::AddProperty: return "AddProperty";
        case Kind::RemoveProperty: return "RemoveProperty";
        case Kind::AddElement: return "AddElement";
        case Kind::RemoveElement: return "RemoveElement";
        case Kind::Destroy: return "Destroy";
        case Kind::Synthesize: return "Synthesize";
        case Kind::PlayAudio: return "PlayAudio";
    }
    return "Unknown";
}

// ---------------------------------------------------------------------------
// Participant tokens — the same vocabulary Publish speaks, resolved to a live
// being: "" = the law's subject, "@event.subject" / "@event.object" = the
// triggering event's participants, anything else = a being id looked up in the
// Universe. An unresolvable token yields nothing, and every node that takes one
// does nothing rather than guessing whom the author meant.
// ---------------------------------------------------------------------------
namespace {

// Per-spawn overrides. A concept remembers the shape and colour its members
// had when it was captured; the hard-coded creation tools instead read the
// author's live selection on every click. Reading that selection off the
// subject here is what lets a law reproduce them without a concept per shape.
void applySpawnOverrides(Object& newborn, Singular* source,
                         const PropertyPath& shapeKindPath,
                         const PropertyPath& colorPath) {
    if (!source) return;

    if (!shapeKindPath.empty()) {
        PropertyValue pv;
        double raw = 0.0;
        if (lawGetValue(*source, shapeKindPath, pv) &&
            propertyValueToNumber(pv, raw)) {
            const int k = static_cast<int>(raw);
            // Out-of-range would be undefined behaviour on the enum, and a
            // saved law from a build with more kinds is an ordinary event.
            if (k == static_cast<int>(Object::ShapeKind::Patch)) {
                if (!newborn.hasPatch()) {
                    newborn.setBezierPatch(geom::makeBezierGrid(3, 3, 0.5f));
                }
            } else if (k == static_cast<int>(Object::ShapeKind::Field)) {
                if (!newborn.hasField()) {
                    newborn.setFieldShape(geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.5f)), 1.0f);
                }
            } else if (k >= 0 && k <= static_cast<int>(Object::ShapeKind::Text2D)) {
                // Keep the template's params: they describe radii and fillets
                // the kind still needs, and the selection carries no params.
                newborn.setShape(static_cast<Object::ShapeKind>(k), newborn.getShapeParams());
            }
        }
    }

    if (!colorPath.empty()) {
        PropertyValue pv;
        if (lawGetValue(*source, colorPath, pv) &&
            std::holds_alternative<glm::vec3>(pv)) {
            const glm::vec3 c = std::get<glm::vec3>(pv);
            // Through setFaceColor, so the author's live colour both PAINTS
            // the newborn (its own material, copy-on-write) and lands in the
            // faceColors slots the "color" property reads back. Writing the
            // slots directly left every spawned object unpainted on screen.
            const int faces = newborn.getFaces() > 0 ? newborn.getFaces() : 6;
            for (int f = 0; f < faces; ++f) newborn.setFaceColor(f, c.x, c.y, c.z);
        }
    }

    // Copy acoustic properties from source (e.g., material) to newborn (e.g., sound-emitter)
    for (const auto& prop : {"acoustic.frequency", "acoustic.waveType", "acoustic.amplitude"}) {
        PropertyValue pv;
        if (lawGetValue(*source, PropertyPath::parse(prop), pv)) {
            newborn.setDynamicProperty(prop, pv);
            if (std::string(prop) == "acoustic.frequency") {
                newborn.setDynamicProperty("acoustic.baseFrequency", pv);
            }
        }
    }
}

Singular* resolveBeingToken(const std::string& token, Singular& subject) {
    if (token.empty()) return &subject;
    if (token == "@event.subject") {
        return Universe::instance().hasApplicationEvent()
                   ? Universe::instance().applicationEventSubject() : nullptr;
    }
    if (token == "@event.object") {
        return Universe::instance().hasApplicationEvent()
                   ? Universe::instance().applicationEventObject() : nullptr;
    }
    const std::string id = (token[0] == '@') ? token.substr(1) : token;
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == id) return being;
    }
    return nullptr;
}

// The Zone a creation law writes into: the law's target when it IS a Zone
// (the container is the womb), otherwise the first Zone in the Universe.
// Identifier "World" (the old bag's slug) resolves the same way — to the
// first Zone the provider listed, which is the active one at boot.
Zone* resolveZone(Singular& target) {
    if (auto* asZone = dynamic_cast<Zone*>(&target)) return asZone;
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == "World") {
            if (auto* z = dynamic_cast<Zone*>(being)) return z;
        }
    }
    for (Singular* being : Universe::instance().beings()) {
        if (auto* z = dynamic_cast<Zone*>(being)) return z;
    }
    return nullptr;
}

// The empty value of whatever a first-mover slot currently holds. Used by
// RemoveProperty, which cannot erase a C++ member and so clears it instead.
PropertyValue emptyLike(const PropertyValue& current) {
    if (std::holds_alternative<bool>(current)) return PropertyValue(false);
    if (std::holds_alternative<int>(current)) return PropertyValue(0);
    if (std::holds_alternative<float>(current)) return PropertyValue(0.0f);
    if (std::holds_alternative<double>(current)) return PropertyValue(0.0);
    if (std::holds_alternative<std::string>(current)) return PropertyValue(std::string());
    if (std::holds_alternative<glm::vec3>(current)) return PropertyValue(glm::vec3(0.0f));
    return PropertyValue{};   // monostate: the nearest thing to nullptr we have
}

} // namespace

nlohmann::json ActionNode::toJson() const {
    nlohmann::json j{{"kind", static_cast<int>(kind)}};
    switch (kind) {
        case Kind::Set:
        case Kind::Add:
        case Kind::Scale:
            j["path"] = path.toString();
            j["operand"] = propertyValueToJson(operand);
            break;
        case Kind::Lerp:
            j["path"] = path.toString();
            j["operand"] = propertyValueToJson(operand);
            j["factor"] = factor;
            break;
        case Kind::Drive:
            j["path"] = path.toString();
            j["curve"] = curve.toJson();
            if (!input.empty()) j["input"] = input.toString();
            break;
        case Kind::Sequence:
        case Kind::Parallel: {
            nlohmann::json kids = nlohmann::json::array();
            for (const auto& c : children) kids.push_back(c.toJson());
            j["children"] = kids;
            break;
        }
        case Kind::Spawn: {
            j["conceptId"] = conceptId;
            if (!spawnParentPath.empty()) j["spawnParentPath"] = spawnParentPath.toString();
            if (!spawnPlacementPath.empty()) j["spawnPlacementPath"] = spawnPlacementPath.toString();
            if (!spawnShapeKindPath.empty()) j["spawnShapeKindPath"] = spawnShapeKindPath.toString();
            if (!spawnColorPath.empty()) j["spawnColorPath"] = spawnColorPath.toString();
            if (!children.empty()) {
                nlohmann::json kids = nlohmann::json::array();
                for (const auto& c : children) kids.push_back(c.toJson());
                j["children"] = kids;
            }
            break;
        }
        case Kind::Synthesize: {
            // Kind 17 used to carry an ObjectConcept id. Synthesis is now
            // expressed in its visible action tree, so no hidden registry
            // dependency is serialized for new law text. Preserve old text
            // verbatim, though: silently dropping an un-migrated law on its
            // next save would make its refusal into data loss.
            if (children.empty() && !conceptId.empty()) {
                j["conceptId"] = conceptId;
                if (!spawnParentPath.empty()) j["spawnParentPath"] = spawnParentPath.toString();
                if (!spawnPlacementPath.empty()) j["spawnPlacementPath"] = spawnPlacementPath.toString();
                if (!spawnShapeKindPath.empty()) j["spawnShapeKindPath"] = spawnShapeKindPath.toString();
                if (!spawnColorPath.empty()) j["spawnColorPath"] = spawnColorPath.toString();
            }
            nlohmann::json kids = nlohmann::json::array();
            for (const auto& c : children) kids.push_back(c.toJson());
            j["children"] = kids;
            break;
        }
        case Kind::PlayAudio:
            if (!path.empty()) j["path"] = path.toString();
            if (!input.empty()) j["input"] = input.toString();
            if (!propertyName.empty()) j["propertyName"] = propertyName;
            break;
        case Kind::Map:
        case Kind::Flow:
            j["path"] = path.toString();
            j["function"] = mapFunction.toJson();
            j["bindings"] = mathBindingsToJson(bindings);
            break;
        case Kind::Publish:
            j["eventType"] = eventType;
            if (!publishSubject.empty()) j["publishSubject"] = publishSubject;
            if (!publishObject.empty()) j["publishObject"] = publishObject;
            break;
        case Kind::Create: {
            j["shapeKind"] = createShapeKind;
            if (!createType.empty()) j["createType"] = createType;
            if (!spawnParentPath.empty()) j["spawnParentPath"] = spawnParentPath.toString();
            if (!spawnPlacementPath.empty()) j["spawnPlacementPath"] = spawnPlacementPath.toString();
            if (!spawnShapeKindPath.empty()) j["spawnShapeKindPath"] = spawnShapeKindPath.toString();
            if (!spawnColorPath.empty()) j["spawnColorPath"] = spawnColorPath.toString();
            if (!children.empty()) {
                nlohmann::json kids = nlohmann::json::array();
                for (const auto& c : children) kids.push_back(c.toJson());
                j["children"] = kids;
            }
            break;
        }
        case Kind::AddProperty:
            if (!path.empty()) j["path"] = path.toString();
            j["propertyName"] = propertyName;
            j["operand"] = propertyValueToJson(operand);
            break;
        case Kind::RemoveProperty:
            if (!path.empty()) j["path"] = path.toString();
            j["propertyName"] = propertyName;
            break;
        case Kind::AddElement:
        case Kind::RemoveElement:
            j["containerToken"] = containerToken;
            j["elementToken"] = elementToken;
            break;
        case Kind::Destroy:
            j["elementToken"] = elementToken;
            break;
    }
    return j;
}

ActionNode ActionNode::fromJson(const nlohmann::json& j) {
    ActionNode n;
    n.kind = static_cast<Kind>(j.value("kind", 0));
    if (j.contains("path")) n.path = PropertyPath::parse(j["path"].get<std::string>());
    if (j.contains("operand")) n.operand = propertyValueFromJson(j["operand"]);
    n.factor = j.value("factor", 1.0);
    if (j.contains("curve")) n.curve = CurveModel::fromJson(j["curve"]);
    if (j.contains("input")) n.input = PropertyPath::parse(j["input"].get<std::string>());
    if (j.contains("conceptId")) n.conceptId = j["conceptId"].get<std::string>();
    if (j.contains("spawnParentPath")) n.spawnParentPath = PropertyPath::parse(j["spawnParentPath"].get<std::string>());
    if (j.contains("spawnPlacementPath")) n.spawnPlacementPath = PropertyPath::parse(j["spawnPlacementPath"].get<std::string>());
    if (j.contains("spawnShapeKindPath")) n.spawnShapeKindPath = PropertyPath::parse(j["spawnShapeKindPath"].get<std::string>());
    if (j.contains("spawnColorPath")) n.spawnColorPath = PropertyPath::parse(j["spawnColorPath"].get<std::string>());
    n.eventType = j.value("eventType", std::string());
    n.publishSubject = j.value("publishSubject", std::string());
    n.publishObject = j.value("publishObject", std::string());
    n.createShapeKind = j.value("shapeKind", 0);
    n.createType = j.value("createType", std::string());
    n.propertyName = j.value("propertyName", std::string());
    n.containerToken = j.value("containerToken", std::string());
    n.elementToken = j.value("elementToken", std::string());
    if (j.contains("function")) n.mapFunction = OntoMath::Piecewise::fromJson(j["function"]);
    if (j.contains("bindings")) n.bindings = mathBindingsFromJson(j["bindings"]);
    if (j.contains("children")) {
        for (const auto& c : j["children"]) n.children.push_back(fromJson(c));
    }
    return n;
}

ECA::ActionExecutor ActionNode::compile() const {
    switch (kind) {
        case Kind::Set: {
            const PropertyPath p = path;
            const PropertyValue v = operand;
            return [p, v](const ECA::Event&, Singular& target) { 
                auto res = lawSetValue(target, p, v);
                emitResult(target, "Set", res, p);
            };
        }
        case Kind::Add:
        case Kind::Scale:
        case Kind::Lerp: {
            const PropertyPath p = path;
            const PropertyValue rhs = operand;
            const double f = factor;
            const Kind k = kind;
            return [p, rhs, f, k](const ECA::Event&, Singular& target) {
                PropertyValue current;
                if (!lawGetValue(target, p, current)) return;
                
                PropertyValue result = current;
                double a_num = 0.0, b_num = 0.0;
                bool isNum = propertyValueToNumber(current, a_num) && propertyValueToNumber(rhs, b_num);
                
                if (isNum) {
                    double r = a_num;
                    if (k == Kind::Add) r = a_num + b_num;
                    else if (k == Kind::Scale) r = a_num * b_num;
                    else r = a_num + (b_num - a_num) * f;
                    result = PropertyValue(r);
                } else if (std::holds_alternative<glm::vec3>(current)) {
                    glm::vec3 a = std::get<glm::vec3>(current);
                    if (std::holds_alternative<glm::vec3>(rhs)) {
                        glm::vec3 b = std::get<glm::vec3>(rhs);
                        if (k == Kind::Add) result = PropertyValue(a + b);
                        else if (k == Kind::Scale) result = PropertyValue(a * b);
                        else result = PropertyValue(a + (b - a) * static_cast<float>(f));
                    } else if (propertyValueToNumber(rhs, b_num)) {
                        float b = static_cast<float>(b_num);
                        if (k == Kind::Add) result = PropertyValue(a + glm::vec3(b));
                        else if (k == Kind::Scale) result = PropertyValue(a * b);
                        else result = PropertyValue(a + (glm::vec3(b) - a) * static_cast<float>(f));
                    } else return;
                } else {
                    return;
                }
                
                auto res = lawSetValue(target, p, result);
                emitResult(target, kindName(k), res, p);
            };
        }
        case Kind::Drive: {
            const PropertyPath p = path;
            const PropertyPath in = input;
            const CurveModel c = curve;
            return [p, in, c](const ECA::Event& event, Singular& target) {
                double x = 0.0;
                if (in.empty()) {
                    // No authored input: the world clock is the domain
                    // (falling back to the event's coarse moment when no
                    // engine has set the clock).
                    x = Universe::instance().hasClock()
                            ? Universe::instance().now()
                            : static_cast<double>(event.timestamp);
                } else {
                    PropertyValue v;
                    if (!lawGetValue(target, in, v) || !propertyValueToNumber(v, x)) return;
                }
                auto res = lawSetValue(target, p, PropertyValue(c.evaluate(x)));
                emitResult(target, "Drive", res, p);
            };
        }
        case Kind::Sequence:
        case Kind::Parallel: {
            std::vector<ECA::ActionExecutor> compiled;
            compiled.reserve(children.size());
            for (const auto& c : children) compiled.push_back(c.compile());
            return [compiled, k=kind](const ECA::Event& e, Singular& target) {
                // A composite does not report for itself: its children each
                // record their own outcome, and the trace already says
                // whether anything landed. Claiming success here on top of
                // three failed children is exactly the lie we removed.
                const std::size_t before =
                    ActionNode::activeTrace() ? ActionNode::activeTrace()->nodes.size() : 0;
                for (const auto& run : compiled) {
                    if (run) run(e, target);
                }
                bool anyChildWrote = false;
                if (auto* trace = ActionNode::activeTrace()) {
                    for (std::size_t i = before; i < trace->nodes.size(); ++i) {
                        if (trace->nodes[i].wrote) { anyChildWrote = true; break; }
                    }
                } else {
                    anyChildWrote = !compiled.empty();
                }
                if (anyChildWrote) {
                    Core::EventBus::instance().publish(ActionNode::ExecutedEvent{kindName(k), &target, std::time(nullptr)});
                }
            };
        }
        case Kind::Spawn: {
            const std::string id = conceptId;
            const PropertyPath pPath = spawnParentPath;
            const PropertyPath placementPath = spawnPlacementPath;
            const PropertyPath shapeKindPath = spawnShapeKindPath;
            const PropertyPath colorPath = spawnColorPath;
            std::vector<ECA::ActionExecutor> compiledChildren;
            compiledChildren.reserve(children.size());
            for (const auto& c : children) compiledChildren.push_back(c.compile());

            return [id, pPath, placementPath, shapeKindPath, colorPath,
                    compiledChildren](const ECA::Event& event, Singular& target) {
                Zone* world = resolveZone(target);
                auto concept = ConceptRegistry::instance().find(id);
                if (!world) {
                    emitEffect("Spawn", false, "no Zone to be born into");
                    return;
                }
                if (!concept) {
                    emitEffect("Spawn", false, "no such concept: " + id);
                    return;
                }

                std::vector<Object*> sources;
                glm::mat4 placement(1.0f);
                if (event.subject) {
                    if (auto* subjectObj = dynamic_cast<Object*>(event.subject)) {
                        sources.push_back(subjectObj);
                    }
                    bool placementSet = false;
                    if (!placementPath.empty()) {
                        PropertyValue pv;
                        if (lawGetValue(*event.subject, placementPath, pv)) {
                            if (std::holds_alternative<glm::mat4>(pv)) {
                                placement = std::get<glm::mat4>(pv);
                                placementSet = true;
                            } else if (std::holds_alternative<glm::vec3>(pv)) {
                                placement = glm::translate(glm::mat4(1.0f), std::get<glm::vec3>(pv));
                                placementSet = true;
                            }
                        }
                        // An authored placement that fails to read must ABORT
                        // the spawn, never guess a fallback: a silent fallback
                        // to the subject's own position is what stacked cubes
                        // into the sky. Refusing is loud; guessing is not.
                        if (!placementSet) {
                            emitEffect("Spawn", false,
                                       "placement path unreadable: " + placementPath.toString());
                            return;
                        }
                    } else if (auto* subjectObj = dynamic_cast<Object*>(event.subject)) {
                        placement = glm::translate(glm::mat4(1.0f), subjectObj->getPosition());
                    } else {
                        PropertyValue posVal;
                        if (lawGetValue(*event.subject, PropertyPath::parse("position"), posVal) &&
                            std::holds_alternative<glm::vec3>(posVal)) {
                            placement = glm::translate(glm::mat4(1.0f), std::get<glm::vec3>(posVal));
                        }
                    }
                }
                auto newborns = concept->instantiate(
                    placement, sources.empty() ? nullptr : &sources);
                
                Object* parent = nullptr;
                if (!pPath.empty() && event.subject) {
                    PropertyValue pv;
                    if (lawGetValue(*event.subject, pPath, pv)) {
                        if (std::holds_alternative<std::string>(pv)) {
                            std::string parentId = std::get<std::string>(pv);
                            for (const auto& obj : world->getOwnedObjects()) {
                                if (obj && obj->getIdentifier() == parentId) {
                                    parent = obj.get();
                                    break;
                                }
                            }
                        } else if (std::holds_alternative<Object*>(pv)) {
                            parent = std::get<Object*>(pv);
                        }
                    }
                }

                for (auto& newborn : newborns) {
                    // Before the children run, so an authored child action can
                    // still override the live selection deliberately.
                    applySpawnOverrides(*newborn, event.subject, shapeKindPath, colorPath);
                    for (const auto& run : compiledChildren) {
                        if (run) run(event, *newborn);
                    }
                    Object* born = newborn.get();
                    if (parent) {
                        // Cast to BodyPart since only BodyParts can have sub-objects currently
                        if (auto* bp = dynamic_cast<BodyPart*>(parent)) {
                            bp->addSubObject(std::move(newborn));
                        } else {
                            world->addObject(std::move(newborn));
                        }
                    } else {
                        world->addObject(std::move(newborn));
                    }
                    // A birth is a birth. Create announces "object-created"
                    // and Spawn did not, which meant a spawned being got no
                    // property-state facts asserted for it and was invisible
                    // to every WhileTrue law taking the Rete fast path — the
                    // emitter existed and nothing continuous could ever see
                    // it. Published AFTER ownership transfers, so no listener
                    // sees a being the world does not yet hold.
                    Core::EventBus::instance().publish(
                        ECA::Event{"object-created", born, event.subject, std::time(nullptr)});
                }
                if (newborns.empty()) {
                    emitEffect("Spawn", false, "concept instantiated nothing");
                    return;
                }
                emitEffect("Spawn", true);
                Core::EventBus::instance().publish(ActionNode::ExecutedEvent{"Spawn", &target, std::time(nullptr)});
            };
        }
        case Kind::PlayAudio: {
            const PropertyPath freqPath = path;
            const PropertyPath ampPath = input;
            const std::string matType = propertyName;
            
            return [freqPath, ampPath, matType](const ECA::Event& event, Singular& subject) {
                // Publish an AudioSynthesisEvent to the bus.
                // The AudioSystem listens to this.
                Core::EventBus::instance().publish(
                    ECA::Event{"audio-synthesized", &subject, nullptr, std::time(nullptr)}
                );
                
                emitEffect("PlayAudio", true);
            };
        }
        case Kind::Publish: {
            // MINT an event: the law contributes to the world's event
            // vocabulary instead of only consuming it. Participants resolve
            // at fire time; an unproven SUBJECT publishes nothing (a law
            // never testifies about a being the world cannot produce).
            // Cascades resolve within the tick, bounded by kMaxChainRounds.
            const std::string type = eventType;
            const std::string subjectToken = publishSubject;
            const std::string objectToken = publishObject;
            const auto resolveToken = [](const std::string& token) -> Singular* {
                if (token == "@event.subject" || token == "@event.object") {
                    if (!Universe::instance().hasApplicationEvent()) return nullptr;
                    return token == "@event.subject"
                               ? Universe::instance().applicationEventSubject()
                               : Universe::instance().applicationEventObject();
                }
                for (Singular* being : Universe::instance().beings()) {
                    if (being && being->getIdentifier() == token) return being;
                }
                return nullptr;
            };
            return [type, subjectToken, objectToken, resolveToken](
                       const ECA::Event&, Singular& lawSubject) {
                if (type.empty()) {
                    emitEffect("Publish", false, "no event type authored");
                    return;
                }
                Singular* eventSubject =
                    subjectToken.empty() ? &lawSubject : resolveToken(subjectToken);
                if (!eventSubject) {   // unproven: no testimony
                    emitEffect("Publish", false, "unproven subject: " + subjectToken);
                    return;
                }
                Singular* eventObject =
                    objectToken.empty() ? nullptr : resolveToken(objectToken);
                Core::EventBus::instance().publish(
                    ECA::Event{type, eventSubject, eventObject, std::time(nullptr)});
                emitEffect("Publish", true);
                Core::EventBus::instance().publish(ActionNode::ExecutedEvent{"Publish", &lawSubject, std::time(nullptr)});
            };
        }
        case Kind::Map: {
            // path := f(bindings) — behavior governed by an authored,
            // exact, piecewise mathematical function. Undefined math writes
            // nothing: a law never manifests undefined values.
            const PropertyPath target = path;
            const OntoMath::Piecewise f = mapFunction;
            const MathBindings binds = bindings;
            return [target, f, binds](const ECA::Event&, Singular& subject) {
                auto vars = readMathBindings(subject, binds);
                if (!vars) {
                    emitEffect("Map", false, "a bound variable does not read on this subject");
                    return;
                }
                std::map<std::string, PropertyValue> pVars;
                for (const auto& [k, v] : *vars) pVars[k] = PropertyValue(v);
                const auto valProp = f.evaluate(pVars, &subject);
                if (!valProp) {
                    emitEffect("Map", false, "outside the authored bounds");
                    return;
                }
                auto res = lawSetValue(subject, target, *valProp);
                emitResult(subject, "Map", res, target);
            };
        }
        case Kind::Flow: {
            // path := path + f(bindings) * dt — the authored model is the
            // RATE of change, integrated one frame at a time. Needs the
            // world clock (dt); undefined math flows nothing.
            const PropertyPath target = path;
            const OntoMath::Piecewise f = mapFunction;
            const MathBindings binds = bindings;
            return [target, f, binds](const ECA::Event&, Singular& subject) {
                if (!Universe::instance().hasClock()) {
                    emitEffect("Flow", false, "no world clock: nothing to integrate over");
                    return;
                }
                // The TARGET is read first, and deliberately. An integrator
                // that cannot read what it integrates cannot do anything with
                // the rate, and a bound variable may be arbitrarily expensive
                // to evaluate — a world reading can cost a raycast against
                // every being in the world. Reading the bindings first meant
                // that work was done and then thrown away on every tick where
                // the target did not resolve, which was every tick, because
                // the property the occlusion law wrote to was never seeded.
                // Cheapest refusal first.
                PropertyValue current;
                if (!lawGetValue(subject, target, current)) {
                    emitEffect("Flow", false, "cannot read " + target.toString());
                    return;
                }

                auto vars = readMathBindings(subject, binds);
                if (!vars) {
                    emitEffect("Flow", false, "a bound variable does not read on this subject");
                    return;
                }
                std::map<std::string, PropertyValue> pVars;
                for (const auto& [k, v] : *vars) pVars[k] = PropertyValue(v);
                const auto valProp = f.evaluate(pVars, &subject);
                // Undefined math flows NOTHING — outside the authored bounds
                // is the law ending, not the law failing. Recorded as a
                // non-write so a drive session sees it and lets go.
                if (!valProp) {
                    emitEffect("Flow", false, "outside the authored bounds");
                    return;
                }

                const double dt = Universe::instance().dt();
                PropertyValue next = current;

                // Numbers are matched by VALUE, not by variant alternative.
                // The old test asked for `double` on both sides, but a
                // component read ("position.y") yields a FLOAT — so every
                // Flow authored against a vector lane silently integrated
                // nothing, forever, with no trace that it had not run.
                double currentNum = 0.0, rateNum = 0.0;
                if (propertyValueToNumber(current, currentNum) &&
                    propertyValueToNumber(*valProp, rateNum)) {
                    next = PropertyValue(currentNum + rateNum * dt);
                } else if (std::holds_alternative<glm::vec3>(current) &&
                           std::holds_alternative<glm::vec3>(*valProp)) {
                    next = PropertyValue(std::get<glm::vec3>(current) +
                                         std::get<glm::vec3>(*valProp) * static_cast<float>(dt));
                } else {
                    emitEffect("Flow", false,
                               "cannot integrate " + target.toString() + ": rate and value disagree");
                    return;
                }

                auto res = lawSetValue(subject, target, next);
                emitResult(subject, "Flow", res, target);
            };
        }

        // ------------------------------------------------------------------
        // Creation from nothing. Spawn needs a concept — a thing the world was
        // already shown. Create needs only a shape kind: a law may author a
        // being nobody captured for it.
        // ------------------------------------------------------------------
        case Kind::Create: {
            const int shapeKind = createShapeKind;
            const std::string type = createType;
            const PropertyPath parentPath = spawnParentPath;
            const PropertyPath placementPath = spawnPlacementPath;
            const PropertyPath shapeKindPath = spawnShapeKindPath;
            const PropertyPath colorPath = spawnColorPath;
            std::vector<ECA::ActionExecutor> compiledChildren;
            compiledChildren.reserve(children.size());
            for (const auto& c : children) compiledChildren.push_back(c.compile());

            return [shapeKind, type, parentPath, placementPath, shapeKindPath, colorPath,
                    compiledChildren](const ECA::Event& event, Singular& target) {
                Zone* world = resolveZone(target);
                if (!world) {   // nowhere to be born: nothing happens
                    emitEffect("Create", false, "no Zone to be born into");
                    return;
                }

                auto newborn = std::make_unique<Object>();
                newborn->setShape(static_cast<Object::ShapeKind>(shapeKind), Object::ShapeParams{});
                if (!type.empty()) newborn->setObjectType(type);

                // Where. An authored placement path wins; otherwise the
                // newborn appears where its law's subject stands.
                glm::vec3 position(0.0f);
                if (!placementPath.empty()) {
                    PropertyValue pv;
                    bool placementSet = false;
                    if (lawGetValue(target, placementPath, pv)) {
                        if (std::holds_alternative<glm::vec3>(pv)) {
                            position = std::get<glm::vec3>(pv);
                            placementSet = true;
                        } else if (std::holds_alternative<glm::mat4>(pv)) {
                            position = glm::vec3(std::get<glm::mat4>(pv)[3]);
                            placementSet = true;
                        }
                    }
                    if (!placementSet) {
                        emitEffect("Create", false,
                                   "placement path unreadable: " + placementPath.toString());
                        return;
                    }
                } else if (auto* asObject = dynamic_cast<Object*>(&target)) {
                    position = asObject->getPosition();
                }
                newborn->setTransform(glm::translate(glm::mat4(1.0f), position));
                newborn->updateCollisionZone(newborn->getTransform());

                // The newborn is the SUBJECT of this node's children, so the
                // whole action vocabulary shapes it: Set its color, Map its
                // radius from the subject's, AddProperty, AddElement.
                Object* born = newborn.get();
                applySpawnOverrides(*born, &target, shapeKindPath, colorPath);
                for (const auto& run : compiledChildren) {
                    if (run) run(event, *born);
                }

                // An authored parent takes it as an element; otherwise the
                // Zone does. Either way something owns it.
                Object* parent = nullptr;
                if (!parentPath.empty()) {
                    PropertyValue pv;
                    if (lawGetValue(target, parentPath, pv) &&
                        std::holds_alternative<std::string>(pv)) {
                        const std::string parentId = std::get<std::string>(pv);
                        for (const auto& obj : world->getOwnedObjects()) {
                            if (obj && obj->getIdentifier() == parentId) {
                                parent = obj.get();
                                break;
                            }
                        }
                    }
                }
                world->addObject(std::move(newborn));
                if (parent) parent->addElement(born);

                emitEffect("Create", true);
                Core::EventBus::instance().publish(
                    ECA::Event{"object-created", born, &target, std::time(nullptr)});
            };
        }

        // ------------------------------------------------------------------
        // Vocabulary a Person adds to a being. The first-mover registry is
        // what the engine granted; this is what a law grants — and it is
        // refused where it would SHADOW a registered name, because a silent
        // shadow means a path that reads one value and writes another.
        // ------------------------------------------------------------------
        case Kind::AddProperty: {
            const PropertyPath owner = path;
            const std::string name = propertyName;
            const PropertyValue initial = operand;
            return [owner, name, initial](const ECA::Event&, Singular& subject) {
                if (name.empty()) {
                    emitEffect("AddProperty", false, "no property name authored");
                    return;
                }
                PropertyPath remainder;
                Singular* being = resolveLawRoot(subject, owner, remainder);
                if (!being) {
                    emitEffect("AddProperty", false, "unproven owner: " + owner.toString());
                    return;
                }
                if (being->findProperty(name)) {   // never shadow a first mover
                    emitEffect("AddProperty", false, "would shadow first-mover '" + name + "'");
                    return;
                }
                being->setDynamicProperty(name, initial);
                emitEffect("AddProperty", true);
            };
        }

        case Kind::RemoveProperty: {
            const PropertyPath owner = path;
            const std::string name = propertyName;
            return [owner, name](const ECA::Event&, Singular& subject) {
                if (name.empty()) {
                    emitEffect("RemoveProperty", false, "no property name authored");
                    return;
                }
                PropertyPath remainder;
                Singular* being = resolveLawRoot(subject, owner, remainder);
                if (!being) {
                    emitEffect("RemoveProperty", false, "unproven owner: " + owner.toString());
                    return;
                }
                // An authored property is erased outright — it was granted by
                // law and law may take it back.
                if (being->removeDynamicProperty(name)) {
                    emitEffect("RemoveProperty", true);
                    return;
                }
                // A first-mover property is a C++ member: the slot cannot be
                // erased, so it is CLEARED. Honest, and never silent about it.
                if (Property* property = being->findProperty(name)) {
                    const PropertyValue empty = emptyLike(property->value());
                    const bool cleared = property->setValue(empty);
                    emitEffect("RemoveProperty", cleared,
                               cleared ? std::string() : "first-mover slot refused clearing");
                    return;
                }
                emitEffect("RemoveProperty", false, "no such property: " + name);
            };
        }

        // ------------------------------------------------------------------
        // Composition: what a being is MADE OF, authorable.
        // ------------------------------------------------------------------
        case Kind::AddElement:
        case Kind::RemoveElement: {
            const bool adding = (kind == Kind::AddElement);
            const std::string container = containerToken;
            const std::string element = elementToken;
            return [adding, container, element](const ECA::Event&, Singular& subject) {
                const char* name = adding ? "AddElement" : "RemoveElement";
                Singular* containerBeing = resolveBeingToken(container, subject);
                Singular* elementBeing = resolveBeingToken(element, subject);
                if (!containerBeing) {
                    emitEffect(name, false, "unproven container: " + container);
                    return;
                }
                if (!elementBeing) {
                    emitEffect(name, false, "unproven element: " + element);
                    return;
                }
                auto* asObject = dynamic_cast<Object*>(containerBeing);
                if (!asObject) {   // only Objects hold elements today
                    emitEffect(name, false, "container is not an Object");
                    return;
                }
                if (adding) asObject->addElement(elementBeing);
                else asObject->removeElement(elementBeing);
                emitEffect(name, true);
            };
        }

        // ------------------------------------------------------------------
        // Unmaking. The delete tool as law-text: "when this is touched by
        // fire, it is gone."
        //
        // The being is ASKED for, not taken. Freeing it here would pull the
        // ground out from under the caller: this action returns into
        // Law::applyTo, which still has to write the record, log the
        // outcome, and publish the applied event — all through a reference
        // to the subject. Destroying your own subject is the most obvious
        // law anyone writes, and it read freed memory three times before
        // returning. The LawManager reaps at the end of the tick, once no
        // pointer to the victim is still live; until then Universe::beings()
        // hides it, so no law acts on a corpse.
        // ------------------------------------------------------------------
        case Kind::Destroy: {
            const std::string victimToken = elementToken;
            return [victimToken](const ECA::Event&, Singular& subject) {
                Singular* victim = resolveBeingToken(victimToken, subject);
                if (!victim) {
                    emitEffect("Destroy", false, "unproven victim: " + victimToken);
                    return;
                }
                if (!dynamic_cast<Object*>(victim)) {
                    emitEffect("Destroy", false, "only Objects can be unmade today");
                    return;
                }
                Universe::instance().requestUnmaking(victim);
                emitEffect("Destroy", true);
            };
        }
        // ------------------------------------------------------------------
        // Synthesis is the visible composition of ordinary actions. A Create
        // child establishes a newborn subject; its Map bindings can read the
        // event participants through @event.subject / @event.object, and its
        // Set/AddProperty/AddElement children shape that newborn. Nothing is
        // hidden in an ObjectConcept or a registry here.
        // ------------------------------------------------------------------
        case Kind::Synthesize: {
            std::vector<ECA::ActionExecutor> compiled;
            compiled.reserve(children.size());
            for (const auto& child : children) compiled.push_back(child.compile());
            return [compiled](const ECA::Event& event, Singular& target) {
                if (compiled.empty()) {
                    emitEffect("Synthesize", false,
                               "no composed creation actions authored; migrate this legacy "
                               "Synthesize into Create plus property actions");
                    return;
                }
                const std::size_t before =
                    ActionNode::activeTrace() ? ActionNode::activeTrace()->nodes.size() : 0;
                for (const auto& run : compiled) {
                    if (run) run(event, target);
                }
                bool anyChildWrote = false;
                if (auto* trace = ActionNode::activeTrace()) {
                    for (std::size_t i = before; i < trace->nodes.size(); ++i) {
                        if (trace->nodes[i].wrote) { anyChildWrote = true; break; }
                    }
                } else {
                    // Direct callers still executed a non-empty authored tree.
                    anyChildWrote = !compiled.empty();
                }
                if (anyChildWrote) {
                    Core::EventBus::instance().publish(
                        ActionNode::ExecutedEvent{"Synthesize", &target, std::time(nullptr)});
                }
            };
        }
    }
    return [](const ECA::Event&, Singular&) {};
}

std::string ActionNode::describe() const {
    switch (kind) {
        case Kind::Set: return "set " + path.toString();
        case Kind::Add: return "add to " + path.toString();
        case Kind::Scale: return "scale " + path.toString();
        case Kind::Lerp: return "lerp " + path.toString();
        case Kind::Drive:
            return "drive " + path.toString() +
                   (input.empty() ? " from event-time" : " from " + input.toString());
        case Kind::Sequence: return "sequence(" + std::to_string(children.size()) + ")";
        case Kind::Parallel: return "parallel(" + std::to_string(children.size()) + ")";
        case Kind::Spawn: return "spawn(" + conceptId + ")";
        case Kind::Map: return path.toString() + " := " + mapFunction.print();
        case Kind::Flow: return "d(" + path.toString() + ")/dt = " + mapFunction.print();
        case Kind::Publish: return "publish '" + eventType + "'";
        case Kind::Create:
            return "create object" + (createType.empty() ? std::string()
                                                         : " '" + createType + "'");
        case Kind::AddProperty: return "grant property '" + propertyName + "'";
        case Kind::RemoveProperty: return "remove property '" + propertyName + "'";
        case Kind::AddElement:
            return "add " + (elementToken.empty() ? std::string("subject") : elementToken) +
                   " as element of " +
                   (containerToken.empty() ? std::string("subject") : containerToken);
        case Kind::RemoveElement:
            return "remove " + (elementToken.empty() ? std::string("subject") : elementToken) +
                   " from " +
                   (containerToken.empty() ? std::string("subject") : containerToken);
        case Kind::Destroy:
            return "destroy " + (elementToken.empty() ? std::string("subject") : elementToken);
        case Kind::Synthesize:
            return "synthesize(" + std::to_string(children.size()) + " composed actions)";
        case Kind::PlayAudio:
            return kindName(kind);
    }
    return "action";
}

// ---------------------------------------------------------------------------
// Drive-session scans.
// ---------------------------------------------------------------------------
namespace {
bool isSinceAppliedPath(const PropertyPath& p) {
    return p.segments.size() == 2 && p.segments[0] == "time" &&
           p.segments[1] == "sinceApplied";
}
} // namespace

bool ActionNode::referencesSinceApplied() const {
    if (kind == Kind::Drive && isSinceAppliedPath(input)) return true;
    if (kind == Kind::Map || kind == Kind::Flow) {
        for (const auto& entry : bindings) {
            if (isSinceAppliedPath(entry.second)) return true;
        }
    }
    for (const auto& c : children) {
        if (c.referencesSinceApplied()) return true;
    }
    return false;
}

bool ActionNode::definedFor(Singular& subject) const {
    switch (kind) {
        case Kind::Map:
        case Kind::Flow: {
            // Defined exactly when the function evaluates: every variable
            // readable AND the values inside some authored piece — whichever
            // variable the bounds cut.
            auto vars = readMathBindings(subject, bindings);
            if (!vars) return false;
            std::map<std::string, PropertyValue> pVars;
            for (const auto& [k, v] : *vars) pVars[k] = PropertyValue(v);
            return mapFunction.evaluate(pVars, &subject).has_value();
        }
        case Kind::Drive: {
            // A curve is total: defined whenever its input is readable.
            if (input.empty()) return Universe::instance().hasClock();
            PropertyValue v;
            return lawGetValue(subject, input, v);
        }
        case Kind::Sequence:
        case Kind::Parallel: {
            if (children.empty()) return true;
            // ONLY BOUNDED CHILDREN VOTE. A plain `any` fold over every child
            // makes authored bounds unwritable: Sequence[bounded arc, Set]
            // stays defined forever because the Set always answers yes, and
            // the arc's bounds — the whole point of authoring them — can
            // never end the drive. So if this composite contains any node
            // whose domain the author wrote, the composite lives exactly as
            // long as one of THOSE is still defined. A composite with no
            // bounded child anywhere is unbounded, as before.
            bool anyBounded = false;
            for (const auto& c : children) {
                if (c.hasAuthoredBounds()) { anyBounded = true; break; }
            }
            if (!anyBounded) return true;
            for (const auto& c : children) {
                if (c.hasAuthoredBounds() && c.definedFor(subject)) return true;
            }
            return false;
        }
        default:
            // Set/Add/Scale/Lerp/Spawn and the creation family carry no
            // authored bounds — they can always act (an eternal drive unless a
            // bounded function ends it). Create's children are deliberately
            // NOT consulted: they are defined against the newborn, who does
            // not exist yet, never against this subject.
            return true;
    }
}

bool ActionNode::hasAuthoredBounds() const {
    // Map and Flow are the authored functions: their Piecewise carries a
    // domain, and outside it the law writes nothing. Drive's curve is total
    // — it has an input but no bounds — so it never ends a drive on its own.
    if (kind == Kind::Map || kind == Kind::Flow) return true;
    if (kind == Kind::Sequence || kind == Kind::Parallel) {
        for (const auto& c : children) {
            if (c.hasAuthoredBounds()) return true;
        }
    }
    return false;
}

void ActionNode::collectPaths(std::vector<PropertyPath>& out) const {
    const auto add = [&out](const PropertyPath& p) {
        if (!p.empty()) out.push_back(p);
    };
    switch (kind) {
        case Kind::Set:
        case Kind::Add:
        case Kind::Scale:
        case Kind::Lerp:
        case Kind::Map:
        case Kind::Flow:
            add(path);
            break;
        case Kind::Drive:
            add(path);
            add(input);
            break;
        case Kind::Spawn:
        case Kind::Synthesize:
            add(spawnParentPath);
            add(spawnPlacementPath);
            add(spawnShapeKindPath);
            add(spawnColorPath);
            break;
        case Kind::Create:
            // The newborn's own paths are addressed against the NEWBORN, not
            // against this subject, so its children are deliberately skipped:
            // filtering a sweep by them would exclude every being that could
            // legitimately create.
            add(spawnParentPath);
            add(spawnPlacementPath);
            add(spawnShapeKindPath);
            add(spawnColorPath);
            return;
        default:
            break;
    }
    for (const auto& b : bindings) add(b.second);
    for (const auto& c : children) c.collectPaths(out);
}

// ---------------------------------------------------------------------------
// Reversal. See the note on ActionNode::Reversibility for what this judges and
// — just as important — what it deliberately does not.
// ---------------------------------------------------------------------------
namespace {

// The parameter a Map or Flow is a function OF: the one bound variable naming
// the world clock. Exactly one is required. With none there is no time axis to
// travel along; with several the model is a function of two clocks at once and
// which one the reversal should walk is not written anywhere.
const std::string* soleTimeVariable(const MathBindings& bindings, std::string& why) {
    const std::string* found = nullptr;
    for (const auto& entry : bindings) {
        if (!isTimePath(entry.second)) continue;
        if (found) {
            why = "several bound variables name the clock: no single time axis to reverse along";
            return nullptr;
        }
        found = &entry.first;
    }
    if (!found) why = "no bound variable names the clock: the model is not a function of time";
    return found;
}

// Every property this tree WRITES (as opposed to merely reads). A rate that
// reads what it writes is an ordinary differential equation, not a quadrature,
// and its closed form is not the antiderivative of its own text.
void collectWrittenPaths(const ActionNode& node, std::vector<std::string>& out) {
    switch (node.kind) {
        case ActionNode::Kind::Set:
        case ActionNode::Kind::Add:
        case ActionNode::Kind::Scale:
        case ActionNode::Kind::Lerp:
        case ActionNode::Kind::Drive:
        case ActionNode::Kind::Map:
        case ActionNode::Kind::Flow:
            if (!node.path.empty()) out.push_back(node.path.toString());
            break;
        default:
            break;
    }
    for (const auto& child : node.children) collectWrittenPaths(child, out);
}

// Does any bound variable other than the time parameter read a property this
// tree writes?
bool readsWhatItWrites(const MathBindings& bindings, const std::string& timeVar,
                       const std::vector<std::string>& written, std::string& why) {
    for (const auto& entry : bindings) {
        if (entry.first == timeVar) continue;
        const std::string path = entry.second.toString();
        if (std::find(written.begin(), written.end(), path) == written.end()) continue;
        why = "'" + entry.first + "' reads " + path + ", which this action also writes: "
              "that is a differential equation, not a quadrature";
        return true;
    }
    return false;
}

void judge(const ActionNode& node, const std::vector<std::string>& written,
           std::vector<std::string>& obstacles) {
    const std::string name = ActionNode::kindName(node.kind);
    switch (node.kind) {
        case ActionNode::Kind::Map:
        case ActionNode::Kind::Flow: {
            std::string why;
            const std::string* timeVar = soleTimeVariable(node.bindings, why);
            if (!timeVar) { obstacles.push_back(name + ": " + why); return; }
            if (readsWhatItWrites(node.bindings, *timeVar, written, why)) {
                obstacles.push_back(name + ": " + why);
                return;
            }
            // Flow integrates its model; Map only re-evaluates it at the past
            // time, so it needs the weaker property — that the model reads no
            // world state whose past is unknown.
            if (node.kind == ActionNode::Kind::Flow) {
                if (!OntoMath::integrable(node.mapFunction, *timeVar, &why)) {
                    obstacles.push_back("Flow on " + node.path.toString() + ": " + why);
                }
                return;
            }
            for (const auto& piece : node.mapFunction.pieces) {
                if (piece.guard || piece.whereLEZero) {
                    obstacles.push_back("Map on " + node.path.toString() +
                                        ": a piece is gated on a guard, whose past "
                                        "applicability is unknown");
                    return;
                }
                if (piece.fold) {
                    obstacles.push_back("Map on " + node.path.toString() +
                                        ": a piece folds over the world, whose past is "
                                        "not in the law text");
                    return;
                }
            }
            return;
        }

        case ActionNode::Kind::Drive:
            // A CurveModel is symbolic (constant, polynomial, sinusoid), so
            // the past value is simply the curve at the past input — provided
            // the input IS time. Driven by anything else, the past of the
            // input is the unknown, one level out.
            if (!isTimePath(node.input)) {
                obstacles.push_back("Drive on " + node.path.toString() +
                                    ": its input is not the clock, so the past of the "
                                    "input is itself unknown");
            }
            return;

        case ActionNode::Kind::Sequence:
        case ActionNode::Kind::Parallel:
            for (const auto& child : node.children) judge(child, written, obstacles);
            return;

        // These write no property. They are not obstacles to carrying a value
        // backwards — though the events they mint cascaded into laws this
        // judgement cannot see, which is why the Zone-level fold is the honest
        // scope for "can the world be rewound".
        case ActionNode::Kind::Publish:
        case ActionNode::Kind::PlayAudio:
            return;

        case ActionNode::Kind::Set:
            obstacles.push_back("Set on " + node.path.toString() +
                                ": the value it overwrote is not in the law text");
            return;

        case ActionNode::Kind::Add:
        case ActionNode::Kind::Scale:
        case ActionNode::Kind::Lerp:
            obstacles.push_back(name + " on " + node.path.toString() +
                                ": invertible per firing, but the law text does not "
                                "record how many times it fired");
            return;

        case ActionNode::Kind::Destroy:
            obstacles.push_back("Destroy: annihilation has no inverse in the law text");
            return;

        case ActionNode::Kind::Create:
        case ActionNode::Kind::Spawn:
        case ActionNode::Kind::Synthesize:
            obstacles.push_back(name + ": bringing a being into the world is not a "
                                       "quantity to integrate backwards");
            return;

        default:
            obstacles.push_back(name + ": changes what a being IS, not what it holds");
            return;
    }
}

}   // namespace

std::string ActionNode::Reversibility::summary() const {
    if (exact) return "exactly reversible";
    if (obstacles.empty()) return "not reversible";
    std::string out = "not reversible: " + obstacles.front();
    if (obstacles.size() > 1) {
        out += " (and " + std::to_string(obstacles.size() - 1) + " more)";
    }
    return out;
}

ActionNode::Reversibility ActionNode::reversibility() const {
    std::vector<std::string> written;
    collectWrittenPaths(*this, written);
    Reversibility result;
    judge(*this, written, result.obstacles);
    result.exact = result.obstacles.empty();
    return result;
}

std::optional<PropertyValue> ActionNode::valueSecondsAgo(Singular& subject,
                                                         double secondsAgo) const {
    if (!reversibility().exact) return std::nullopt;
    if (secondsAgo == 0.0) {
        PropertyValue now;
        if (!lawGetValue(subject, path, now)) return std::nullopt;
        return now;
    }

    if (kind == Kind::Drive) {
        // path := curve(input), and the input is the clock: evaluate the same
        // curve one interval earlier.
        PropertyValue inputNow;
        double t = 0.0;
        if (!lawGetValue(subject, input, inputNow) || !propertyValueToNumber(inputNow, t)) {
            return std::nullopt;
        }
        return PropertyValue(curve.evaluate(t - secondsAgo));
    }

    if (kind != Kind::Map && kind != Kind::Flow) return std::nullopt;

    std::string why;
    const std::string* timeVar = soleTimeVariable(bindings, why);
    if (!timeVar) return std::nullopt;

    auto vars = readMathBindings(subject, bindings);
    if (!vars) return std::nullopt;
    const auto clock = vars->find(*timeVar);
    if (clock == vars->end()) return std::nullopt;
    const double now = clock->second;
    const double then = now - secondsAgo;

    if (kind == Kind::Map) {
        // p = F(t): the past is the same function, one interval earlier.
        std::map<std::string, PropertyValue> pVars;
        for (const auto& [key, value] : *vars) pVars[key] = PropertyValue(value);
        pVars[*timeVar] = PropertyValue(then);
        return mapFunction.evaluate(pVars, &subject);
    }

    // dp/dt = f: subtract exactly what flowed over [then, now].
    PropertyValue current;
    double currentNum = 0.0;
    if (!lawGetValue(subject, path, current) ||
        !propertyValueToNumber(current, currentNum)) {
        return std::nullopt;
    }
    std::map<std::string, double> others = *vars;
    others.erase(*timeVar);
    const auto travelled =
        OntoMath::definiteIntegral(mapFunction, *timeVar, then, now, others, &why);
    if (!travelled) return std::nullopt;
    return PropertyValue(currentNum - *travelled);
}

ActionNode ActionNode::set(const std::string& dottedPath, PropertyValue v) {
    ActionNode n;
    n.kind = Kind::Set;
    n.path = PropertyPath::parse(dottedPath);
    n.operand = std::move(v);
    return n;
}

ActionNode ActionNode::add(const std::string& dottedPath, double delta) {
    ActionNode n;
    n.kind = Kind::Add;
    n.path = PropertyPath::parse(dottedPath);
    n.operand = PropertyValue(delta);
    return n;
}

ActionNode ActionNode::scale(const std::string& dottedPath, double factor) {
    ActionNode n;
    n.kind = Kind::Scale;
    n.path = PropertyPath::parse(dottedPath);
    n.operand = PropertyValue(factor);
    return n;
}

ActionNode ActionNode::drive(const std::string& dottedPath, CurveModel curve,
                             const std::string& inputPath) {
    ActionNode n;
    n.kind = Kind::Drive;
    n.path = PropertyPath::parse(dottedPath);
    n.curve = std::move(curve);
    if (!inputPath.empty()) n.input = PropertyPath::parse(inputPath);
    return n;
}

ActionNode ActionNode::map(const std::string& dottedPath, OntoMath::Piecewise function,
                           MathBindings bindings) {
    ActionNode n;
    n.kind = Kind::Map;
    n.path = PropertyPath::parse(dottedPath);
    n.mapFunction = std::move(function);
    n.bindings = std::move(bindings);
    return n;
}

ActionNode ActionNode::flow(const std::string& dottedPath, OntoMath::Piecewise function,
                            MathBindings bindings) {
    ActionNode n;
    n.kind = Kind::Flow;
    n.path = PropertyPath::parse(dottedPath);
    n.mapFunction = std::move(function);
    n.bindings = std::move(bindings);
    return n;
}

ActionNode ActionNode::publish(const std::string& type, const std::string& subjectToken,
                               const std::string& objectToken) {
    ActionNode n;
    n.kind = Kind::Publish;
    n.eventType = type;
    n.publishSubject = subjectToken;
    n.publishObject = objectToken;
    return n;
}

ActionNode ActionNode::sequence(std::vector<ActionNode> children) {
    ActionNode n;
    n.kind = Kind::Sequence;
    n.children = std::move(children);
    return n;
}

ActionNode ActionNode::parallel(std::vector<ActionNode> children) {
    ActionNode n;
    n.kind = Kind::Parallel;
    n.children = std::move(children);
    return n;
}

ActionNode ActionNode::spawn(const std::string& conceptId, const std::string& spawnParentPath) {
    ActionNode n;
    n.kind = Kind::Spawn;
    n.conceptId = conceptId;
    if (!spawnParentPath.empty()) {
        n.spawnParentPath = PropertyPath::parse(spawnParentPath);
    }
    return n;
}

ActionNode ActionNode::create(int shapeKind, const std::string& createType,
                              std::vector<ActionNode> children) {
    ActionNode n;
    n.kind = Kind::Create;
    n.createShapeKind = shapeKind;
    n.createType = createType;
    n.children = std::move(children);
    return n;
}

ActionNode ActionNode::addProperty(const std::string& ownerPath,
                                   const std::string& propertyName,
                                   PropertyValue initial) {
    ActionNode n;
    n.kind = Kind::AddProperty;
    if (!ownerPath.empty()) n.path = PropertyPath::parse(ownerPath);
    n.propertyName = propertyName;
    n.operand = std::move(initial);
    return n;
}

ActionNode ActionNode::removeProperty(const std::string& ownerPath,
                                      const std::string& propertyName) {
    ActionNode n;
    n.kind = Kind::RemoveProperty;
    if (!ownerPath.empty()) n.path = PropertyPath::parse(ownerPath);
    n.propertyName = propertyName;
    return n;
}

ActionNode ActionNode::addElement(const std::string& containerToken,
                                  const std::string& elementToken) {
    ActionNode n;
    n.kind = Kind::AddElement;
    n.containerToken = containerToken;
    n.elementToken = elementToken;
    return n;
}

ActionNode ActionNode::removeElement(const std::string& containerToken,
                                     const std::string& elementToken) {
    ActionNode n;
    n.kind = Kind::RemoveElement;
    n.containerToken = containerToken;
    n.elementToken = elementToken;
    return n;
}

ActionNode ActionNode::destroy(const std::string& targetToken) {
    ActionNode n;
    n.kind = Kind::Destroy;
    n.elementToken = targetToken;
    return n;
}

ActionNode ActionNode::playAudio(const std::string& freqPath, const std::string& ampPath, const std::string& waveType) {
    ActionNode n;
    n.kind = Kind::PlayAudio;
    n.path = PropertyPath::parse(freqPath);
    n.input = PropertyPath::parse(ampPath);
    n.propertyName = waveType;
    return n;
}
