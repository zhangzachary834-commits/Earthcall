#!/usr/bin/env python3
"""§6ek Multi-Scale Scaling Screen ($T(K) = F + t * K$) & Indirect Fallback Consumer.
Evaluates Modes 0, 1, 3, 4 across K in {1, 2, 4, 8, 16} using active non-blocking poll (waitStyle=1)
and blocking poll (waitStyle=0). Historical DIAGNOSTIC path, not clean performance.
Fits are descriptive; viewport projections below are unmeasured, not frame/FPS evidence.

Antigravity / Gemini 3.8 Flash (Sparkly Guy 🐉✨)
Session: 50cf28df-09e9-4c2b-a4d5-1f796abbc73a
"""

import argparse
import hashlib
import importlib.util
import json
import math
import os
import re
import subprocess
import statistics
import sys
from pathlib import Path


def load(root: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, root / "scratch/probes" / f"{name}.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def generate(root: Path) -> str:
    t6ek = load(root, "ray_transport_gpu_6ek_indirect_consumer")
    source = t6ek.generate(root, mutation="fixed")

    # 1. Parameterize shaders to support variable tile count via arrayLength(&tileOffsets) - 1u
    source = source.replace("var<storage, read> tileOffsets: array<u32, 2305>;", "var<storage, read> tileOffsets: array<u32>;")

    # In kClassifyWgsl: replace hardcoded 2304u bound
    old_classify_bound = "    let tileId = gid.x;\n    if (tileId >= 2304u) { return; }"
    new_classify_bound = "    let tileId = gid.x;\n    let totalTiles = arrayLength(&tileOffsets) - 1u;\n    if (tileId >= totalTiles) { return; }"
    assert source.count(old_classify_bound) == 1
    source = source.replace(old_classify_bound, new_classify_bound)

    # In kRouteArgsWgsl: clamp to total tiles or use counts[id.x] directly
    old_route_args = "    let n = min(counts[id.x], 2304u);"
    new_route_args = "    let n = counts[id.x];"
    assert source.count(old_route_args) == 1
    source = source.replace(old_route_args, new_route_args)

    # In all 5 producer and consumer shaders: inject totalTiles and replace 2304u checks
    old_entry = "    let localTile = lid >> 4u;"
    new_entry = "    let totalTiles = arrayLength(&tileOffsets) - 1u;\n    let localTile = lid >> 4u;"
    assert source.count(old_entry) == 5
    source = source.replace(old_entry, new_entry)

    source = source.replace("min(routeCounts.x, 2304u)", "min(routeCounts.x, totalTiles)")
    source = source.replace("min(routeCounts[1u], 2304u)", "min(routeCounts[1u], totalTiles)")
    source = source.replace("min(routeCounts.y, 2304u)", "min(routeCounts.y, totalTiles)")
    source = source.replace("min(routeCounts[2u], 2304u)", "min(routeCounts[2u], totalTiles)")
    source = source.replace("if (tileId >= 2304u) { return; }", "if (tileId >= totalTiles) { return; }")

    # 2. In C++ host code: scale rays and tile structures by scale K
    needle_ray_load = """    std::map<uint32_t, std::vector<uint32_t>> groups4x4;
    for (size_t i = 0; i < count; ++i) {
        int px = R[i][0].get<int>(), py = R[i][1].get<int>();
        int gx = px / 10, gy = py / 10;
        uint32_t tx = gx / 4, ty = gy / 4;
        uint32_t key = (ty << 16) | tx;
        groups4x4[key].push_back(uint32_t(i));
    }
    if (groups4x4.size() != 2304u) throw std::runtime_error("pinned tile layout requires exactly 2304 tiles");
    for (const auto& [key, indices] : groups4x4) {
        if (indices.empty() || indices.size() > 16u) throw std::runtime_error("pinned tile capacity exceeded");
    }
    std::vector<uint32_t> tileOffsets(2305, 0);
    std::vector<uint32_t> tileRayIndices;
    tileRayIndices.reserve(count);
    size_t tIdx = 0;
    for (const auto& [k, rVec] : groups4x4) {
        tileOffsets[tIdx] = uint32_t(tileRayIndices.size());
        tileRayIndices.insert(tileRayIndices.end(), rVec.begin(), rVec.end());
        tIdx++;
    }
    tileOffsets[2304] = uint32_t(tileRayIndices.size());

    // Schedule buffers
    std::vector<GpuRayHeader> gpuRays(count);
    std::vector<GpuSubpanel> gpuSubpanels;
    std::vector<GpuOverlap> gpuOverlaps;

    uint32_t maxSubpanelsPerRay = 0;


    std::vector<uint32_t> producerFlags(count,0);
    for(size_t i=0;i<count;++i){
        const auto& row=R[i];auto& ray=gpuRays[i];
        for(int k=0;k<3;++k){ray.ro[k]=float(ro[k]);ray.rd[k]=row[2+k];}
        ray.clipA=row[5];ray.clipB=row[6];
    }"""

    scaled_ray_load = """    std::map<uint32_t, std::vector<uint32_t>> groups4x4;
    for (size_t i = 0; i < count; ++i) {
        int px = R[i][0].get<int>(), py = R[i][1].get<int>();
        int gx = px / 10, gy = py / 10;
        uint32_t tx = gx / 4, ty = gy / 4;
        uint32_t key = (ty << 16) | tx;
        groups4x4[key].push_back(uint32_t(i));
    }
    if (groups4x4.size() != 2304u) throw std::runtime_error("pinned tile layout requires exactly 2304 tiles");
    std::vector<uint32_t> baseTileOffsets(2305, 0);
    std::vector<uint32_t> baseTileRayIndices;
    baseTileRayIndices.reserve(count);
    size_t baseTIdx = 0;
    for (const auto& [k, rVec] : groups4x4) {
        baseTileOffsets[baseTIdx] = uint32_t(baseTileRayIndices.size());
        baseTileRayIndices.insert(baseTileRayIndices.end(), rVec.begin(), rVec.end());
        baseTIdx++;
    }
    baseTileOffsets[2304] = uint32_t(baseTileRayIndices.size());

    uint32_t scaleK = 1;
    if (const char* envK = std::getenv("EC_GPU_SCALE")) {
        scaleK = uint32_t(std::atoi(envK));
        if (scaleK == 0) scaleK = 1;
    }
    uint32_t baseRayCount = count;
    uint32_t baseTileCount = 2304;
    count = baseRayCount * scaleK;
    uint32_t numTiles = baseTileCount * scaleK;

    std::cerr << "Scale K=" << scaleK << " -> Rays=" << count << " (" << count * sizeof(GpuRayHeader) / 1048576.0 << " MB), Tiles=" << numTiles << std::endl;

    // Schedule buffers
    std::vector<GpuRayHeader> gpuRays(count);
    std::vector<GpuSubpanel> gpuSubpanels;
    std::vector<GpuOverlap> gpuOverlaps;
    uint32_t maxSubpanelsPerRay = 0;
    std::vector<uint32_t> producerFlags(count, 0);

    std::vector<uint32_t> tileOffsets(numTiles + 1, 0);
    std::vector<uint32_t> tileRayIndices;
    tileRayIndices.reserve(count);

    for (uint32_t c = 0; c < scaleK; ++c) {
        float jx = (c == 0) ? 0.0f : (std::fmod(float(c) * 0.6180339887f, 1.0f) - 0.5f) * 0.0005f;
        float jy = (c == 0) ? 0.0f : (std::fmod(float(c) * 0.4142135623f, 1.0f) - 0.5f) * 0.0005f;
        for (uint32_t i = 0; i < baseRayCount; ++i) {
            uint32_t rIdx = c * baseRayCount + i;
            const auto& row = R[i];
            auto& ray = gpuRays[rIdx];
            for (int k = 0; k < 3; ++k) {
                ray.ro[k] = float(ro[k]);
                ray.rd[k] = row[2 + k];
            }
            if (c > 0) {
                ray.rd[0] += jx;
                ray.rd[1] += jy;
            }
            ray.clipA = row[5];
            ray.clipB = row[6];
        }
        for (uint32_t t = 0; t < baseTileCount; ++t) {
            uint32_t tId = c * baseTileCount + t;
            tileOffsets[tId] = uint32_t(tileRayIndices.size());
            uint32_t rStart = baseTileOffsets[t];
            uint32_t rEnd = baseTileOffsets[t + 1];
            for (uint32_t r = rStart; r < rEnd; ++r) {
                tileRayIndices.push_back(c * baseRayCount + baseTileRayIndices[r]);
            }
        }
    }
    tileOffsets[numTiles] = uint32_t(tileRayIndices.size());"""

    assert source.count(needle_ray_load) == 1
    source = source.replace(needle_ray_load, scaled_ray_load)

    # 3. Scale buffers in C++:
    old_tile_bufs = """    std::vector<uint32_t> initList(2304, 0u);
    auto s1TileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());
    auto s2TileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());
    auto fbTileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, 2304 * sizeof(uint32_t), initList.data());"""

    new_tile_bufs = """    std::vector<uint32_t> initList(numTiles, 0u);
    auto s1TileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, numTiles * sizeof(uint32_t), initList.data());
    auto s2TileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, numTiles * sizeof(uint32_t), initList.data());
    auto fbTileBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst, numTiles * sizeof(uint32_t), initList.data());"""

    assert source.count(old_tile_bufs) == 1
    source = source.replace(old_tile_bufs, new_tile_bufs)

    # Replace bind group sizes with numTiles * 4
    source = source.replace("makeEntry(4, s1TileBuf, 0, 2304 * 4)", "makeEntry(4, s1TileBuf, 0, numTiles * 4)")
    source = source.replace("makeEntry(5, s2TileBuf, 0, 2304 * 4)", "makeEntry(5, s2TileBuf, 0, numTiles * 4)")
    source = source.replace("makeEntry(6, fbTileBuf, 0, 2304 * 4)", "makeEntry(6, fbTileBuf, 0, numTiles * 4)")
    source = source.replace("makeEntry(7, s1TileBuf, 0, 2304 * 4)", "makeEntry(7, s1TileBuf, 0, numTiles * 4)")
    source = source.replace("makeEntry(7, s2TileBuf, 0, 2304 * 4)", "makeEntry(7, s2TileBuf, 0, numTiles * 4)")
    source = source.replace("makeEntry(7, fbTileBuf, 0, 2304 * 4)", "makeEntry(7, fbTileBuf, 0, numTiles * 4)")
    source = source.replace("makeEntry(10, fbTileBuf, 0, 2304 * 4)", "makeEntry(10, fbTileBuf, 0, numTiles * 4)")

    # Replace cpClassify dispatch workgroups with (numTiles + 63) / 64
    old_classify_dispatch = "wgpuComputePassEncoderDispatchWorkgroups(p, 36, 1, 1);"
    new_classify_dispatch = "wgpuComputePassEncoderDispatchWorkgroups(p, (numTiles + 63u) / 64u, 1, 1);"
    assert source.count(old_classify_dispatch) >= 1
    source = source.replace(old_classify_dispatch, new_classify_dispatch)

    # Replace counter accumulation check:
    old_counter_check = "if(c.s1+c.s2+c.fb!=2304u)throw std::runtime_error(\"routed counter accumulation mismatch\");"
    new_counter_check = "if(c.s1+c.s2+c.fb!=numTiles)throw std::runtime_error(\"routed counter accumulation mismatch\");"
    assert source.count(old_counter_check) == 1
    source = source.replace(old_counter_check, new_counter_check)


    # 4. Inject active poll (waitStyle=1) & blocking poll (waitStyle=0) in execute
    start_ex = source.index("        auto execute=")
    end_ex = source.index("        auto correctness=", start_ex)
    execute_code = source[start_ex:end_ex]

    poll_hook = "wgpuDevicePoll(gpu.device,true,nullptr);auto end=std::chrono::steady_clock::now();"
    assert execute_code.count(poll_hook) == 1
    poll_replacement = """lastPolls = 0;
            if (waitStyle == 0) {
                wgpuDevicePoll(gpu.device, true, nullptr);
                lastPolls = 1;
            } else {
                while (!wgpuDevicePoll(gpu.device, false, nullptr)) {
                    ++lastPolls;
                    if (std::chrono::steady_clock::now() - submitted > std::chrono::seconds(30)) {
                        throw std::runtime_error("active completion timeout");
                    }
                }
                ++lastPolls;
            }
            auto end = std::chrono::steady_clock::now();"""
    execute_code = execute_code.replace(poll_hook, poll_replacement)
    source = source[:start_ex] + "        int waitStyle = 1; uint64_t lastPolls = 0;\n" + execute_code + source[end_ex:]

    # Replace count with baseRayCount in 5 legacy verification loops that index R[i] or resFused[i]
    p1 = "for(size_t i=0;i<count;++i) {\n        currentDeltaT=std::max(currentDeltaT,std::abs(double(resCurrent[i].rgb_T[3])-R[i][7].get<double>()));"
    assert source.count(p1) == 1
    source = source.replace(p1, p1.replace("count;", "baseRayCount;"))

    p2 = "if(!outputPath.empty()) {J a=J::array();for(size_t i=0;i<count;++i)a.push_back({R[i][0]"
    assert source.count(p2) == 1
    source = source.replace(p2, p2.replace("count;", "baseRayCount;"))

    p3 = "for (size_t i = 0; i < count; ++i) {\n        double dR = std::abs(resFused[i].rgb_T[0]"
    assert source.count(p3) == 1
    source = source.replace(p3, p3.replace("count;", "baseRayCount;"))

    p4 = "for(size_t i=0;i<count;++i)raw.push_back({R[i][0]"
    assert source.count(p4) == 1
    source = source.replace(p4, p4.replace("count;", "baseRayCount;"))

    p5 = "for(size_t i=0;i<count;++i){\n        std::array<std::vector<glm::vec2>,4> spans;const auto& ray=gpuRays[i];"
    assert source.count(p5) == 1
    source = source.replace(p5, p5.replace("count;", "baseRayCount;"))

    # Gate all 30 matrix checks to scaleK == 1 only
    old_matrix_start = "for(int pose:{0,1,0,2,3,0}){"
    new_matrix_start = "if (scaleK == 1) {\n        for(int pose:{0,1,0,2,3,0}){"
    assert source.count(old_matrix_start) == 1
    source = source.replace(old_matrix_start, new_matrix_start)

    old_matrix_end = """for(int kind:{0,1}){auto limited=cfg;if(kind==0)limited.maxSubPerRay=4;else limited.maxOverPerRay=1;wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&limited,sizeof(limited));checkReview("capacity-"+std::to_string(kind),1,false);}
        wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&cfg,sizeof(cfg));"""
    new_matrix_end = """for(int kind:{0,1}){auto limited=cfg;if(kind==0)limited.maxSubPerRay=4;else limited.maxOverPerRay=1;wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&limited,sizeof(limited));checkReview("capacity-"+std::to_string(kind),1,false);}
        wgpuQueueWriteBuffer(gpu.queue,cfgBuf,0,&cfg,sizeof(cfg));
        }"""
    assert source.count(old_matrix_end) == 1
    source = source.replace(old_matrix_end, new_matrix_end)

    # 5. Inject Multi-Scale screen timing section replacing the old reviewOut return 0;
    # The qualification writer now includes a route census; replace its whole
    # return block rather than matching the historical single-line JSON shape.
    output_start = source.index("std::ofstream reviewOut(outputPath);")
    output_end = source.index("return 0;", output_start) + len("return 0;")

    timing_section = """std::cerr << "Starting Multi-Scale Screen for K=" << scaleK << " (Rays=" << count << ")..." << std::endl;
        const std::vector<int> activeModes = {0, 1, 3, 4};

        // Warmup: 1 run for each mode under each wait style
        for (int style : {0, 1}) {
            waitStyle = style;
            for (int m : activeModes) {
                execute(m, 1, false);
            }
        }

        J observations = J::array();
        // 4 balanced measurement blocks with alternating mode order
        for (int block = 0; block < 4; ++block) {
            auto order = activeModes;
            if (block % 2 == 1) {
                std::reverse(order.begin(), order.end());
            }
            for (int m : order) {
                // Measure both wait styles for each mode
                std::vector<int> styles = (block % 2 == 0) ? std::vector<int>{1, 0} : std::vector<int>{0, 1};
                for (int style : styles) {
                    waitStyle = style;
                    auto t = execute(m, 1, false);
                    observations.push_back({
                        {"scale_k", scaleK},
                        {"rays", count},
                        {"mode", m},
                        {"wait_style", waitStyle},
                        {"block", block},
                        {"encode_ms", t[0]},
                        {"completion_ms", t[1]},
                        {"total_ms", t[2]},
                        {"poll_calls", lastPolls}
                    });
                }
            }
        }

        J scaleReport = {
            {"scope", "Multi-Scale Screen T(K) = F + t*K across Modes {0, 1, 3, 4}"},
            {"scale_k", scaleK},
            {"workload_rays", count},
            {"num_tiles", numTiles},
            {"active_modes", activeModes},
            {"observations", observations}
        };

        std::ofstream(outputPath) << scaleReport.dump(2) << "\\n";
        std::cerr << "Scale K=" << scaleK << " complete. Saved " << observations.size() << " timing observations." << std::endl;
        return 0;"""
    source = source[:output_start] + timing_section + source[output_end:]

    return source


def linear_fit(xs, ys):
    """Fits y = F + t * x via ordinary least squares."""
    n = len(xs)
    mean_x = sum(xs) / n
    mean_y = sum(ys) / n
    num = sum((xs[i] - mean_x) * (ys[i] - mean_y) for i in range(n))
    den = sum((xs[i] - mean_x) ** 2 for i in range(n))
    slope = num / den if den != 0 else 0.0
    intercept = mean_y - slope * mean_x
    
    # Compute R^2
    ss_tot = sum((ys[i] - mean_y) ** 2 for i in range(n))
    ss_res = sum((ys[i] - (intercept + slope * xs[i])) ** 2 for i in range(n))
    r2 = 1.0 - (ss_res / ss_tot) if ss_tot != 0 else 1.0
    return intercept, slope, r2


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root", type=Path, default=Path("/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport"))
    p.add_argument("--build-root", type=Path, default=Path("/Users/zacharyzhang/.gemini/antigravity/worktrees/Earthcall/ourverse_navigator_prototype_loop"))
    p.add_argument("--work", type=Path, default=Path("/private/tmp/earthcall-6ek-ms-run"))
    p.add_argument("--scale", type=int, default=1)
    p.add_argument("--all-scales", action="store_true")
    p.add_argument("--run", action="store_true")
    args = p.parse_args()

    root = args.root.resolve()
    build_root = args.build_root.resolve()
    worktree_root = build_root.parent if build_root.name == "build" else build_root
    work = args.work.resolve()

    if args.all_scales:
        # Run across K in {1, 2, 4, 8, 16}
        scales = [1, 2, 4, 8, 16]
        all_obs = []
        # Preserve prior evidence; use an explicitly fresh external run root.
        if work.exists() or work == root or root in work.parents:
            raise ValueError("fresh external work required; existing directories are never deleted")
        work.mkdir(parents=True, exist_ok=False)

        for k in scales:
            k_work = work / f"K{k}"

            print(f"\n==========================================")
            print(f"=== Compiling & Running Scale K={k} ({k * 32784:,} rays) ===")
            print(f"==========================================")

            cmd = [
                sys.executable, "-B", str(root / "scratch/probes/ray_transport_gpu_6ek_multi_scale.py"),
                "--root", str(root),
                "--build-root", str(worktree_root),
                "--work", str(k_work),
                "--scale", str(k),
                "--run"
            ]
            res = subprocess.run(cmd, check=True)

            rep_file = k_work / "scale_review.json"
            assert rep_file.exists(), f"Missing output for scale K={k}"
            rep = json.load(rep_file.open())
            all_obs.extend(rep["observations"])

        # Aggregate and analyze data
        print("\n==========================================")
        print("=== MULTI-SCALE SCALING SCREEN RESULTS ===")
        print("==========================================")

        modes = [0, 1, 3, 4]
        mode_names = {
            0: "Mode 0: Current Monolithic Baseline",
            1: "Mode 1: Bulk Bisection",
            3: "Mode 3: Terminal S1",
            4: "Mode 4: Terminal S1+S2+Indirect Consumer"
        }

        analysis = {"scales": scales, "fits": {}}

        for style, style_name in [(1, "Active Poll (waitStyle=1)"), (0, "Blocking Poll (waitStyle=0)")]:
            print(f"\n### {style_name}")
            print(f"| Mode | K=1 (32k) | K=2 (65k) | K=4 (131k) | K=8 (262k) | K=16 (524k) | OLS intercept (ms) | t (Slope ms/K) | R^2 | K=112.5 (2560x1440) | Projected FPS |")
            print(f"|---|---|---|---|---|---|---|---|---|---|---|")

            style_fits = {}
            for m in modes:
                # Group by scale K, compute median total_ms and median completion_ms
                k_medians = []
                for k in scales:
                    samples = [o["completion_ms"] for o in all_obs if o["scale_k"] == k and o["mode"] == m and o["wait_style"] == style]
                    samples.sort()
                    med = statistics.median(samples)
                    k_medians.append(med)

                F, t, r2 = linear_fit(scales, k_medians)
                # Projected at K=112.5 (3,686,400 rays)
                t_112_5 = F + t * 112.5
                fps = 1000.0 / t_112_5 if t_112_5 > 0 else 0.0

                k_strs = [f"{med:.3f} ms" for med in k_medians]
                print(f"| {mode_names[m]} | {k_strs[0]} | {k_strs[1]} | {k_strs[2]} | {k_strs[3]} | {k_strs[4]} | {F:.4f} ms | {t:.4f} ms | {r2:.4f} | {t_112_5:.2f} ms | {fps:.1f} FPS |")

                style_fits[str(m)] = {
                    "mode_name": mode_names[m],
                    "medians_per_k": dict(zip(scales, k_medians)),
                    "OLS_intercept_ms": F,
                    "t_slope_ms_per_k": t,
                    "r2": r2,
                    "projected_cost_112_5_ms": t_112_5,
                    "projected_fps_112_5": fps
                }

            analysis["fits"][f"waitStyle_{style}"] = style_fits

            # Compute crossover between Mode 4 and Mode 0:
            fit0 = style_fits["0"]
            fit4 = style_fits["4"]
            fit3 = style_fits["3"]

            delta_t_4_vs_0 = fit0["t_slope_ms_per_k"] - fit4["t_slope_ms_per_k"]
            delta_F_4_vs_0 = fit4["OLS_intercept_ms"] - fit0["OLS_intercept_ms"]

            print(f"\nMarginal Slope Comparison for {style_name}:")
            print(f"  Mode 0 slope: {fit0['t_slope_ms_per_k']:.4f} ms/K ({fit0['t_slope_ms_per_k']/32784.0*1e6:.2f} ns/ray)")
            print(f"  Mode 3 slope: {fit3['t_slope_ms_per_k']:.4f} ms/K ({fit3['t_slope_ms_per_k']/32784.0*1e6:.2f} ns/ray)")
            print(f"  Mode 4 slope: {fit4['t_slope_ms_per_k']:.4f} ms/K ({fit4['t_slope_ms_per_k']/32784.0*1e6:.2f} ns/ray)")
            print(f"  Mode 4 vs Mode 0 marginal ratio: {fit4['t_slope_ms_per_k'] / fit0['t_slope_ms_per_k']:.3f}x")
            print(f"  Mode 4 vs Mode 3 marginal ratio: {fit4['t_slope_ms_per_k'] / fit3['t_slope_ms_per_k']:.3f}x")

            if delta_t_4_vs_0 > 0:
                k_star = delta_F_4_vs_0 / delta_t_4_vs_0
                n_star = k_star * 32784
                print(f"  Crossover point: K* = {k_star:.2f} (N* = {n_star:,.0f} rays)")
                if k_star < 112.5:
                    print(f"  => Unmeasured linear extrapolation only; no viewport/FPS qualification (Projected {fit4['projected_fps_112_5']:.1f} FPS vs {fit0['projected_fps_112_5']:.1f} FPS)")
                else:
                    print(f"  => Crossover K* ({k_star:.1f}) is beyond 2560x1440 (K=112.5).")
            else:
                print(f"  Mode 4 marginal slope ({fit4['t_slope_ms_per_k']:.4f}) does not beat Mode 0 ({fit0['t_slope_ms_per_k']:.4f}). Barrier delta is {delta_F_4_vs_0:.4f} ms.")

        # Save JSON output
        out_json_path = work / "analysis.json"
        analysis["raw_observations"] = all_obs
        out_json_path.write_text(json.dumps(analysis, indent=2))
        print(f"\nSaved Multi-Scale Screen JSON to {out_json_path}")
        return

    # Single scale run
    if work.exists() or work == root or root in work.parents:
        raise ValueError("fresh external work required; existing directories are never deleted")
    work.mkdir(parents=True, exist_ok=False)

    print(f"Generating Multi-Scale probe source for scale K={args.scale}...")
    cpp_source = generate(root)
    (work / "probe.cpp").write_text(cpp_source)

    build_dir = work / "build"
    subprocess.run([
        sys.executable, "-B", str(root / "scratch/probes/ray_transport_probe_build.py"),
        "--root", str(root),
        "--build-root", str(worktree_root),
        "--source", str(work / "probe.cpp"),
        "--work", str(build_dir)
    ], check=True)
    print("Compilation successful!")

    if args.run:
        spec = importlib.util.spec_from_file_location("wide", root / "scratch/probes/ray_transport_gpu_wide_stream.py")
        m = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(m)
        zone = root / "saves/zones/Northern Veil/zone.ecform"
        before = hashlib.sha256(zone.read_bytes()).hexdigest()
        case_path = Path("/private/tmp/earthcall-wide-case/manifest.json").resolve()
        meta, rays, _ = m.verify_case(root, case_path, before)
        env = dict(
            os.environ,
            EC_POSE_RAYS=meta["rays"],
            EC_POSE_REFERENCE=meta["reference"],
            EC_POSE_CLOCK=str(rays["time"]),
            EC_MATCH_REPEAT="1",
            EC_GPU_BUDGET="1024",
            EC_GPU_SUB_CAP="64",
            EC_GPU_OVER_CAP="16",
            EC_GPU_MARGIN=".001",
            EC_GPU_SCALE=str(args.scale),
        )
        env.pop("EC_GPU_GEOMETRY_CASE", None)
        env.pop("METAL_CAPTURE_ENABLED", None)

        output = work / "scale_review.json"
        print(f"Executing Multi-Scale probe for K={args.scale}...")
        result = subprocess.run([str(build_dir / "probe"), str(output)], cwd=root, env=env)
        if result.returncode != 0:
            print(f"Probe exited with code {result.returncode}")
            sys.exit(result.returncode)
        print(f"PASS Multi-Scale probe for K={args.scale}!")


if __name__ == "__main__":
    main()
