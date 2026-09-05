#pragma once

#include "json.hpp"
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"

// Ourverse is a Singular root.  Its record names the gathering Zone and the
// three Formation identities; it does not inline Zones or Relations.
nlohmann::json ourverseToJson(const Ourverse& ourverse);
