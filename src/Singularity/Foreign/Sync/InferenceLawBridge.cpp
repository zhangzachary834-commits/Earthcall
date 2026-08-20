#include "InferenceLawBridge.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

// See ForeignChannel.cpp on why buildProperties is not called here.
InferenceLawBridge::InferenceLawBridge(const std::string& bridgeName)
    : Law("inference: " + bridgeName),
      _bridgeName(bridgeName),
      _enabled(true),
      _confidenceThreshold(0.85f),
      _activeClassifier("default_heuristic") {}

void InferenceLawBridge::buildProperties() {
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<InferenceLawBridge, bool>>(
        "enabled", this, &InferenceLawBridge::propEnabled, &InferenceLawBridge::propSetEnabled));
        
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<InferenceLawBridge, float>>(
        "confidence_threshold", this, &InferenceLawBridge::propConfidenceThreshold, &InferenceLawBridge::propSetConfidenceThreshold));
        
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<InferenceLawBridge, std::string>>(
        "active_classifier", this, &InferenceLawBridge::propActiveClassifier, &InferenceLawBridge::propSetActiveClassifier));
}

bool InferenceLawBridge::propEnabled() const {
    return _enabled;
}

void InferenceLawBridge::propSetEnabled(const bool& v) {
    _enabled = v;
}

float InferenceLawBridge::propConfidenceThreshold() const {
    return _confidenceThreshold;
}

void InferenceLawBridge::propSetConfidenceThreshold(const float& v) {
    _confidenceThreshold = v;
}

std::string InferenceLawBridge::propActiveClassifier() const {
    return _activeClassifier;
}

void InferenceLawBridge::propSetActiveClassifier(const std::string& v) {
    _activeClassifier = v;
}

void InferenceLawBridge::syncRegister(LawManager& laws) {
    // Scaffold: Ensures the Inference bridges are registered and managed by the LawManager
}
