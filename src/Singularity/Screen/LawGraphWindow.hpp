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

// One entry in the path picker: a property path the authoring window offers a
// Person, with who owns it and what it holds.
struct PathOption {
    std::string path;
    const char* group;         // owning Singular + facet
    const char* type;          // what the property holds
    bool wholeVector = false;  // vec3 as a whole: numbers won't apply to it
};

// Every property path the substrate advertises to an author. Probed live from
// the registries where it can be, hand-listed where the owning being is not a
// prototype the window holds. Exposed because a hand-listed entry that no
// registry answers is a promise the substrate does not keep — see
// `tests/channel_paths_test.cpp`, which holds this list to its word.
const std::vector<PathOption>& knownPathOptions();

// Navigate a model tree by child indices (empty path = root). Null if invalid.
// These are exactly the handles the editor mutates through — mutate a COPY,
// then setConditionModel/setActionModel so the law recompiles.
ConditionNode* conditionAt(ConditionNode& root, const std::vector<int>& path);
ActionNode* actionAt(ActionNode& root, const std::vector<int>& path);

// The Law & Concept authoring window: law list + New Law (authored by
// `player`), card-graph view of the selected law, per-node editors, trigger
// binding into the Rete network, and the concept registry listing.
// `testSubject` (usually the selected 3D object) enables the "apply now"
// feedback loop — authoring without feedback is guessing.
void renderLawGraphWindow(bool* open, LawManager& laws, Singular& player,
                          Singular* testSubject = nullptr);

} // namespace Rendering
