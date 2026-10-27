"""Ray Transport GPU 6eh Clean Complete-Cost Screen.

Evaluates 5 resident execution lanes on Apple M5:
- Lane 0: Current Baseline (currentCP)
- Lane 1: Unchanged Bulk Bisection (cpProducer + cpUnfused + fallbackCP)
- Lane 2: Unchanged Routed Bisection (cpReset + cpClassify + cpArgs + cpS1Unfused + cpS2 + cpFallback + cpUnfused + fallbackCP)
- Lane 3: Unchanged Terminal Bisection (cpReset + cpClassify + cpArgs + cpS1Terminal + cpS2 + cpFallback + cpUnfused + fallbackCP)
- Lane 5: §6eg Conservative Candidate (cpReset + cpClassify + cpArgs + cpSbiS1 + cpSbiS2 + cpSbiGeneral + cpUnfused + fallbackCP)

Runs ordered raw repeats 1, 2, 4 across 2 independent series (blocks) of 4 alternating sub-blocks
(ABBA order: {0,1,2,3,5,5,3,2,1,0} and {5,3,2,1,0,0,1,2,3,5}).
Captures host encoding, submit-to-completion wait, and total time, reporting means and medians.
Includes candidate parity verification on Mode 5 outside timed batches.
"""

from __future__ import annotations

import argparse
import ast
import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path


