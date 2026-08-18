#include "CreationChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace Singularity {
namespace Core {

// buildProperties is NOT called here — see the standing note in
// ForeignChannel.cpp. Singular builds the registry lazily behind
// _propertiesBuilt, which a constructor call does not set, so calling it here
// registered every one of this channel's ~20 properties a SECOND time on first
// access: the law-authoring picker offered each path twice, and every
// listProperties() walk did double the work. PhysicsLawBridge, PhysicalChannel
// and ForeignChannel all leave it to the lazy path; this one did not, and
// no_black_box_test now holds all of them to it.
CreationChannel::CreationChannel() = default;

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
    registerEnabledProperty();
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

std::shared_ptr<Law> createShapeGenerator3DLaw(Singular& author) {
    auto law = std::make_shared<FirstMoverLaw>("Tool: Shape Generator 3D");
    law->setLawIdentifier("shape-generator-3d-law");   // NOT setObjectID -- see Law.hpp
    law->setActivation(Law::Activation::OnEvent);
    law->ecaLoop().eventType = "onMouseClicked";
    law->addAuthor(author);

    // THE MODE GATE. The click that reaches this law is published globally
    // from the GLFW mouse callback (EngineInit::registerCallbacks) for every
    // left press outside ImGui, so the law's own condition is the only thing
    // standing between "the Person clicked" and "a cube is born". It used to
    // read `type == "onMouseClicked"` -- a path the CreationChannel does not
    // carry, so it was never satisfiable, and had it been, it would have been
    // trivially true and spawned on every click in every mode.
    //
    // The channel's active3DMode is the tool selection itself, which is what
    // the pre-law tool actually branched on and what the authored twin in
    // saves/tests/shape_generator_3d_law.json says.
    law->setConditionModel(ConditionNode::compare(
        "active3DMode", ConditionNode::Op::Eq, PropertyValue(std::string("Create"))));

    ActionNode spawn = ActionNode::spawn("concept-shape-3d");
    spawn.spawnPlacementPath = PropertyPath::parse("cursorSpawnTransform");
    spawn.spawnColorPath     = PropertyPath::parse("activeColor");
    spawn.spawnShapeKindPath = PropertyPath::parse("activeShapeKind");
    law->setActionModel(spawn);

    // ConceptRegistry::add is first-wins and silent, so this is a no-op when a
    // world save has already brought its own concept-shape-3d.
    if (!ConceptRegistry::instance().find("concept-shape-3d")) {
        auto concept = std::make_shared<ObjectConcept>("Shape Generator 3D Cube");
        concept->setConceptId("concept-shape-3d");
        ObjectConcept::MemberTemplate tmpl;
        tmpl.beingKind = ConditionNode::BeingKind::Object;
        tmpl.kind = Object::ShapeKind::Cube;
        concept->members().push_back(tmpl);
        ConceptRegistry::instance().add(concept);
    }

    return law;
}

} // namespace Core
} // namespace Singularity
