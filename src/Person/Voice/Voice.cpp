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

VoicePart* Voice::getVoicePart(const std::string& nameOrId) const {
    VoicePart* idMatch = nullptr;
    for (auto* part : parts) {
        if (part && part->getIdentifier() == nameOrId) {
            if (idMatch) return nullptr; // Duplicate IDs cannot select a being.
            idMatch = part;
        }
    }
    if (idMatch) return idMatch;

    VoicePart* nameMatch = nullptr;
    for (auto* part : parts) {
        if (part && part->getName() == nameOrId) {
            if (nameMatch) return nullptr; // Ambiguous display names must not collapse identities.
            nameMatch = part;
        }
    }
    return nameMatch;
}

std::vector<VoicePart*> Voice::getVoicePartsByType(VoicePart::Type type) const {
    std::vector<VoicePart*> result;
    for (auto* part : parts) {
        if (part && part->getType() == type) {
            result.push_back(part);
        }
    }
    return result;
}

void Voice::removeVoicePart(const std::string& nameOrId) {
    VoicePart* target = getVoicePart(nameOrId);
    if (!target) return; // Refuse ambiguous names/IDs rather than remove multiple beings.
    formation.releaseMember(target);
    parts.erase(std::remove(parts.begin(), parts.end(), target), parts.end());
}

void Voice::buildProperties() {
    registerProperty(std::make_unique<PropertyRef<Voice, std::string>>(
        "style", this, &Voice::style));
}
