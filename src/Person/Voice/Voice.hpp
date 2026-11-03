#pragma once
#include <string>
#include <vector>
#include "ConstructedBeing/Singular/Singular.hpp"
#include "Relation/Formation/Formation.hpp"
#include "VoicePart/VoicePart.hpp"

class Voice : public Singular {
public:
    Voice(std::string style = "natural");
    Voice(Voice&&) = default;
    Voice& operator=(Voice&&) = default;
    Voice(const Voice&) = default;
    Voice& operator=(const Voice&) = default;
    
    std::string style;
    
    // Collection of voice parts (owned elsewhere)
    std::vector<VoicePart*> parts;
    Formation               formation;  // group managing voice parts

    void describe() const;

    // Add a voice part to this voice (and formation)
    void addPart(VoicePart* part);

    // Factory: build a simple voice composed of basic parts
    static Voice createBasicVoice(const std::string& style = "natural");
    
    // Voice part management
    VoicePart* getVoicePart(const std::string& name) const;
    std::vector<VoicePart*> getVoicePartsByType(VoicePart::Type type) const;
    void removeVoicePart(const std::string& name);

    // Singular interface
    std::string getIdentifier() const override { return style + "_voice"; }

protected:
    void buildProperties() override;
};