def sha(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def load(root: Path, name: str):
    path = root / f"scratch/probes/{name}.py"
    ns: dict = {"__file__": str(path)}
    exec(compile(path.read_text(), str(path), "exec"), ns)
    return type(name, (), ns)


def clean_cpp(root: Path, variant: str = "bound-stack-15") -> str:
    base = load(root, "ray_transport_gpu_6du_true_terminal_s1")
    dx = load(root, "ray_transport_gpu_6dx_stack_bound")
    eg = load(root, "ray_transport_gpu_6eg_root_review")

    constants = {
        x.targets[0].id: ast.literal_eval(x.value)
        for x in ast.parse((root / "scratch/probes/ray_transport_metal_diagnostic.py").read_text()).body
        if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)
    }
    cpp = base.make_terminal_s1_cpp_source(constants["CPP_SOURCE"], root)
    cpp = dx.apply_variant(cpp, variant)

    # 1. Shaders
    safe = eg.function(eg.shaders(root)["safe"], "isolateMonotone")
    fn_finite = "fn finiteValue(x:f32)->bool{return (bitcast<u32>(x)&0x7f800000u)!=0x7f800000u;}\n"

    orig_s1_term = re.search(r'static const char\* kS1TerminalProducerWgsl = R"WGSL\((.*?)\)WGSL";', cpp, re.S)[1]
    orig_s2 = re.search(r'static const char\* kS2ProducerWgsl = R"WGSL\((.*?)\)WGSL";', cpp, re.S)[1]
    orig_fb = re.search(r'static const char\* kFallbackProducerWgsl = R"WGSL\((.*?)\)WGSL";', cpp, re.S)[1]

    sbi_s1 = orig_s1_term.replace(eg.function(orig_s1_term, "isolateMonotone"), fn_finite + safe)
    sbi_s2 = orig_s2.replace(eg.function(orig_s2, "isolateMonotone"), fn_finite + safe)
    sbi_fb = orig_fb.replace(eg.function(orig_fb, "isolateMonotone"), fn_finite + safe)

    decl = f"""
static const char* kSbiS1ProducerWgsl = R"WGSL({sbi_s1})WGSL";
static const char* kSbiS2ProducerWgsl = R"WGSL({sbi_s2})WGSL";
static const char* kSbiGeneralProducerWgsl = R"WGSL({sbi_fb})WGSL";
"""

    hook_main = "int main(int argc, char** argv) {"
    if cpp.count(hook_main) != 1:
        raise ValueError("main hook mismatch")
    cpp = cpp.replace(hook_main, decl + "\n" + hook_main, 1)

    # 2. Pipelines
    pipe_hook = 'auto cpFallback = buildCompute(kFallbackProducerWgsl, "fallbackProducerMain");'
    if cpp.count(pipe_hook) != 1:
        raise ValueError("pipeline hook mismatch")
    pipe_decl = """auto cpFallback = buildCompute(kFallbackProducerWgsl, "fallbackProducerMain");
    auto cpSbiS1 = buildCompute(kSbiS1ProducerWgsl, "s1ProducerMain");
    auto cpSbiS2 = buildCompute(kSbiS2ProducerWgsl, "s2ProducerMain");
    auto cpSbiGeneral = buildCompute(kSbiGeneralProducerWgsl, "fallbackProducerMain");"""
    cpp = cpp.replace(pipe_hook, pipe_decl, 1)

    # 3. Bind Groups
    bg_hook = "auto bgFallbackForced = makeGroup(cpFallback, {"
    idx = cpp.find(bg_hook)
    if idx == -1:
        raise ValueError("bgFallbackForced hook mismatch")
    end_bg = cpp.find("});", idx) + len("});")
    bg_decl = """
    auto bgS1TerminalSbi = makeGroup(cpSbiS1, {
        makeEntry(0, cfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(4, outBuf, 0, outBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s1TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt)),
        makeEntry(9, mBuf, 0, mediaBytes),
        makeEntry(10, pBuf, 0, paramBytes)
    });
    auto bgS1TerminalForcedSbi = makeGroup(cpSbiS1, {
        makeEntry(0, forcedCfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(4, outBuf, 0, outBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s1TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt)),
        makeEntry(9, mBuf, 0, mediaBytes),
        makeEntry(10, pBuf, 0, paramBytes)
    });
    auto bgS2Sbi = makeGroup(cpSbiS2, {
        makeEntry(0, cfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s2TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt))
    });
    auto bgS2ForcedSbi = makeGroup(cpSbiS2, {
        makeEntry(0, forcedCfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, s2TileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt))
    });
    auto bgFallbackSbi = makeGroup(cpSbiGeneral, {
        makeEntry(0, cfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(4, oBuf, 0, overlapBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, fbTileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt))
    });
    auto bgFallbackForcedSbi = makeGroup(cpSbiGeneral, {
        makeEntry(0, forcedCfgBuf, 0, sizeof(cfg)),
        makeEntry(1, specBuf, 0, curtainSpecs.size() * sizeof(GpuCurtainSpec)),
        makeEntry(2, rBuf, 0, rayBytes),
        makeEntry(3, sBuf, 0, subpanelBytes),
        makeEntry(4, oBuf, 0, overlapBytes),
        makeEntry(5, offBuf, 0, tileOffsets.size() * sizeof(uint32_t)),
        makeEntry(6, idxBuf, 0, tileRayIndices.size() * sizeof(uint32_t)),
        makeEntry(7, fbTileBuf, 0, 2304 * 4),
        makeEntry(8, cntBuf, 0, sizeof(initCnt))
    });
"""
    cpp = cpp[:end_bg] + bg_decl + cpp[end_bg:]

    # 4. Dispatch in execute
    mode3_needle = "dispatch(fallbackCP, fallbackBG, count);\n                }"
    idx_m3 = cpp.find(mode3_needle)
    if idx_m3 == -1:
        raise ValueError("mode3 dispatch hook mismatch")
    end_m3 = idx_m3 + len(mode3_needle)

    cand_dispatch = """
                else if(mode==4){
                    throw std::runtime_error("Mode 4 unused in 6eh");
                }
                else if(mode==5){
                    // Mode 5: §6eg Conservative Candidate
                    // Pass 0: Reset Counters
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpReset);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgReset, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 1, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 1: Classify Tiles
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpClassify);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgClassify, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 36, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 2: Route Args
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpArgs);
                        wgpuComputePassEncoderSetBindGroup(p, 0, bgArgs, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroups(p, 3, 1, 1);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 3: S1 Producer Terminal SBI (cpSbiS1)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpSbiS1);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgS1TerminalForcedSbi : bgS1TerminalSbi, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 0);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 4: S2 Producer SBI (cpSbiS2)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpSbiS2);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgS2ForcedSbi : bgS2Sbi, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 12);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 5: Fallback Producer SBI (cpSbiGeneral)
                    {
                        auto p = wgpuCommandEncoderBeginComputePass(enc, nullptr);
                        wgpuComputePassEncoderSetPipeline(p, cpSbiGeneral);
                        wgpuComputePassEncoderSetBindGroup(p, 0, forced ? bgFallbackForcedSbi : bgFallbackSbi, 0, nullptr);
                        wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 24);
                        wgpuComputePassEncoderEnd(p);
                        wgpuComputePassEncoderRelease(p);
                    }
                    // Pass 6: Canonical Unfused A* Consumer
                    dispatch(cpUnfused, bgUnfused, count);
                    // Pass 7: Fallback Marcher
                    dispatch(fallbackCP, fallbackBG, count);
                }"""
    cpp = cpp[:end_m3] + cand_dispatch + cpp[end_m3:]

    # 5. Candidate verification check before timing loop
    corr_hook = "correctness(3,false);"
    idx_c3 = cpp.find(corr_hook)
    if idx_c3 == -1:
        raise ValueError("correctness(3,false) hook mismatch")
    end_c3 = idx_c3 + len(corr_hook)

    cand_check = """
        // Mode 5 Candidate Verification (outside timed batches)
        execute(5, 1, false);
        auto candResults = readResults();
        auto candHdrsBytes = readBackBuffer(rBuf, rayBytes);
        const auto* candHdrs = reinterpret_cast<const GpuRayHeader*>(candHdrsBytes.data());
        unsigned candTerminalCount = 0;
        unsigned candFallbackCount = 0;
        double candMaxDeltaRGB = 0.0;
        double candMaxDeltaT = 0.0;
        unsigned candOutputDiffs = 0;
        for (size_t i = 0; i < count; ++i) {
            if ((candHdrs[i].overlapCount & 0x40000000u) != 0) candTerminalCount++;
            if ((candHdrs[i].overlapCount & 0x80000000u) != 0) candFallbackCount++;
            bool diff = false;
            for (int k = 0; k < 3; ++k) {
                double delta = std::abs(double(candResults[i].rgb_T[k]) - double(resCurrent[i].rgb_T[k]));
                candMaxDeltaRGB = std::max(candMaxDeltaRGB, delta);
                if (std::bit_cast<uint32_t>(candResults[i].rgb_T[k]) != std::bit_cast<uint32_t>(resCurrent[i].rgb_T[k])) diff = true;
            }
            double deltaT = std::abs(double(candResults[i].rgb_T[3]) - double(resCurrent[i].rgb_T[3]));
            candMaxDeltaT = std::max(candMaxDeltaT, deltaT);
            if (std::bit_cast<uint32_t>(candResults[i].rgb_T[3]) != std::bit_cast<uint32_t>(resCurrent[i].rgb_T[3])) diff = true;
            if (diff) candOutputDiffs++;
        }
        report["candidate_verification"] = {
            {"mode", 5},
            {"terminal_rays", candTerminalCount},
            {"fallback_rays", candFallbackCount},
            {"output_differences_vs_current", candOutputDiffs},
            {"current_max_rgb_delta", candMaxDeltaRGB},
            {"current_max_t_delta", candMaxDeltaT}
        };
        std::cerr << "CANDIDATE VERIFICATION: terminal=" << candTerminalCount << " fallback=" << candFallbackCount << " outputDiffs=" << candOutputDiffs << std::endl;
"""
    cpp = cpp[:end_c3] + cand_check + cpp[end_c3:]

    # 6. Timing loop replacement
    old_timing_block = """        // 4 Warmup cycles
        for(int k=0;k<4;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);execute(3,1,false);}

        J series = J::array();
        J cleanTiming = J::object();
        J orderedTiming = J::object();
        for (int sIdx = 1; sIdx <= 2; ++sIdx) {
            std::string sKey = "series_" + std::to_string(sIdx);
            J seriesData = J::object();
            for (int rep : {1, 2, 4}) {
                std::string repKey = std::to_string(rep);
                J repData = J::object();
                J series1Samples = J::array();
                std::vector<std::vector<double>> modeSamples(4);
                for (int block = 0; block < 4; ++block) {
                    const std::vector<int> order = block % 2 ? std::vector<int>{3, 2, 1, 0, 0, 1, 2, 3} : std::vector<int>{0, 1, 2, 3, 3, 2, 1, 0};
                    for (int mode : order) {
                        auto t = execute(mode, rep, false);
                        modeSamples[mode].push_back(t[1]); // completion_ms
                        series1Samples.push_back({{"mode", mode}, {"encode_ms", t[0]}, {"completion_ms", t[1]}, {"total_ms", t[2]}});
                    }
                }
                orderedTiming[sKey][repKey] = series1Samples;
                if (sIdx == 1) {
                    series.push_back({{"repeats", rep}, {"samples", series1Samples}});
                }
                for (int m = 0; m < 4; ++m) {
                    auto& sVec = modeSamples[m];
                    std::sort(sVec.begin(), sVec.end());
                    double sum = std::accumulate(sVec.begin(), sVec.end(), 0.0);
                    double mean = sum / sVec.size();
                    double med = 0.5 * (sVec[sVec.size() / 2 - 1] + sVec[sVec.size() / 2]);
                    repData[std::to_string(m)] = {
                        {"samples", sVec},
                        {"min_ms", sVec.front()},
                        {"median_ms", med},
                        {"mean_ms", mean},
                        {"max_ms", sVec.back()}
                    };
                }
                seriesData[repKey] = repData;
            }
            cleanTiming[sKey] = seriesData;
        }

        // Pool both blocks: 64 total observations, 16 per mode per repeat.
        J pooled = J::object();
        for (int rep : {1, 2, 4}) {
            std::string repKey = std::to_string(rep);
            J repPooled = J::object();
            for (int m = 0; m < 4; ++m) {
                std::string mKey = std::to_string(m);
                auto s1 = cleanTiming["series_1"][repKey][mKey]["samples"].get<std::vector<double>>();
                auto s2 = cleanTiming["series_2"][repKey][mKey]["samples"].get<std::vector<double>>();
                std::vector<double> allSamples = s1;
                allSamples.insert(allSamples.end(), s2.begin(), s2.end());
                std::sort(allSamples.begin(), allSamples.end());
                double sum = std::accumulate(allSamples.begin(), allSamples.end(), 0.0);
                double mean = sum / allSamples.size();
                double med = 0.5 * (allSamples[allSamples.size() / 2 - 1] + allSamples[allSamples.size() / 2]);
                repPooled[mKey] = {
                    {"samples", allSamples},
                    {"min_ms", allSamples.front()},
                    {"median_ms", med},
                    {"mean_ms", mean},
                    {"max_ms", allSamples.back()}
                };
            }
            pooled[repKey] = repPooled;
        }
        cleanTiming["pooled"] = pooled;
        report["clean_timing_series"] = cleanTiming;

        report["clean_timing_ordered_samples"] = orderedTiming;
        report["clean_timing_scope"] = {
            {"independent_native_invocations", 1},
            {"blocks_per_invocation", 2},
            {"observations_per_mode_per_block_per_repeat", 8},
            {"clock", "host submit-to-completion; includes GPU work and host completion wait"},
            {"encode_and_total_samples_retained", true},
            {"gpu_frequency_observed", false}
        };"""

    new_timing_block = """        // 4 Warmup cycles across all 5 active modes: {0, 1, 2, 3, 5}
        for(int k=0;k<4;++k){for(int m:{0,1,2,3,5})execute(m,1,false);}

        J series = J::array();
        J cleanTiming = J::object();
        J orderedTiming = J::object();
        for (int sIdx = 1; sIdx <= 2; ++sIdx) {
            std::string sKey = "series_" + std::to_string(sIdx);
            J seriesData = J::object();
            for (int rep : {1, 2, 4}) {
                std::string repKey = std::to_string(rep);
                J repData = J::object();
                J seriesSamples = J::array();
                std::map<int, std::vector<double>> modeEncode;
                std::map<int, std::vector<double>> modeCompletion;
                std::map<int, std::vector<double>> modeTotal;
                for (int block = 0; block < 4; ++block) {
                    const std::vector<int> order = block % 2 ? std::vector<int>{5, 3, 2, 1, 0, 0, 1, 2, 3, 5} : std::vector<int>{0, 1, 2, 3, 5, 5, 3, 2, 1, 0};
                    for (int mode : order) {
                        auto t = execute(mode, rep, false);
                        modeEncode[mode].push_back(t[0]);
                        modeCompletion[mode].push_back(t[1]);
                        modeTotal[mode].push_back(t[2]);
                        seriesSamples.push_back({{"mode", mode}, {"encode_ms", t[0]}, {"completion_ms", t[1]}, {"total_ms", t[2]}});
                    }
                }
                orderedTiming[sKey][repKey] = seriesSamples;
                if (sIdx == 1) {
                    series.push_back({{"repeats", rep}, {"samples", seriesSamples}});
                }
                auto computeStats = [](std::vector<double> v) {
                    std::sort(v.begin(), v.end());
                    double sum = std::accumulate(v.begin(), v.end(), 0.0);
                    double mean = sum / v.size();
                    double med = 0.5 * (v[v.size() / 2 - 1] + v[v.size() / 2]);
                    return J{{"samples", v}, {"min_ms", v.front()}, {"median_ms", med}, {"mean_ms", mean}, {"max_ms", v.back()}};
                };
                for (int m : {0, 1, 2, 3, 5}) {
                    std::string mKey = std::to_string(m);
                    auto encStats = computeStats(modeEncode[m]);
                    auto compStats = computeStats(modeCompletion[m]);
                    auto totStats = computeStats(modeTotal[m]);
                    repData[mKey] = {
                        {"encode_ms", encStats},
                        {"completion_ms", compStats},
                        {"total_ms", totStats},
                        {"samples", compStats["samples"]},
                        {"min_ms", compStats["min_ms"]},
                        {"median_ms", compStats["median_ms"]},
                        {"mean_ms", compStats["mean_ms"]},
                        {"max_ms", compStats["max_ms"]}
                    };
                }
                seriesData[repKey] = repData;
            }
            cleanTiming[sKey] = seriesData;
        }

        // Pool both blocks: 16 observations per mode per repeat
        J pooled = J::object();
        for (int rep : {1, 2, 4}) {
            std::string repKey = std::to_string(rep);
            J repPooled = J::object();
            auto computePooledStats = [&](int m, const std::string& field) {
                std::string mKey = std::to_string(m);
                auto s1 = cleanTiming["series_1"][repKey][mKey][field]["samples"].get<std::vector<double>>();
                auto s2 = cleanTiming["series_2"][repKey][mKey][field]["samples"].get<std::vector<double>>();
                std::vector<double> allSamples = s1;
                allSamples.insert(allSamples.end(), s2.begin(), s2.end());
                std::sort(allSamples.begin(), allSamples.end());
                double sum = std::accumulate(allSamples.begin(), allSamples.end(), 0.0);
                double mean = sum / allSamples.size();
                double med = 0.5 * (allSamples[allSamples.size() / 2 - 1] + allSamples[allSamples.size() / 2]);
                return J{{"samples", allSamples}, {"min_ms", allSamples.front()}, {"median_ms", med}, {"mean_ms", mean}, {"max_ms", allSamples.back()}};
            };
            for (int m : {0, 1, 2, 3, 5}) {
                std::string mKey = std::to_string(m);
                auto encStats = computePooledStats(m, "encode_ms");
                auto compStats = computePooledStats(m, "completion_ms");
                auto totStats = computePooledStats(m, "total_ms");
                repPooled[mKey] = {
                    {"encode_ms", encStats},
                    {"completion_ms", compStats},
                    {"total_ms", totStats},
                    {"samples", compStats["samples"]},
                    {"min_ms", compStats["min_ms"]},
                    {"median_ms", compStats["median_ms"]},
                    {"mean_ms", compStats["mean_ms"]},
                    {"max_ms", compStats["max_ms"]}
                };
            }
            pooled[repKey] = repPooled;
        }
        cleanTiming["pooled"] = pooled;
        report["clean_timing_series"] = cleanTiming;
        report["clean_timing_ordered_samples"] = orderedTiming;
        report["clean_timing_scope"] = {
            {"independent_native_invocations", 1},
            {"blocks_per_invocation", 2},
            {"subblocks_per_block", 4},
            {"observations_per_mode_per_subblock", 2},
            {"observations_per_mode_per_block_per_repeat", 8},
            {"pooled_observations_per_mode_per_repeat", 16},
            {"active_modes", {0, 1, 2, 3, 5}},
            {"lane_mapping", {
                {"0", "Current baseline (currentCP)"},
                {"1", "Unchanged bulk bisection (cpProducer + cpUnfused + fallbackCP)"},
                {"2", "Unchanged routed bisection (cpReset + cpClassify + cpArgs + cpS1Unfused + cpS2 + cpFallback + cpUnfused + fallbackCP)"},
                {"3", "Unchanged terminal bisection (cpReset + cpClassify + cpArgs + cpS1Terminal + cpS2 + cpFallback + cpUnfused + fallbackCP)"},
                {"5", "§6eg conservative candidate (cpReset + cpClassify + cpArgs + cpSbiS1 + cpSbiS2 + cpSbiGeneral + cpUnfused + fallbackCP)"}
            }},
            {"clock", "host submit-to-completion; includes GPU work and host completion wait"},
            {"encode_and_total_samples_retained", true},
            {"gpu_frequency_observed", false}
        };"""

    if cpp.count(old_timing_block) != 1:
        raise ValueError("timing block hook mismatch")
    cpp = cpp.replace(old_timing_block, new_timing_block, 1)

    # 7. Cleanup at end of main
    cleanup_hook = "wgpuComputePipelineRelease(cpFastCompact);"
    if cpp.count(cleanup_hook) != 1:
        raise ValueError("cleanup hook mismatch")
    cleanup_decl = """wgpuComputePipelineRelease(cpFastCompact);
    wgpuComputePipelineRelease(cpSbiS1);
    wgpuComputePipelineRelease(cpSbiS2);
    wgpuComputePipelineRelease(cpSbiGeneral);
    wgpuBindGroupRelease(bgS1TerminalSbi);
    wgpuBindGroupRelease(bgS1TerminalForcedSbi);
    wgpuBindGroupRelease(bgS2Sbi);
    wgpuBindGroupRelease(bgS2ForcedSbi);
    wgpuBindGroupRelease(bgFallbackSbi);
    wgpuBindGroupRelease(bgFallbackForcedSbi);"""
    cpp = cpp.replace(cleanup_hook, cleanup_decl, 1)

    # 8. Scope string update
    old_scope = '"§6du Corrective Qualification: Unthrottled 4-Mode Timing Series & Mode 2 Frozen §6dr Routed Unfused Control"'
    new_scope = '"§6eh Clean Complete-Cost Screen: Modes 0, 1, 2, 3, 5 (Frozen Controls & §6eg Conservative Candidate)"'
    if cpp.count(old_scope) != 1:
        raise ValueError("old_scope hook mismatch")
    cpp = cpp.replace(old_scope, new_scope, 1)

    return cpp


