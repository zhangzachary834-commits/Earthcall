#pragma once

// Compatibility facade for the storage channel.  The concrete declarations live
// under Serialization/ by ontology; existing callers keep this stable include while
// the migration proceeds one persistence root at a time.
#include "Singularity/Storage/Serialization/Serialization.hpp"
