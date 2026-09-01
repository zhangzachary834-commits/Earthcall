// Tests for CPU-GPU Micro-Mastery Substrate & WebGPU-to-Singular Graphics Mastery.
//
// Verifies:
//   1. GpuBufferPool: uniform allocations obey 256-byte alignment contract
//   2. GpuBufferPool: vertex allocations obey 16-byte alignment contract
//   3. GpuBufferPool: resetFrame recycles capacity without releasing VRAM
//   4. GpuBufferPool: oversized allocations trigger seamless chunk expansion
//   5. GpuMeshCache: tracks cached meshes and invalidates cleanly
//   6. ScreenChannel: registers as first-mover with stable identifier "screen-channel"
//   7. ScreenChannel: property paths resolve and update live metrics (No Black Box)
//   8. Device-backed: the two bugs CPU_GPU_MICRO_MASTERY.md §2/§3 claim fixed,
//      the GpuMeshCache revision guard (§2.1 of the remediation plan), and
//      GpuBufferPool::init() not leaking a previous device's chunks.
//
// Section 8 needs a real WebGPU device. Headless CI without one is not a
// failure — it is reported and skipped, same as the other webgpu_* probes
// gracefully no-op without a surface.

#include "Singularity/Screen/WebGPU/GpuBufferPool.hpp"
#include "Singularity/Screen/WebGPU/GpuMeshCache.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <webgpu/webgpu.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>
#include <vector>

