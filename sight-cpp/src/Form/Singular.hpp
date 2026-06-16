#pragma once
#include <string>

class Formation;

// ------------------------------------------------------------------
// A Singular represents any entity or concept with meaning greater than the sum of its parts.
// Marker interface representing a "singular being" in the system.
// Any class that wishes to participate in Relation should inherit
// from this interface and provide a stable textual identifier
// via getIdentifier().
// ------------------------------------------------------------------
class Singular {
public:
    virtual ~Singular() = default;
    virtual std::string getIdentifier() const = 0;

    Formation* properties() { return _properties; }
    const Formation* properties() const { return _properties; }

protected:
    Formation* _properties = nullptr;
    virtual void buildProperties() = 0;
}; 
