// Object — transform / rotation / automation subsystem (split from Object.cpp).
// setTransform, authoritative-axis & euler rotation state, rotation advance, automation clips.

#include "Object.hpp"
#include "Contour.hpp"
#include "AngleTools.hpp"
#include "Automation/AutomationEvents.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include <GLFW/glfw3.h>
#include <OpenGL/glu.h>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cstring>
#include <cstdlib> // for rand()
#include <cmath>   // for mathematical functions
#include <limits>  // for numeric_limits
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include "Rendering/HighlightSystem.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
glm::vec3 extractScaleFromTransform(const glm::mat4& transform) {
    glm::vec3 scale(glm::length(glm::vec3(transform[0])),
                    glm::length(glm::vec3(transform[1])),
                    glm::length(glm::vec3(transform[2])));
    if (scale.x <= 1e-6f) scale.x = 1.0f;
    if (scale.y <= 1e-6f) scale.y = 1.0f;
    if (scale.z <= 1e-6f) scale.z = 1.0f;
    return scale;
}

glm::vec3 extractRotationDegreesFromTransform(const glm::mat4& transform) {
    glm::vec3 scale = extractScaleFromTransform(transform);
    glm::mat3 rotationBasis;
    rotationBasis[0] = glm::vec3(transform[0]) / scale.x;
    rotationBasis[1] = glm::vec3(transform[1]) / scale.y;
    rotationBasis[2] = glm::vec3(transform[2]) / scale.z;

    if (glm::determinant(rotationBasis) < 0.0f) {
        rotationBasis[0] = -rotationBasis[0];
    }

    glm::quat rotation = glm::normalize(glm::quat_cast(rotationBasis));
    return glm::degrees(glm::eulerAngles(rotation));
}

float wrapDegrees(float degrees) {
    float wrapped = std::fmod(degrees, 360.0f);
    if (wrapped > 180.0f) wrapped -= 360.0f;
    if (wrapped < -180.0f) wrapped += 360.0f;
    return wrapped;
}

float shortestAngleDelta(float current, float target) {
    return wrapDegrees(target - current);
}
} // namespace

void Object::setTransform(const glm::mat4& t) {
    transform = t;
    syncRotationStateFromTransform(transform, !preserveRotationTargetOnTransformSet);
    updateCollisionZone(transform);
}

void Object::setAuthoritativeAxis(const glm::vec3& axis) {
    if (glm::dot(axis, axis) <= 1e-12f) {
        authoritativeAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        return;
    }
    authoritativeAxis = glm::normalize(axis);
}

void Object::setRotationEulerDegrees(const glm::vec3& degrees) {
    glm::vec3 wrapped(wrapDegrees(degrees.x), wrapDegrees(degrees.y), wrapDegrees(degrees.z));
    rotationEulerDegrees = wrapped;
    targetRotationEulerDegrees = wrapped;

    preserveRotationTargetOnTransformSet = true;
    setTransform(composeTransformWithRotation(transform, rotationEulerDegrees));
    preserveRotationTargetOnTransformSet = false;
}

void Object::setTargetRotationEulerDegrees(const glm::vec3& degrees) {
    targetRotationEulerDegrees = glm::vec3(wrapDegrees(degrees.x),
                                           wrapDegrees(degrees.y),
                                           wrapDegrees(degrees.z));
}

void Object::addTargetRotationDegrees(const glm::vec3& deltaDegrees) {
    setTargetRotationEulerDegrees(targetRotationEulerDegrees + deltaDegrees);
}

void Object::setRotationResponsiveness(float responsiveness) {
    rotationResponsiveness = std::max(0.1f, responsiveness);
}

bool Object::hasPendingRotation() const {
    return std::abs(shortestAngleDelta(rotationEulerDegrees.x, targetRotationEulerDegrees.x)) > 0.01f ||
           std::abs(shortestAngleDelta(rotationEulerDegrees.y, targetRotationEulerDegrees.y)) > 0.01f ||
           std::abs(shortestAngleDelta(rotationEulerDegrees.z, targetRotationEulerDegrees.z)) > 0.01f;
}

