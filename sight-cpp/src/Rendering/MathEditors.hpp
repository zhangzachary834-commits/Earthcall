#pragma once

#include "Singularity/OntoMath/ScalarForm.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"

#include <functional>

// Shared authoring editors for OntoMath — the SAME full editor (variables,
// pieces with open/closed bounds, terms, exponents, transcendental factors)
// wherever a Person authors exact mathematics: the Law Author's Zone/Map/Flow
// nodes and the Creation Console's mapping transforms.
namespace Rendering {
namespace MathEd {

// How the caller offers its property vocabulary for binding variables —
// each window supplies its own picker (the Law Author's subject-grouped
// combo; the Creation Console's source-set union).
using PathPickerFn = std::function<bool(const char* label, PropertyPath& path)>;

bool editMathBindings(MathBindings& bindings, const PathPickerFn& pathPicker);
bool editExpression(OntoMath::ScalarForm& e, const MathBindings& bindings);
bool editPiecewise(OntoMath::Piecewise& f, const MathBindings& bindings);

// The registry of NAMED functions — define once, call anywhere (recursion
// included, depth-bounded). Hosted by the Law Author window.
void editFunctionRegistry();

} // namespace MathEd
} // namespace Rendering
