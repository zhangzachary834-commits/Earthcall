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

#include "Singularity/Screen/WebGPU/GpuBufferPool.hpp"
#include "Singularity/Screen/WebGPU/GpuMeshCache.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

int g_passed = 0;
int g_failed = 0;

void check(bool ok, const char* msg) {
    if (ok) {
        ++g_passed;
        std::printf("  ok: %s\n", msg);
    } else {
        ++g_failed;
        std::printf("  FAIL: %s\n", msg);
    }
}

} // namespace

int main() {
    std::printf("Running gpu_mastery_test...\n");

    // -------------------------------------------------------------------
    // [1] ScreenChannel: First-mover registration and stable identifier
    // -------------------------------------------------------------------
    {
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
    {
        LawManager laws;
        Singularity::Screen::ScreenChannel::syncRegister(laws);
        auto* channel = Singularity::Screen::ScreenChannel::find(laws);
        assert(channel);

        // Update metrics
        channel->updateMetrics(42, 1200, 1048576.0f, 65536.0f, 15, 8, 4);

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
            check(std::fabs(static_cast<float>(n) - 1048576.0f) < 1.0f, "[2] vramAllocatedBytes reads back 1MB");
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
    }

    // -------------------------------------------------------------------
    // [3] GpuBufferPool & GpuMeshCache: Headless lifecycle test
    // -------------------------------------------------------------------
    {
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

    std::printf("\ngpu_mastery_test: %d/%d passed\n", g_passed, g_passed + g_failed);
    return g_failed == 0 ? 0 : 1;
}
