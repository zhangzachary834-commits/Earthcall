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
    void updateMetrics(int drawCalls, int trianglesDrawn, double vramBytes,
                       double uniformBytes, int suballocations, int pipelineSwitches,
                       int cachedMeshes);

    int    drawCalls = 0;
    int    trianglesDrawn = 0;
    double vramAllocatedBytes = 0.0;
    double uniformBytesWritten = 0.0;
    int    bufferSuballocations = 0;
    int    pipelineSwitches = 0;
    int    cachedMeshesCount = 0;
    bool   wireframe = false;

private:
    void buildProperties() override;

    // Getters for the derived metrics below: NO_BLACK_BOX.md §3 says a Law may
    // read anything, but "writable unless genuinely derived" — these seven are
    // the definition of derived (the renderer computes them; nothing upstream
    // of it should get to override what actually happened last frame). Each is
    // registered as a ComputedProperty with a null setter, which resolves to a
    // refused write rather than a value a Law could quietly clobber and have
    // the next updateMetrics silently overwrite again. `wireframe` is the one
    // exception and stays a plain PropertyRef, since it genuinely drives the
    // rasterizer rather than reporting on it.
    int    getDrawCalls() const { return drawCalls; }
    int    getTrianglesDrawn() const { return trianglesDrawn; }
    double getVramAllocatedBytes() const { return vramAllocatedBytes; }
    double getUniformBytesWritten() const { return uniformBytesWritten; }
    int    getBufferSuballocations() const { return bufferSuballocations; }
    int    getPipelineSwitches() const { return pipelineSwitches; }
    int    getCachedMeshesCount() const { return cachedMeshesCount; }

    std::string _name{"screen-channel"};
};

} // namespace Screen
} // namespace Singularity
