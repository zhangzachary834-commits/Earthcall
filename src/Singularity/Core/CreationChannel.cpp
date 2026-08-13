#include "CreationChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace Singularity {
namespace Core {

CreationChannel::CreationChannel() {
    buildProperties();
}

void CreationChannel::syncRegister(LawManager& laws) {
    bool bridged = false;
    for (const auto& law : laws.getAll()) {
        auto* bridge = dynamic_cast<CreationChannel*>(law.get());
        if (bridge && bridge->name() == "creation-channel") {
            bridged = true;
            break;
        }
    }
    if (!bridged) {
        laws.add(std::make_shared<CreationChannel>());
    }
}

void CreationChannel::buildProperties() {
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, std::string>>(
        "activeTool", this, &CreationChannel::activeTool));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, std::string>>(
        "active3DMode", this, &CreationChannel::active3DMode));
    // The selected shape kind is law-readable like the rest of the selection:
    // ActionNode::spawn's spawnShapeKindPath points at it, and the authoring
    // window already offers "activeShapeKind" as a Creation-channel path. The
    // field was here but never registered, so every such law silently kept the
    // concept's template kind instead of the author's live choice.
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, int>>(
        "activeShapeKind", this, &CreationChannel::activeShapeKind));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "cursorHitPos", this, &CreationChannel::cursorHitPos));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "cursorHitNormal", this, &CreationChannel::cursorHitNormal));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "cursorSpawnPos", this, &CreationChannel::cursorSpawnPos));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "cursorSpawnRot", this, &CreationChannel::cursorSpawnRot));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "cursorSpawnScale", this, &CreationChannel::cursorSpawnScale));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<CreationChannel, glm::mat4>>(
        "cursorSpawnTransform", this, &CreationChannel::getCursorSpawnTransform, nullptr));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, std::string>>(
        "placementMode", this, &CreationChannel::placementMode));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, bool>>(
        "gridSnap", this, &CreationChannel::gridSnap));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, float>>(
        "gridSnapSize", this, &CreationChannel::gridSnapSize));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, float>>(
        "inFrontDistance", this, &CreationChannel::inFrontDistance));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "manualOffset", this, &CreationChannel::manualOffset));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, bool>>(
        "manualAnchorValid", this, &CreationChannel::manualAnchorValid));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "manualAnchorPos", this, &CreationChannel::manualAnchorPos));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "manualAnchorRight", this, &CreationChannel::manualAnchorRight));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "manualAnchorUp", this, &CreationChannel::manualAnchorUp));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "manualAnchorForward", this, &CreationChannel::manualAnchorForward));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, std::string>>(
        "cursorHoveredBodyPart", this, &CreationChannel::cursorHoveredBodyPart));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "activeColor", this, &CreationChannel::activeColor));
}

float CreationChannel::spawnSurfaceOffset(const glm::vec3& normal) const {
    const glm::vec3 n = glm::length(normal) > 1e-6f ? glm::normalize(normal)
                                                    : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 rotation(1.0f);
    rotation = glm::rotate(rotation, glm::radians(cursorSpawnRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(cursorSpawnRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(cursorSpawnRot.z), glm::vec3(0.0f, 0.0f, 1.0f));

    const glm::vec3 half = cursorSpawnScale * 0.5f;
    const glm::vec3 axisX = glm::normalize(glm::vec3(rotation * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
    const glm::vec3 axisY = glm::normalize(glm::vec3(rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
    const glm::vec3 axisZ = glm::normalize(glm::vec3(rotation * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));

    return std::abs(glm::dot(n, axisX)) * half.x +
           std::abs(glm::dot(n, axisY)) * half.y +
           std::abs(glm::dot(n, axisZ)) * half.z;
}

glm::vec3 CreationChannel::computeSpawnPosition(const glm::vec3& cameraPos, const glm::vec3& cameraForward) const {
    glm::vec3 spawnPos;

    if (placementMode == "ManualDistance") {
        spawnPos = manualAnchorPos +
                   manualAnchorRight * manualOffset.x +
                   manualAnchorUp * manualOffset.y +
                   manualAnchorForward * manualOffset.z;
    } else if (placementMode == "CursorSnap") {
        spawnPos = cursorHitPos + cursorHitNormal * spawnSurfaceOffset(cursorHitNormal);
    } else {
        spawnPos = cameraPos + cameraForward * inFrontDistance;
    }

    if (gridSnap && gridSnapSize > 1e-6f) {
        spawnPos.x = std::round(spawnPos.x / gridSnapSize) * gridSnapSize;
        spawnPos.y = std::round(spawnPos.y / gridSnapSize) * gridSnapSize;
        spawnPos.z = std::round(spawnPos.z / gridSnapSize) * gridSnapSize;
    }
    return spawnPos;
}

void CreationChannel::updatePlacement(const glm::vec3& cameraPos, const glm::vec3& cameraForward) {
    cursorSpawnPos = computeSpawnPosition(cameraPos, cameraForward);
}

glm::mat4 CreationChannel::getCursorSpawnTransform() const {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), cursorSpawnPos);
    t = glm::rotate(t, glm::radians(cursorSpawnRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
    t = glm::rotate(t, glm::radians(cursorSpawnRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
    t = glm::rotate(t, glm::radians(cursorSpawnRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
    t = glm::scale(t, cursorSpawnScale);
    return t;
}

} // namespace Core
} // namespace Singularity
