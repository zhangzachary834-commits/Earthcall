#include "ScreenChannel.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

namespace Singularity {
namespace Screen {

ScreenChannel::ScreenChannel() = default;

void ScreenChannel::syncRegister(LawManager& laws) {
    if (find(laws)) return;
    auto channel = std::make_shared<ScreenChannel>();
    laws.add(channel);
}

ScreenChannel* ScreenChannel::find(LawManager& laws) {
    for (const auto& law : laws.getAll()) {
        if (auto* channel = dynamic_cast<ScreenChannel*>(law.get())) {
            return channel;
        }
    }
    return nullptr;
}

void ScreenChannel::updateMetrics(int dCalls, int tris, float vramBytes,
                                 float uBytes, int suballocs, int pipeSwitches,
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

    const auto integer = [this](const char* name, int ScreenChannel::*member) {
        _propertyRegistry.push_back(
            std::make_unique<PropertyRef<ScreenChannel, int>>(name, this, member));
    };
    const auto flt = [this](const char* name, float ScreenChannel::*member) {
        _propertyRegistry.push_back(
            std::make_unique<PropertyRef<ScreenChannel, float>>(name, this, member));
    };
    const auto boolean = [this](const char* name, bool ScreenChannel::*member) {
        _propertyRegistry.push_back(
            std::make_unique<PropertyRef<ScreenChannel, bool>>(name, this, member));
    };

    integer("drawCalls", &ScreenChannel::drawCalls);
    integer("trianglesDrawn", &ScreenChannel::trianglesDrawn);
    flt("vramAllocatedBytes", &ScreenChannel::vramAllocatedBytes);
    flt("uniformBytesWritten", &ScreenChannel::uniformBytesWritten);
    integer("bufferSuballocations", &ScreenChannel::bufferSuballocations);
    integer("pipelineSwitches", &ScreenChannel::pipelineSwitches);
    integer("cachedMeshesCount", &ScreenChannel::cachedMeshesCount);
    boolean("wireframe", &ScreenChannel::wireframe);
}

} // namespace Screen
} // namespace Singularity
