#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <string>
#include <vector>

namespace Rendering {

// One card in the law graph: a flattened view of a Law's event binding,
// condition tree, and action tree. Pure data so the flatten step (and the
// model-mutation path the editor uses) is testable without a window.
struct LawCard {
    enum class Kind { Law, Event, Condition, Action };
    Kind kind = Kind::Condition;
    std::string label;
    std::vector<int> children;       // indices into the flat card array
    bool isCondition = false;        // which model tree modelPath addresses
    std::vector<int> modelPath;      // child indices from the model root
};

// Card 0 is the law itself; then the event card (when a binding is known),
// the condition tree, and the action tree.
std::vector<LawCard> flattenLaw(const Law& law, const std::string& eventBinding);

// Navigate a model tree by child indices (empty path = root). Null if invalid.
// These are exactly the handles the editor mutates through — mutate a COPY,
// then setConditionModel/setActionModel so the law recompiles.
ConditionNode* conditionAt(ConditionNode& root, const std::vector<int>& path);
ActionNode* actionAt(ActionNode& root, const std::vector<int>& path);

// The Law & Concept authoring window: law list + New Law (authored by
// `player`), card-graph view of the selected law, per-node editors, trigger
// binding into the Rete network, and the concept registry listing.
void renderLawGraphWindow(bool* open, LawManager& laws, Singular& player);

} // namespace Rendering
