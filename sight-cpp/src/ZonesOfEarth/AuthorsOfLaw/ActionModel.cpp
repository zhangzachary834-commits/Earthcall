#include "ActionModel.hpp"

#include "Form/Object/Creation/ObjectConcept.hpp"
#include "Form/Singular/Property/PropertyValueJson.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/World/World.hpp"

#include <ctime>

#include <glm/gtc/matrix_transform.hpp>
#include <utility>

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
        case Kind::Spawn:
            j["conceptId"] = conceptId;
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
    n.conceptId = j.value("conceptId", std::string());
    n.eventType = j.value("eventType", std::string());
    n.publishSubject = j.value("publishSubject", std::string());
    n.publishObject = j.value("publishObject", std::string());
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
            return [p, v](const ECA::Event&, Singular& target) { lawSetValue(target, p, v); };
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
                double a = 0.0, b = 0.0;
                if (!lawGetValue(target, p, current)) return;
                if (!propertyValueToNumber(current, a) || !propertyValueToNumber(rhs, b)) return;
                double result = a;
                if (k == Kind::Add) result = a + b;
                else if (k == Kind::Scale) result = a * b;
                else result = a + (b - a) * f;   // Lerp
                lawSetValue(target, p, PropertyValue(result));   // coercion matches the slot
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
                lawSetValue(target, p, PropertyValue(c.evaluate(x)));
            };
        }
        case Kind::Sequence:
        case Kind::Parallel: {
            std::vector<ECA::ActionExecutor> compiled;
            compiled.reserve(children.size());
            for (const auto& c : children) compiled.push_back(c.compile());
            return [compiled](const ECA::Event& e, Singular& target) {
                for (const auto& run : compiled) {
                    if (run) run(e, target);
                }
            };
        }
        case Kind::Spawn: {
            // Creation IS a law application (LAW_AND_CREATION_SYSTEM.md §7c):
            // the law's TARGET is the World that receives the newborns — the
            // container is the womb. The event's subject seeds the mappings
            // (a one-member source set) and, when spatial, the placement.
            // Reaching this executor at all means the full applyTo gauntlet
            // passed: unauthored laws cannot create.
            const std::string id = conceptId;
            return [id](const ECA::Event& event, Singular& target) {
                auto* world = dynamic_cast<World*>(&target);
                auto concept = ConceptRegistry::instance().find(id);
                if (!world || !concept) return;

                std::vector<Object*> sources;
                glm::mat4 placement(1.0f);
                if (auto* subject = dynamic_cast<Object*>(event.subject)) {
                    sources.push_back(subject);
                    placement = glm::translate(glm::mat4(1.0f), subject->getPosition());
                }
                auto newborns = concept->instantiate(
                    placement, sources.empty() ? nullptr : &sources);
                for (auto& newborn : newborns) {
                    world->addObject(std::move(newborn));
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
                const auto value = f.evaluate(*vars);
                if (!value) return;
                lawSetValue(subject, target, PropertyValue(*value));
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
                const auto rate = f.evaluate(*vars);
                if (!rate) return;
                PropertyValue current;
                double x = 0.0;
                if (!lawGetValue(subject, target, current) ||
                    !propertyValueToNumber(current, x)) return;
                lawSetValue(subject, target,
                            PropertyValue(x + *rate * Universe::instance().dt()));
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
            return vars && mapFunction.evaluate(*vars).has_value();
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
            // Set/Add/Scale/Lerp/Spawn carry no authored bounds — they can
            // always act (an eternal drive unless a bounded function ends it).
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
