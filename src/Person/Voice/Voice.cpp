#include "Voice.hpp"
#include <iostream>
#include <algorithm>
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

Voice::Voice(std::string style) : style(std::move(style)) {
    formation.setIdentifier(this->getIdentifier() + ".formation");
}

void Voice::describe() const {
    std::cout << "Voice Style: " << style << "\nParts:\n";
    for (const auto* part : parts) {
        std::cout << " - " << part->getName() << "\n";
    }
}

void Voice::addPart(VoicePart* part) {
    if (!part) return;
    parts.push_back(part);
    // As in Body, we add to the formation to manage relations
    formation.addElement(*part);
}

Voice Voice::createBasicVoice(const std::string& style) {
    Voice v(style);
    // In a real usage, parts would be managed/owned by an entity manager or similar,
    // but here we just instantiate the structural concept.
    return v;
}

VoicePart* Voice::getVoicePart(const std::string& name) const {
    for (auto* part : parts) {
        if (part->getName() == name) {
            return part;
        }
    }
    return nullptr;
}

std::vector<VoicePart*> Voice::getVoicePartsByType(VoicePart::Type type) const {
    std::vector<VoicePart*> result;
    for (auto* part : parts) {
        if (part->getType() == type) {
            result.push_back(part);
        }
    }
    return result;
}

void Voice::removeVoicePart(const std::string& name) {
    auto it = std::remove_if(parts.begin(), parts.end(), [&](const VoicePart* p) {
        return p->getName() == name;
    });
    if (it != parts.end()) {
        parts.erase(it, parts.end());
        // Formation element removal logic would be handled here as well.
    }
}

void Voice::buildProperties() {
    registerProperty(std::make_unique<PropertyRef<Voice, std::string>>(
        "style", this, &Voice::style));
}