def verify_control_shaders(root: Path, cpp: str):
    base = load(root, "ray_transport_gpu_6du_true_terminal_s1")
    dx = load(root, "ray_transport_gpu_6dx_stack_bound")
    constants = {
        x.targets[0].id: ast.literal_eval(x.value)
        for x in ast.parse((root / "scratch/probes/ray_transport_metal_diagnostic.py").read_text()).body
        if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)
    }
    raw_base_cpp = base.make_terminal_s1_cpp_source(constants["CPP_SOURCE"], root)
    bound_base_cpp = dx.apply_variant(raw_base_cpp, "bound-stack-15")

    orig_blocks = dict(re.findall(r'static const char\* (\w+) = R"WGSL\((.*?)\)WGSL";', bound_base_cpp, re.S))
    curr_blocks = dict(re.findall(r'static const char\* (\w+) = R"WGSL\((.*?)\)WGSL";', cpp, re.S))

    for name in [
        "kResetCountersWgsl",
        "kClassifyWgsl",
        "kRouteArgsWgsl",
        "kS1UnfusedProducerWgsl",
        "kS1TerminalProducerWgsl",
        "kS2ProducerWgsl",
        "kFallbackProducerWgsl",
    ]:
        if name not in curr_blocks:
            raise ValueError(f"missing control shader: {name}")
        if curr_blocks[name] != orig_blocks[name]:
            raise ValueError(f"control shader drift: {name}")


