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
        if (part && part->getIdentifier() == name) return part;
    }
    for (auto* part : parts) {
        if (part && part->getName() == name) return part;
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
    auto byId = std::find_if(parts.begin(), parts.end(),
        [&name](const VoicePart* part) {
            return part && part->getIdentifier() == name;
        });
    if (byId != parts.end()) {
        formation.releaseMember(*byId);
        parts.erase(byId);
        return;
    }
    for (auto it = parts.begin(); it != parts.end();) {
        if (*it && (*it)->getName() == name) {
            formation.releaseMember(*it);
            it = parts.erase(it);
        } else {
            ++it;
        }
    }
}

void Voice::buildProperties() {
    registerProperty(std::make_unique<PropertyRef<Voice, std::string>>(
        "style", this, &Voice::style));
}
