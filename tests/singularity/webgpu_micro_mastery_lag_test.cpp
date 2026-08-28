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
#include "Singularity/OntoMath/ScalarForm.hpp"
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

// ---------------------------------------------------------------------------
// "Ultra power" stress shapes (EARTHCALL_ULTRA_POWER=1 only — see main()).
// These never appear in the standard run, so the tracked baseline above
// stays a fixed, comparable scene no matter what this section grows into.
// ---------------------------------------------------------------------------

// SmoothUnion CSG: a box and a sphere melted into one surface, not just
// placed side by side — the case that actually exercises SdfOp::SmoothUnion
// composition (children + blend factor `t`), not merely a Leaf primitive.
geom::SdfNode makeMeltedBoxSphere() {
    auto box = std::make_shared<geom::SdfNode>();
    box->op = geom::SdfOp::Leaf;
    box->prim = geom::SdfPrim::Box;
    box->dims = glm::vec3(0.4f);

    auto sphere = std::make_shared<geom::SdfNode>();
    sphere->op = geom::SdfOp::Leaf;
    sphere->prim = geom::SdfPrim::Sphere;
    sphere->dims = glm::vec3(0.35f);
    sphere->offset = glm::vec3(0.3f, 0.0f, 0.0f);

    geom::SdfNode melt;
    melt.op = geom::SdfOp::SmoothUnion;
    melt.t = 0.25f;
    melt.children = { box, sphere };
    return melt;
}

// A genuinely OntoMath::Piecewise-driven surface, not the plain compiled-expr
// path makeImplicit(string) alone would exercise: the string compiles to a
// MathNode (sin/cos/add — real algebraic composition, "wildly intricate" in
// the sense the remediation plan asked for), then Piecewise::continuous wraps
// it so SdfNode::piecewise (not just ::mathNode) is what the field actually
// carries — see Sdf.cpp's evaluation of `n.piecewise` at raymarch/tessellate
// time.
geom::SdfNode makeSinusoidalManifold() {
    const geom::SdfNode expr = geom::makeImplicit(std::string("sin(x*4)+cos(y*4)+sin(z*4)-0.3"));
    auto pw = std::make_shared<OntoMath::Piecewise>(OntoMath::Piecewise::continuous(expr.mathNode));
    return geom::makeImplicit(pw);
}