def build_probe(root: Path, build_root: Path, work: Path, variant: str = "bound-stack-15") -> Path:
    if work.exists() or root == work or root in work.parents:
        raise ValueError("fresh external work directory required")
    work.mkdir(parents=True, exist_ok=True)

    cpp = clean_cpp(root, variant)
    verify_control_shaders(root, cpp)

    source = work / "probe.cpp"
    source.write_text(cpp)

    (work / "source.json").write_text(
        json.dumps(
            {
                "probe": "ray_transport_gpu_6eh_clean_timing",
                "variant": variant,
                "cpp_sha256": sha(source.read_bytes()),
                "engine_src_tree": subprocess.check_output(["git", "rev-parse", "HEAD:src"], cwd=root, text=True).strip(),
            },
            indent=2,
        )
        + "\n"
    )

    build_dir = work / "build"
    subprocess.run(
        [
            sys.executable,
            "-B",
            str(root / "scratch/probes/ray_transport_probe_build.py"),
            "--root",
            str(root),
            "--build-root",
            str(build_root),
            "--source",
            str(source),
            "--work",
            str(build_dir),
        ],
        check=True,
    )
    binary = build_dir / "probe"
    if not binary.exists():
        raise FileNotFoundError(f"compiled probe missing at {binary}")
    return binary


