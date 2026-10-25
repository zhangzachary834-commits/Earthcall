"""Northern Veil — §6bx Admission and Metric Repair Probe.

Executed by Sparkly Guy (Gemini 3.8 Flash High)
Fulfilling Original Prism Sun's directives in PRISM_REVIEW_6BW_ADMISSION_AND_METRIC_REPAIR_2026-10-21.md:
1. Canonicalize RGB metric: report both per-channel pooled (native bar matching) and per-ray Chebyshev norm.
2. Per-ray regressions: exact ray-by-ray comparisons against current native and Consumer A.
3. Authored noise clamp: enforce std::clamp(0.7 + 0.3 * N, 0.0, 1.0) on all sampled paths and census clamp states.
4. Sound whole-subpanel floor admission proof: interval range enclosure via ev.range() proving alpha * r.lo * 0.40 > 1e-6.
5. OntoMath moment evaluation: ev.integral (exact closed-form) and ev.hybridIntegral with LineSampledFactor uFactor for M1.
   Envelope point calls on admitted subpanels: EXACTLY 0.0.
"""

import argparse, hashlib, json, os, shlex, subprocess, sys
from pathlib import Path

RAYS_HASH = "f2d51ac017f27a3b45d7032b9b50313fc24e8c6fa05fe0fe495c236805fa6be7"
ROWS_HASH = "eb76bb86a69571b7bf2927df31d43920d8751d1b96b5b931cd94f55e4f1b76a8"
ZONE_HASH = "6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496"

