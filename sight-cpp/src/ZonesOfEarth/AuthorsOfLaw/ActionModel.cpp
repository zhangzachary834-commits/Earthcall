#include "ActionModel.hpp"

#include "Form/Object/Creation/ObjectConcept.hpp"
#include "Form/Singular/Property/PropertyValueJson.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/World/World.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"

#include <ctime>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>
#include <iostream>

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
        case Kind::AddElement: return "AddElement";
        case Kind::RemoveProperty: return "RemoveProperty";
        case Kind::RemoveElement: return "RemoveElement";
        case Kind::Destroy: return "Destroy";
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

// The World a creation law writes into: the law's target when it IS a world
// (the container is the womb), otherwise the first World in the Universe.
World* resolveWorld(Singular& target) {
    if (auto* asWorld = dynamic_cast<World*>(&target)) return asWorld;
    for (Singular* being : Universe::instance().beings()) {
        if (auto* w = dynamic_cast<World*>(being)) return w;
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
            if (!children.empty()) {
                nlohmann::json kids = nlohmann::json::array();
                for (const auto& c : children) kids.push_back(c.toJson());
                j["children"] = kids;
            }
            break;
        }
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
                if (lawSetValue(target, p, v)) {
                    std::cout << "[Action Fired] Set evaluated successfully on " << target.getIdentifier() << std::endl;
                    Core::EventBus::instance().publish(ActionNode::ExecutedEvent{"Set", &target, std::time(nullptr)});
                }
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
                
                if (lawSetValue(target, p, result)) {   // coercion matches the slot
                    std::cout << "[Action Fired] " << kindName(k) << " evaluated successfully on " << target.getIdentifier() << std::endl;
                    Core::EventBus::instance().publish(ActionNode::ExecutedEvent{kindName(k), &target, std::time(nullptr)});
                }
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
                if (lawSetValue(target, p, PropertyValue(c.evaluate(x)))) {
                    std::cout << "[Action Fired] Drive evaluated successfully on " << target.getIdentifier() << std::endl;
                    Core::EventBus::instance().publish(ActionNode::ExecutedEvent{"Drive", &target, std::time(nullptr)});
                }
            };
        }
        case Kind::Sequence:
        case Kind::Parallel: {
            std::vector<ECA::ActionExecutor> compiled;
            compiled.reserve(children.size());
            for (const auto& c : children) compiled.push_back(c.compile());
            return [compiled, k=kind](const ECA::Event& e, Singular& target) {
                for (const auto& run : compiled) {
                    if (run) run(e, target);
                }
                std::cout << "[Action Fired] " << kindName(k) << " executed successfully on " << target.getIdentifier() << std::endl;
                Core::EventBus::instance().publish(ActionNode::ExecutedEvent{kindName(k), &target, std::time(nullptr)});
            };
        }
        case Kind::Spawn: {
            const std::string id = conceptId;
            const PropertyPath pPath = spawnParentPath;
            const PropertyPath placementPath = spawnPlacementPath;
            std::vector<ECA::ActionExecutor> compiledChildren;
            compiledChildren.reserve(children.size());
            for (const auto& c : children) compiledChildren.push_back(c.compile());

            return [id, pPath, placementPath, compiledChildren](const ECA::Event& event, Singular& target) {
                World* world = nullptr;
                if (auto* tWorld = dynamic_cast<World*>(&target)) {
                    world = tWorld;
                }
                if (!world) {
                    for (auto* being : Universe::instance().beings()) {
                        if (auto* w = dynamic_cast<World*>(being)) {
                            world = w;
                            break;
                        }
                    }
                }
                auto concept = ConceptRegistry::instance().find(id);
                if (!world || !concept) return;

                std::vector<Object*> sources;
                glm::mat4 placement(1.0f);
                if (auto* subject = dynamic_cast<Object*>(event.subject)) {
                    sources.push_back(subject);
                    bool placementSet = false;
                    if (!placementPath.empty()) {
                        PropertyValue pv;
                        if (lawGetValue(*subject, placementPath, pv)) {
                            if (std::holds_alternative<glm::mat4>(pv)) {
                                placement = std::get<glm::mat4>(pv);
                                placementSet = true;
                            } else if (std::holds_alternative<glm::vec3>(pv)) {
                                placement = glm::translate(glm::mat4(1.0f), std::get<glm::vec3>(pv));
                                placementSet = true;
                            }
                        }
                    }
                    if (!placementSet) {
                        placement = glm::translate(glm::mat4(1.0f), subject->getPosition());
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
                    for (const auto& run : compiledChildren) {
                        if (run) run(event, *newborn);
                    }
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
                }
                if (!newborns.empty()) {
                    std::cout << "[Action Fired] Spawn executed successfully on " << target.getIdentifier() << std::endl;
                    Core::EventBus::instance().publish(ActionNode::ExecutedEvent{"Spawn", &target, std::time(nullptr)});
                }
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
                if (type.empty()) return;
                Singular* eventSubject =
                    subjectToken.empty() ? &lawSubject : resolveToken(subjectToken);
                if (!eventSubject) return;   // unproven: no testimony
                Singular* eventObject =
                    objectToken.empty() ? nullptr : resolveToken(objectToken);
                Core::EventBus::instance().publish(
                    ECA::Event{type, eventSubject, eventObject, std::time(nullptr)});
                std::cout << "[Action Fired] Publish executed successfully on " << lawSubject.getIdentifier() << std::endl;
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
                if (!vars) return;
                std::map<std::string, PropertyValue> pVars;
                for (const auto& [k, v] : *vars) pVars[k] = PropertyValue(v);
                const auto valProp = f.evaluate(pVars, &subject);
                if (!valProp) return;
                lawSetValue(subject, target, *valProp);
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
                if (!Universe::instance().hasClock()) return;
                auto vars = readMathBindings(subject, binds);
                if (!vars) return;
                std::map<std::string, PropertyValue> pVars;
                for (const auto& [k, v] : *vars) pVars[k] = PropertyValue(v);
                const auto valProp = f.evaluate(pVars, &subject);
                if (!valProp) return;
                
                PropertyValue current;
                if (!lawGetValue(subject, target, current)) return;
                
                double dt = Universe::instance().dt();
                PropertyValue next = current;

                // Read the slot NUMERICALLY, the way Add/Scale/Lerp do. Most of
                // the world's scalar properties (position.y, scale, mass, …) are
                // registered as float, so demanding the `double` alternative here
                // made every Flow law over them a silent no-op. lawSetValue
                // coerces the result back to whatever the slot actually holds.
                double curNum = 0.0, rateNum = 0.0;
                if (propertyValueToNumber(current, curNum) && propertyValueToNumber(*valProp, rateNum)) {
                    next = PropertyValue(curNum + rateNum * dt);
                } else if (std::holds_alternative<glm::vec3>(current) && std::holds_alternative<glm::vec3>(*valProp)) {
                    next = PropertyValue(std::get<glm::vec3>(current) + std::get<glm::vec3>(*valProp) * static_cast<float>(dt));
                } else {
                    return;
                }
                
                lawSetValue(subject, target, next);
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
            std::vector<ECA::ActionExecutor> compiledChildren;
            compiledChildren.reserve(children.size());
            for (const auto& c : children) compiledChildren.push_back(c.compile());

            return [shapeKind, type, parentPath, placementPath, compiledChildren](
                       const ECA::Event& event, Singular& target) {
                World* world = resolveWorld(target);
                if (!world) return;   // nowhere to be born: nothing happens

                auto newborn = std::make_unique<Object>();
                newborn->setShape(static_cast<Object::ShapeKind>(shapeKind), Object::ShapeParams{});
                if (!type.empty()) newborn->setObjectType(type);

                // Where. An authored placement path wins; otherwise the
                // newborn appears where its law's subject stands.
                glm::vec3 position(0.0f);
                if (!placementPath.empty()) {
                    PropertyValue pv;
                    if (lawGetValue(target, placementPath, pv)) {
                        if (std::holds_alternative<glm::vec3>(pv)) {
                            position = std::get<glm::vec3>(pv);
                        } else if (std::holds_alternative<glm::mat4>(pv)) {
                            position = glm::vec3(std::get<glm::mat4>(pv)[3]);
                        }
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
                for (const auto& run : compiledChildren) {
                    if (run) run(event, *born);
                }

                // An authored parent takes it as an element; otherwise the
                // World does. Either way something owns it.
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
                if (name.empty()) return;
                PropertyPath remainder;
                Singular* being = resolveLawRoot(subject, owner, remainder);
                if (!being) return;
                if (being->findProperty(name)) return;   // never shadow a first mover
                being->setDynamicProperty(name, initial);
            };
        }

        case Kind::RemoveProperty: {
            const PropertyPath owner = path;
            const std::string name = propertyName;
            return [owner, name](const ECA::Event&, Singular& subject) {
                if (name.empty()) return;
                PropertyPath remainder;
                Singular* being = resolveLawRoot(subject, owner, remainder);
                if (!being) return;
                // An authored property is erased outright — it was granted by
                // law and law may take it back.
                if (being->removeDynamicProperty(name)) return;
                // A first-mover property is a C++ member: the slot cannot be
                // erased, so it is CLEARED. Honest, and never silent about it.
                if (Property* property = being->findProperty(name)) {
                    const PropertyValue empty = emptyLike(property->value());
                    property->setValue(empty);
                }
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
                Singular* containerBeing = resolveBeingToken(container, subject);
                Singular* elementBeing = resolveBeingToken(element, subject);
                if (!containerBeing || !elementBeing) return;
                auto* asObject = dynamic_cast<Object*>(containerBeing);
                if (!asObject) return;   // only Objects hold elements today
                if (adding) asObject->addElement(elementBeing);
                else asObject->removeElement(elementBeing);
            };
        }

        // ------------------------------------------------------------------
        // Unmaking. The delete tool as law-text: "when this is touched by
        // fire, it is gone." World::removeObject releases every element
        // membership first, so nothing is left pointing at a dead being.
        // ------------------------------------------------------------------
        case Kind::Destroy: {
            const std::string victimToken = elementToken;
            return [victimToken](const ECA::Event&, Singular& subject) {
                Singular* victim = resolveBeingToken(victimToken, subject);
                auto* asObject = dynamic_cast<Object*>(victim);
                if (!asObject) return;
                World* world = resolveWorld(subject);
                if (!world) return;
                world->removeObject(asObject);   // publishes object-destroyed
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
            for (const auto& c : children) {
                if (c.definedFor(subject)) return true;
            }
            return children.empty();
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
