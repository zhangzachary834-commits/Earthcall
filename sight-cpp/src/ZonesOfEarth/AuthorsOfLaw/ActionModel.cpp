#include "ActionModel.hpp"

#include "Form/Object/Creation/ObjectConcept.hpp"
#include "Form/Singular/Property/PropertyValueJson.hpp"
#include "ZonesOfEarth/World/World.hpp"

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
            return [p, v](const ECA::Event&, Singular& target) { p.setValue(target, v); };
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
                if (!p.getValue(target, current)) return;
                if (!propertyValueToNumber(current, a) || !propertyValueToNumber(rhs, b)) return;
                double result = a;
                if (k == Kind::Add) result = a + b;
                else if (k == Kind::Scale) result = a * b;
                else result = a + (b - a) * f;   // Lerp
                p.setValue(target, PropertyValue(result));   // coercion matches the slot
            };
        }
        case Kind::Drive: {
            const PropertyPath p = path;
            const PropertyPath in = input;
            const CurveModel c = curve;
            return [p, in, c](const ECA::Event& event, Singular& target) {
                double x = 0.0;
                if (in.empty()) {
                    // No authored input: the event's moment is the domain.
                    // (Frame-time domains arrive with the tick loop, commit 4.)
                    x = static_cast<double>(event.timestamp);
                } else {
                    PropertyValue v;
                    if (!in.getValue(target, v) || !propertyValueToNumber(v, x)) return;
                }
                p.setValue(target, PropertyValue(c.evaluate(x)));
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
    }
    return "action";
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

ActionNode ActionNode::sequence(std::vector<ActionNode> children) {
    ActionNode n;
    n.kind = Kind::Sequence;
    n.children = std::move(children);
    return n;
}
