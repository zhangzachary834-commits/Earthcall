#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <string>

// InferenceLawBridge
//
// Represents the external Python ML First Mover in Earthcall.
// While the ML logic runs externally, its *drivers* (hyperparameters, active models)
// are exposed here as native Properties. 
// 
// This allows Persons to author Laws that govern the ML model.
// For example: `@inference.calendar_classifier.confidence_threshold := 0.9`
//
class InferenceLawBridge : public Law {
public:
    explicit InferenceLawBridge(const std::string& bridgeName);

    const std::string& bridgeName() const { return _bridgeName; }

    bool isFirstMover() const override { return true; }

    // Stable slug, for the same reason as ForeignChannel::getIdentifier: law
    // text addresses this bridge as `@inference.calendar_classifier....`, and
    // a first mover's generated law id is not the same twice.
    std::string getIdentifier() const override { return "inference." + _bridgeName; }

    static void syncRegister(LawManager& laws);

private:
    void buildProperties() override;

    bool propEnabled() const;
    void propSetEnabled(const bool& v);

    float propConfidenceThreshold() const;
    void propSetConfidenceThreshold(const float& v);

    std::string propActiveClassifier() const;
    void propSetActiveClassifier(const std::string& v);

    std::string _bridgeName;

    bool _enabled;
    float _confidenceThreshold;
    std::string _activeClassifier;
};