// Every 4th field object cycles through: a plain sphere (the control), the
// CSG melt, the Piecewise manifold, and back to a plain sphere at an extreme
// extent — the "scale(1000) beside scale(0.01)" case from the plan, expressed
// as the field's bounding extent (a Field object's actual scale knob; Object
// has no general setScale) rather than an invented transform API.
geom::SdfNode stressFieldShape(int i, float& extentOut) {
    extentOut = 1.0f;
    switch (i % 4) {
        case 0: {
            geom::SdfNode s;
            s.op = geom::SdfOp::Leaf; s.prim = geom::SdfPrim::Sphere; s.dims = glm::vec3(0.5f);
            return s;
        }
        case 1:
            return makeMeltedBoxSphere();
        case 2:
            return makeSinusoidalManifold();
        default: {
            geom::SdfNode s;
            s.op = geom::SdfOp::Leaf; s.prim = geom::SdfPrim::Sphere; s.dims = glm::vec3(0.5f);
            // Alternate extreme extents so the population carries both a
            // 1000x-oversized and a 0.01x-undersized field, per frame.
            extentOut = (i % 8 == 3) ? 1000.0f : 0.01f;
            return s;
        }
    }
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

    // Ultra power: an opt-in 10x-plus population with the same shapes as the
    // standard scene PLUS the CSG-melt / Piecewise-manifold / extreme-extent
    // edge cases above. Never runs by default — ctest stays exactly the fast,
    // baseline-tracked probe it always was; a Person asks for this by hand:
    //   EARTHCALL_ULTRA_POWER=1 ctest -R webgpu_micro_mastery_lag_test -V
    // The mesh/field split is deliberately asymmetric under ultra power, not
    // a straight 10x of both: Object::setFieldShape EAGERLY tessellates
    // (Object::rebuildGeometryCaches, unconditionally, pre-existing engine
    // behavior this session did not touch) even though WebGPU never reads
    // that mesh for a Field object (drawFieldModel raymarches it exactly).
    // Measured on this machine: ~30-100ms PER FIELD OBJECT depending on shape
    // complexity — 25,000 of them would cost 15+ minutes on construction
    // alone before a single frame renders, dominated by a cost this test
    // isn't even trying to measure. Cubes pay none of this (rebuildGeometryCaches
    // has no branch for ShapeKind::Cube), so that's where "ultra" scale
    // actually stresses what Phase 4.3 changed — the instanced batch path.
    const char* ultraEnv = std::getenv("EARTHCALL_ULTRA_POWER");
    const bool ultraPower = ultraEnv && ultraEnv[0] != '\0' && std::string(ultraEnv) != "0";
    const int kMeshCount  = ultraPower ? 48000 : 3000;
    const int kFieldCount = ultraPower ? 2000 : 1500;
    const int kFrames     = ultraPower ? 20 : 60; // fewer frames: this is a manual run, not ctest's

    std::printf("Creating massive object population (%s: %d meshes, %d fields)...\n",
                ultraPower ? "ULTRA POWER" : "standard", kMeshCount, kFieldCount);
    std::vector<std::unique_ptr<Object>> meshes;
    meshes.reserve(kMeshCount);
    for (int i = 0; i < kMeshCount; ++i) {
        auto obj = std::make_unique<Object>("mesh_" + std::to_string(i));
        obj->setShapeKind(Object::ShapeKind::Cube);
        const int side = static_cast<int>(std::sqrt(static_cast<double>(kMeshCount))) + 1;
        obj->setPosition(glm::vec3( (i % side) * 1.5f - side * 0.75f,
                                    (i / side) * 1.5f - side * 0.75f, 0.0f ));
        meshes.push_back(std::move(obj));
    }

    std::vector<std::unique_ptr<Object>> fields;
    fields.reserve(kFieldCount);
    const int fieldSide = static_cast<int>(std::sqrt(static_cast<double>(kFieldCount))) + 1;
    for (int i = 0; i < kFieldCount; ++i) {
        auto obj = std::make_unique<Object>("field_" + std::to_string(i));
        if (ultraPower) {
            float extent = 1.0f;
            const geom::SdfNode shape = stressFieldShape(i, extent);
            obj->setFieldShape(shape, glm::vec3(extent));
        } else {
            geom::SdfNode sphere;
            sphere.op = geom::SdfOp::Leaf;
            sphere.prim = geom::SdfPrim::Sphere;
            sphere.dims = glm::vec3(0.5f);
            obj->setFieldShape(sphere, glm::vec3(1.0f));
        }
        obj->setPosition(glm::vec3( (i % fieldSide) * 2.0f - fieldSide, (i / fieldSide) * 2.0f - fieldSide, 10.0f ));
        fields.push_back(std::move(obj));
    }

    if (ultraPower) {
        // No baseline judgment: the tracked baseline is the standard scene's
        // number, and comparing a 50,000-object frame against a 4,500-object
        // baseline is exactly the category error Phase 0 exists to prevent —
        // "never quiet a STANDING line by widening the baseline" applies just
        // as much to comparing it against the wrong scene entirely. This run
        // reports what it measures and asserts only that nothing crashed or
        // hung; it is not a regression gate.
        std::printf("ULTRA POWER: informational only — not compared to the standard baseline.\n");
    }

    loadBaseline();
    const double gCalibrationBefore = calibrate() / kReferenceCalibrationMs;
    std::printf("machine calibration: x%.2f versus the reference machine "
                "(>1 means slower; every time below is divided by it)\n", gCalibrationBefore);
    std::printf("baseline: %s\n", gHaveBaseline
                ? resolveRepoFile(kBaselineFile).c_str()
                : "none on disk — this run cannot fail on timing, only record one");

    std::printf("Running frames...\n");

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
        // Ultra power gets a proportionally longer leash — 25000+25000 CSG/
        // Piecewise-heavy objects legitimately cost more CPU per frame than
        // the standard scene, and this ceiling exists to catch a hang, not to
        // re-litigate whether ultra power is fast enough (it isn't a gate).
        const double hardCeilingMs = ultraPower ? 600000.0 : 60000.0;
        auto elapsed = std::chrono::duration<double, std::milli>(ClockT::now() - t0).count();
        if (elapsed > hardCeilingMs) {
             const auto& stats = renderer.frameStats();
             std::printf("FAIL: Frame rendering exceeded the hard time ceiling (%.0fs at frame %d).\n",
                         hardCeilingMs / 1000.0, frame);
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
    if (ultraPower) {
        // Informational only — see the note printed when the population was
        // built. Comparing this number to the standard scene's baseline would
        // fail every single run for having 16x the objects, not for lag.
        std::printf("  --        frame.avg_ms = %.3f ms  (ULTRA POWER: not judged against "
                    "the standard-scene baseline)\n", averageMsNormalised);
    } else {
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
    }

    if (!clockTrustworthy) {
        std::printf("  the machine's speed moved under the measurement (drift %.2fx > %.2fx). "
                    "The timing verdict above is reported, not enforced, this run.\n",
                    drift, kClockTrustDrift);
        lag = false;
    }

    if (rebaseline) {
        if (ultraPower) {
            std::printf("\n--rebaseline ignored under EARTHCALL_ULTRA_POWER: the baseline file "
                        "records the standard scene, not this one.\n");
        } else if (!clockTrustworthy) {
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
