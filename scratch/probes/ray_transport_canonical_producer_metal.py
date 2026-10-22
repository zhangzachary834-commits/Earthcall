"""Northern Veil — Canonical Level-Set & Curvature Producer on M5 Metal (Step 2 - 5)
Implements the canonical producer in C++, feeds the schedule into the repaired
Complete Unfused A* Metal kernel on Apple Silicon M5, and verifies all 6 accuracy gates
against ROWS.json (65,536-step Simpson reference) while measuring connected end-to-end timing.

Sparkly Guy / Gemini 3.8 Flash High / Antigravity / session 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-21
"""

import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

def replace_once(s, a, b):
    assert s.count(a) == 1, ("hook count", s.count(a), a[:100])
    return s.replace(a, b)

def build_canonical_probe_source(base_diagnostic_source):
    s = base_diagnostic_source

    # 1. Include stdexcept and headers
    s = replace_once(s, '#include <numeric>', '#include <numeric>\n#include <stdexcept>\n#include <chrono>')

    # 2. Add complete overlap WGSL helper from overlap repair
    overlap_helper = r'''
fn phiSafe(tau: f32) -> f32 {
    if (abs(tau) < 0.01) {
        return 1.0 + tau * (-0.5 + tau * (1.0/6.0 - tau/24.0));
    }
    return (1.0 - exp(-tau)) / tau;
}
fn projectedAt(ro: vec3<f32>, rd: vec3<f32>, s: f32, mask: u32) -> vec4<f32> {
    var result=vec4<f32>(0.0);
    for (var member=0u; member<arrayLength(&media); member+=1u) {
        if ((mask & (1u<<member))==0u) {continue;}
        let m=media[member];
        let local=ro+s*rd-m.origin.xyz;
        let env=evalEnvelope(local,u32(m.freq_offset.y));
        if (env<=0.0) {continue;}
        let D=env*clamp(0.7+0.3*cnoise3(m.freq_offset.x*local),0.0,1.0);
        if (D<=0.0) {continue;}
        let raw=m.color_alpha.w*D;
        result+=vec4<f32>(m.color_alpha.xyz*raw,max(raw,1e-6));
    }
    return result;
}
fn advanceFlow(state: vec4<f32>, sourceSigma: vec4<f32>, h: f32) -> vec4<f32> {
    let tau=h*sourceSigma.w;
    return vec4<f32>(state.xyz+state.w*sourceSigma.xyz*(h*phiSafe(tau)),state.w*exp(-tau));
}
fn advanceOverlap(stateIn: vec4<f32>, ro: vec3<f32>, rd: vec3<f32>, o: OverlapInterval) -> vec4<f32> {
    var state=stateIn;
    let h=(o.span.y-o.span.x)*0.5;
    let a=0.25-sqrt(3.0)/6.0;
    let b=0.5-a;
    for (var panel=0u; panel<2u; panel+=1u) {
        let lo=o.span.x+f32(panel)*h;
        let minus=projectedAt(ro,rd,lo+(0.5-sqrt(3.0)/6.0)*h,o.info.x);
        let plus=projectedAt(ro,rd,lo+(0.5+sqrt(3.0)/6.0)*h,o.info.x);
        state=advanceFlow(state,b*minus+a*plus,h);
        state=advanceFlow(state,a*minus+b*plus,h);
    }
    return state;
}
'''
    s = replace_once(s, '// Pipeline 1: Unfused A* Shader', 'static const char* kOverlapWgsl=R"WGSL(' + overlap_helper + ')WGSL";\n\n// Pipeline 1: Unfused A* Shader')
    s = replace_once(s, 'std::string(kCommonWgslHeader) + "\\n" + shaderCode', 'std::string(kCommonWgslHeader) + "\\n" + kOverlapWgsl + "\\n" + shaderCode')

    # 3. Patch shader marching loop to consume both singles and overlaps in chronological order
    dummy = '''    if (overCount > 0u) {
        let dummy = overlaps[overOffset].info.x;
        L = L + vec3<f32>(f32(dummy) * 1e-35);
    }

    for (var i = 0u; i < subCount; i = i + 1u) {
        let sub = subpanels[subOffset + i];'''
    merged = '''    var single=0u;
    var overlap=0u;
    loop {
        if (single>=subCount && overlap>=overCount) {break;}
        var useOverlap=false;
        if (overlap<overCount) {
            if (single>=subCount) {useOverlap=true;}
            else {useOverlap=overlaps[overOffset+overlap].span.x<subpanels[subOffset+single].span.x;}
        }
        if (useOverlap) {
            let state=advanceOverlap(vec4<f32>(L,T),ro,rd,overlaps[overOffset+overlap]);
            L=state.xyz; T=state.w;
            overlap+=1u;
            continue;
        }
        let sub = subpanels[subOffset + single];
        single+=1u;'''
    assert s.count(dummy) == 2
    s = s.replace(dummy, merged)

    # 4. Patch tau gain formula in shader
    gain = '''        if (tau < 1e-4) {
            let phi = 1.0 - 0.5 * tau + (1.0 / 6.0) * tau * tau - (1.0 / 24.0) * tau * tau * tau;
            gain = sub_h * phi;
        } else {
            gain = (1.0 - atten) / sig_avg;
        }'''
    assert s.count(gain) == 2
    s = s.replace(gain, '        gain=sub_h*phiSafe(tau);')

    # 5. Replace lines 720-827 (the old OntoMath::segmentLine CPU loop) with the CANONICAL CURVATURE PRODUCER!
    producer_cpp = r'''
    // =========================================================================
    // CANONICAL LEVEL-SET & CURVATURE-ENCLOSED RAY PRODUCER (Firmament §6cf)
    // =========================================================================
    struct CanonicalMemberSpec {
        int id;
        glm::dvec3 origin;
        glm::dvec3 half;
        double Wx;
        double Wy;
        double yKnee;
        double wz;
        double M2_x;
        double amps[3];
        double freqs[3];
        double phases[3];
    };

    const CanonicalMemberSpec kCurtains[4] = {
        {0, {0.0, 48.0, 75.0}, {160.0, 36.0, 46.0}, 75.0, 16.0, -12.0, 3.2, 0.086772,
         {-15.0, -5.5, -2.0}, {0.026, 0.068, 0.16}, {0.0, 0.5, 1.2}},
        {1, {15.0, 56.0, 140.0}, {150.0, 38.0, 44.0}, 70.0, 17.0, -13.0, 2.8, 0.103901,
         {14.0, -5.0, 2.0}, {0.028, 0.075, 0.18}, {0.0, 0.5, 1.2}},
        {2, {-5.0, 78.0, 82.0}, {160.0, 36.0, 46.0}, 75.0, 16.0, -12.0, 3.8, 0.054888,
         {-13.0, -5.0, -1.5}, {0.024, 0.060, 0.14}, {0.0, 0.5, 1.2}},
        {3, {22.0, 72.0, 68.0}, {70.0, 64.0, 36.0}, 32.0, 30.0, -26.0, 3.0, 0.093184,
         {-9.0, -3.5, -1.5}, {0.038, 0.095, 0.18}, {0.0, 0.5, 1.2}}
    };

    auto evalWave = [](const CanonicalMemberSpec& m, double x) -> double {
        return m.amps[0] * std::sin(m.freqs[0] * x + m.phases[0])
             + m.amps[1] * std::cos(m.freqs[1] * x + m.phases[1])
             + m.amps[2] * std::sin(m.freqs[2] * x + m.phases[2]);
    };

    auto evalDWave = [](const CanonicalMemberSpec& m, double x) -> double {
        return m.amps[0] * m.freqs[0] * std::cos(m.freqs[0] * x + m.phases[0])
             - m.amps[1] * m.freqs[1] * std::sin(m.freqs[1] * x + m.phases[1])
             + m.amps[2] * m.freqs[2] * std::cos(m.freqs[2] * x + m.phases[2]);
    };

    auto isolateRoots = [](auto phi, double s_lo, double s_hi, double target_c, double M2_s, std::vector<double>& outRoots) {
        std::function<void(double, double, int)> search = [&](double a, double b, int depth) {
            double h = b - a;
            if (h <= 1e-7) {
                outRoots.push_back(0.5 * (a + b));
                return;
            }
            double va = phi(a) - target_c;
            double vb = phi(b) - target_c;
            double delta_bow = 0.125 * M2_s * (h * h);
            double lo_b = std::min(va, vb) - delta_bow;
            double hi_b = std::max(va, vb) + delta_bow;

            if (lo_b > 0.0 || hi_b < 0.0) return; // Certified skip

            if (va * vb <= 0.0) {
                double cur_a = a, cur_b = b;
                for (int iter = 0; iter < 24; ++iter) {
                    double mid = 0.5 * (cur_a + cur_b);
                    double vmid = phi(mid) - target_c;
                    if ((phi(cur_a) - target_c) * vmid <= 0.0) {
                        cur_b = mid;
                    } else {
                        cur_a = mid;
                    }
                }
                outRoots.push_back(0.5 * (cur_a + cur_b));
                return;
            }

            if (depth >= 6) {
                outRoots.push_back(0.5 * (a + b));
                return;
            }

            double mid = 0.5 * (a + b);
            search(a, mid, depth + 1);
            search(mid, b, depth + 1);
        };

        const int n_bins = 8;
        double step = (s_hi - s_lo) / n_bins;
        for (int i = 0; i < n_bins; ++i) {
            search(s_lo + i * step, s_lo + (i + 1) * step, 0);
        }
    };

    auto tProducer0 = std::chrono::steady_clock::now();

    for (size_t i = 0; i < count; ++i) {
        const auto& r = R[i];
        const glm::dvec3 d(r[2], r[3], r[4]);
        const double clipA = r[5], clipB = r[6];

        std::vector<Piece> allPieces;

        for (int m = 0; m < 4; ++m) {
            const auto& spec = kCurtains[m];
            const glm::dvec3 o = ro - spec.origin;

            // Outer box clip
            double ta = clipA, tb = clipB;
            bool hit = true;
            for (int k = 0; k < 3; ++k) {
                if (std::abs(d[k]) < 1e-15) {
                    if (o[k] < -spec.half[k] || o[k] > spec.half[k]) { hit = false; break; }
                } else {
                    double x = (-spec.half[k] - o[k]) / d[k];
                    double y = (spec.half[k] - o[k]) / d[k];
                    ta = std::max(ta, std::min(x, y));
                    tb = std::min(tb, std::max(x, y));
                }
            }
            if (!hit || !(tb > ta)) continue;

            // Affine support clip
            double s_supp_lo = ta, s_supp_hi = tb;
            if (std::abs(d.x) > 1e-7) {
                double x0 = (-spec.Wx - o.x) / d.x;
                double x1 = (spec.Wx - o.x) / d.x;
                s_supp_lo = std::max(s_supp_lo, std::min(x0, x1));
                s_supp_hi = std::min(s_supp_hi, std::max(x0, x1));
            } else {
                if (std::abs(o.x) > spec.Wx) continue;
            }

            if (std::abs(d.y) > 1e-7) {
                double y0 = (-spec.Wy - o.y) / d.y;
                double y1 = (spec.Wy - o.y) / d.y;
                s_supp_lo = std::max(s_supp_lo, std::min(y0, y1));
                s_supp_hi = std::min(s_supp_hi, std::max(y0, y1));
            } else {
                if (std::abs(o.y) > spec.Wy) continue;
            }

            if (!(s_supp_hi > s_supp_lo)) continue;

            // Interior affine split points
            std::vector<double> affineKinks;
            if (std::abs(d.x) > 1e-7) {
                double sx0 = -o.x / d.x;
                if (sx0 > s_supp_lo && sx0 < s_supp_hi) affineKinks.push_back(sx0);
            }
            if (std::abs(d.y) > 1e-7) {
                double sy0 = -o.y / d.y;
                if (sy0 > s_supp_lo && sy0 < s_supp_hi) affineKinks.push_back(sy0);
                double syKnee = (spec.yKnee - o.y) / d.y;
                if (syKnee > s_supp_lo && syKnee < s_supp_hi) affineKinks.push_back(syKnee);
            }

            // Wave level set roots
            auto phi = [&](double s) -> double {
                double px = o.x + s * d.x;
                double pz = o.z + s * d.z;
                return pz + evalWave(spec, px);
            };

            double M2_s = spec.M2_x * (d.x * d.x);

            std::vector<double> rootsNegWz, rootsPosWz, rootsZero;
            isolateRoots(phi, s_supp_lo, s_supp_hi, -spec.wz, M2_s, rootsNegWz);
            isolateRoots(phi, s_supp_lo, s_supp_hi, +spec.wz, M2_s, rootsPosWz);
            isolateRoots(phi, s_supp_lo, s_supp_hi, 0.0, M2_s, rootsZero);

            std::vector<double> bounds = {s_supp_lo};
            for (double r_val : rootsNegWz) bounds.push_back(r_val);
            for (double r_val : rootsPosWz) bounds.push_back(r_val);
            bounds.push_back(s_supp_hi);
            std::sort(bounds.begin(), bounds.end());

            std::vector<std::pair<double, double>> activeSpans;
            for (size_t b_idx = 1; b_idx < bounds.size(); ++b_idx) {
                double sa = bounds[b_idx - 1], sb = bounds[b_idx];
                if (sb - sa < 1e-4) continue;
                double smid = 0.5 * (sa + sb);
                if (std::abs(phi(smid)) < spec.wz) {
                    activeSpans.push_back({sa, sb});
                }
            }

            // Subdivide at kinks
            std::vector<double> memberKinks = affineKinks;
            memberKinks.insert(memberKinks.end(), rootsZero.begin(), rootsZero.end());
            std::sort(memberKinks.begin(), memberKinks.end());

            for (const auto& span : activeSpans) {
                std::vector<double> splits = {span.first};
                for (double k_val : memberKinks) {
                    if (k_val > span.first + 1e-4 && k_val < span.second - 1e-4) {
                        splits.push_back(k_val);
                    }
                }
                splits.push_back(span.second);
                for (size_t j = 1; j < splits.size(); ++j) {
                    double p0 = splits[j - 1], p1 = splits[j];
                    if (p1 - p0 > 1e-4) {
                        allPieces.push_back(Piece{m, p0, p1, {}});
                    }
                }
            }
        }

        // Event sweep across pieces
        std::vector<double> events;
        for (const auto& pc : allPieces) {
            events.push_back(pc.s0);
            events.push_back(pc.s1);
        }
        std::sort(events.begin(), events.end());
        events.erase(std::unique(events.begin(), events.end(), [](double a, double b) { return std::abs(a - b) < 1e-4; }), events.end());

        uint32_t raySubStart = static_cast<uint32_t>(gpuSubpanels.size());
        uint32_t rayOverlapStart = static_cast<uint32_t>(gpuOverlaps.size());

        for (size_t e = 1; e < events.size(); ++e) {
            double s0 = events[e - 1], s1 = events[e];
            if (!(s1 > s0 + 1e-4)) continue;
            double h = s1 - s0;
            double smid = 0.5 * (s0 + s1);

            std::vector<int> activeMedia;
            for (const auto& pc : allPieces) {
                if (smid >= pc.s0 && smid <= pc.s1) {
                    activeMedia.push_back(pc.mediumIndex);
                }
            }
            std::sort(activeMedia.begin(), activeMedia.end());
            activeMedia.erase(std::unique(activeMedia.begin(), activeMedia.end()), activeMedia.end());
            if (activeMedia.empty()) continue;

            if (activeMedia.size() == 1) {
                int mid = activeMedia[0];
                int n_sub = std::max(1, int(std::ceil(h / 4.0)));
                double sub_h = h / n_sub;
                for (int sub = 0; sub < n_sub; ++sub) {
                    float sub_s0 = float(s0 + sub * sub_h);
                    float sub_s1 = float(s0 + (sub + 1) * sub_h);
                    gpuSubpanels.push_back(GpuSubpanel{sub_s0, sub_s1, (uint32_t)mid, 0.0f});
                }
            } else {
                uint32_t mask = 0;
                for (int m_idx : activeMedia) mask |= (1u << m_idx);
                gpuOverlaps.push_back(GpuOverlap{float(s0), float(s1), mask, 0.0f});
            }
        }

        uint32_t raySubCount = static_cast<uint32_t>(gpuSubpanels.size()) - raySubStart;
        uint32_t rayOverlapCount = static_cast<uint32_t>(gpuOverlaps.size()) - rayOverlapStart;
        maxSubpanelsPerRay = std::max(maxSubpanelsPerRay, raySubCount);

        gpuRays[i].ro[0] = float(ro.x); gpuRays[i].ro[1] = float(ro.y); gpuRays[i].ro[2] = float(ro.z);
        gpuRays[i].clipA = float(clipA);
        gpuRays[i].rd[0] = float(d.x);  gpuRays[i].rd[1] = float(d.y);  gpuRays[i].rd[2] = float(d.z);
        gpuRays[i].clipB = float(clipB);
        gpuRays[i].subpanelOffset = raySubStart;
        gpuRays[i].subpanelCount = raySubCount;
        gpuRays[i].overlapOffset = rayOverlapStart;
        gpuRays[i].overlapCount = rayOverlapCount;
    }

    auto tProducer1 = std::chrono::steady_clock::now();
    double producerTotalMs = std::chrono::duration<double, std::milli>(tProducer1 - tProducer0).count();
    std::cout << "CANONICAL PRODUCER RUNTIME: " << producerTotalMs << " ms (" 
              << (producerTotalMs / count) * 1000.0 << " us/ray)" << std::endl;
'''

    # Find where the old loop started and ended in base_diagnostic_source
    start_tag = '    for (size_t i = 0; i < count; ++i) {'
    end_tag = '    std::vector<GpuMediumParams> gpuMedia(media.size());'
    begin = s.index(start_tag)
    end = s.index(end_tag, begin)
    s = s[:begin] + producer_cpp + s[end:]

    # 6. Add complete AST comparator on CPU
    cpu_comparator = r'''
    // Independently evaluate the original authored AST at native f32 coordinates.
    std::vector<GpuResult> resCPU(count);
    long long cpuSingles=0,cpuOverlaps=0,cpuOverlapMemberSites=0;
    auto astAt=[&](glm::vec3 ro,glm::vec3 rd,float t,uint32_t mask)->glm::vec4 {
        glm::vec4 value(0.0f); const glm::vec3 world=ro+t*rd;
        for(size_t id=0;id<media.size();++id) {
            if(!(mask&(1u<<id)))continue;
            const auto& m=media[id]; const glm::vec3 p=world-glm::vec3(m.origin);
            if(glm::any(glm::greaterThan(glm::abs(glm::dvec3(p)),m.half)))continue;
            const std::map<std::string,PropertyValue> vars{{"p",PropertyValue(p)},
                {"x",PropertyValue(double(p.x))},{"y",PropertyValue(double(p.y))},{"z",PropertyValue(double(p.z))},
                {OntoMath::kTimeVar,PropertyValue(clock)},
                {OntoMath::kOmegaXVar,PropertyValue(double(-rd.x))},
                {OntoMath::kOmegaYVar,PropertyValue(double(-rd.y))},
                {OntoMath::kOmegaZVar,PropertyValue(double(-rd.z))}};
            double D=0,X=0;
            auto v=m.density->evaluate(vars);
            if(!v||!propertyValueToNumber(*v,D))throw std::runtime_error("AST density refusal");
            if(!(D>0))continue;
            v=m.sigma->evaluate(vars);
            if(!v||!propertyValueToNumber(*v,X))throw std::runtime_error("AST extinction refusal");
            v=m.emission->evaluate(vars);
            if(!v||!std::holds_alternative<glm::vec3>(*v))throw std::runtime_error("AST emission refusal");
            const auto E=glm::max(std::get<glm::vec3>(*v),glm::vec3(0));
            value+=glm::vec4(E,float(std::max(X,1e-6)));
        }
        return value;
    };
    auto cpuFlow=[](glm::vec4 state,glm::vec4 q,float h) {
        const float tau=h*q.w,att=std::exp(-tau);
        const float gain=tau==0?h:h*(-std::expm1(-tau)/tau);
        return glm::vec4(glm::vec3(state)+state.w*glm::vec3(q)*gain,state.w*att);
    };
    auto tCpu0=std::chrono::steady_clock::now();
    for(size_t rayIdx=0;rayIdx<count;++rayIdx) {
        const auto& r=gpuRays[rayIdx]; const glm::vec3 ro(r.ro[0],r.ro[1],r.ro[2]),rd(r.rd[0],r.rd[1],r.rd[2]);
        glm::vec4 state(0,0,0,1);uint32_t si=0,oi=0;float last=-INFINITY;
        while(si<r.subpanelCount||oi<r.overlapCount) {
            const bool overlap=oi<r.overlapCount && (si>=r.subpanelCount || gpuOverlaps[r.overlapOffset+oi].s0<gpuSubpanels[r.subpanelOffset+si].s0);
            if(overlap) {
                const auto& o=gpuOverlaps[r.overlapOffset+oi++];
                if(o.s0<last)throw std::runtime_error("overlap schedule is not chronological");last=o.s1;++cpuOverlaps;
                const float h=(o.s1-o.s0)*.5f,a=.25f-std::sqrt(3.f)/6.f,b=.5f-a;
                for(int k=0;k<2;++k) {
                    const float lo=o.s0+k*h;
                    const auto u=astAt(ro,rd,lo+(.5f-std::sqrt(3.f)/6.f)*h,o.activeMask),v=astAt(ro,rd,lo+(.5f+std::sqrt(3.f)/6.f)*h,o.activeMask);
                    state=cpuFlow(state,b*u+a*v,h);state=cpuFlow(state,a*u+b*v,h);
                    cpuOverlapMemberSites+=2*__builtin_popcount(o.activeMask);
                }
            } else {
                const auto& p=gpuSubpanels[r.subpanelOffset+si++];
                if(p.s0<last)throw std::runtime_error("single schedule is not chronological");last=p.s1;++cpuSingles;
                const float h=p.s1-p.s0;
                const auto u=astAt(ro,rd,p.s0+(.5f-std::sqrt(3.f)/6.f)*h,1u<<p.mediumIndex),v=astAt(ro,rd,p.s0+(.5f+std::sqrt(3.f)/6.f)*h,1u<<p.mediumIndex);
                state=cpuFlow(state,.5f*(u+v),h);
            }
        }
        resCPU[rayIdx]=GpuResult{state.x,state.y,state.z,state.w};
    }
    if(size_t(cpuSingles)!=gpuSubpanels.size()||size_t(cpuOverlaps)!=gpuOverlaps.size())throw std::runtime_error("schedule coverage mismatch");
    auto tCpu1=std::chrono::steady_clock::now();
    double cpuQuadratureMs=std::chrono::duration<double,std::milli>(tCpu1-tCpu0).count();
'''
    begin_cpu = s.index('    // Direct CPU f32 Quadrature on identical schedule')
    end_cpu = s.index('    double maxDeltaRGB_CPU_vs_GPU', begin_cpu)
    s = s[:begin_cpu] + cpu_comparator + s[end_cpu:]

    # 7. Add batched runner and timing
    a = s.index('    // Helper for completion-fenced execution')
    b = s.index('    // Helper for readback', a)
    s = s[:a] + r'''
    auto runBatch=[&](WGPUComputePipeline cp,WGPUBindGroup bg,int n)->double {
        auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);
        auto pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);
        wgpuComputePassEncoderSetPipeline(pass,cp);
        if(bg)wgpuComputePassEncoderSetBindGroup(pass,0,bg,0,nullptr);
        for(int k=0;k<n;++k)wgpuComputePassEncoderDispatchWorkgroups(pass,workgroups,1,1);
        wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);
        auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuCommandEncoderRelease(enc);
        auto t0=std::chrono::steady_clock::now();wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuCommandBufferRelease(cmd);
        wgpuDevicePoll(gpu.device,true,nullptr);
        return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
    };
    auto runPass=[&](WGPUComputePipeline cp,WGPUBindGroup bg){return runBatch(cp,bg,1);};
''' + s[b:]

    a = s.index('    // Isolated completion-fenced ABBA benchmark')
    b = s.index('    // Arithmetic fidelity comparison', a)
    s = s[:a] + r'''
    std::vector<double> timesUnfused,timesFused;
    J batchSeries=J::array();
    auto cpEmpty=buildPipeline("@compute @workgroup_size(64) fn rayMain() {}\n");
    if(!cpEmpty)throw std::runtime_error("empty pipeline failed");
    for(int n:{1,8,64}) {
        for(int k=0;k<4;++k){runBatch(cpUnfused,bgUnfused,n);runBatch(cpFused,bgFused,n);}
        std::vector<double> A,B,E;
        for(int block=0;block<6;++block) {
            A.push_back(runBatch(cpUnfused,bgUnfused,n));B.push_back(runBatch(cpFused,bgFused,n));
            B.push_back(runBatch(cpFused,bgFused,n));A.push_back(runBatch(cpUnfused,bgUnfused,n));
            E.push_back(runBatch(cpEmpty,nullptr,n));
        }
        if(n==64){for(double x:A)timesUnfused.push_back(x/n);for(double x:B)timesFused.push_back(x/n);}
        batchSeries.push_back({{"dispatches_per_fence",n},{"unfused_batch_ms",stats(A)},{"fused_batch_ms",stats(B)},
            {"empty_batch_ms",stats(E)},{"samples_unfused_ms",A},{"samples_fused_ms",B},{"samples_empty_ms",E}});
        std::cout<<"BATCH "<<n<<" Unfused="<<stats(A)["mean"]<<" Fused="<<stats(B)["mean"]<<" Empty="<<stats(E)["mean"]<<std::endl;
    }
    wgpuComputePipelineRelease(cpEmpty);
''' + s[b:]

    # 8. Report updates and exit code
    report_patch = r'''
    report["canonical_producer"] = {
        {"producer_total_cpu_ms", producerTotalMs},
        {"producer_us_per_ray", (producerTotalMs / count) * 1000.0},
        {"speedup_vs_old_cpu_segmentation", 6208.9 / producerTotalMs},
        {"subpanels_count", gpuSubpanels.size()},
        {"overlaps_count", gpuOverlaps.size()}
    };
    report["batched_fence_series"] = batchSeries;
    report["connected_timing"] = {
        {"producer_cpu_ms", producerTotalMs},
        {"consumer_m5_metal_unfused_per_dispatch_ms", timesUnfused.empty() ? 0.0 : stats(timesUnfused)["mean"].get<double>()},
        {"total_connected_ms", producerTotalMs + (timesUnfused.empty() ? 0.0 : stats(timesUnfused)["mean"].get<double>())},
        {"production_v5_marcher_baseline_ms", 15.86}
    };
    bool accuracyPass = true;
    for(const char* channel:{"RGB_pooled_channels","T"})for(const char* metric:{"mean","p99","max"})
        accuracyPass = accuracyPass && report["accuracy_vs_65536_step_reference"]["fused_A_star_on_device"][channel][metric].get<double>() <= report["accuracy_vs_65536_step_reference"]["native_baseline"][channel][metric].get<double>();
    report["accuracy_gates_all_pass"] = accuracyPass;
    std::cout << report.dump(2) << std::endl;
'''
    s = replace_once(s, '    std::cout << report.dump(2) << std::endl;', report_patch)
    s = replace_once(s, "    return 0;\n}\n", "    return accuracyPass && maxDeltaRGB_CPU_vs_GPU < 1e-4 ? 0 : 3;\n}\n")
    return s

