#include "ZonesOfEarth/ZoneManager.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/CategoryManager.hpp"

// Global ZoneManager used by various modules (e.g., Person.cpp).
ZoneManager mgr;

// Global MaterialManager. Materials are cross-zone shared beings (a "clay" used
// in any zone is the same being), so they live once, globally, like `mgr` — not
// per-zone. Objects reference a material by identifier; the renderer resolves it
// here. Always holds an undeletable material.default.
MaterialManager materials;

CategoryManager categories;
