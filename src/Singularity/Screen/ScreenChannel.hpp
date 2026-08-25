#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <string>

namespace Singularity {
namespace Screen {

// First-mover modality channel for Screen / GPU graphics rendering.
//
// Governs and exposes the graphics rendering substrate to the Law system
// (Refusal #6 — No Black Box).
//
// Laws can read and govern:
//   - @screen-channel.enabled: master switch for screen rendering
//   - @screen-channel.drawCalls: total GPU draw passes executed this frame
//   - @screen-channel.trianglesDrawn: total geometric triangles rendered
//   - @screen-channel.vramAllocatedBytes: total VRAM occupied by GPU buffer pools & textures
//   - @screen-channel.uniformBytesWritten: uniform buffer bytes streamed this frame
//   - @screen-channel.bufferSuballocations: number of suballocations served from the buffer pool
//   - @screen-channel.pipelineSwitches: number of pipeline state transitions
//   - @screen-channel.cachedMeshesCount: number of persistent VBO meshes retained in VRAM
//   - @screen-channel.wireframe: whether the screen renders in wireframe mode
class ScreenChannel : public Law {
public:
    ScreenChannel();

    bool isFirstMover() const override { return true; }
    std::string getIdentifier() const override { return "screen-channel"; }
    const std::string& name() const { return _name; }

    static void syncRegister(LawManager& laws);
    static ScreenChannel* find(LawManager& laws);

    // Update live metrics from the active Renderer at the end of each frame.
    void updateMetrics(int drawCalls, int trianglesDrawn, float vramBytes,
                       float uniformBytes, int suballocations, int pipelineSwitches,
                       int cachedMeshes);

    int   drawCalls = 0;
    int   trianglesDrawn = 0;
    float vramAllocatedBytes = 0.0f;
    float uniformBytesWritten = 0.0f;
    int   bufferSuballocations = 0;
    int   pipelineSwitches = 0;
    int   cachedMeshesCount = 0;
    bool  wireframe = false;

private:
    void buildProperties() override;

    std::string _name{"screen-channel"};
};

} // namespace Screen
} // namespace Singularity
