#pragma once

#include <vector>
#include <memory>
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

namespace Physics {

// Creates and returns the default set of physics laws (Gravity, Air Resistance, etc.)
// modeled purely through ActionNode::flow and ActionNode::add over properties.
std::vector<std::shared_ptr<Law>> createDefaultPhysicsLaws();

} // namespace Physics