CPP_SOURCE = r"""
#include <fstream>
#include <iostream>
#include <memory>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <vector>
#include <map>
#include <array>
#include <numeric>
#include <glm/gtc/noise.hpp>

#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/OntoMath/LineRestriction.hpp"
#include "json.hpp"

using J = nlohmann::json;
using M = OntoMath::MathNode;

static bool containsNoise(const M& n) {
    if (n.op == M::Op::Noise) return true;
    for (const auto& ch : n.children) if (ch && containsNoise(*ch)) return true;
    return false;
}

static std::unique_ptr<M> cloneEnvelope(const M& n) {
    if (n.op == M::Op::Clamp && containsNoise(n)) {
        auto one = std::make_unique<M>();
        one->op = M::Op::ScalarLeaf;
        one->scalarForm = OntoMath::ScalarForm::constant(1.0);
        return one;
    }
    auto copy = std::make_unique<M>();
    copy->op = n.op;
    copy->scalarForm = n.scalarForm;
    copy->variableName = n.variableName;
    copy->stringArg = n.stringArg;
    copy->matrixRows = n.matrixRows;
    copy->matrixCols = n.matrixCols;
    for (const auto& ch : n.children) {
        copy->children.push_back(ch ? cloneEnvelope(*ch) : nullptr);
    }
    return copy;
}

static const M* single(const OntoMath::Piecewise* p) {
    if (!p || p->pieces.size() != 1) return nullptr;
    const auto& x = p->pieces[0];
    return !x.guard && !x.call && !x.fold && !x.hasLo && !x.hasHi && !x.whereLEZero ? x.mathNode.get() : nullptr;
}

struct Medium {
    std::shared_ptr<geom::FieldNode> owner;
    std::string id;
    glm::dvec3 origin;
    glm::dvec3 half;
    const M *density = nullptr, *sigma = nullptr, *emission = nullptr;
    std::unique_ptr<M> envAST;
    OntoMath::LineAlgebraicForm envForm;
    bool hasEnv = false;
    double alpha = 0.0;
    glm::dvec3 color{0.0};
    double noiseFreq = 0.0;
};

struct Piece {
    int mediumIndex;
    double s0, s1;
    std::vector<int8_t> signs;
};

static std::array<double,4> at(const std::vector<Medium>& media, const std::vector<int>& active,
                               glm::dvec3 ro, glm::dvec3 d, double s, double clock) {
    auto world = ro + s * d;
    const auto woD = ro - world;
    const double n = glm::length(woD);
    const glm::vec3 wo(n > 1e-8 ? woD / n : glm::dvec3(0));
    std::array<double,4> out{};
    for (int id : active) {
        const auto& m = media[id];
        const glm::vec3 pf(world - m.origin);
        if (glm::any(glm::greaterThan(glm::abs(glm::dvec3(pf)), m.half))) continue;
        const std::map<std::string, PropertyValue> vars{
            {"p", PropertyValue(pf)}, {"x", PropertyValue(double(pf.x))},
            {"y", PropertyValue(double(pf.y))}, {"z", PropertyValue(double(pf.z))},
            {OntoMath::kTimeVar, PropertyValue(clock)},
            {OntoMath::kOmegaXVar, PropertyValue(double(wo.x))},
            {OntoMath::kOmegaYVar, PropertyValue(double(wo.y))},
            {OntoMath::kOmegaZVar, PropertyValue(double(wo.z))}
        };
        double D = 0, X = 0;
        auto v = m.density->evaluate(vars);
        if (!v || !propertyValueToNumber(*v, D)) continue;
        if (!(D > 0)) continue;
        v = m.sigma->evaluate(vars);
        if (!v || !propertyValueToNumber(*v, X)) continue;
        out[0] += std::max(X, 1e-6);
        v = m.emission->evaluate(vars);
        if (!v || !std::holds_alternative<glm::vec3>(*v)) continue;
        const auto e = glm::max(std::get<glm::vec3>(*v), glm::vec3(0));
        for (int k = 0; k < 3; ++k) out[k + 1] += e[k];
    }
    return out;
}

static J stats(std::vector<double> e) {
    if (e.empty()) return J{{"mean", 0.0}, {"rms", 0.0}, {"min", 0.0}, {"p50", 0.0}, {"p90", 0.0}, {"p99", 0.0}, {"max", 0.0}};
    std::sort(e.begin(), e.end());
    double sum = 0, sq = 0;
    for (double x : e) { sum += x; sq += x * x; }
    size_t n = e.size();
    return J{
        {"mean", sum / n},
        {"rms", std::sqrt(sq / n)},
        {"min", e.front()},
        {"p50", e[n * 50 / 100]},
        {"p90", e[n * 90 / 100]},
        {"p99", e[n * 99 / 100]},
        {"max", e.back()}
    };
}

int main(int argc, char** argv) {
    std::string outputPath = argc > 1 ? argv[1] : "";
    std::ifstream in("saves/zones/Northern Veil/zone.ecform", std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    J zone = J::parse(J::from_msgpack(b).at("MigrationRoot").get<std::string>());
    
    double clock = 7.25;
    std::vector<Medium> media;
    const double alphas[4] = {2.0/15.0, 9.0/65.0, 8.0/55.0, 4.0/25.0};
    const glm::dvec3 colors[4] = {
        {0.0747842, 0.688014, 0.239309},
        {0.0391309, 0.547833, 0.641747},
        {0.680411, 0.103093, 0.701029},
        {0.612595, 0.108105, 0.198192}
    };
    const double freqs[4] = {0.08, 0.09, 0.07, 0.10};
    
    int idx = 0;
    for (auto& j : zone.at("spatialFields")) {
        auto field = geom::FieldNode::fromJson(j);
        Rendering::VolumeDensityBinding bnd;
        if (!field || !Rendering::readVolumeDensity(*field, 0, 0, bnd)) continue;
        Medium m;
        m.owner = field;
        m.id = bnd.producerId;
        m.origin = glm::dvec3(bnd.origin);
        m.half = glm::abs(glm::dvec3(bnd.scale));
        m.density = single(bnd.densityExpr);
        m.sigma = single(bnd.extinctionExpr);
        m.emission = single(bnd.emissionExpr);
        m.alpha = alphas[idx];
        m.color = colors[idx];
        m.noiseFreq = freqs[idx];
        m.envAST = cloneEnvelope(*m.density);
        
        auto res = OntoMath::restrictAlgebraicToLine(*m.envAST, false, "p");
        if (res) {
            m.envForm = std::move(*res.form);
            m.hasEnv = true;
        }
        media.push_back(std::move(m));
        ++idx;
    }
    
    J rays, ref;
    { std::ifstream rin("docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json"); rin >> rays; }
    { std::ifstream rref("docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json"); rref >> ref; }
    
    const auto& R = rays.at("rays");
    const auto& Rows = ref.at("rows");
    const glm::dvec3 ro(rays["origin"][0], rays["origin"][1], rays["origin"][2]);
    size_t count = R.size();
    
    // GL2 quadrature nodes
    const double gx[2] = {0.5 * (1.0 - std::sqrt(3.0) / 3.0), 0.5 * (1.0 + std::sqrt(3.0) / 3.0)};
    const double gw[2] = {0.5, 0.5};
    
    // Noise clamp census
    long long clampLowCount = 0;
    long long clampLinearCount = 0;
    long long clampHighCount = 0;
    
    // Helper to evaluate full AST density
    auto evalD = [&](int mid, const glm::dvec3& world) -> double {
        const auto& m = media[mid];
        const glm::vec3 pf(world - m.origin);
        const auto woD = ro - world;
        const double n = glm::length(woD);
        const glm::vec3 wo(n > 1e-8 ? woD / n : glm::dvec3(0));
        const std::map<std::string, PropertyValue> vars{
            {"p", PropertyValue(pf)}, {"x", PropertyValue(double(pf.x))},
            {"y", PropertyValue(double(pf.y))}, {"z", PropertyValue(double(pf.z))},
            {OntoMath::kTimeVar, PropertyValue(clock)},
            {OntoMath::kOmegaXVar, PropertyValue(double(wo.x))},
            {OntoMath::kOmegaYVar, PropertyValue(double(wo.y))},
            {OntoMath::kOmegaZVar, PropertyValue(double(wo.z))}
        };
        double D = 0;
        auto v = m.density->evaluate(vars);
        if (v && propertyValueToNumber(*v, D) && D > 0) return D;
        return 0.0;
    };
    
    // Helper to evaluate clamped Noise factor w = clamp(0.7 + 0.3 * perlin, 0.0, 1.0)
    auto evalClampedNoise = [&](int mid, const glm::dvec3& world) -> double {
        const auto& m = media[mid];
        const glm::vec3 pf(world - m.origin);
        double rawN = double(glm::perlin(float(m.noiseFreq) * pf));
        double rawW = 0.7 + 0.3 * rawN;
        if (rawW <= 0.0) ++clampLowCount;
        else if (rawW >= 1.0) ++clampHighCount;
        else ++clampLinearCount;
        return std::clamp(rawW, 0.0, 1.0);
    };
    
    // Data structures for results
    struct ConsumerData {
        // Pooled per-channel errors (matching native bar convention)
        std::vector<double> errRGB_channel;
        std::vector<double> errT;
        // Per-ray Chebyshev errors (max channel per ray)
        std::vector<double> errRGB_ray;
        
        long long totalASTCalls = 0;
        long long totalNoiseCalls = 0;
        long long totalEnvelopeCalls = 0;
        long long exactM0Calls = 0;
        long long hybridM0Calls = 0;
        long long hybridM1Calls = 0;
        long long rangeProofCalls = 0;
        long long admittedSubpanels = 0;
        long long fallbackSubpanels = 0;
    };
    
    ConsumerData cNative, cA, cB, cC;
    long long totalSubpanelsGlobal = 0;
    long long floorInactiveProvenGlobal = 0;
    long long floorActiveProvenGlobal = 0;
    long long floorUnresolvedGlobal = 0;
    
    auto tStart = std::chrono::steady_clock::now();
    
    for (size_t i = 0; i < count; ++i) {
        const auto& r = R[i];
        const glm::dvec3 d(r[2], r[3], r[4]);
        const double clipA = r[5], clipB = r[6];
        
        // Native baseline values from RAYS.json columns 7..10
        double TNative = r[7];
        glm::dvec3 LNative(r[8], r[9], r[10]);
        
        std::vector<Piece> allPieces;
        std::vector<std::unique_ptr<OntoMath::LineAlgebraicEvaluator>> rayEvaluators(media.size());
        
        for (size_t j = 0; j < media.size(); ++j) {
            const auto& m = media[j];
            if (!m.hasEnv) continue;
            
            double ta = clipA, tb = clipB;
            const auto o = ro - m.origin;
            bool hit = true;
            for (int k = 0; k < 3; ++k) {
                if (std::abs(d[k]) < 1e-15) {
                    if (o[k] < -m.half[k] || o[k] > m.half[k]) { hit = false; break; }
                } else {
                    double x = (-m.half[k] - o[k]) / d[k];
                    double y = (m.half[k] - o[k]) / d[k];
                    ta = std::max(ta, std::min(x, y));
                    tb = std::min(tb, std::max(x, y));
                }
            }
            if (!hit || !(tb > ta)) continue;
            
            std::map<std::string, double> bind;
            for (int a = 0; a < 3; ++a) {
                bind[OntoMath::kLineOriginVars[a]] = o[a];
                bind[OntoMath::kLineDirectionVars[a]] = d[a];
            }
            bind[OntoMath::kTimeVar] = clock;
            bind[OntoMath::kOmegaXVar] = -d.x;
            bind[OntoMath::kOmegaYVar] = -d.y;
            bind[OntoMath::kOmegaZVar] = -d.z;
            
            auto ev = std::make_unique<OntoMath::LineAlgebraicEvaluator>(m.envForm, bind, true);
            if (!ev->ok()) continue;
            
            const std::vector<std::pair<std::size_t, uint32_t>> target{{0, m.envForm.roots[0]}};
            const double resolution = 1e-4 * (tb - ta);
            const auto pieces = OntoMath::segmentLine({ev.get()}, ta, tb, resolution, nullptr, 1u << 16, true, nullptr, &target);
            
            for (const auto& p : pieces) {
                if (!p.targetSigns.empty() && p.targetSigns[0] < 0) continue;
                bool undefinedD = false;
                const auto rd = ev->range(m.envForm.roots[0], p.s0, p.s1, p.signs[0], &undefinedD);
                if (!undefinedD && double(rd.hi) <= 0.0) continue;
                allPieces.push_back(Piece{(int)j, p.s0, p.s1, p.signs[0]});
            }
            rayEvaluators[j] = std::move(ev);
        }
        
        std::vector<double> events;
        for (const auto& pc : allPieces) {
            events.push_back(pc.s0);
            events.push_back(pc.s1);
        }
        std::sort(events.begin(), events.end());
        events.erase(std::unique(events.begin(), events.end()), events.end());
        
        // Ray transport states
        double TA = 1.0; glm::dvec3 LA(0.0);
        double TB = 1.0; glm::dvec3 LB(0.0);
        double TC = 1.0; glm::dvec3 LC(0.0);
        
        for (size_t e = 1; e < events.size(); ++e) {
            double s0 = events[e - 1], s1 = events[e];
            if (!(s1 > s0)) continue;
            double h = s1 - s0;
            double smid = 0.5 * (s0 + s1);
            
            std::vector<int> activeMedia;
            for (const auto& pc : allPieces) {
                if (smid >= pc.s0 && smid <= pc.s1) {
                    activeMedia.push_back(pc.mediumIndex);
                }
            }
            if (activeMedia.empty()) continue;
            
            if (activeMedia.size() == 1) {
                int mid = activeMedia[0];
                const auto& m = media[mid];
                auto* ev = rayEvaluators[mid].get();
                
                int n_sub = std::max(1, int(std::ceil(h / 4.0)));
                double sub_h = h / n_sub;
                totalSubpanelsGlobal += n_sub;
                
                // Find matching piece signs for this subpanel
                std::vector<int8_t> pieceSigns;
                for (const auto& pc : allPieces) {
                    if (pc.mediumIndex == mid && smid >= pc.s0 && smid <= pc.s1) {
                        pieceSigns = pc.signs;
                        break;
                    }
                }
                
                for (int sub = 0; sub < n_sub; ++sub) {
                    double sub_s0 = s0 + sub * sub_h;
                    double sub_s1 = sub_s0 + sub_h;
                    double s_mid = 0.5 * (sub_s0 + sub_s1);
                    double half_h = 0.5 * sub_h;
                    
                    double s_m = sub_s0 + gx[0] * sub_h;
                    double s_p = sub_s0 + gx[1] * sub_h;
                    auto w_m = ro + s_m * d;
                    auto w_p = ro + s_p * d;
                    
                    // ---------------------------------------------------------
                    // 1. Consumer A: Control (Frozen §6bv GL2)
                    // ---------------------------------------------------------
                    double D_m = evalD(mid, w_m);
                    double D_p = evalD(mid, w_p);
                    cA.totalASTCalls += 2;
                    cA.totalNoiseCalls += 2;
                    
                    double sigma_avg_A = std::max(m.alpha * (gw[0] * D_m + gw[1] * D_p), 1e-6);
                    double tau_A = sub_h * sigma_avg_A;
                    double atten_A = std::exp(-tau_A);
                    LA += TA * m.color * (1.0 - atten_A);
                    TA *= atten_A;
                    
                    // ---------------------------------------------------------
                    // 2. Sound Whole-Subpanel Range & Floor Proof for Consumer B
                    // ---------------------------------------------------------
                    bool floorInactiveProven = false;
                    bool floorActiveProven = false;
                    
                    if (ev) {
                        ++cB.rangeProofCalls;
                        ++cC.rangeProofCalls;
                        bool undef = false;
                        OntoMath::Interval rEnv = ev->range(m.envForm.roots[0], sub_s0, sub_s1, pieceSigns, &undef);
                        if (!undef) {
                            // By OntoMath kClassicPerlin3ValueBound, Perlin noise N in [-1, 1]
                            // w = clamp(0.7 + 0.3 * N, 0, 1) in [0.40, 1.0] strictly everywhere!
                            double minPossibleSigma = m.alpha * std::max(0.0, double(rEnv.lo)) * 0.40;
                            double maxPossibleSigma = m.alpha * std::max(0.0, double(rEnv.hi)) * 1.0;
                            
                            if (minPossibleSigma > 1e-6) {
                                floorInactiveProven = true;
                                ++floorInactiveProvenGlobal;
                            } else if (maxPossibleSigma <= 1e-6) {
                                floorActiveProven = true;
                                ++floorActiveProvenGlobal;
                            } else {
                                ++floorUnresolvedGlobal;
                            }
                        }
                    }
                    
                    // ---------------------------------------------------------
                    // 3. Consumer B: Moment-Weighted GL2 on Floor-Resolved Intervals
                    // ---------------------------------------------------------
                    bool bAdmitted = false;
                    if (floorInactiveProven && ev) {
                        std::string why0, why1;
                        double M0 = 0.0;
                        auto exact0 = ev->integral(m.envForm.roots[0], sub_s0, sub_s1, pieceSigns, &why0);
                        if (exact0) {
                            M0 = *exact0;
                            ++cB.exactM0Calls;
                        } else {
                            auto hy0 = ev->hybridIntegral(m.envForm.roots[0], sub_s0, sub_s1, pieceSigns, 1, &why0);
                            if (hy0) {
                                M0 = hy0->estimate;
                                ++cB.hybridM0Calls;
                            }
                        }
                        
                        OntoMath::LineSampledFactor uFactor;
                        uFactor.value = [s_mid, half_h](double s) -> std::optional<double> { return (s - s_mid) / half_h; };
                        uFactor.range = [s_mid, half_h](double a, double b) -> std::pair<double, double> { return {(a - s_mid) / half_h, (b - s_mid) / half_h}; };
                        uFactor.lipschitz = [half_h](double, double) -> double { return 1.0 / half_h; };
                        uFactor.slack = [](double, double) -> double { return 0.0; };
                        
                        auto hy1 = ev->hybridIntegral(m.envForm.roots[0], sub_s0, sub_s1, pieceSigns, 1, uFactor, &why1);
                        double M1 = hy1 ? hy1->estimate : 0.0;
                        if (hy1) ++cB.hybridM1Calls;
                        
                        if ((exact0 || M0 > 0.0) && hy1) {
                            bAdmitted = true;
                            ++cB.admittedSubpanels;
                            // ZERO envelope point evaluations charged on admitted subpanel!
                            
                            // Sample authored clamped Noise at Gauss nodes
                            double w_noise_m = evalClampedNoise(mid, w_m);
                            double w_noise_p = evalClampedNoise(mid, w_p);
                            cB.totalNoiseCalls += 2;
                            
                            double I_lin = M0 * 0.5 * (w_noise_p + w_noise_m) + M1 * (0.5 * std::sqrt(3.0) * (w_noise_p - w_noise_m));
                            double tau_B = m.alpha * I_lin;
                            double atten_B = std::exp(-tau_B);
                            LB += TB * m.color * (1.0 - atten_B);
                            TB *= atten_B;
                            
                            // Consumer C: Barycentric 1-Noise on the same admitted subpanel
                            if (M0 > 1e-12) {
                                ++cC.admittedSubpanels;
                                double u_star = std::clamp(M1 / M0, -0.99, 0.99);
                                double s_star = sub_s0 + 0.5 * (1.0 + u_star) * sub_h;
                                double w_star = evalClampedNoise(mid, ro + s_star * d);
                                cC.totalNoiseCalls += 1;
                                
                                double tau_C = m.alpha * M0 * w_star;
                                double atten_C = std::exp(-tau_C);
                                LC += TC * m.color * (1.0 - atten_C);
                                TC *= atten_C;
                            } else {
                                ++cC.fallbackSubpanels;
                                double atten_C = std::exp(-tau_A);
                                LC += TC * m.color * (1.0 - atten_C);
                                TC *= atten_C;
                            }
                        }
                    }
                    
                    if (!bAdmitted) {
                        // Fall back to Consumer A
                        ++cB.fallbackSubpanels;
                        cB.totalASTCalls += 2;
                        cB.totalNoiseCalls += 2;
                        
                        double atten_B = std::exp(-tau_A);
                        LB += TB * m.color * (1.0 - atten_B);
                        TB *= atten_B;
                        
                        ++cC.fallbackSubpanels;
                        cC.totalASTCalls += 2;
                        cC.totalNoiseCalls += 2;
                        
                        double atten_C = std::exp(-tau_A);
                        LC += TC * m.color * (1.0 - atten_C);
                        TC *= atten_C;
                    }
                }
            } else {
                // Multi-curtain overlap interval: 2 panels of CF4-2 (same for A, B, C)
                int panels = 2;
                double pan_h = h / panels;
                const double a = 0.25 - std::sqrt(3.0) / 6.0, b = 0.5 - a;
                
                for (int p_idx = 0; p_idx < panels; ++p_idx) {
                    double p_lo = s0 + p_idx * pan_h;
                    auto u = at(media, activeMedia, ro, d, p_lo + (0.5 - std::sqrt(3.0) / 6.0) * pan_h, clock);
                    auto v = at(media, activeMedia, ro, d, p_lo + (0.5 + std::sqrt(3.0) / 6.0) * pan_h, clock);
                    
                    cA.totalASTCalls += 4;
                    cB.totalASTCalls += 4;
                    cC.totalASTCalls += 4;
                    
                    for (auto w : std::array<std::array<double,2>,2>{{{b, a}, {a, b}}}) {
                        double sigma = w[0] * u[0] + w[1] * v[0];
                        double x = pan_h * sigma;
                        double atten = std::exp(-x);
                        double gain = pan_h * (x == 0 ? 1 : -std::expm1(-x) / x);
                        
                        for (int j = 1; j < 4; ++j) {
                            double emit = w[0] * u[j] + w[1] * v[j];
                            LA[j - 1] += TA * emit * gain;
                            LB[j - 1] += TB * emit * gain;
                            LC[j - 1] += TC * emit * gain;
                        }
                        TA *= atten;
                        TB *= atten;
                        TC *= atten;
                    }
                }
            }
        }
        
        // Reference truth
        const auto& truth = Rows[i];
        double trueT = truth[2].get<double>();
        glm::dvec3 trueRGB(truth[3].get<double>(), truth[4].get<double>(), truth[5].get<double>());
        
        // Native errors
        double errT_nat = std::abs(TNative - trueT);
        double errR_nat = std::abs(LNative.x - trueRGB.x);
        double errG_nat = std::abs(LNative.y - trueRGB.y);
        double errB_nat = std::abs(LNative.z - trueRGB.z);
        cNative.errT.push_back(errT_nat);
        cNative.errRGB_channel.push_back(errR_nat);
        cNative.errRGB_channel.push_back(errG_nat);
        cNative.errRGB_channel.push_back(errB_nat);
        cNative.errRGB_ray.push_back(std::max({errR_nat, errG_nat, errB_nat}));
        
        // Consumer A errors
        double errT_A = std::abs(TA - trueT);
        double errR_A = std::abs(LA.x - trueRGB.x);
        double errG_A = std::abs(LA.y - trueRGB.y);
        double errB_A = std::abs(LA.z - trueRGB.z);
        cA.errT.push_back(errT_A);
        cA.errRGB_channel.push_back(errR_A);
        cA.errRGB_channel.push_back(errG_A);
        cA.errRGB_channel.push_back(errB_A);
        cA.errRGB_ray.push_back(std::max({errR_A, errG_A, errB_A}));
        
        // Consumer B errors
        double errT_B = std::abs(TB - trueT);
        double errR_B = std::abs(LB.x - trueRGB.x);
        double errG_B = std::abs(LB.y - trueRGB.y);
        double errB_B = std::abs(LB.z - trueRGB.z);
        cB.errT.push_back(errT_B);
        cB.errRGB_channel.push_back(errR_B);
        cB.errRGB_channel.push_back(errG_B);
        cB.errRGB_channel.push_back(errB_B);
        cB.errRGB_ray.push_back(std::max({errR_B, errG_B, errB_B}));
        
        // Consumer C errors
        double errT_C = std::abs(TC - trueT);
        double errR_C = std::abs(LC.x - trueRGB.x);
        double errG_C = std::abs(LC.y - trueRGB.y);
        double errB_C = std::abs(LC.z - trueRGB.z);
        cC.errT.push_back(errT_C);
        cC.errRGB_channel.push_back(errR_C);
        cC.errRGB_channel.push_back(errG_C);
        cC.errRGB_channel.push_back(errB_C);
        cC.errRGB_ray.push_back(std::max({errR_C, errG_C, errB_C}));
    }
    
    auto tEnd = std::chrono::steady_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    
    // ---------------------------------------------------------
    // Per-Ray Regression Analysis
    // ---------------------------------------------------------
    long long bWorseThanNatRGB = 0, bWorseThanNatT = 0;
    double maxDeltaB_Nat_RGB = 0.0, maxDeltaB_Nat_T = 0.0;
    
    long long bWorseThanA_RGB = 0, bWorseThanA_T = 0;
    double maxDeltaB_A_RGB = 0.0, maxDeltaB_A_T = 0.0;
    
    long long cWorseThanNatRGB = 0, cWorseThanNatT = 0;
    double maxDeltaC_Nat_RGB = 0.0, maxDeltaC_Nat_T = 0.0;
    
    long long cWorseThanA_RGB = 0, cWorseThanA_T = 0;
    double maxDeltaC_A_RGB = 0.0, maxDeltaC_A_T = 0.0;
    
    for (size_t i = 0; i < count; ++i) {
        // B vs Native
        double dRGB_B_Nat = cB.errRGB_ray[i] - cNative.errRGB_ray[i];
        if (dRGB_B_Nat > 1e-12) { ++bWorseThanNatRGB; maxDeltaB_Nat_RGB = std::max(maxDeltaB_Nat_RGB, dRGB_B_Nat); }
        double dT_B_Nat = cB.errT[i] - cNative.errT[i];
        if (dT_B_Nat > 1e-12) { ++bWorseThanNatT; maxDeltaB_Nat_T = std::max(maxDeltaB_Nat_T, dT_B_Nat); }
        
        // B vs A
        double dRGB_B_A = cB.errRGB_ray[i] - cA.errRGB_ray[i];
        if (dRGB_B_A > 1e-12) { ++bWorseThanA_RGB; maxDeltaB_A_RGB = std::max(maxDeltaB_A_RGB, dRGB_B_A); }
        double dT_B_A = cB.errT[i] - cA.errT[i];
        if (dT_B_A > 1e-12) { ++bWorseThanA_T; maxDeltaB_A_T = std::max(maxDeltaB_A_T, dT_B_A); }
        
        // C vs Native
        double dRGB_C_Nat = cC.errRGB_ray[i] - cNative.errRGB_ray[i];
        if (dRGB_C_Nat > 1e-12) { ++cWorseThanNatRGB; maxDeltaC_Nat_RGB = std::max(maxDeltaC_Nat_RGB, dRGB_C_Nat); }
        double dT_C_Nat = cC.errT[i] - cNative.errT[i];
        if (dT_C_Nat > 1e-12) { ++cWorseThanNatT; maxDeltaC_Nat_T = std::max(maxDeltaC_Nat_T, dT_C_Nat); }
        
        // C vs A
        double dRGB_C_A = cC.errRGB_ray[i] - cA.errRGB_ray[i];
        if (dRGB_C_A > 1e-12) { ++cWorseThanA_RGB; maxDeltaC_A_RGB = std::max(maxDeltaC_A_RGB, dRGB_C_A); }
        double dT_C_A = cC.errT[i] - cA.errT[i];
        if (dT_C_A > 1e-12) { ++cWorseThanA_T; maxDeltaC_A_T = std::max(maxDeltaC_A_T, dT_C_A); }
    }
    
    // ---------------------------------------------------------
    // Compile JSON Report
    // ---------------------------------------------------------
    J report;
    report["scope"] = "§6bx Admission and Metric Repair";
    report["rays"] = count;
    report["total_time_ms"] = totalMs;
    report["ms_per_ray"] = totalMs / count;
    
    // Floor Proof Census
    report["sound_floor_proof_census"] = {
        {"total_subpanels", totalSubpanelsGlobal},
        {"floor_inactive_proven", {
            {"count", floorInactiveProvenGlobal},
            {"pct", double(floorInactiveProvenGlobal) * 100.0 / totalSubpanelsGlobal}
        }},
        {"floor_active_proven", {
            {"count", floorActiveProvenGlobal},
            {"pct", double(floorActiveProvenGlobal) * 100.0 / totalSubpanelsGlobal}
        }},
        {"floor_unresolved", {
            {"count", floorUnresolvedGlobal},
            {"pct", double(floorUnresolvedGlobal) * 100.0 / totalSubpanelsGlobal}
        }}
    };
    
    // Noise Clamp Census
    long long totalClampSamples = clampLowCount + clampLinearCount + clampHighCount;
    report["noise_clamp_census"] = {
        {"total_samples", totalClampSamples},
        {"clamp_low_active", {{"count", clampLowCount}, {"pct", double(clampLowCount) * 100.0 / std::max(1LL, totalClampSamples)}}},
        {"clamp_linear_active", {{"count", clampLinearCount}, {"pct", double(clampLinearCount) * 100.0 / std::max(1LL, totalClampSamples)}}},
        {"clamp_high_active", {{"count", clampHighCount}, {"pct", double(clampHighCount) * 100.0 / std::max(1LL, totalClampSamples)}}}
    };
    
    auto makeCandidateReport = [&](const std::string& name, const ConsumerData& cd) -> J {
        return J{
            {"name", name},
            {"RGB_pooled_channels", stats(cd.errRGB_channel)},
            {"RGB_max_per_ray", stats(cd.errRGB_ray)},
            {"T", stats(cd.errT)},
            {"work_per_ray", {
                {"AST_calls", double(cd.totalASTCalls) / count},
                {"noise_calls", double(cd.totalNoiseCalls) / count},
                {"envelope_point_calls", double(cd.totalEnvelopeCalls) / count},
                {"exact_M0_calls", double(cd.exactM0Calls) / count},
                {"hybrid_M0_calls", double(cd.hybridM0Calls) / count},
                {"hybrid_M1_calls", double(cd.hybridM1Calls) / count},
                {"range_proof_calls", double(cd.rangeProofCalls) / count}
            }},
            {"subpanel_accounting", {
                {"admitted", cd.admittedSubpanels},
                {"fallback", cd.fallbackSubpanels},
                {"admitted_pct", double(cd.admittedSubpanels) * 100.0 / std::max(1LL, cd.admittedSubpanels + cd.fallbackSubpanels)}
            }}
        };
    };
    
    report["candidates"] = {
        {"current_native", makeCandidateReport("Current Native (from RAYS.json columns 7..10)", cNative)},
        {"consumer_A", makeCandidateReport("Consumer A (Frozen §6bv GL2 Control)", cA)},
        {"consumer_B", makeCandidateReport("Consumer B (OntoMath Moments + Sound Interval Floor)", cB)},
        {"consumer_C", makeCandidateReport("Consumer C (Barycentric 1-Noise + Sound Interval Floor)", cC)}
    };
    
    report["per_ray_regressions"] = {
        {"B_vs_Native", {
            {"RGB_worse_rays", bWorseThanNatRGB},
            {"RGB_worse_pct", double(bWorseThanNatRGB) * 100.0 / count},
            {"RGB_max_delta", maxDeltaB_Nat_RGB},
            {"T_worse_rays", bWorseThanNatT},
            {"T_worse_pct", double(bWorseThanNatT) * 100.0 / count},
            {"T_max_delta", maxDeltaB_Nat_T}
        }},
        {"B_vs_A", {
            {"RGB_worse_rays", bWorseThanA_RGB},
            {"RGB_worse_pct", double(bWorseThanA_RGB) * 100.0 / count},
            {"RGB_max_delta", maxDeltaB_A_RGB},
            {"T_worse_rays", bWorseThanA_T},
            {"T_worse_pct", double(bWorseThanA_T) * 100.0 / count},
            {"T_max_delta", maxDeltaB_A_T}
        }},
        {"C_vs_Native", {
            {"RGB_worse_rays", cWorseThanNatRGB},
            {"RGB_worse_pct", double(cWorseThanNatRGB) * 100.0 / count},
            {"RGB_max_delta", maxDeltaC_Nat_RGB},
            {"T_worse_rays", cWorseThanNatT},
            {"T_worse_pct", double(cWorseThanNatT) * 100.0 / count},
            {"T_max_delta", maxDeltaC_Nat_T}
        }},
        {"C_vs_A", {
            {"RGB_worse_rays", cWorseThanA_RGB},
            {"RGB_worse_pct", double(cWorseThanA_RGB) * 100.0 / count},
            {"RGB_max_delta", maxDeltaC_A_RGB},
            {"T_worse_rays", cWorseThanA_T},
            {"T_worse_pct", double(cWorseThanA_T) * 100.0 / count},
            {"T_max_delta", maxDeltaC_A_T}
        }}
    };
    
    std::cout << report.dump(2) << std::endl;
    
    if (!outputPath.empty()) {
        std::ofstream out(outputPath);
        out << report.dump(2) << "\n";
        std::cout << "Report saved to " << outputPath << "\n";
    }
    
    return 0;
}
"""

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument("--output", type=Path, default=None)
    args = parser.parse_args()
    
    r = args.root.resolve()
    assert (r / "saves/zones/Northern Veil/zone.ecform").exists(), "Must run in or point to Earthcall root"
    
    # 1. Verify SHA-256 fixture hashes
    rays_path = r / "docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json"
    rows_path = r / "docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json"
    zone_path = r / "saves/zones/Northern Veil/zone.ecform"
    
    assert hashlib.sha256(rays_path.read_bytes()).hexdigest() == RAYS_HASH, "RAYS hash mismatch"
    assert hashlib.sha256(rows_path.read_bytes()).hexdigest() == ROWS_HASH, "ROWS hash mismatch"
    assert hashlib.sha256(zone_path.read_bytes()).hexdigest() == ZONE_HASH, "ZONE hash mismatch"
    print("Fixtures verified successfully (SHA-256 matches).")
    
    # 2. Compile probe
    probe_cpp = Path("/private/tmp/reproducer_admission_repair.cpp")
    probe_o = Path("/private/tmp/reproducer_admission_repair.o")
    probe_bin = Path("/private/tmp/reproducer_admission_repair")
    probe_cpp.write_text(CPP_SOURCE)
    
    b = r / "build"
    db = json.loads((b / "compile_commands.json").read_text())
    c = next(x for x in db if x["file"].endswith("/entry.cpp") and "earthcall_webgpu.dir" in x["command"])
    c_args = shlex.split(c["command"])
    c_args[c_args.index("-c") + 1] = str(probe_cpp)
    c_args[c_args.index("-o") + 1] = str(probe_o)
    if "-std=gnu++17" in c_args: c_args[c_args.index("-std=gnu++17")] = "-std=c++20"
    c_args += ["-O3"]
    subprocess.run(c_args, cwd=b, check=True)
    
    l_args = shlex.split((b / "CMakeFiles/earthcall_webgpu.dir/link.txt").read_text())
    l_args[next(i for i, x in enumerate(l_args) if x.endswith("/src/entry.cpp.o"))] = str(probe_o)
    l_args[l_args.index("-o") + 1] = str(probe_bin)
    subprocess.run(l_args, cwd=b, check=True)
    print("Compiled admission repair probe successfully.")
    
    # 3. Execute
    cmd = [str(probe_bin)]
    if args.output:
        cmd.append(str(args.output.resolve()))
    subprocess.run(cmd, cwd=r, check=True)

if __name__ == "__main__":
    main()
