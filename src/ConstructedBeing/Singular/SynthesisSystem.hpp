#pragma once

#include "ConstructedBeing/Singular/Concept.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "json.hpp"

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <optional>

struct SynthesisMapping {
    PropertyPath source;             
    PropertyPath target;             
    
    // Using a multivariable equation evaluated by OntoMath to map multiple inputs to an output
    bool hasExact = false;
    OntoMath::Piecewise exact;

    nlohmann::json toJson() const;
    static SynthesisMapping fromJson(const nlohmann::json& j);

    // Apply the transformation returning a generic PropertyValue
    std::optional<PropertyValue> apply(double x, const std::map<std::string, double>& numericVars, 
                                       const std::map<std::string, std::string>& stringVars, 
                                       const Singular* guardSubject = nullptr) const;
};

class SynthesisSystem {
public:
    static SynthesisSystem& instance();

    // The core set-to-set synthesis function.
    // Takes a set of input Singulars and a Concept blueprint.
    // Maps properties according to SynthesisMapping rules.
    // Spawns new Singulars based on the Concept's SingularTemplates.
    std::vector<std::shared_ptr<Singular>> synthesize(
        const std::vector<Singular*>& inputs, 
        const Concept& blueprint,
        const std::vector<SynthesisMapping>& mappings,
        const class Law* activeLaw = nullptr,
        const Singular* author = nullptr
    );

private:
    SynthesisSystem() = default;
    
    // Instantiates a specific Singular derived class by its class name (e.g. "Lexeme", "Zone")
    // initialProperties is consulted for values that must be set at birth:
    // some beings register their identity read-only ("identity is not a slot"),
    // so applying it as a property afterwards silently does nothing.
    std::shared_ptr<Singular> instantiateClass(
        const std::string& classType,
        const std::map<std::string, PropertyValue>& initialProperties = {});
};
