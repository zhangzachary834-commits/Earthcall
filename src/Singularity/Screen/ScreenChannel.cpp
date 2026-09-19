#include "ScreenChannel.hpp"
#include "Singularity/Screen/ScreenRecorder.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

namespace Singularity {
namespace Screen {

ScreenChannel::ScreenChannel() = default;

void ScreenChannel::syncRegister(LawManager& laws) {
    // Idempotent-by-replacement, including after a test/channel teardown.
    // ActionModel sees only this sink; Object and texture storage stay below
    // the Screen boundary.
    registerPixelWriteSink([](Singular& subject, int face, double u, double v,
                              const glm::vec3& color, std::string& reason) {
        auto* object = dynamic_cast<Object*>(&subject);
        if (!object) {
            reason = "pixel target is not an Object";
            return false;
        }
        if (!object->writeSurfacePixel(face, glm::vec2(u, v), color)) {
            reason = "face or UV is outside the target's paintable surface";
            return false;
        }
        return true;
    });
    registerPixelPropertySink([](Singular& subject, const std::string& propertyName,
                                 int face, const OntoMath::Piecewise& selector,
                                 std::string& reason) {
        auto* object = dynamic_cast<Object*>(&subject);
        if (!object) {
            reason = "pixel-property target is not an Object";
            return false;
        }
        return object->elevateSurfaceRegionProperty(propertyName, face, selector, reason);
    });

    if (!find(laws)) {
        auto channel = std::make_shared<ScreenChannel>();
        laws.add(channel);
        // Rung 4 Migration: Seed Laws for Rendering Optimization
        // These laws displace the old C++ Engine decision of "if exact supported, do analytic".
        // They only operate when renderMode is Auto (0), honoring explicit Person authored modes.
        
        // Law 1: Auto -> Analytic (when supported)
        auto renderOptAnalytic = std::make_shared<FirstMoverLaw>("Renderer Optimization: Analytic Path");
        renderOptAnalytic->setLawIdentifier("seed.renderer.opt.analytic");
        renderOptAnalytic->setScope(Law::Scope::Everyone);
        renderOptAnalytic->setActivation(Law::Activation::WhileTrue);
        
        ConditionNode exactSupported = ConditionNode::compare("screen.rendersImplicitExactly", ConditionNode::Op::Eq, true);
        ConditionNode isAutoMode1 = ConditionNode::compare("renderMode", ConditionNode::Op::Eq, 0); // RenderMode::Auto
        
        ConditionNode allAnalytic;
        allAnalytic.kind = ConditionNode::Kind::All;
        allAnalytic.children.push_back(exactSupported);
        allAnalytic.children.push_back(isAutoMode1);
        renderOptAnalytic->setConditionModel(allAnalytic);
        
        renderOptAnalytic->setActionModel(ActionNode::set("renderMode", 1)); // RenderMode::Analytic
        laws.add(renderOptAnalytic);
        
        // Law 2: Auto -> Mesh (when analytic not supported)
        auto renderOptMesh = std::make_shared<FirstMoverLaw>("Renderer Optimization: Mesh Fallback");
        renderOptMesh->setLawIdentifier("seed.renderer.opt.mesh");
        renderOptMesh->setScope(Law::Scope::Everyone);
        renderOptMesh->setActivation(Law::Activation::WhileTrue);
        
        ConditionNode exactUnsupported = ConditionNode::compare("screen.rendersImplicitExactly", ConditionNode::Op::Eq, false);
        ConditionNode isAutoMode2 = ConditionNode::compare("renderMode", ConditionNode::Op::Eq, 0); // RenderMode::Auto
        
        ConditionNode allMesh;
        allMesh.kind = ConditionNode::Kind::All;
        allMesh.children.push_back(exactUnsupported);
        allMesh.children.push_back(isAutoMode2);
        renderOptMesh->setConditionModel(allMesh);
        
        renderOptMesh->setActionModel(ActionNode::set("renderMode", 2)); // RenderMode::Mesh
        laws.add(renderOptMesh);
    }
}

ScreenChannel* ScreenChannel::find(LawManager& laws) {
    for (const auto& law : laws.getAll()) {
        if (auto* channel = dynamic_cast<ScreenChannel*>(law.get())) {
            return channel;
        }
    }
    return nullptr;
}

void ScreenChannel::updateMetrics(int dCalls, int tris, double vramBytes,
                                 double uBytes, int suballocs, int pipeSwitches,
                                 int cachedMeshes) {
    drawCalls = dCalls;
    trianglesDrawn = tris;
    vramAllocatedBytes = vramBytes;
    uniformBytesWritten = uBytes;
    bufferSuballocations = suballocs;
    pipelineSwitches = pipeSwitches;
    cachedMeshesCount = cachedMeshes;
}

void ScreenChannel::buildProperties() {
    registerEnabledProperty();

    // Derived telemetry: readable, never writable — see the getters' comment
    // in ScreenChannel.hpp. A null setter is ComputedProperty's read-only form
    // (ComputedProperty.hpp), which no_black_box_test already treats as a
    // valid answer rather than a hidden field.
    const auto readOnlyInt = [this](const char* name, int (ScreenChannel::*getter)() const) {
        registerProperty(
            std::make_unique<ComputedProperty<ScreenChannel, int>>(name, this, getter));
    };
    const auto readOnlyDouble = [this](const char* name, double (ScreenChannel::*getter)() const) {
        registerProperty(
            std::make_unique<ComputedProperty<ScreenChannel, double>>(name, this, getter));
    };
    const auto boolean = [this](const char* name, bool ScreenChannel::*member) {
        registerProperty(
            std::make_unique<PropertyRef<ScreenChannel, bool>>(name, this, member));
    };
    const auto vector3 = [this](const char* name, glm::vec3 ScreenChannel::*member) {
        registerProperty(
            std::make_unique<PropertyRef<ScreenChannel, glm::vec3>>(name, this, member));
    };

    readOnlyInt("drawCalls", &ScreenChannel::getDrawCalls);
    readOnlyInt("trianglesDrawn", &ScreenChannel::getTrianglesDrawn);
    readOnlyDouble("vramAllocatedBytes", &ScreenChannel::getVramAllocatedBytes);
    readOnlyDouble("uniformBytesWritten", &ScreenChannel::getUniformBytesWritten);
    readOnlyInt("bufferSuballocations", &ScreenChannel::getBufferSuballocations);
    readOnlyInt("pipelineSwitches", &ScreenChannel::getPipelineSwitches);
    readOnlyInt("cachedMeshesCount", &ScreenChannel::getCachedMeshesCount);
    boolean("wireframe", &ScreenChannel::wireframe);
    boolean("heightGridDdaEnabled", &ScreenChannel::heightGridDdaEnabled);
    vector3("backgroundColor", &ScreenChannel::backgroundColor);

    // Illumination placement is first-order authored state. These names are
    // deliberately under `light.*` instead of inventing a C++ Light kind: the
    // Screen modality is the first-mover bridge to GPU illumination today, and
    // later FieldNodes/OntoMath can drive these same properties through Laws.
    // The picker probes this registry, so these paths become authorable without
    // a second hand-maintained vocabulary.
    boolean("light.cameraRelative", &ScreenChannel::lightCameraRelative);
    vector3("light.position", &ScreenChannel::lightPosition);
    vector3("light.cameraOffset", &ScreenChannel::lightCameraOffset);

    const auto readOnlyBool = [this](const char* name, bool (ScreenChannel::*getter)() const) {
        registerProperty(
            std::make_unique<ComputedProperty<ScreenChannel, bool>>(name, this, getter));
    };
    boolean("recording", &ScreenChannel::recording);
    boolean("screen.recording", &ScreenChannel::recording);
    boolean("snapshot", &ScreenChannel::snapshotTrigger);
    boolean("screen.snapshot", &ScreenChannel::snapshotTrigger);
    readOnlyBool("hasScreenCapturePermission", &ScreenChannel::getHasScreenCapturePermission);
    readOnlyBool("screen.hasScreenCapturePermission", &ScreenChannel::getHasScreenCapturePermission);
    readOnlyBool("hasAccessibilityPermission", &ScreenChannel::getHasAccessibilityPermission);
    readOnlyBool("screen.hasAccessibilityPermission", &ScreenChannel::getHasAccessibilityPermission);
    readOnlyBool("rendersImplicitExactly", &ScreenChannel::getRendersImplicitExactly);
    readOnlyBool("screen.rendersImplicitExactly", &ScreenChannel::getRendersImplicitExactly);
}

bool ScreenChannel::getRendersImplicitExactly() const {
    return currentRenderer().rendersImplicitExactly();
}

bool ScreenChannel::getHasScreenCapturePermission() const {
    return ScreenRecorder::hasScreenCapturePermission();
}

bool ScreenChannel::getHasAccessibilityPermission() const {
    return ScreenRecorder::hasAccessibilityPermission();
}

} // namespace Screen
} // namespace Singularity
