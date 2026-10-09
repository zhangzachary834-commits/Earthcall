#include "ScreenChannel.hpp"
#include "Singularity/Screen/ScreenRecorder.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"

#include <utility>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace Singularity {
namespace Screen {

ScreenChannel::ScreenChannel() = default;

std::shared_ptr<PropertyDict> ScreenChannel::getSampleResult() const {
    // Sensor truth must not leak mutable canonical storage through ValueLeaf
    // or Map. Authored containers may share; a carried observation is a copy.
    // The existing typed codec also separates nested fields and sample lists.
    return std::get<std::shared_ptr<PropertyDict>>(propertyValueFromJson(propertyValueToJson(_sampleResult)));
}

void ScreenChannel::senseOutput(Renderer& renderer, uint32_t width, uint32_t height) {
    ++_sampleFrame;
    Singular::notifyPropertyChanged(this,"sample.frame");
    PropertyValue requestValue;
    if (!isEnabled() || !getDynamicProperty("sample.request",requestValue)) return;
    auto result=std::make_shared<PropertyDict>();
    result->elements["frame"]=_sampleFrame;
    result->elements["width"]=long(width);
    result->elements["height"]=long(height);
    result->elements["stage"]=std::string("completed-viewport");
    result->elements["ok"]=false;
    result->elements["samples"]=std::make_shared<PropertyList>();
    result->elements["count"]=0;
    std::string refusal;
    try {
        const auto* request=std::get_if<std::shared_ptr<PropertyDict>>(&requestValue);
        if (!request || !*request) throw std::runtime_error("sample.request requires a property record");
        const auto item=[&](const char* key)->const PropertyValue& {
            auto it=(*request)->elements.find(key);
            if (it==(*request)->elements.end()) throw std::runtime_error(std::string("missing sample argument: ")+key);
            return it->second;
        };
        const auto text=[&](const char* key)->std::string {
            const auto* value=std::get_if<std::string>(&item(key));
            if (!value || value->empty()) throw std::runtime_error(std::string(key)+" requires nonempty text");
            return *value;
        };
        const std::string token=text("token");
        if (token==_sampleLastToken) return;
        _sampleLastToken=token; // consumes success OR refusal; no per-frame retries
        Singular::notifyPropertyChanged(this,"sample.lastToken");
        result->elements["token"]=token;
        for (const auto& [key,value]:(*request)->elements) {
            (void)value;
            if (key!="token" && key!="region" && key!="x" && key!="y" && key!="width" && key!="height" && key!="limit" && key!="time")
                throw std::runtime_error("unused sample argument: "+key);
        }
        const std::string regionPath=text("region");
        result->elements["region"]=regionPath;
        if (regionPath.front()!='@') throw std::runtime_error("region requires a qualified PropertyPath");
        const auto integer=[&](const char* key,int lo,int hi)->int {
            double value=0;
            if (std::holds_alternative<bool>(item(key)) || !propertyValueToNumber(item(key),value) || !std::isfinite(value) || std::floor(value)!=value || value<lo || value>hi)
                throw std::runtime_error(std::string(key)+" is outside the integer sample bounds");
            return static_cast<int>(value);
        };
        if (!width || !height || uint64_t(width)*height>uint64_t(getSampleReadbackByteCeiling())/4)
            throw std::runtime_error("framebuffer exceeds the 256 MiB readback ceiling");
        const int x=integer("x",0,width-1), y=integer("y",0,height-1);
        const int w=integer("width",1,width-x), h=integer("height",1,height-y);
        const int limit=integer("limit",1,getSampleCountCeiling());
        if (int64_t(w)*h>getSampleScanCeiling()) throw std::runtime_error("sample rectangle exceeds sample.scanCeiling");
        PropertyValue selectorValue;
        if (!lawGetValue(*this,PropertyPath::parse(regionPath+".selector"),selectorValue))
            throw std::runtime_error("region selector does not resolve");
        const auto* field=std::get_if<std::shared_ptr<OntoMath::ScalarField>>(&selectorValue);
        if (!field || !*field || (*field)->mode!=OntoMath::ScalarField::EvaluationMode::AST)
            throw std::runtime_error("region.selector requires a typed AST ScalarField");
        double time=0;
        const auto timeIt=(*request)->elements.find("time");
        const bool bindTime=timeIt!=(*request)->elements.end();
        if (bindTime && (!propertyValueToNumber(timeIt->second,time) || !std::isfinite(time)))
            throw std::runtime_error("sample time must be finite");
        // Local mathematical admission is shared with Law's type calculus;
        // the sensed region cannot capture world guards/calls/folds implicitly.
        OntoMath::TypeEnv types{{"x",OntoMath::ValueKind::Scalar},{"y",OntoMath::ValueKind::Scalar},
            {"z",OntoMath::ValueKind::Scalar},{"u",OntoMath::ValueKind::Scalar},{"v",OntoMath::ValueKind::Scalar},
            {"p",OntoMath::ValueKind::Vector},{"width",OntoMath::ValueKind::Scalar},{"height",OntoMath::ValueKind::Scalar}};
        if (bindTime) types["t"]=OntoMath::ValueKind::Scalar;
        const auto& form=(*field)->astDefinition;
        if (!types.count(form.inputVariable)) throw std::runtime_error("unbound selector interval coordinate");
        for (const auto& piece:form.pieces) {
            if (piece.guard || piece.call || piece.fold) throw std::runtime_error("sample selector requires local mathematics");
            const auto validate=[&](const OntoMath::MathNode& node) {
                auto type=node.typeOf(types);
                if (!type || *type!=OntoMath::ValueKind::Scalar) throw std::runtime_error("sample selector has unbound or invalid mathematics");
            };
            if (piece.mathNode) validate(*piece.mathNode);
            else throw std::runtime_error("sample selector piece has no mathematical value");
            if (piece.whereLEZero) validate(*piece.whereLEZero);
        }
        // Membership is recomputed per request, never cached. This loop is
        // bounded modality sampling, not domain behavior (Algorithms as Law §3).
        std::vector<glm::ivec2> selected;
        for (int iy=y;iy<y+h;++iy) for (int ix=x;ix<x+w;++ix) {
            const double px=ix+.5,py=iy+.5;
            std::map<std::string,PropertyValue> env{{"x",px},{"y",py},{"z",0.0},{"u",px/width},{"v",py/height},
                {"p",glm::vec3(px,py,0)},{"width",double(width)},{"height",double(height)}};
            if (bindTime) env["t"]=time;
            auto value=(*field)->astDefinition.evaluate(env);
            if (!value) continue; // authored undefined domain is excluded
            double signedValue=0;
            if (!propertyValueToNumber(*value,signedValue) || !std::isfinite(signedValue))
                throw std::runtime_error("sample selector produced a nonfinite/non-scalar value");
            if (signedValue>0) continue;
            if (selected.size()==static_cast<size_t>(limit)) throw std::runtime_error("selected region exceeds authored sample limit; no partial result");
            selected.emplace_back(ix,iy);
        }
        // GPU transfer storage is Kernel machinery and is not authored state.
        std::vector<uint8_t> pixels(size_t(width)*height*4);
        if (!renderer.readPixels(pixels.data(),width,height)) throw std::runtime_error("renderer refused completed viewport readback");
        auto samples=std::make_shared<PropertyList>();
        for (const auto& point:selected) {
            const size_t index=(size_t(point.y)*width+point.x)*4;
            auto sample=std::make_shared<PropertyDict>();
            sample->elements["x"]=point.x; sample->elements["y"]=point.y;
            sample->elements["color"]=glm::vec3(pixels[index],pixels[index+1],pixels[index+2])/255.f;
            sample->elements["alpha"]=double(pixels[index+3])/255.0;
            samples->elements.push_back(sample);
        }
        result->elements["selector"]=propertyValueFromJson(propertyValueToJson(selectorValue));
        result->elements["timeBound"]=bindTime;
        if (bindTime) result->elements["time"]=time;
        result->elements["count"]=int(samples->elements.size());
        result->elements["samples"]=samples;
        result->elements["ok"]=true;
    } catch (const std::exception& error) { refusal=error.what(); }
    result->elements["refusal"]=refusal;
    _sampleResult=std::move(result);
    Singular::notifyPropertyChanged(this,"sample.result");
}

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
    registerProperty(std::make_unique<ComputedProperty<ScreenChannel,long>>("sample.frame",this,&ScreenChannel::getSampleFrame));
    registerProperty(std::make_unique<ComputedProperty<ScreenChannel,std::string>>("sample.lastToken",this,&ScreenChannel::getSampleLastToken));
    registerProperty(std::make_unique<ComputedProperty<ScreenChannel,std::shared_ptr<PropertyDict>>>("sample.result",this,&ScreenChannel::getSampleResult));
    registerProperty(std::make_unique<ComputedProperty<ScreenChannel,int>>("sample.scanCeiling",this,&ScreenChannel::getSampleScanCeiling));
    registerProperty(std::make_unique<ComputedProperty<ScreenChannel,int>>("sample.countCeiling",this,&ScreenChannel::getSampleCountCeiling));
    registerProperty(std::make_unique<ComputedProperty<ScreenChannel,long>>("sample.readbackByteCeiling",this,&ScreenChannel::getSampleReadbackByteCeiling));
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
    boolean("volumeZeroProofEnabled", &ScreenChannel::volumeZeroProofEnabled);
    readOnlyInt("volumeZeroProofCellsProven", &ScreenChannel::getVolumeZeroProofCellsProven);
    readOnlyInt("volumeZeroProofCellsTotal", &ScreenChannel::getVolumeZeroProofCellsTotal);
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
