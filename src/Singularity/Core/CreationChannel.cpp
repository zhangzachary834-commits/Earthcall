#include "CreationChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <string>

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
    if (find(laws)) return;
    laws.add(std::make_shared<CreationChannel>());
}

CreationChannel* CreationChannel::find(LawManager& laws) {
    for (const auto& law : laws.getAll()) {
        if (auto* channel = dynamic_cast<CreationChannel*>(law.get())) {
            return channel;
        }
    }
    return nullptr;
}

void CreationChannel::writeLiveSelection(const std::string& tool,
                                         const std::string& mode,
                                         int shapeKind,
                                         const glm::vec3& spawnRot,
                                         const glm::vec3& spawnScale,
                                         bool gridSnap,
                                         float gridSnapSize,
                                         const glm::vec3& color) {
    activeTool = tool;
    active3DMode = mode;
    activeShapeKind = shapeKind;
    cursorSpawnRot = spawnRot;
    cursorSpawnScale = spawnScale;
    this->gridSnap = gridSnap;
    this->gridSnapSize = gridSnapSize;
    activeColor = color;
}

void CreationChannel::buildProperties() {
    registerEnabledProperty();
    registerProperty(std::make_unique<PropertyRef<CreationChannel, std::string>>(
        "activeTool", this, &CreationChannel::activeTool));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, std::string>>(
        "active3DMode", this, &CreationChannel::active3DMode));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, bool>>(
        "spawnLawArmed", this, &CreationChannel::spawnLawArmed));
    // The selected shape kind is law-readable like the rest of the selection:
    // ActionNode::spawn's spawnShapeKindPath points at it, and the authoring
    // window already offers "activeShapeKind" as a Creation-channel path. The
    // field was here but never registered, so every such law silently kept the
    // concept's template kind instead of the author's live choice.
    registerProperty(std::make_unique<PropertyRef<CreationChannel, int>>(
        "activeShapeKind", this, &CreationChannel::activeShapeKind));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "cursorHitPos", this, &CreationChannel::cursorHitPos));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "cursorHitNormal", this, &CreationChannel::cursorHitNormal));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "cursorSpawnPos", this, &CreationChannel::cursorSpawnPos));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "cursorSpawnRot", this, &CreationChannel::cursorSpawnRot));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "cursorSpawnScale", this, &CreationChannel::cursorSpawnScale));
    registerProperty(std::make_unique<ComputedProperty<CreationChannel, glm::mat4>>(
        "cursorSpawnTransform", this, &CreationChannel::getCursorSpawnTransform, nullptr));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, std::string>>(
        "placementMode", this, &CreationChannel::placementMode));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, bool>>(
        "gridSnap", this, &CreationChannel::gridSnap));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, float>>(
        "gridSnapSize", this, &CreationChannel::gridSnapSize));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, float>>(
        "inFrontDistance", this, &CreationChannel::inFrontDistance));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "manualOffset", this, &CreationChannel::manualOffset));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, bool>>(
        "manualAnchorValid", this, &CreationChannel::manualAnchorValid));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "manualAnchorPos", this, &CreationChannel::manualAnchorPos));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "manualAnchorRight", this, &CreationChannel::manualAnchorRight));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "manualAnchorUp", this, &CreationChannel::manualAnchorUp));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "manualAnchorForward", this, &CreationChannel::manualAnchorForward));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, std::string>>(
        "cursorHoveredBodyPart", this, &CreationChannel::cursorHoveredBodyPart));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, glm::vec3>>(
        "activeColor", this, &CreationChannel::activeColor));
    registerProperty(std::make_unique<PropertyRef<CreationChannel, std::string>>(
        "activeImplicitExpr", this, &CreationChannel::activeImplicitExpr));
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

    law->setConditionModel(ConditionNode::compare(
        "spawnLawArmed", ConditionNode::Op::Eq, PropertyValue(true)));

    ActionNode spawn = ActionNode::spawn("concept-shape-3d");
    spawn.spawnPlacementPath = PropertyPath::parse("cursorSpawnTransform");
    spawn.spawnColorPath     = PropertyPath::parse("activeColor");
    spawn.spawnShapeKindPath = PropertyPath::parse("activeShapeKind");
    law->setActionModel(spawn);

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

