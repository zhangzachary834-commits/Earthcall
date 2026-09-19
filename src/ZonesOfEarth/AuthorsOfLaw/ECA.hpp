#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include "Time/Moment/Moment.hpp"

#include <functional>
#include <string>
#include <vector>

/*
 * Event-Condition-Action (ECA) loop
 *
 *   Event     -> something that happened (the trigger)
 *   Condition -> test that must pass before acting
 *   Action    -> effect applied when conditions pass
 *
 * A Loop binds one event type to ordered conditions and actions.
 * Property.hpp describes the same idea at the property level:
 *   Condition = predicate over PropertyPath
 *   Action    = mutation over PropertyPath
 */
namespace ECA {

struct Event {
    std::string type;
    Singular* subject = nullptr;
    Singular* object = nullptr;
    Moment timestamp{};
};

using ConditionPredicate = std::function<bool(const Event&, const Singular&)>;
using ActionExecutor = std::function<void(const Event&, Singular&)>;

struct Condition {
    std::string description;
    ConditionPredicate predicate;
    bool required = true;

    bool evaluate(const Event& event, const Singular& target) const;
};

struct Action {
    std::string description;
    ActionExecutor execute;

    void run(const Event& event, Singular& target) const;
};

enum class ConditionMode { All, Any };

struct Loop {
    std::string name;
    std::string eventType;
    ConditionMode conditionMode = ConditionMode::All;
    std::vector<Condition> conditions;
    std::vector<Action> actions;

    bool accepts(const Event& event) const;
    bool conditionsMet(const Event& event, const Singular& target) const;
    void fire(const Event& event, Singular& target) const;
};

} // namespace ECA