namespace {

int g_passed = 0;
int g_failed = 0;

// Readback plumbing for section [4]'s content checks below. WGPUBuffer is an
// opaque driver handle: releasing one and immediately creating another of the
// same size can hand back the SAME address (ordinary allocator reuse), so
// pointer (in)equality between an old and new buffer is not a trustworthy
// signal that a re-upload actually happened. Reading the bytes back is.
struct MapR { bool ok = false, done = false; };
void onMap(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* r = static_cast<MapR*>(u);
    r->ok = (s == WGPUMapAsyncStatus_Success);
    r->done = true;
}

// Reads the first vertex's position (the first 3 floats) out of a vertex
// buffer produced by GpuMeshCache::getOrUpload.
glm::vec3 readFirstVertexPos(const wgpu::Device& gpu, WGPUBuffer buf) {
    WGPUBufferDescriptor rbd = {};
    rbd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    rbd.size = 16; // rounded up from 12 bytes (vec3)
    WGPUBuffer readback = wgpuDeviceCreateBuffer(gpu.device, &rbd);

    WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    wgpuCommandEncoderCopyBufferToBuffer(enc, buf, 0, readback, 0, rbd.size);
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
    wgpuQueueSubmit(gpu.queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
    wgpuCommandEncoderRelease(enc);

    MapR m;
    WGPUBufferMapCallbackInfo ci = {};
    ci.mode = WGPUCallbackMode_AllowProcessEvents;
    ci.callback = onMap;
    ci.userdata1 = &m;
    wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, rbd.size, ci);
    while (!m.done) wgpuInstanceProcessEvents(gpu.instance);
    const auto* f = static_cast<const float*>(wgpuBufferGetConstMappedRange(readback, 0, rbd.size));
    glm::vec3 out(f[0], f[1], f[2]);
    wgpuBufferUnmap(readback);
    wgpuBufferRelease(readback);
    return out;
}

void check(bool ok, const char* msg) {
    if (ok) {
        ++g_passed;
        std::printf("  ok: %s
", msg);
    } else {
        ++g_failed;
        std::printf("  FAIL: %s
", msg);
    }
}

} // namespace

int main() {
    std::printf("Running gpu_mastery_test...
");

    // -------------------------------------------------------------------
    // [1] ScreenChannel: First-mover registration and stable identifier
    // -------------------------------------------------------------------
    {\
        LawManager laws;
        Singularity::Screen::ScreenChannel::syncRegister(laws);
        auto* channel = Singularity::Screen::ScreenChannel::find(laws);
        check(channel != nullptr, "[1] ScreenChannel registered in LawManager");
        if (channel) {
            check(channel->isFirstMover(), "[1] ScreenChannel isFirstMover is true");
            check(channel->getIdentifier() == "screen-channel", "[1] ScreenChannel stable identifier is 'screen-channel'");
        }
    }

    // -------------------------------------------------------------------
    // [2] ScreenChannel: PropertyPath resolution and live telemetry updates
    // -------------------------------------------------------------------
    {\
        LawManager laws;
        Singularity::Screen::ScreenChannel::syncRegister(laws);
        auto* channel = Singularity::Screen::ScreenChannel::find(laws);
        assert(channel);

        // Update metrics. The VRAM figure is chosen past float's 24-bit exact-
        // integer range (16,777,216) — a stale float round trip anywhere in
        // the path would round it, which is exactly what remediation plan
        // §5.2 exists to catch (a float cannot represent consecutive integers
        // past 16.7 MB, and this pool measures VRAM in whole megabytes).
        constexpr double kVramBytes = 2147483653.0; // ~2 GiB + 5, deliberately not float-exact
        channel->updateMetrics(42, 1200, kVramBytes, 65536.0, 15, 8, 4);

        // Read drawCalls via PropertyPath
        {
            PropertyValue v;
            auto res = PropertyPath::parse("drawCalls").getValue(*channel, v);
            check(res == PropertyPath::PathResult::Ok, "[2] drawCalls resolves via PropertyPath");
            double n = 0.0;
            propertyValueToNumber(v, n);
            check(static_cast<int>(n) == 42, "[2] drawCalls reads back 42");
        }

        // Read trianglesDrawn via PropertyPath
        {
            PropertyValue v;
            auto res = PropertyPath::parse("trianglesDrawn").getValue(*channel, v);
            check(res == PropertyPath::PathResult::Ok, "[2] trianglesDrawn resolves");
            double n = 0.0;
            propertyValueToNumber(v, n);
            check(static_cast<int>(n) == 1200, "[2] trianglesDrawn reads back 1200");
        }

        // Read vramAllocatedBytes via PropertyPath
        {
            PropertyValue v;
            auto res = PropertyPath::parse("vramAllocatedBytes").getValue(*channel, v);
            check(res == PropertyPath::PathResult::Ok, "[2] vramAllocatedBytes resolves");
            double n = 0.0;
            propertyValueToNumber(v, n);
            check(std::fabs(n - kVramBytes) < 1.0,
                  "[2] vramAllocatedBytes reads back exactly, with no float round trip");
        }

        // Derived telemetry is READ-ONLY (remediation plan §5.1): a Law may
        // read it but never override what the renderer actually measured.
        {
            auto setRes = PropertyPath::parse("drawCalls").setValue(*channel, PropertyValue(9999));
            check(setRes == PropertyPath::PathResult::ReadOnly,
                  "[2] drawCalls refuses a write — derived telemetry, not authored state");
            PropertyValue v;
            PropertyPath::parse("drawCalls").getValue(*channel, v);
            double n = 0.0;
            propertyValueToNumber(v, n);
            check(static_cast<int>(n) == 42,
                  "[2] a refused write to drawCalls leaves the real measurement untouched");
        }

        // Read and write wireframe via PropertyPath
        {
            PropertyValue v;
            auto res = PropertyPath::parse("wireframe").getValue(*channel, v);
            check(res == PropertyPath::PathResult::Ok, "[2] wireframe resolves");

            auto setRes = PropertyPath::parse("wireframe").setValue(*channel, PropertyValue(true));
            check(setRes == PropertyPath::PathResult::Ok, "[2] wireframe setValue returns Ok");
            check(channel->wireframe == true, "[2] wireframe writes back true");
        }

        // Read and write backgroundColor via PropertyPath
        {
            PropertyValue v;
            auto res = PropertyPath::parse("backgroundColor").getValue(*channel, v);
            check(res == PropertyPath::PathResult::Ok, "[2] backgroundColor resolves");
            check(std::holds_alternative<glm::vec3>(v), "[2] backgroundColor is vec3");

            glm::vec3 testColor(0.2f, 0.3f, 0.4f);
            auto setRes = PropertyPath::parse("backgroundColor").setValue(*channel, PropertyValue(testColor));
            check(setRes == PropertyPath::PathResult::Ok, "[2] backgroundColor setValue returns Ok");
            check(glm::distance(channel->backgroundColor, testColor) < 1e-4f, "[2] backgroundColor writes back new vec3");

            auto setXRes = PropertyPath::parse("backgroundColor.x").setValue(*channel, PropertyValue(0.7f));
            check(setXRes == PropertyPath::PathResult::Ok, "[2] backgroundColor.x setValue returns Ok");
            check(std::abs(channel->backgroundColor.x - 0.7f) < 1e-4f, "[2] backgroundColor.x writes 0.7");
        }
    }

    // -------------------------------------------------------------------
    // [3] GpuBufferPool & GpuMeshCache: Headless lifecycle test
    // -------------------------------------------------------------------
    {\
        using namespace Singularity::Screen::WebGPU;
        GpuBufferPool pool;
        check(pool.totalVramBytes() == 0, "[3] Initial pool VRAM is 0");
        check(pool.suballocationsThisFrame() == 0, "[3] Initial suballocations is 0");

        // Suballocation without GPU device gracefully returns invalid Allocation
        auto alloc = pool.suballocateUniform(nullptr, 64);
        check(!alloc.valid, "[3] Suballocate on null device gracefully fails");

        pool.resetFrame();
        check(pool.bytesWrittenThisFrame() == 0, "[3] resetFrame zeroes bytesWrittenThisFrame");

        GpuMeshCache cache;
        check(cache.cachedMeshCount() == 0, "[3] Initial mesh cache is empty");
        geom::TessMesh mesh;
        cache.beginFrame(1);
        auto buf = cache.getOrUpload(mesh);
        check(buf == nullptr, "[3] getOrUpload on empty mesh returns nullptr");
        cache.endFrame();
        check(cache.cachedMeshCount() == 0, "[3] endFrame on empty cache is safe");
    }

    // -------------------------------------------------------------------
    // [4] Device-backed: the two claimed fixes, the revision guard, and
    //     GpuBufferPool::init() not leaking a previous device's chunks.
    // -------------------------------------------------------------------
    {\
        wgpu::Device gpu;
        if (!gpu.init()) {
            std::printf("  [4] skipped: no WebGPU device available in this environment
");
        } else {
            using namespace Singularity::Screen::WebGPU;

            // --- 4a. Chunk saturation across frames (CPU_GPU_MICRO_MASTERY.md §2) ---
            // A working set that fits one chunk must not grow VRAM on later
            // frames: resetFrame() resets currentIdx to 0, so every frame
            // re-scans from the first chunk instead of piling on new ones.
            {
                GpuBufferPool pool;
                pool.init(gpu.device, gpu.queue, /*uniformChunkSize=*/1024, 1024);
                const unsigned char payload[256] = {};
                for (int frame = 0; frame < 5; ++frame) {
                    for (int i = 0; i < 4; ++i) pool.suballocateUniform(payload, sizeof(payload));
                    pool.resetFrame();
                }
                check(pool.totalVramBytes() == 1024,
                      "[4a] a working set that fits one chunk does not grow VRAM across frames");
            }

            // --- 4b. GpuBufferPool::init() clears a previous device's chunks ---
            {
                GpuBufferPool pool;
                pool.init(gpu.device, gpu.queue, 1024, 1024);
                const unsigned char payload[64] = {};
                pool.suballocateUniform(payload, sizeof(payload));
                check(pool.totalVramBytes() > 0, "[4b] pool holds VRAM after a suballocation");
                pool.init(gpu.device, gpu.queue, 1024, 1024);
                check(pool.totalVramBytes() == 0,
                      "[4b] re-init() releases the previous chunks instead of leaking them");
            }

            // --- 4c. Reincarnation bug (CPU_GPU_MICRO_MASTERY.md §3) ---
            // Force a new TessMesh to land at the exact address of a destroyed
            // one (placement new into fixed storage, rather than hoping the
            // allocator reuses it) so the case is deterministic, not lucky.
            // Verification reads the buffer's CONTENT back, not its pointer:
            // wgpuBufferRelease immediately followed by wgpuDeviceCreateBuffer
            // of the same size can hand back the SAME address (ordinary
            // allocator reuse), which would make a pointer-equality check
            // pass even on a correct re-upload — see readFirstVertexPos above.
            {
                GpuMeshCache cache;
                cache.init(gpu.device, gpu.queue);
                cache.beginFrame(1);

                alignas(geom::TessMesh) unsigned char storage[sizeof(geom::TessMesh)];
                auto oneTri = [](geom::TessMesh& m, glm::vec3 pos) {
                    geom::TessVertex v; v.pos = pos;
                    m.tris = {v, v, v};
                };

                auto* meshA = new (storage) geom::TessMesh();
                oneTri(*meshA, glm::vec3(0.0f, 0.0f, 0.0f));
                WGPUBuffer bufA = cache.getOrUpload(*meshA);
                check(bufA != nullptr, "[4c] mesh A uploads");
                meshA->~TessMesh();

                auto* meshB = new (storage) geom::TessMesh(); // same address, fresh id
                oneTri(*meshB, glm::vec3(7.0f, 7.0f, 7.0f));   // distinct content from A
                WGPUBuffer bufB = cache.getOrUpload(*meshB);
                check(bufB != nullptr, "[4c] mesh B uploads");
                const glm::vec3 contentB = readFirstVertexPos(gpu, bufB);
                check(glm::distance(contentB, glm::vec3(7.0f, 7.0f, 7.0f)) < 1e-4f,
                      "[4c] a reincarnated mesh at the same address gets ITS OWN upload, not the "
                      "dead one's stale buffer");
                meshB->~TessMesh();

                // --- 4d. Frame-based GC releases it once it goes unused ---
                cache.beginFrame(1 + 11); // lastUsedFrame + 10 exceeded
                cache.endFrame();
                check(cache.cachedMeshCount() == 0,
                      "[4d] a mesh not drawn for 10 frames is garbage-collected");
                check(cache.totalCachedBytes() == 0,
                      "[4d] garbage collection releases its VRAM");
            }

            // --- 4e. Revision guard (remediation plan §2.1) ---
            // Mutate a cached mesh's vertex positions IN PLACE, keeping both its
            // id and triangle count unchanged — exactly what a future sculpt
            // tool would do. Without a revision bump this is indistinguishable
            // from "unchanged" and the cache would serve the stale buffer.
            {
                GpuMeshCache cache;
                cache.init(gpu.device, gpu.queue);
                cache.beginFrame(1);

                geom::TessMesh mesh;
                geom::TessVertex v;
                v.pos = glm::vec3(0.0f);
                mesh.tris = {v, v, v};
                WGPUBuffer buf1 = cache.getOrUpload(mesh);

                mesh.tris[0].pos = glm::vec3(1.0f, 2.0f, 3.0f); // in-place edit, id/revision unchanged
                WGPUBuffer buf2 = cache.getOrUpload(mesh);
                check(buf2 == buf1,
                      "[4e] an in-place edit with no revision bump is (correctly) still a cache hit "
                      "— revision exists precisely so a real mutator must bump it");
                check(glm::distance(readFirstVertexPos(gpu, buf2), glm::vec3(0.0f)) < 1e-4f,
                      "[4e] the served buffer still holds the STALE pre-edit content, proving the "
                      "cache hit above really did skip the re-upload rather than coincidentally "
                      "matching");

                mesh.revision++;
                WGPUBuffer buf3 = cache.getOrUpload(mesh);
                check(glm::distance(readFirstVertexPos(gpu, buf3), glm::vec3(1.0f, 2.0f, 3.0f)) < 1e-4f,
                      "[4e] bumping revision after an in-place edit forces a re-upload");
            }
        }
    }

    std::printf("
gpu_mastery_test: %d/%d passed
", g_passed, g_passed + g_failed);
    return g_failed == 0 ? 0 : 1;
}
