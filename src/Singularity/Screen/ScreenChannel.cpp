#include "ScreenChannel.hpp"
#include "Singularity/Screen/ScreenRecorder.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "Singularity/OntoMath/Field.hpp"

#include <utility>
#include <cmath>
#include <cstdio>

namespace Singularity {
namespace Screen {

ScreenChannel::ScreenChannel() = default;

bool ScreenChannel::manifestOutput(Renderer& renderer, uint32_t width, uint32_t height) {
    std::string refusal;
    bool drawn = false;
    // Binding values remain on their authored Singulars. Resolve afresh rather
    // than retaining raw being pointers or stale field state across Zone loads.
    auto read = [&](const char* pathName, const char* localName, PropertyValue& value) {
        PropertyValue binding;
        if (!getDynamicProperty(pathName, binding)) return getDynamicProperty(localName, value);
        const auto* path = std::get_if<std::string>(&binding);
        if (!path || path->empty() || path->front() != '@') {
            refusal = std::string(pathName) + " must name a qualified PropertyPath";
            return false;
        }
        if (!lawGetValue(*this, PropertyPath::parse(*path), value)) {
            refusal = std::string(pathName) + " does not resolve: " + *path;
            return false;
        }
        return true;
    };
    PropertyValue colorValue, opacityValue, timeValue;
    if (isEnabled() && read("output.colorPath", "output.color", colorValue)) {
        const auto* color = std::get_if<std::shared_ptr<OntoMath::VectorField>>(&colorValue);
        std::shared_ptr<OntoMath::ScalarField> opacity;
        double time = 0.0;
        bool hasTime = false;
        if (!color || !*color || (*color)->mode != OntoMath::VectorField::EvaluationMode::AST) {
            refusal = "direct Screen color requires a typed AST VectorField";
        }
        if (read("output.opacityPath", "output.opacity", opacityValue)) {
            const auto* field = std::get_if<std::shared_ptr<OntoMath::ScalarField>>(&opacityValue);
            if (!field || !*field || (*field)->mode != OntoMath::ScalarField::EvaluationMode::AST)
                refusal = "direct Screen opacity requires a typed AST ScalarField";
            else opacity = *field;
        }
        if (read("output.timePath", "output.time", timeValue)) {
            hasTime = propertyValueToNumber(timeValue, time) && std::isfinite(time);
            if (!hasTime) refusal = "direct Screen time requires a finite numeric coordinate";
        }
        if (refusal.empty())
            drawn = renderer.drawScreenForm((*color)->astDefinition,
                opacity ? &opacity->astDefinition : nullptr, width, height,
                hasTime ? &time : nullptr, refusal);
    }
    const auto observe = [&](auto& slot, const auto& value, const char* name) {
        if (slot != value) { slot = value; Singular::notifyPropertyChanged(this, name); }
    };
    observe(_outputWidth, static_cast<int>(width), "output.width");
    observe(_outputHeight, static_cast<int>(height), "output.height");
    observe(_outputDrawn, drawn, "output.drawn");
    if (refusal != _outputLastRefusal && !refusal.empty())
        std::fprintf(stderr, "[screen] REFUSED direct output: %s\n", refusal.c_str());
    observe(_outputLastRefusal, refusal, "output.lastRefusal");
    return drawn;
}

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
                                 int cachedMeshes, int sdfCompiles,
                                 int sdfCacheHits, int sdfCacheMisses,
                                 int sdfRefusals, std::string sdfLastRefusal,
                                 double sdfWgslBytes, double sdfParamBytes,
                                 int rangeBuilds, int rangeProxyDraws,
                                 int rangeProxyCulledDraws,
                                 int rangeTraversalDraws,
                                 double rangeNodeBytesUploaded) {
    drawCalls = dCalls;
    trianglesDrawn = tris;
    vramAllocatedBytes = vramBytes;
    uniformBytesWritten = uBytes;
    bufferSuballocations = suballocs;
    pipelineSwitches = pipeSwitches;
    cachedMeshesCount = cachedMeshes;
    sdfProgramCompiles = sdfCompiles;
    sdfProgramCacheHits = sdfCacheHits;
    sdfProgramCacheMisses = sdfCacheMisses;
    sdfProgramRefusals = sdfRefusals;
    sdfLastProgramRefusal = std::move(sdfLastRefusal);
    sdfWgslBytesGenerated = sdfWgslBytes;
    sdfParameterBytesUploaded = sdfParamBytes;
    sdfRangeHierarchyBuilds = rangeBuilds;
    sdfRangeProxyDraws = rangeProxyDraws;
    sdfRangeProxyCulledDraws = rangeProxyCulledDraws;
    sdfRangeTraversalDraws = rangeTraversalDraws;
    sdfRangeNodeBytesUploaded = rangeNodeBytesUploaded;
}

void ScreenChannel::buildProperties() {
    registerEnabledProperty();
    registerProperty(std::make_unique<ComputedProperty<ScreenChannel, int>>(
        "output.width", this, &ScreenChannel::getOutputWidth));
    registerProperty(std::make_unique<ComputedProperty<ScreenChannel, int>>(
        "output.height", this, &ScreenChannel::getOutputHeight));
    registerProperty(std::make_unique<ComputedProperty<ScreenChannel, bool>>(
        "output.drawn", this, &ScreenChannel::getOutputDrawn));
    registerProperty(std::make_unique<ComputedProperty<ScreenChannel, std::string>>(
        "output.lastRefusal", this, &ScreenChannel::getOutputLastRefusal));

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
    const auto readOnlyString = [this](const char* name, std::string (ScreenChannel::*getter)() const) {
        registerProperty(
            std::make_unique<ComputedProperty<ScreenChannel, std::string>>(name, this, getter));
    };
    const auto boolean = [this](const char* name, bool ScreenChannel::*member) {
        registerProperty(
            std::make_unique<PropertyRef<ScreenChannel, bool>>(name, this, member));
    };
    const auto vector3 = [this](const char* name, glm::vec3 ScreenChannel::*member) {
        registerProperty(
            std::make_unique<PropertyRef<ScreenChannel, glm::vec3>>(name, this, member));
    };
    const auto floating = [this](const char* name, double ScreenChannel::*member) {
        registerProperty(
            std::make_unique<PropertyRef<ScreenChannel, double>>(name, this, member));
    };

    readOnlyInt("drawCalls", &ScreenChannel::getDrawCalls);
    readOnlyInt("trianglesDrawn", &ScreenChannel::getTrianglesDrawn);
    readOnlyDouble("vramAllocatedBytes", &ScreenChannel::getVramAllocatedBytes);
    readOnlyDouble("uniformBytesWritten", &ScreenChannel::getUniformBytesWritten);
    readOnlyInt("bufferSuballocations", &ScreenChannel::getBufferSuballocations);
    readOnlyInt("pipelineSwitches", &ScreenChannel::getPipelineSwitches);
    readOnlyInt("cachedMeshesCount", &ScreenChannel::getCachedMeshesCount);
    readOnlyInt("sdfProgramCompiles", &ScreenChannel::getSdfProgramCompiles);
    readOnlyInt("sdfProgramCacheHits", &ScreenChannel::getSdfProgramCacheHits);
    readOnlyInt("sdfProgramCacheMisses", &ScreenChannel::getSdfProgramCacheMisses);
    readOnlyInt("sdfProgramRefusals", &ScreenChannel::getSdfProgramRefusals);
    readOnlyString("sdfLastProgramRefusal", &ScreenChannel::getSdfLastProgramRefusal);
    readOnlyDouble("sdfWgslBytesGenerated", &ScreenChannel::getSdfWgslBytesGenerated);
    readOnlyDouble("sdfParameterBytesUploaded", &ScreenChannel::getSdfParameterBytesUploaded);
    readOnlyInt("sdfRangeHierarchyBuilds", &ScreenChannel::getSdfRangeHierarchyBuilds);
    readOnlyInt("sdfRangeProxyDraws", &ScreenChannel::getSdfRangeProxyDraws);
    readOnlyInt("sdfRangeProxyCulledDraws", &ScreenChannel::getSdfRangeProxyCulledDraws);
    readOnlyInt("sdfRangeTraversalDraws", &ScreenChannel::getSdfRangeTraversalDraws);
    readOnlyDouble("sdfRangeNodeBytesUploaded", &ScreenChannel::getSdfRangeNodeBytesUploaded);
    boolean("wireframe", &ScreenChannel::wireframe);
    boolean("heightGridDdaEnabled", &ScreenChannel::heightGridDdaEnabled);
    floating("spaceDistortion", &ScreenChannel::spaceDistortion);
    boolean("sdfRangeProxyEnabled", &ScreenChannel::sdfRangeProxyEnabled);
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
    const auto intRef = [this](const char* name, int ScreenChannel::*member) {
        registerProperty(std::make_unique<PropertyRef<ScreenChannel, int>>(name, this, member));
    };
    const auto doubleRef = [this](const char* name, double ScreenChannel::*member) {
        registerProperty(std::make_unique<PropertyRef<ScreenChannel, double>>(name, this, member));
    };

    intRef("fieldMeshMinRes", &ScreenChannel::fieldMeshMinRes);
    intRef("screen.fieldMeshMinRes", &ScreenChannel::fieldMeshMinRes);
    intRef("fieldMeshMaxRes", &ScreenChannel::fieldMeshMaxRes);
    intRef("screen.fieldMeshMaxRes", &ScreenChannel::fieldMeshMaxRes);
    doubleRef("fieldMeshMaxCells", &ScreenChannel::fieldMeshMaxCells);
    doubleRef("screen.fieldMeshMaxCells", &ScreenChannel::fieldMeshMaxCells);

    registerProperty(std::make_unique<ComputedProperty<ScreenChannel, bool>>(
        "recording", this, &ScreenChannel::getRecording, &ScreenChannel::setRecording));
    registerProperty(std::make_unique<ComputedProperty<ScreenChannel, bool>>(
        "screen.recording", this, &ScreenChannel::getRecording, &ScreenChannel::setRecording));
    registerProperty(std::make_unique<ComputedProperty<ScreenChannel, bool>>(
        "snapshot", this, &ScreenChannel::getSnapshotTrigger, &ScreenChannel::setSnapshotTrigger));
    registerProperty(std::make_unique<ComputedProperty<ScreenChannel, bool>>(
        "screen.snapshot", this, &ScreenChannel::getSnapshotTrigger, &ScreenChannel::setSnapshotTrigger));
    readOnlyBool("hasScreenCapturePermission", &ScreenChannel::getHasScreenCapturePermission);
    readOnlyBool("screen.hasScreenCapturePermission", &ScreenChannel::getHasScreenCapturePermission);
    readOnlyBool("hasAccessibilityPermission", &ScreenChannel::getHasAccessibilityPermission);
    readOnlyBool("screen.hasAccessibilityPermission", &ScreenChannel::getHasAccessibilityPermission);
    readOnlyBool("rendersImplicitExactly", &ScreenChannel::getRendersImplicitExactly);
    readOnlyBool("screen.rendersImplicitExactly", &ScreenChannel::getRendersImplicitExactly);
}

bool ScreenChannel::getRendersImplicitExactly() const {
    PropertyValue v;
    if (getDynamicProperty("rendersImplicitExactly", v)) {
        if (const bool* b = std::get_if<bool>(&v)) return *b;
    }
    return currentRenderer().rendersImplicitExactly();
}

bool ScreenChannel::getHasScreenCapturePermission() const {
    return ScreenRecorder::hasScreenCapturePermission();
}

bool ScreenChannel::getHasAccessibilityPermission() const {
    return ScreenRecorder::hasAccessibilityPermission();
}

bool ScreenChannel::getRecording() const {
    if (auto* rec = ScreenRecorder::activeInstance()) {
        return rec->isRecording();
    }
    return recording;
}

void ScreenChannel::setRecording(const bool& v) {
    recording = v;
    if (auto* rec = ScreenRecorder::activeInstance()) {
        if (v) rec->startRecording();
        else rec->stopRecording();
    }
}

bool ScreenChannel::getSnapshotTrigger() const {
    if (auto* rec = ScreenRecorder::activeInstance()) {
        return rec->isSnapshotPending();
    }
    return snapshotTrigger;
}

void ScreenChannel::setSnapshotTrigger(const bool& v) {
    snapshotTrigger = v;
    if (v) {
        if (auto* rec = ScreenRecorder::activeInstance()) {
            rec->captureSnapshot();
        }
    }
}

} // namespace Screen
} // namespace Singularity