void Object::syncRotationStateFromTransform(const glm::mat4& sourceTransform, bool syncTarget) {
    rotationEulerDegrees = extractRotationDegreesFromTransform(sourceTransform);
    rotationEulerDegrees.x = wrapDegrees(rotationEulerDegrees.x);
    rotationEulerDegrees.y = wrapDegrees(rotationEulerDegrees.y);
    rotationEulerDegrees.z = wrapDegrees(rotationEulerDegrees.z);
    if (syncTarget) {
        targetRotationEulerDegrees = rotationEulerDegrees;
    }
}

glm::mat4 Object::composeTransformWithRotation(const glm::mat4& sourceTransform,
                                               const glm::vec3& rotationDegrees) const {
    glm::vec3 translation = glm::vec3(sourceTransform[3]);
    glm::vec3 scale = extractScaleFromTransform(sourceTransform);

    glm::mat4 rebuilt = glm::translate(glm::mat4(1.0f), translation);
    rebuilt = glm::rotate(rebuilt, glm::radians(rotationDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rebuilt = glm::rotate(rebuilt, glm::radians(rotationDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rebuilt = glm::rotate(rebuilt, glm::radians(rotationDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
    rebuilt = glm::scale(rebuilt, scale);
    return rebuilt;
}

bool Object::advanceRotation(const glm::mat4& sourceTransform, float dt, glm::mat4& outTransform) {
    syncRotationStateFromTransform(sourceTransform, false);

    glm::vec3 next = rotationEulerDegrees;
    float blend = 1.0f - std::exp(-std::max(0.1f, rotationResponsiveness) * std::max(0.0f, dt));
    bool changed = false;

    for (int axis = 0; axis < 3; ++axis) {
        float delta = shortestAngleDelta(next[axis], targetRotationEulerDegrees[axis]);
        if (std::abs(delta) <= 0.01f) {
            next[axis] = targetRotationEulerDegrees[axis];
            continue;
        }
        next[axis] = wrapDegrees(next[axis] + delta * blend);
        changed = true;
    }

    rotationEulerDegrees = next;
    outTransform = composeTransformWithRotation(sourceTransform, rotationEulerDegrees);
    return changed;
}

bool Object::updateRotation(float dt) {
    glm::mat4 nextTransform(1.0f);
    if (!advanceRotation(transform, dt, nextTransform)) {
        return false;
    }

    preserveRotationTargetOnTransformSet = true;
    setTransform(nextTransform);
    preserveRotationTargetOnTransformSet = false;
    return true;
}

// ---------------------------------------------------------------------
// Automation
// ---------------------------------------------------------------------
void Object::setAutomationRest(const glm::mat4& rest) {
    _automation.rest = rest;
    _automation.restValid = true;
}

void Object::addAutomation(const Automation::Clip& clip) {
    if (!_automation.restValid) {
        _automation.rest = transform;
        _automation.restValid = true;
    }
    _automation.clips.push_back(clip);
}

void Object::clearAutomations() {
    _automation.clips.clear();
}

void Object::advanceAutomations(float dt) {
    std::vector<std::string> finished;
    Automation::advance(_automation, dt, &finished);
    for (const auto& name : finished) {
        Core::EventBus::instance().publish(Automation::ClipFinished{this, name});
        Core::EventBus::instance().publish(ECA::Event{"automation-clip-finished", this, nullptr, std::time(nullptr)});
    }
}

glm::mat4 Object::sampleAutomations(const glm::mat4& base) const {
    return Automation::compose(_automation, base);
}

bool Object::updateAutomations(float dt) {
    if (!Automation::active(_automation)) return false;
    if (!_automation.restValid) {
        _automation.rest = transform;
        _automation.restValid = true;
    }
    std::vector<std::string> finished;
    Automation::advance(_automation, dt, &finished);
    setTransform(Automation::compose(_automation, transform));
    for (const auto& name : finished) {
        Core::EventBus::instance().publish(Automation::ClipFinished{this, name});
        Core::EventBus::instance().publish(ECA::Event{"automation-clip-finished", this, nullptr, std::time(nullptr)});
    }
    return true;
}
