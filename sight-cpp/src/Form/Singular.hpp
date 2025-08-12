#pragma once
#include <string>

// Marker interface representing a "singular being" in the system.
// Any class that wishes to participate in Relation should inherit
// from this interface and provide a stable textual identifier
// via getIdentifier().
class Singular {
public:
    virtual ~Singular() = default;
    virtual std::string getIdentifier() const = 0;
}; 