namespace {

struct CreatorToolSeed {
    const char* identifier;
    const char* name;
    const char* active3DMode;
};

constexpr CreatorToolSeed kCreatorTools[] = {
    {"tool-create-3d-law",     "Tool: Create 3D",          "Create"},
    {"tool-select-3d-law",     "Tool: Select 3D",          "Select"},
    {"tool-face-brush-law",    "Tool: Face Brush",         "FaceBrush"},
    {"tool-face-paint-law",    "Tool: Face Fill",          "FacePaint"},
    {"tool-pottery-3d-law",    "Tool: Pottery 3D",         "Pottery"},
    {"tool-rotate-3d-law",     "Tool: Rotate 3D",          "Rotate"},
    {"tool-morph-3d-law",      "Tool: Morph",              "Morph"},
    {"tool-combine-3d-law",    "Tool: Combine",            "Combine"},
    {"tool-sculpt-3d-law",     "Tool: Clay",               "Sculpt"},
    {"tool-graph-3d-law",      "Tool: Graph",              "Graph"},
};

struct ManualDistanceKeySeed {
    const char* identifier;
    const char* name;
    int keyCode;
    const char* offsetPath;
    double delta;
};

constexpr ManualDistanceKeySeed kManualDistanceKeys[] = {
    {"tool-manual-offset-right-law",    "Tool: Manual offset right",    GLFW_KEY_RIGHT,     "manualOffset.x",  0.1},
    {"tool-manual-offset-left-law",     "Tool: Manual offset left",     GLFW_KEY_LEFT,      "manualOffset.x", -0.1},
    {"tool-manual-offset-up-law",       "Tool: Manual offset up",       GLFW_KEY_PAGE_UP,   "manualOffset.y",  0.1},
    {"tool-manual-offset-down-law",     "Tool: Manual offset down",     GLFW_KEY_PAGE_DOWN, "manualOffset.y", -0.1},
    {"tool-manual-offset-forward-law",  "Tool: Manual offset forward",  GLFW_KEY_UP,        "manualOffset.z",  0.1},
    {"tool-manual-offset-backward-law", "Tool: Manual offset backward", GLFW_KEY_DOWN,      "manualOffset.z", -0.1},
};

} // namespace

const char* creatorToolLawIdForMode(const std::string& active3DMode) {
    if (active3DMode.empty()) return "";
    if (active3DMode == "Clay") return "tool-sculpt-3d-law";
    for (const auto& seed : kCreatorTools) {
        if (active3DMode == seed.active3DMode) return seed.identifier;
    }
    return "";
}

void syncRegisterCreatorTools(LawManager& laws, Singular& author) {
    if (!laws.find("shape-generator-3d-law")) {
        auto spawn = createShapeGenerator3DLaw(author);
        laws.add(spawn);
        laws.bindTrigger(spawn->getIdentifier(), "onMouseClicked");
    }

    for (const auto& seed : kCreatorTools) {
        if (laws.find(seed.identifier)) continue;
        auto law = std::make_shared<FirstMoverLaw>(seed.name);
        law->setLawIdentifier(seed.identifier);
        law->addAuthor(author);
        law->setConditionModel(ConditionNode::compare(
            "active3DMode", ConditionNode::Op::Eq,
            PropertyValue(std::string(seed.active3DMode))));
        laws.add(law);
    }

    syncRegisterManualDistanceKeyLaws(laws, author);
}

void syncRegisterManualDistanceKeyLaws(LawManager& laws, Singular& author) {
    CreationChannel* channel = CreationChannel::find(laws);
    if (!channel) return;

    // GameUpdate's retired pre-law path nudged by 0.1 once per frame while a
    // key was held. Keep the raw key level on InteractionChannel (Sense) and
    // put the meaning here (Decide): six authored, named, set-down-able Laws.
    for (const auto& seed : kManualDistanceKeys) {
        if (laws.find(seed.identifier)) continue;
        auto law = std::make_shared<FirstMoverLaw>(seed.name);
        law->setLawIdentifier(seed.identifier);
        law->addAuthor(author);
        law->setActivation(Law::Activation::WhileTrue);
        law->addTarget(*channel);
        law->setConditionModel(ConditionNode::all({
            ConditionNode::compare("placementMode", ConditionNode::Op::Eq,
                                   PropertyValue(std::string("ManualDistance"))),
            ConditionNode::compare("active3DMode", ConditionNode::Op::Eq,
                                   PropertyValue(std::string("Create"))),
            ConditionNode::compare("@interaction-channel.keyDown", ConditionNode::Op::Eq,
                                   PropertyValue(true)),
            ConditionNode::compare("@interaction-channel.lastKeyCode", ConditionNode::Op::Eq,
                                   PropertyValue(seed.keyCode))
        }));
        law->setActionModel(ActionNode::add(seed.offsetPath, seed.delta));
        laws.add(law);
    }
}

} // namespace Core
} // namespace Singularity
