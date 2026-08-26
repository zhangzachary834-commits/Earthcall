// Why 60 toruses lag the app while 4,500 objects pass webgpu_micro_mastery_lag_test.
// Hypothesis: the heavy probe is fragment-blind (512x512, camera at z=150, sphere
// SDFs) and the torus path is raymarched per-fragment, not meshed.
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SmoothSurface.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>

extern MaterialManager materials;
using Clock = std::chrono::steady_clock;
static double ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

struct Scene { const char* name; uint32_t W, H; float camZ; bool torus; int n; };

static double run(WebGpuRenderer& r, wgpu::Device& gpu, const Scene& s, int frames) {
    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D;
    td.size = { s.W, s.H, 1 };
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1; td.sampleCount = 1;
    WGPUTexture target = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView view = wgpuTextureCreateView(target, nullptr);

    const glm::vec3 eye(0.0f, 0.0f, s.camZ);
    const glm::mat4 proj = glm::perspectiveZO(glm::radians(45.0f), float(s.W) / s.H, 0.1f, 1000.0f);
    const glm::mat4 v3d  = glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0, 1, 0));

    // Spread them over a patch that fills the view at camZ, the way a Person
    // spawning into a room would see them.
    const float spread = s.camZ * 0.35f;
    std::vector<std::unique_ptr<Object>> objs;
    auto t0 = Clock::now();
    for (int i = 0; i < s.n; ++i) {
        auto o = std::make_unique<Object>("o_" + std::to_string(i));
        if (s.torus) o->setShapeKind(Object::ShapeKind::Torus);
        else         o->setShapeKind(Object::ShapeKind::Cube);
        const int cols = 8;
        o->setPosition(glm::vec3((i % cols - cols / 2) * (spread / cols),
                                 (i / cols - s.n / cols / 2) * (spread / cols), 0.0f));
        objs.push_back(std::move(o));
    }
    const double spawnMs = ms(t0, Clock::now());

    // Warm the pipeline cache so shader creation is not charged to frame 1.
    r.setCamera(v3d, proj, eye);
    r.beginFrameOffscreen(view, s.W, s.H, glm::vec4(0, 0, 0, 1));
    for (auto& o : objs) o->drawObject();
    r.endFrame();

    auto t1 = Clock::now();
    for (int f = 0; f < frames; ++f) {
        r.setCamera(v3d, proj, eye);
        r.beginFrameOffscreen(view, s.W, s.H, glm::vec4(0, 0, 0, 1));
        for (auto& o : objs) {
            o->setRotationEulerDegrees(glm::vec3(f * 0.5f, f * 0.5f, 0.0f));
            o->drawObject();
        }
        r.endFrame();
    }
    const double avg = ms(t1, Clock::now()) / frames;
    const auto& st = r.frameStats();
    std::printf("%-34s %5ux%-5u cam z=%-6.1f n=%-4d  spawn %8.1f ms | frame %8.2f ms (%5.1f fps)  draws=%-6u\n",
                s.name, s.W, s.H, s.camZ, s.n, spawnMs, avg, 1000.0 / avg, st.drawCalls);
    wgpuTextureViewRelease(view);
    wgpuTextureRelease(target);
    return avg;
}

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    wgpu::Device gpu;
    if (!gpu.init()) { std::printf("no WebGPU device\n"); return 0; }
    WebGpuRenderer r;
    if (!r.init(gpu)) { std::printf("renderer init failed\n"); return 0; }
    setCurrentRenderer(&r);

    std::printf("\n=== A. Does the shape matter, at the heavy probe's own conditions? ===\n");
    run(r, gpu, {"60 cubes   (probe conditions)", 512, 512, 150.0f, false, 60}, 30);
    run(r, gpu, {"60 toruses (probe conditions)", 512, 512, 150.0f, true,  60}, 30);

    std::printf("\n=== B. Same 60 toruses, at what a Person's window actually is ===\n");
    run(r, gpu, {"60 toruses (1080p, close)",   1920, 1080, 8.0f, true,  60}, 30);
    run(r, gpu, {"60 cubes   (1080p, close)",   1920, 1080, 8.0f, false, 60}, 30);
    run(r, gpu, {"60 toruses (Retina 2x)",      3840, 2160, 8.0f, true,  60}, 20);

    std::printf("\n=== C. How the torus cost scales with count (1080p, close) ===\n");
    for (int n : {1, 10, 30, 60, 120})
        run(r, gpu, {"toruses", 1920, 1080, 8.0f, true, n}, 20);

    std::printf("\n=== D. Per-frame CPU cost of the implicit path, per object ===\n");
    {
        Object o("t");
        o.setShapeKind(Object::ShapeKind::Torus);
        const geom::SmoothSurfaceData& sd = o.getSmoothData();
        const int N = 2000;
        auto t0 = Clock::now();
        size_t wgslBytes = 0;
        for (int i = 0; i < N; ++i) {
            const geom::SdfNode f = geom::sdfFromSmooth(sd);
            const sdfwgsl::Program p = sdfwgsl::compile(f, nullptr);
            wgslBytes = p.wgsl.size();
        }
        const double per = ms(t0, Clock::now()) / N;
        std::printf("sdfFromSmooth + sdfwgsl::compile : %.4f ms/object/frame  (WGSL %zu bytes, regenerated every call)\n",
                    per, wgslBytes);
        std::printf("  -> at 60 objects x 60 fps      : %.1f ms/s of CPU spent rebuilding shader source\n",
                    per * 60 * 60);
    }

    std::printf("\n=== E. What the app pays per frame that this probe skipped: hover picking ===\n");
    {
        std::vector<std::unique_ptr<Object>> objs;
        std::vector<Object*> raw;
        for (int i = 0; i < 60; ++i) {
            auto o = std::make_unique<Object>("t_" + std::to_string(i));
            o->setShapeKind(Object::ShapeKind::Torus);
            o->setPosition(glm::vec3((i % 8) * 1.5f - 6.0f, (i / 8) * 1.5f - 4.0f, 0.0f));
            raw.push_back(o.get());
            objs.push_back(std::move(o));
        }
        std::printf("torus tessellation is a private cache; measuring the pick cost directly.\n");

        // One pickSurface is what InteractionChannel::step does every frame.
        const glm::vec3 ro(0.0f, 0.0f, 20.0f), rd(0.0f, 0.0f, -1.0f);
        const int N = 300;
        SurfaceHit hit;
        auto t0 = Clock::now();
        for (int i = 0; i < N; ++i) pickSurface(raw, ro, rd, hit);
        const double per = ms(t0, Clock::now()) / N;
        std::printf("pickSurface over 60 toruses  : %.3f ms  (once per frame, every frame)\n", per);
        std::printf("  -> that alone is           : %.1f fps ceiling before anything is drawn\n", 1000.0 / per);

        // Same thing for cubes, as the control.
        std::vector<std::unique_ptr<Object>> cubes; std::vector<Object*> craw;
        for (int i = 0; i < 60; ++i) {
            auto o = std::make_unique<Object>("c_" + std::to_string(i));
            o->setShapeKind(Object::ShapeKind::Cube);
            o->setPosition(glm::vec3((i % 8) * 1.5f - 6.0f, (i / 8) * 1.5f - 4.0f, 0.0f));
            craw.push_back(o.get()); cubes.push_back(std::move(o));
        }
        t0 = Clock::now();
        for (int i = 0; i < N; ++i) pickSurface(craw, ro, rd, hit);
        std::printf("pickSurface over 60 cubes    : %.3f ms  (control)\n", ms(t0, Clock::now()) / N);
    }

    setCurrentRenderer(nullptr);
    r.shutdown();
    std::fflush(stdout);
    std::_Exit(0);
}
