#pragma once
#include <string>
#include <vector>
#include "Relation/Formation/Formation.hpp"

class VoicePart : public Formation {
public:
    enum class Type {
        Undefined,
        Timbre,
        Pitch,
        Resonance,
        Articulation,
        LanguageCapacity
    };

    VoicePart(const std::string& name = "", Type type = Type::Undefined);

    const std::string& getName() const { return partName; }
    Type getType() const { return partType; }
    void setIdentifier(const std::string& id) { _identifier = id; }
    std::string getIdentifier() const override { return _identifier.empty() ? partName : _identifier; }

    // A literal voice part is an actual audible aspect or organic capacity.
    bool isLiteral = true;
    bool isSymbolic = true;

private:
    std::string _identifier;
    std::string partName;
    Type        partType {Type::Undefined};
};
