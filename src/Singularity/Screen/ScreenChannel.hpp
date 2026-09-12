#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <glm/glm.hpp>
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
//   - @screen-channel.backgroundColor: screen clear / background color (vec3)
//   - @screen-channel.light.cameraRelative: whether the renderer derives the
//     light's world position from the Person's camera plus light.cameraOffset
//   - @screen-channel.light.position: absolute world-space light position when
//     cameraRelative is false
//   - @screen-channel.light.cameraOffset: camera-relative displacement when
//     cameraRelative is true; defaults to the historical (2,5,2)
//   - @screen-channel.wireframe: whether the screen renders in wireframe mode
//   - @screen-channel.heightGridDdaEnabled: whether the WebGPU marcher skips
//     proven-empty stretches of a heightfield ray via its min/max grid
//     (rendering-optimization Phase C); disabling falls back to the
//     unmodified per-step marcher
//   - @screen-channel.drawCalls: total GPU draw passes executed this frame
//   - @screen-channel.trianglesDrawn: total geometric triangles rendered
//   - @screen-channel.vramAllocatedBytes: total VRAM occupied by GPU buffer pools & textures
//   - @screen-channel.uniformBytesWritten: uniform buffer bytes streamed this frame
//   - @screen-channel.bufferSuballocations: number of suballocations served from the buffer pool
//   - @screen-channel.pipelineSwitches: number of pipeline state transitions
//   - @screen-channel.cachedMeshesCount: number of persistent VBO meshes retained in VRAM
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

    int       drawCalls = 0;
    int       trianglesDrawn = 0;
    double    vramAllocatedBytes = 0.0;
    double    uniformBytesWritten = 0.0;
    int       bufferSuballocations = 0;
    int       pipelineSwitches = 0;
    int       cachedMeshesCount = 0;
    bool      wireframe = false;
    bool      heightGridDdaEnabled = true;
    bool      recording = false;
    bool      snapshotTrigger = false;
    glm::vec3 backgroundColor{0.1f, 0.1f, 0.15f};

    // First-order authored illumination placement.
    //
    // These are WORLD MEANING, not GPU mechanism: a Person can mean something
    // by changing where illumination comes from, so NO_BLACK_BOX.md requires
    // them to be ordinary Properties. The backend still owns pipelines,
    // uniforms and driver handles beneath the Kernel; ScreenChannel owns only
    // the authored facts the renderer consumes.
    //
    // The defaults preserve the old ShadingSystem behavior exactly: the active
    // light follows the Person's camera at cameraPos + (2,5,2). A Law can make
    // it world-fixed by setting light.cameraRelative=false and writing
    // light.position, or can animate either vector as any other property.
    bool      lightCameraRelative = true;
    glm::vec3 lightPosition{2.0f, 5.0f, 2.0f};
    glm::vec3 lightCameraOffset{2.0f, 5.0f, 2.0f};

    bool getHasScreenCapturePermission() const;
    bool getHasAccessibilityPermission() const;

private:
    void buildProperties() override;

    // Getters for the derived metrics below: NO_BLACK_BOX.md §3 says a Law may
    // read anything, but "writable unless genuinely derived" — these seven are
    // the definition of derived (the renderer computes them; nothing upstream
    // of it should get to override what actually happened last frame). Each is
    // registered as a ComputedProperty with a null setter, which resolves to a
    // refused write rather than a value a Law could quietly clobber and have
    // the next updateMetrics silently overwrite again. `wireframe`,
    // `heightGridDdaEnabled`, `backgroundColor`, and `light.*` are the
    // exceptions and stay plain PropertyRefs, since they genuinely DRIVE the
    // renderer rather than report on it — they are read back out in
    // EngineRender.cpp every frame and can be written by Laws and UI authoring.
    // That shared writability is the point: one property, two hands.
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
