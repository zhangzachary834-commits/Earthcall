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
}

bool ScreenChannel::getHasScreenCapturePermission() const {
    return ScreenRecorder::hasScreenCapturePermission();
}

bool ScreenChannel::getHasAccessibilityPermission() const {
    return ScreenRecorder::hasAccessibilityPermission();
}

} // namespace Screen
} // namespace Singularity
