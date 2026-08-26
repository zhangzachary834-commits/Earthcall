// Real Earthcall content rendered through WebGPU, headless.
//
// WHY THIS EXISTS: smoke_renderer proves each Renderer verb works, but it feeds
// them by hand. That is exactly how the "every cube is white" bug survived —
// RenderMaterial::albedoPixels was read correctly by WebGpuRenderer and set
// correctly by the test fixture, while NOTHING in the app ever populated it.
// A verb tested only through a fixture passes while the production path feeding
// it does not exist. So this test drives the genuine Object draw path instead.
//
// It also demonstrates that Objects can now be constructed with no GL context:
// texture upload goes through the Renderer boundary, so selecting the WebGPU
// backend first means Object construction never calls into OpenGL.

#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <vector>

extern MaterialManager materials;   // global Material beings (globals.cpp)

namespace {

const uint32_t W = 16, H = 16;

struct MapR { bool ok = false, done = false; };
void onMap(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* r = static_cast<MapR*>(u);
    r->ok = (s == WGPUMapAsyncStatus_Success);
    r->done = true;
}

} // namespace

int main() {
    // Unbuffered: this test can crash inside the GPU driver, and a buffered
    // stdout would swallow every progress line written before the fault,
    // making the failure look like it happened at startup.
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    wgpu::Device gpu;
    if (!gpu.init()) { std::printf("FAIL: no WebGPU device\n"); return 1; }

    WebGpuRenderer renderer;
    if (!renderer.init(gpu)) { std::printf("FAIL: renderer init\n"); return 1; }

    // MUST precede any Object construction: Object paints its face textures on
    // construction, and the default backend is OpenGL, which would try to call
    // glGenTextures with no context.
    setCurrentRenderer(&renderer);

    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D;
    td.size = { W, H, 1 };
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1; td.sampleCount = 1;
    WGPUTexture target = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView view = wgpuTextureCreateView(target, nullptr);

    WGPUBufferDescriptor rbd = {};
    rbd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    rbd.size = 256 * H;
    WGPUBuffer readback = wgpuDeviceCreateBuffer(gpu.device, &rbd);

    auto readAt = [&](uint32_t px_, uint32_t py_, unsigned char out[4]) {
        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUTexelCopyTextureInfo src = {};
        src.texture = target; src.aspect = WGPUTextureAspect_All; src.origin = {0,0,0};
        WGPUTexelCopyBufferInfo dst = {};
        dst.buffer = readback; dst.layout.bytesPerRow = 256; dst.layout.rowsPerImage = H;
        WGPUExtent3D ext = { W, H, 1 };
        wgpuCommandEncoderCopyTextureToBuffer(enc, &src, &dst, &ext);
        WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &cmd);
        wgpuCommandBufferRelease(cmd);
        wgpuCommandEncoderRelease(enc);

        MapR m;
        WGPUBufferMapCallbackInfo ci = {};
        ci.mode = WGPUCallbackMode_AllowProcessEvents;
        ci.callback = onMap; ci.userdata1 = &m;
        wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, 256 * H, ci);
        while (!m.done) wgpuInstanceProcessEvents(gpu.instance);
        const auto* px = static_cast<const unsigned char*>(
            wgpuBufferGetConstMappedRange(readback, 0, 256 * H));
        const size_t at = size_t(py_) * 256 + size_t(px_) * 4;
        for (int i = 0; i < 4; ++i) out[i] = px[at + i];
        wgpuBufferUnmap(readback);
    };
    auto readCentre = [&](unsigned char out[4]) { readAt(W / 2, H / 2, out); };

    // Camera: straight down -Z at a unit cube, so its +Z face fills the frame.
    // frustum/perspective must be the [0,1]-depth form for WebGPU.
    const glm::vec3 eye(0.0f, 0.0f, 2.0f);
    const glm::mat4 proj = glm::perspectiveZO(glm::radians(45.0f), float(W) / H, 0.1f, 100.0f);
    const glm::mat4 view3d = glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0, 1, 0));

    // --- The actual subject: an Object painted red, drawn the way the app draws it.
    // setFaceColor paints the object's OWN material — the first stroke
    // diverges it from the shared material.default it was referencing, which
    // is what makes "this object is red and that one is green" possible at
    // all now that the per-face textures live on the Material being.
    Object cube;
    cube.setShapeKind(Object::ShapeKind::Cube);
    for (int f = 0; f < cube.getFaces(); ++f)
        cube.setFaceColor(f, 1.0f, 0.0f, 0.0f); // pure red paint

    renderer.setCamera(view3d, proj, eye);
    renderer.setModel(glm::mat4(1.0f));
    renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    cube.drawObject();
    renderer.endFrame();

    unsigned char p[4];
    readCentre(p);
    std::printf("painted cube centre = (%d,%d,%d,%d)\n", p[0], p[1], p[2], p[3]);

    // Lighting scales the value, so do not assert an exact colour — assert the
    // HUE. An unpainted surface falls back to the 1x1 white texture, which makes
    // r == g == b; red paint must make red dominate. That difference is precisely
    // the bug this test exists to catch.
    assert(p[0] > 40 && "surface rendered black — nothing reached the framebuffer");
    assert(p[0] > p[1] + 40 && p[0] > p[2] + 40 &&
           "face albedo did not reach WebGPU: surface is grey/white, not red");

    // Control: a SECOND object painted a different colour must come out that
    // colour. Asserting "the first one is red" alone would still pass if every
    // surface were tinted red by some shared state; this pins the paint to the
    // object. It also catches the per-draw texture upload leaking between draws.
    Object green;
    green.setShapeKind(Object::ShapeKind::Cube);
    for (int f = 0; f < green.getFaces(); ++f)
        green.setFaceColor(f, 0.0f, 1.0f, 0.0f);

    renderer.setModel(glm::mat4(1.0f));
    renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    green.drawObject();
    renderer.endFrame();

    unsigned char q[4];
    readCentre(q);
    std::printf("green cube centre   = (%d,%d,%d,%d)\n", q[0], q[1], q[2], q[3]);
    assert(q[1] > q[0] + 40 && q[1] > q[2] + 40 &&
           "second object did not get its own paint — albedo is shared or stale");

    // An UNPAINTED object has no face textures at all — paint lives on the
    // Material being, and an object that has never been painted is still
    // sharing one rather than owning one. What it shows is that material's
    // baseColor, so assert THAT: give it a material tinted blue and read blue
    // back. (It used to seed a per-face RGB palette on construction; that
    // palette was a debug default which no longer has anywhere to live now
    // that a material is shared by name, and an object which shows its
    // material's colour is the more truthful default anyway.)
    auto tint = materials.create("webgpu_object_tint");
    tint->baseColor = glm::vec3(0.1f, 0.1f, 0.9f);

    Object plain;
    plain.setShapeKind(Object::ShapeKind::Cube);
    plain.setMaterialId("material.webgpu_object_tint");
    renderer.setModel(glm::mat4(1.0f));
    renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
    plain.drawObject();
    renderer.endFrame();

    unsigned char d[4];
    readCentre(d);
    std::printf("unpainted cube centre = (%d,%d,%d,%d)\n", d[0], d[1], d[2], d[3]);
    assert(d[2] > d[0] + 40 && d[2] > d[1] + 40 &&
           "material baseColor did not reach WebGPU (+Z face should be blue)");
    // The original bug's signature: an all-white surface has r == g == b.
    assert(!(abs(int(d[0]) - int(d[1])) < 12 && abs(int(d[1]) - int(d[2])) < 12) &&
           "surface is grey — the material's colour never reached the shader");

    // And painting it now DIVERGES it: the shared tint is untouched afterwards,
    // which is the whole promise of copy-on-write over a shared being.
    plain.setFaceColor(0, 1.0f, 0.0f, 0.0f);
    assert(plain.materialId() != "material.webgpu_object_tint" &&
           "painting an object must give it its own material, not repaint the shared one");
    assert(tint->faceTextures.empty() &&
           "the shared material took paint meant for one object");

    // Unhook before the renderer (a stack object) goes out of scope: globals such
    // as ZoneManager are destroyed after main returns and can still reach for the
    // active backend, which by then would be a dangling pointer.
    // --- A FIELD object: raymarched, not tessellated (Milestone 6) -------------
    // The point of routing through the real Object path: drawFieldModel asks the
    // backend whether it renders implicits exactly, and only WebGPU says yes. If
    // that wiring breaks, this silently falls back to the mesh and still looks
    // plausible — so assert the SHAPE, which the bounding box would not produce.
    {
        geom::SdfNode sphere;
        sphere.op   = geom::SdfOp::Leaf;
        sphere.prim = geom::SdfPrim::Sphere;
        sphere.dims = glm::vec3(0.55f);

        Object field;
        field.setFieldShape(sphere, 1.0f);
        // Painted explicitly: a field is one face, and nothing gives an object
        // paint it was not given any more, so the colour under test has to be
        // put there rather than inherited from a construction-time palette.
        field.setFaceColor(0, 1.0f, 0.0f, 0.0f);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        field.drawObject();
        renderer.endFrame();
    }
    unsigned char f0[4], fcorner[4];
    readCentre(f0);
    readAt(0, 0, fcorner);
    std::printf("field centre        = (%d,%d,%d,%d)  corner = (%d,%d,%d,%d)\n",
                f0[0], f0[1], f0[2], f0[3], fcorner[0], fcorner[1], fcorner[2], fcorner[3]);
    assert(f0[0] > 30 && "raymarched field produced no surface at the centre");
    // The field's face-0 paint is RED, and tessellateSdf samples one texel, so
    // the raymarched surface must be red too — not white. This is the check
    // that catches a field silently changing colour with the backend.
    assert(f0[0] > f0[1] + 30 && f0[0] > f0[2] + 30 &&
           "field ignored its face paint — raymarch and mesh paths disagree on colour");
    assert(fcorner[0] < 12 && fcorner[1] < 12 && fcorner[2] < 12 &&
           "corner is lit — the bounding box was painted instead of the sphere traced");
    assert(currentRenderer().rendersImplicitExactly() &&
           "WebGPU should report exact implicit rendering");

    // --- An unpainted cube draws as ONE merged mesh; painting a single face
    // must drop it straight back to the six-face path (remediation plan Phase
    // 4.2). This is the guard the plan calls for: the merge decision reads the
    // resolved albedo fresh every draw rather than being cached on the Object.
    {
        Object mergeCube;
        mergeCube.setShapeKind(Object::ShapeKind::Cube);

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        mergeCube.drawObject();
        renderer.endFrame();
        const int unpaintedDrawCalls = renderer.frameStats().drawCalls;
        std::printf("unpainted cube draw calls = %d\n", unpaintedDrawCalls);
        assert(unpaintedDrawCalls == 1 &&
               "an unpainted cube's six faces resolve identical paint and should merge to one draw");

        mergeCube.setFaceColor(0, 1.0f, 0.0f, 0.0f); // one stroke, one face

        renderer.setModel(glm::mat4(1.0f));
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        mergeCube.drawObject();
        renderer.endFrame();
        const int paintedDrawCalls = renderer.frameStats().drawCalls;
        std::printf("single-face-painted cube draw calls = %d\n", paintedDrawCalls);
        assert(paintedDrawCalls == 6 &&
               "a single painted face must fall back to the six-draw path, not merge over the paint");
    }

    {
        std::vector<Object> toruses(10);
        for (int i = 0; i < 10; ++i) {
            toruses[i].setShapeKind(Object::ShapeKind::Torus);
            // Force Mesh path. WebGPU raymarches Parametrics exactly (drawImplicit) which does not batch.
            // This tests the Phase 4 tessellation cache, which applies to the Mesh path.
            toruses[i].setRenderMode(Object::RenderMode::Mesh);
        }
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));
        for (int i = 0; i < 10; ++i) {
            renderer.setModel(glm::mat4(1.0f));
            toruses[i].drawObject();
        }
        renderer.endFrame();
        const int torusDrawCalls = renderer.frameStats().drawCalls;
        std::printf("10 identical toruses draw calls = %d\n", torusDrawCalls);
        assert(torusDrawCalls == 1 &&
               "10 identical smooth shapes must batch into 1 draw call via shared tessellation");
    }

    setCurrentRenderer(nullptr);
    renderer.shutdown();
    std::printf("webgpu_object_test: ALL OK\n");

    // _Exit, not return: bringing up a wgpu-native device inside a process that
    // also carries this app's global objects aborts during STATIC DESTRUCTION,
    // after main. Verified as unrelated to anything here — it reproduces with no
    // Objects created at all and with the texture cache disabled, while
    // smoke_renderer (same GPU bring-up, none of the app's globals) exits 0, and
    // the real earthcall_webgpu shuts down cleanly. This skips those destructors
    // so the test reports its own result rather than that fault. Every assertion
    // above has already run, so a genuine regression still aborts loudly.
    // Tracked in docs/rendering/WEBGPU_HANDOFF.md — do NOT treat this as fixed.
    std::fflush(stdout);
    std::_Exit(0);
}
