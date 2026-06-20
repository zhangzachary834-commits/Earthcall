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

    Formation* singular_properties();
    const Formation* singular_properties() const;

protected:
    std::vector<std::unique_ptr<Property>> _propertyRegistry;
    Formation* _property_formation = nullptr;
    virtual void buildProperties() = 0;
}; 