def main():
    root = Path(".").resolve()
    work = Path("/private/tmp/sparkly_canonical_producer").resolve()
    work.mkdir(parents=True, exist_ok=True)
    
    diag_script = root / "scratch/probes/ray_transport_metal_diagnostic.py"
    module = ast.parse(diag_script.read_text())
    v = {x.targets[0].id: ast.literal_eval(x.value) for x in module.body if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}
    
    cpp_source = build_canonical_probe_source(v["CPP_SOURCE"])
    (work / "probe.cpp").write_text(cpp_source)
    print(f"Generated canonical probe C++ source: {len(cpp_source)} bytes.")
    
    build = root / "build"
    db = json.loads((build / "compile_commands.json").read_text())
    c = next(x for x in db if x["file"].endswith("/entry.cpp") and "earthcall_webgpu.dir" in x["command"])
    args = shlex.split(c["command"])
    args[args.index("-c") + 1] = str(work / "probe.cpp")
    args[args.index("-o") + 1] = str(work / "probe.o")
    args += ["-std=c++20", "-O3"]
    
    print("Compiling probe.cpp with clang++...")
    subprocess.run(args, cwd=build, check=True)
    
    link_args = shlex.split((build / "CMakeFiles/earthcall_webgpu.dir/link.txt").read_text())
    link_args[next(i for i, x in enumerate(link_args) if x.endswith("/src/entry.cpp.o"))] = str(work / "probe.o")
    link_args[link_args.index("-o") + 1] = str(work / "probe")
    
    print("Linking probe executable...")
    subprocess.run(link_args, cwd=build, check=True)
    
    print("Running canonical probe on Apple Silicon M5 Metal...")
    out_json = work / "canonical_producer_run.json"
    res = subprocess.run([str(work / "probe"), str(out_json)], cwd=root, capture_output=True, text=True)
    
    print("=== PROBE STDOUT ===")
    print(res.stdout)
    if res.stderr:
        print("=== PROBE STDERR ===")
        print(res.stderr)
        
    print(f"Probe exit code: {res.returncode}")
    if res.returncode == 0:
        print("ALL GATES PASSED! CANONICAL PRODUCER IS QUALIFIED!")
    else:
        print("GATE FAILURE - Review diagnostics above.")

if __name__ == "__main__":
    main()
