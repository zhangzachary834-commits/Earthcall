#include "ECA.hpp"

namespace ECA {

bool Condition::evaluate(const Event& event, const Singular& target) const {
    if (!predicate) return true;
    return predicate(event, target);
}

void Action::run(const Event& event, Singular& target) const {
    if (execute) execute(event, target);
}

bool Loop::accepts(const Event& event) const {
    return eventType.empty() || event.type == eventType;
}

bool Loop::conditionsMet(const Event& event, const Singular& target) const {
    if (conditions.empty()) return true;

    bool anySatisfied = false;
    for (const auto& condition : conditions) {
        if (!condition.predicate) {
            if (condition.required && conditionMode == ConditionMode::All) return false;
            continue;
        }

        const bool passed = condition.evaluate(event, target);
        anySatisfied = anySatisfied || passed;

        if (conditionMode == ConditionMode::All && condition.required && !passed) return false;
        if (conditionMode == ConditionMode::Any && passed) return true;
    }

    return conditionMode == ConditionMode::All ? true : anySatisfied;
}

void Loop::fire(const Event& event, Singular& target) const {
    if (!accepts(event) || !conditionsMet(event, target)) return;
    for (const auto& action : actions) {
        action.run(event, target);
    }
}

} // namespace ECA