def run_probe(root: Path, binary: Path, work: Path) -> dict:
    wide = load(root, "ray_transport_gpu_wide_stream")
    zone = root / "saves/zones/Northern Veil/zone.ecform"
    before = sha(zone.read_bytes())
    manifest = Path("/private/tmp/earthcall-wide-case/manifest.json")
    meta, rays, _ = wide.verify_case(root, manifest, before)

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
    )
    env.pop("EC_GPU_GEOMETRY_CASE", None)
    env.pop("METAL_CAPTURE_ENABLED", None)

    out_json = work / "result.json"
    log_path = work / "native.log"
    with log_path.open("xb") as f:
        res = subprocess.run([str(binary), str(out_json)], cwd=root, env=env, stdout=f, stderr=subprocess.STDOUT)

    if res.returncode != 0:
        log_content = log_path.read_text(errors="replace")
        raise RuntimeError(f"probe execution failed with code {res.returncode}:\n{log_content}")

    return json.loads(out_json.read_text())


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--root", type=Path, required=True)
    p.add_argument("--build-root", type=Path, required=True)
    p.add_argument("--work", type=Path, required=True)
    p.add_argument("--variant", default="bound-stack-15")
    p.add_argument("--run", action="store_true")
    a = p.parse_args()

    root = a.root.resolve()
    build_root = a.build_root.resolve()
    work = a.work.resolve()

    binary = build_probe(root, build_root, work, a.variant)
    print(f"Probe binary built at {binary}")

    if a.run:
        print("Executing clean complete-cost screen on native Apple M5...")
        report = run_probe(root, binary, work)
        print("Native execution complete! Verification summary:")
        print(json.dumps(report.get("candidate_verification", {}), indent=2))
        print("Clean timing pooled summary:")
        print(json.dumps(report.get("clean_timing_series", {}).get("pooled", {}), indent=2))


if __name__ == "__main__":
    main()
