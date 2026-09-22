#pragma once

#include <vector>
#include <memory>
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

namespace Physics {

// Engine-seeded physics, as first movers. Gravity, kinematics, and the
// acoustic suite are bootstrap — they belong in the Law Author's First
// Mover block, not in the authored-law list a Person writes into.
std::vector<std::shared_ptr<Law>> createDefaultPhysicsLaws();

} // namespace Physics
