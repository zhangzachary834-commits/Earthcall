// Heavy duty CPU-GPU micro-mastery lag test.
//
// This is a duration probe — like frame_lag_test, whose CMakeLists.txt comment
// states the principle this file now follows too: "the lag probe measures
// durations, so it must not be measured while three other tests are fighting
// it for the machine... a run where nothing is enforced is a run that checked
// nothing." CMakeLists.txt gives it RUN_SERIAL for that reason (see the "lag"
// label). This file adds the second half of that principle: even alone on the
// machine, a laptop under a browser tab or a spotlight-index sweep can still
// slow every timing here, and that is lag in the room, not lag in Earthcall.
// So a fixed, allocation-free calibration workload is timed immediately before
// and after the render loop; every duration below is normalised by it, and if
// the machine's speed moved by more than kClockTrustDrift while the loop ran,
// the timing verdict is reported but not allowed to fail the run.
//
// See docs/CPU_GPU_MICRO_MASTERY_REMEDIATION_PLAN.md Phase 0 for why this
// guard has to land before any phase that is judged by this probe's number.

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
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <chrono>
#include <memory>

extern MaterialManager materials;

namespace {

using Clock = std::chrono::steady_clock;

double msSince(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

// Same shape of workload as frame_lag_test's calibrate() (mixed pointer-chasing
// float arithmetic + small-map churn), kept independent rather than shared: the
// two probes measure unrelated subsystems and a shared calibration header would
// only couple them for no benefit. See frame_lag_test.cpp for the rationale.
// Measured on an idle Apple M-series laptop, Debug -O0, 2026-08-25.
constexpr double kReferenceCalibrationMs = 2.0;
constexpr double kClockTrustDrift = 1.4;       // machine moved >40%: don't trust the clock
constexpr double kTimeRegressionTolerance = 1.5; // 50% dearer than the baseline is LAG

double calibrate() {
    constexpr int kN = 1 << 16;
    std::vector<int> hops(kN);
    for (int i = 0; i < kN; ++i) hops[i] = (i * 7919 + 13) & (kN - 1);

    double best = 1e9;
    for (int rep = 0; rep < 3; ++rep) {
        const auto t0 = Clock::now();
        int at = 0;
        double acc = 0.0;
        for (int i = 0; i < kN * 4; ++i) {
            at = hops[at];
            acc += std::sqrt(static_cast<double>(at) + 1.0);
        }
        std::map<std::string, double> churn;
        for (int i = 0; i < 2000; ++i) {
            churn["path.segment." + std::to_string(i & 255)] += acc;
        }
        volatile double sink = acc + churn.size();
        (void)sink;
        best = std::min(best, msSince(t0));
    }
    return best;
}

std::string resolveRepoFile(const std::string& relative) {
    for (const char* prefix : {"", "../", "../../"}) {
        const std::string candidate = std::string(prefix) + relative;
        if (std::filesystem::exists(candidate)) return candidate;
    }
    return relative;
}

const char* kBaselineFile = "tests/singularity/webgpu_micro_mastery_lag_baseline.txt";

std::map<std::string, double> gBaseline;
bool gHaveBaseline = false;

void loadBaseline() {
    std::ifstream in(resolveRepoFile(kBaselineFile));
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        gBaseline[line.substr(0, eq)] = std::strtod(line.c_str() + eq + 1, nullptr);
        gHaveBaseline = true;
    }
}

void writeBaseline(double avgMsNormalised, uint32_t drawCalls, uint32_t suballocations,
                   double vramBytes, uint32_t pipelineSwitches) {
    const std::string path = resolveRepoFile(kBaselineFile);
    std::ofstream out(path);
    if (!out) { std::printf("could not write %s\n", path.c_str()); return; }
    const std::time_t now = std::time(nullptr);
    char stamp[64];
    std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    out << "# webgpu_micro_mastery_lag_test baseline — what the 4,500-object heavy\n"
        << "# probe costs today. Recorded " << stamp << ".\n"
        << "# frame.avg_ms is normalised by this file's calibration factor, so it\n"
        << "# means the same thing on another machine. The other four are exact\n"
        << "# counts for a fixed scene (3000 meshes x 6 faces + 1500 fields) and\n"
        << "# are recorded for the record, not regression-judged here — a change in\n"
        << "# them is either the intended effect of a phase (e.g. Phase 1's pipeline\n"
        << "# cache collapsing pipelineSwitches) or a correctness bug, and either way\n"
        << "# a percentage tolerance on an exact count would hide the wrong thing.\n"
        << "#\n"
        << "# Re-record with:  ./build/webgpu_micro_mastery_lag_test --rebaseline\n"
        << "# Only re-record DOWNWARD, or after a change whose cost was accepted\n"
        << "# deliberately — say which in the commit message.\n"
        << "frame.avg_ms=" << avgMsNormalised << "\n"
        << "frame.drawCalls=" << drawCalls << "\n"
        << "frame.bufferSuballocations=" << suballocations << "\n"
        << "frame.vramAllocatedBytes=" << vramBytes << "\n"
        << "frame.pipelineSwitches=" << pipelineSwitches << "\n";
    std::printf("\nbaseline written to %s\n", path.c_str());
}

} // namespace

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    bool rebaseline = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--rebaseline") rebaseline = true;
    }

    wgpu::Device gpu;
    if (!gpu.init()) { std::printf("FAIL: no WebGPU device\n"); return 1; }

    WebGpuRenderer renderer;
    if (!renderer.init(gpu)) { std::printf("FAIL: renderer init\n"); return 1; }

    setCurrentRenderer(&renderer);

    const uint32_t W = 512, H = 512;
    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D;
    td.size = { W, H, 1 };
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1; td.sampleCount = 1;
    WGPUTexture target = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView view = wgpuTextureCreateView(target, nullptr);

    const glm::vec3 eye(0.0f, 0.0f, 150.0f);
    const glm::mat4 proj = glm::perspectiveZO(glm::radians(45.0f), float(W) / H, 0.1f, 1000.0f);
    const glm::mat4 view3d = glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0, 1, 0));

    std::printf("Creating massive object population...\n");
    std::vector<std::unique_ptr<Object>> meshes;
    for (int i = 0; i < 3000; ++i) {
        auto obj = std::make_unique<Object>("mesh_" + std::to_string(i));
        obj->setShapeKind(Object::ShapeKind::Cube);
        obj->setPosition(glm::vec3( (i % 100) * 1.5f - 75.0f, (i / 100) * 1.5f - 75.0f, 0.0f ));
        meshes.push_back(std::move(obj));
    }

    std::vector<std::unique_ptr<Object>> fields;
    geom::SdfNode sphere;
    sphere.op = geom::SdfOp::Leaf;
    sphere.prim = geom::SdfPrim::Sphere;
    sphere.dims = glm::vec3(0.5f);
    for (int i = 0; i < 1500; ++i) {
        auto obj = std::make_unique<Object>("field_" + std::to_string(i));
        obj->setFieldShape(sphere, 1.0f);
        obj->setPosition(glm::vec3( (i % 50) * 2.0f - 50.0f, (i / 50) * 2.0f - 50.0f, 10.0f ));
        fields.push_back(std::move(obj));
    }

    loadBaseline();
    const double gCalibrationBefore = calibrate() / kReferenceCalibrationMs;
    std::printf("machine calibration: x%.2f versus the reference machine "
                "(>1 means slower; every time below is divided by it)\n", gCalibrationBefore);
    std::printf("baseline: %s\n", gHaveBaseline
                ? resolveRepoFile(kBaselineFile).c_str()
                : "none on disk — this run cannot fail on timing, only record one");

    std::printf("Running frames...\n");
    const int kFrames = 60;

    using ClockT = Clock;
    auto t0 = ClockT::now();

    for (int frame = 0; frame < kFrames; ++frame) {
        renderer.setCamera(view3d, proj, eye);
        renderer.beginFrameOffscreen(view, W, H, glm::vec4(0, 0, 0, 1));

        for (size_t i = 0; i < meshes.size(); ++i) {
            meshes[i]->setRotationEulerDegrees(glm::vec3(frame * 0.5f, frame * 0.5f, 0.0f));
            meshes[i]->drawObject();
        }

        for (size_t i = 0; i < fields.size(); ++i) {
            fields[i]->setRotationEulerDegrees(glm::vec3(frame * -0.5f, frame * -0.5f, 0.0f));
            fields[i]->drawObject();
        }

        renderer.endFrame();

        // A hard wall-clock ceiling independent of calibration: this is a "did
        // the loop hang" tripwire, not a lag verdict, so it stays absolute.
        auto elapsed = std::chrono::duration<double, std::milli>(ClockT::now() - t0).count();
        if (elapsed > 60000.0) {
             const auto& stats = renderer.frameStats();
             std::printf("FAIL: Frame rendering exceeded the hard time ceiling (60s at frame %d).\n", frame);
             std::printf("Failure frame stats:\n");
             std::printf("  drawCalls: %d\n", stats.drawCalls);
             std::printf("  vramAllocatedBytes: %.2f MB\n", stats.vramAllocatedBytes / 1048576.0);
             std::printf("  bufferSuballocations: %d\n", stats.bufferSuballocations);
             setCurrentRenderer(nullptr);
             renderer.shutdown();
             std::fflush(stdout);
             std::_Exit(1);
        }
    }

    auto t1 = ClockT::now();
    double totalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double averageMs = totalMs / kFrames;
    const double averageMsNormalised = averageMs / std::max(0.05, gCalibrationBefore);

    std::printf("Finished %d frames in %.2f ms. Average frame time: %.2f ms (normalised %.2f ms)\n",
                kFrames, totalMs, averageMs, averageMsNormalised);

    const auto& stats = renderer.frameStats();
    std::printf("Last frame stats:\n");
    std::printf("  drawCalls: %d\n", stats.drawCalls);
    std::printf("  vramAllocatedBytes: %.2f MB\n", stats.vramAllocatedBytes / 1048576.0);
    std::printf("  bufferSuballocations: %d\n", stats.bufferSuballocations);
    std::printf("  pipelineSwitches: %d\n", stats.pipelineSwitches);

    // Was the machine steady while the loop ran? Same question frame_lag_test
    // asks: a calibration re-measured after the work, compared to before.
    const double gCalibrationAfter = calibrate() / kReferenceCalibrationMs;
    const double drift = std::max(gCalibrationAfter, gCalibrationBefore) /
                         std::max(1e-6, std::min(gCalibrationAfter, gCalibrationBefore));
    const bool clockTrustworthy = drift <= kClockTrustDrift;
    std::printf("machine steadiness: calibration x%.2f at the start, x%.2f at the end "
                "(drift %.2fx)\n", gCalibrationBefore, gCalibrationAfter, drift);

    bool lag = false;
    const auto found = gBaseline.find("frame.avg_ms");
    if (found != gBaseline.end()) {
        const double limit = found->second * kTimeRegressionTolerance;
        if (averageMsNormalised > limit) {
            std::printf("  LAG       frame.avg_ms = %.3f ms (baseline %.3f, limit %.3f)\n",
                        averageMsNormalised, found->second, limit);
            lag = true;
        } else {
            std::printf("  ok        frame.avg_ms = %.3f ms (baseline %.3f)\n",
                        averageMsNormalised, found->second);
        }
    } else {
        std::printf("  STANDING  frame.avg_ms = %.3f ms  <- no baseline yet; record one with "
                    "--rebaseline\n", averageMsNormalised);
    }

    if (!clockTrustworthy) {
        std::printf("  the machine's speed moved under the measurement (drift %.2fx > %.2fx). "
                    "The timing verdict above is reported, not enforced, this run.\n",
                    drift, kClockTrustDrift);
        lag = false;
    }

    if (rebaseline) {
        if (!clockTrustworthy) {
            std::printf("\n--rebaseline refused: recorded on a contended machine, which sets "
                        "the tripwire at the wrong height. Re-run on a quiet one.\n");
        } else {
            writeBaseline(averageMsNormalised, stats.drawCalls, stats.bufferSuballocations,
                         stats.vramAllocatedBytes, stats.pipelineSwitches);
        }
    }

    if (lag) {
        setCurrentRenderer(nullptr);
        renderer.shutdown();
        std::fflush(stdout);
        std::_Exit(1);
    }

    std::printf("CPU-GPU Micro-Mastery Heavy-Duty Test: PASSED\n");

    setCurrentRenderer(nullptr);
    renderer.shutdown();
    std::fflush(stdout);
    std::_Exit(0);
}
