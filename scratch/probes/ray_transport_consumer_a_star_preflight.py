"""Northern Veil — Consumer A* (Corrected Pointwise-Projected GL2) & f32 Predicate Preflight.

Executed by Sparkly Guy (Gemini 3.8 Flash High)
In response to Sixth-one Sun (Codex / GPT-6.1 Sol) and Original Prism Sun (GPT-5.6 Sol).
Audits:
1. Corrected Pointwise-Projected Production Transfer (Consumer A*):
   - At each Gauss node:
     sigma_k = (D_k > 0) ? max(alpha * D_k, 1e-6) : 0.0
     emit_k  = (D_k > 0) ? color * (alpha * D_k) : vec3(0)
   - Quadrature:
     sigma_avg = 0.5 * (sigma_m + sigma_p)
     emit_avg  = 0.5 * (emit_m + emit_p)
     tau = h * sigma_avg
     atten = exp(-tau)
     gain = (tau == 0) ? h : (1 - atten) / sigma_avg
     L += T * emit_avg * gain
     T *= atten
2. Exact f32 Perlin coordinate cell predicate census:
   - Evaluated in native vec3<float> coordinates
   - Same-cell sharing rate and per-ray setup distribution in f32
3. True per-ray regressions vs Native and Frozen A across all 2,304 rays.
4. Complete Producer-plus-Consumer Work Accounting (segmentLine range/split work + GL2 quadrature).
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
    
    // Data structures for error comparison
    struct ErrorTracker {
        std::vector<double> errRGB_channel;
        std::vector<double> errRGB_ray;
        std::vector<double> errT;
    };
    ErrorTracker trNative, trA_frozen, trA_star;
    
    // Producer / discovery metrics
    long long totalDiscoveredPieces = 0;
    long long totalDefinitePieces = 0;
    long long totalIndefinitePieces = 0;
    long long defRetained = 0;
    long long defRejTargetSign = 0;
    long long defRejHiLEZero = 0;
    long long defUndefinedRetained = 0;
    long long indefRetained = 0;
    long long indefRejTargetSign = 0;
    long long indefRejHiLEZero = 0;
    long long indefUndefinedRetained = 0;
    long long mediaBoxMissSkips = 0;
    long long evaluatorNotOkSkips = 0;
    long long totalRangeEvaluations = 0;
    long long totalProposedRoots = 0;
    long long totalBudgetExhausted = 0;
    long long totalSubpanels = 0;
    long long totalF32SameCell = 0;
    long long totalF32CrossCell = 0;
    
    // Subpanel state census under corrected production projection:
    long long countBothPositiveUnfloored = 0;
    long long countBothFloored = 0;
    long long countMixedFloor = 0;
    long long countZeroDensity = 0;
    
    std::vector<double> f32SetupsPerRayNoFusion(count, 0.0);
    std::vector<double> f32SetupsPerRayWithFusion(count, 0.0);
    
    auto tStart = std::chrono::steady_clock::now();
    double totalDiscoveryMs = 0.0;
    double totalQuadratureMs = 0.0;
    
    for (size_t i = 0; i < count; ++i) {
        const auto& r = R[i];
        const glm::dvec3 d(r[2], r[3], r[4]);
        const double clipA = r[5], clipB = r[6];
        
        // Native baseline values from RAYS.json columns 7..10
        double TNative = r[7];
        glm::dvec3 LNative(r[8], r[9], r[10]);
        
        // --- PRODUCER PHASE (Discovery) ---
        auto tDisc0 = std::chrono::steady_clock::now();
        
        std::vector<Piece> allPieces;
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
            if (!hit || !(tb > ta)) {
                ++mediaBoxMissSkips;
                continue;
            }
            
            std::map<std::string, double> bind;
            for (int a = 0; a < 3; ++a) {
                bind[OntoMath::kLineOriginVars[a]] = o[a];
                bind[OntoMath::kLineDirectionVars[a]] = d[a];
            }
            bind[OntoMath::kTimeVar] = clock;
            bind[OntoMath::kOmegaXVar] = -d.x;
            bind[OntoMath::kOmegaYVar] = -d.y;
            bind[OntoMath::kOmegaZVar] = -d.z;
            
            OntoMath::LineAlgebraicEvaluator ev(m.envForm, bind, true);
            if (!ev.ok()) {
                ++evaluatorNotOkSkips;
                continue;
            }
            
            const std::vector<std::pair<std::size_t, uint32_t>> target{{0, m.envForm.roots[0]}};
            const double resolution = 1e-4 * (tb - ta);
            OntoMath::LineSegmentationStats segStats;
            const auto pieces = OntoMath::segmentLine({&ev}, ta, tb, resolution, &segStats, 1u << 16, true, nullptr, &target);
            totalRangeEvaluations += segStats.rangeEvaluations;
            totalProposedRoots += segStats.proposedRoots;
            if (segStats.budgetExhausted) ++totalBudgetExhausted;
            
            for (const auto& p : pieces) {
                bool isDef = p.definite;
                if (isDef) ++totalDefinitePieces;
                else ++totalIndefinitePieces;
                
                if (!p.targetSigns.empty() && p.targetSigns[0] < 0) {
                    if (isDef) ++defRejTargetSign;
                    else ++indefRejTargetSign;
                    continue;
                }
                bool undefinedD = false;
                const auto rd = ev.range(m.envForm.roots[0], p.s0, p.s1, p.signs[0], &undefinedD);
                if (undefinedD) {
                    if (isDef) ++defUndefinedRetained;
                    else ++indefUndefinedRetained;
                    allPieces.push_back(Piece{(int)j, p.s0, p.s1, p.signs[0]});
                    continue;
                }
                if (double(rd.hi) <= 0.0) {
                    if (isDef) ++defRejHiLEZero;
                    else ++indefRejHiLEZero;
                    continue;
                }
                if (isDef) ++defRetained;
                else ++indefRetained;
                allPieces.push_back(Piece{(int)j, p.s0, p.s1, p.signs[0]});
            }
        }
        
        std::vector<double> events;
        for (const auto& pc : allPieces) {
            events.push_back(pc.s0);
            events.push_back(pc.s1);
        }
        std::sort(events.begin(), events.end());
        events.erase(std::unique(events.begin(), events.end()), events.end());
        
        auto tDisc1 = std::chrono::steady_clock::now();
        totalDiscoveryMs += std::chrono::duration<double, std::milli>(tDisc1 - tDisc0).count();
        totalDiscoveredPieces += allPieces.size();
        
        // --- CONSUMER PHASE (Quadrature) ---
        auto tQuad0 = std::chrono::steady_clock::now();
        
        double TA_frozen = 1.0; glm::dvec3 LA_frozen(0.0);
        double TA_star = 1.0;   glm::dvec3 LA_star(0.0);
        
        long long raySetupsNoFusion = 0;
        long long raySetupsWithFusion = 0;
        
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
                
                int n_sub = std::max(1, int(std::ceil(h / 4.0)));
                double sub_h = h / n_sub;
                totalSubpanels += n_sub;
                
                for (int sub = 0; sub < n_sub; ++sub) {
                    double sub_s0 = s0 + sub * sub_h;
                    double s_m = sub_s0 + gx[0] * sub_h;
                    double s_p = sub_s0 + gx[1] * sub_h;
                    auto w_m = ro + s_m * d;
                    auto w_p = ro + s_p * d;
                    
                    // Native f32 coordinate cell predicate:
                    glm::vec3 pf_m(w_m - m.origin);
                    glm::vec3 pf_p(w_p - m.origin);
                    glm::vec3 Pf_m = float(m.noiseFreq) * pf_m;
                    glm::vec3 Pf_p = float(m.noiseFreq) * pf_p;
                    glm::ivec3 cf_m(std::floor(Pf_m.x), std::floor(Pf_m.y), std::floor(Pf_m.z));
                    glm::ivec3 cf_p(std::floor(Pf_p.x), std::floor(Pf_p.y), std::floor(Pf_p.z));
                    
                    bool f32SameCell = (cf_m == cf_p);
                    if (f32SameCell) ++totalF32SameCell;
                    else ++totalF32CrossCell;
                    
                    raySetupsNoFusion += 2;
                    raySetupsWithFusion += (f32SameCell ? 1 : 2);
                    
                    // Evaluate density at nodes
                    double D_m = evalD(mid, w_m);
                    double D_p = evalD(mid, w_p);
                    
                    // 1. Frozen Consumer A (from §6bv/§6bw)
                    double sigma_avg_A = std::max(m.alpha * (gw[0] * D_m + gw[1] * D_p), 1e-6);
                    double tau_A = sub_h * sigma_avg_A;
                    double atten_A = std::exp(-tau_A);
                    LA_frozen += TA_frozen * m.color * (1.0 - atten_A);
                    TA_frozen *= atten_A;
                    
                    // 2. Corrected Consumer A* (Pointwise-Projected Production Semantics)
                    double sig_m = (D_m > 0) ? std::max(m.alpha * D_m, 1e-6) : 0.0;
                    glm::dvec3 em_m = (D_m > 0) ? (m.color * (m.alpha * D_m)) : glm::dvec3(0.0);
                    
                    double sig_p = (D_p > 0) ? std::max(m.alpha * D_p, 1e-6) : 0.0;
                    glm::dvec3 em_p = (D_p > 0) ? (m.color * (m.alpha * D_p)) : glm::dvec3(0.0);
                    
                    // Census state
                    bool m_pos = (D_m > 0), p_pos = (D_p > 0);
                    if (!m_pos || !p_pos) ++countZeroDensity;
                    else {
                        bool m_floor = (m.alpha * D_m <= 1e-6);
                        bool p_floor = (m.alpha * D_p <= 1e-6);
                        if (!m_floor && !p_floor) ++countBothPositiveUnfloored;
                        else if (m_floor && p_floor) ++countBothFloored;
                        else ++countMixedFloor;
                    }
                    
                    double sig_avg = gw[0] * sig_m + gw[1] * sig_p;
                    glm::dvec3 em_avg = gw[0] * em_m + gw[1] * em_p;
                    double tau_star = sub_h * sig_avg;
                    double atten_star = std::exp(-tau_star);
                    double gain = (tau_star == 0.0) ? sub_h : ((1.0 - atten_star) / sig_avg);
                    
                    LA_star += TA_star * em_avg * gain;
                    TA_star *= atten_star;
                }
            } else {
                // Overlap interval: 2 panels CF4-2 (production coupled transfer)
                int panels = 2;
                double pan_h = h / panels;
                const double a = 0.25 - std::sqrt(3.0) / 6.0, b = 0.5 - a;
                raySetupsNoFusion += 4 * activeMedia.size();
                raySetupsWithFusion += 4 * activeMedia.size();
                
                for (int p_idx = 0; p_idx < panels; ++p_idx) {
                    double p_lo = s0 + p_idx * pan_h;
                    auto u = at(media, activeMedia, ro, d, p_lo + (0.5 - std::sqrt(3.0) / 6.0) * pan_h, clock);
                    auto v = at(media, activeMedia, ro, d, p_lo + (0.5 + std::sqrt(3.0) / 6.0) * pan_h, clock);
                    
                    for (auto w : std::array<std::array<double,2>,2>{{{b, a}, {a, b}}}) {
                        double sigma = w[0] * u[0] + w[1] * v[0];
                        double x = pan_h * sigma;
                        double atten = std::exp(-x);
                        double gain = pan_h * (x == 0 ? 1 : -std::expm1(-x) / x);
                        
                        for (int j = 1; j < 4; ++j) {
                            double emit = w[0] * u[j] + w[1] * v[j];
                            LA_frozen[j - 1] += TA_frozen * emit * gain;
                            LA_star[j - 1] += TA_star * emit * gain;
                        }
                        TA_frozen *= atten;
                        TA_star *= atten;
                    }
                }
            }
        }
        
        f32SetupsPerRayNoFusion[i] = raySetupsNoFusion;
        f32SetupsPerRayWithFusion[i] = raySetupsWithFusion;
        
        auto tQuad1 = std::chrono::steady_clock::now();
        totalQuadratureMs += std::chrono::duration<double, std::milli>(tQuad1 - tQuad0).count();
        
        // Reference truth
        const auto& truth = Rows[i];
        double trueT = truth[2].get<double>();
        glm::dvec3 trueRGB(truth[3].get<double>(), truth[4].get<double>(), truth[5].get<double>());
        
        // Native baseline errors
        double errT_nat = std::abs(TNative - trueT);
        double errR_nat = std::abs(LNative.x - trueRGB.x);
        double errG_nat = std::abs(LNative.y - trueRGB.y);
        double errB_nat = std::abs(LNative.z - trueRGB.z);
        trNative.errT.push_back(errT_nat);
        trNative.errRGB_channel.push_back(errR_nat);
        trNative.errRGB_channel.push_back(errG_nat);
        trNative.errRGB_channel.push_back(errB_nat);
        trNative.errRGB_ray.push_back(std::max({errR_nat, errG_nat, errB_nat}));
        
        // Frozen A errors
        double errT_Af = std::abs(TA_frozen - trueT);
        double errR_Af = std::abs(LA_frozen.x - trueRGB.x);
        double errG_Af = std::abs(LA_frozen.y - trueRGB.y);
        double errB_Af = std::abs(LA_frozen.z - trueRGB.z);
        trA_frozen.errT.push_back(errT_Af);
        trA_frozen.errRGB_channel.push_back(errR_Af);
        trA_frozen.errRGB_channel.push_back(errG_Af);
        trA_frozen.errRGB_channel.push_back(errB_Af);
        trA_frozen.errRGB_ray.push_back(std::max({errR_Af, errG_Af, errB_Af}));
        
        // Corrected A* errors
        double errT_As = std::abs(TA_star - trueT);
        double errR_As = std::abs(LA_star.x - trueRGB.x);
        double errG_As = std::abs(LA_star.y - trueRGB.y);
        double errB_As = std::abs(LA_star.z - trueRGB.z);
        trA_star.errT.push_back(errT_As);
        trA_star.errRGB_channel.push_back(errR_As);
        trA_star.errRGB_channel.push_back(errG_As);
        trA_star.errRGB_channel.push_back(errB_As);
        trA_star.errRGB_ray.push_back(std::max({errR_As, errG_As, errB_As}));
    }
    
    auto tEnd = std::chrono::steady_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    
    // Per-Ray Regression Analysis for Corrected A*
    long long aStarWorseThanNat_RGB = 0, aStarWorseThanNat_T = 0;
    double aStarMaxDeltaNat_RGB = 0.0, aStarMaxDeltaNat_T = 0.0;
    
    long long aStarWorseThanAf_RGB = 0, aStarWorseThanAf_T = 0;
    double aStarMaxDeltaAf_RGB = 0.0, aStarMaxDeltaAf_T = 0.0;
    
    for (size_t i = 0; i < count; ++i) {
        // A* vs Native
        double dRGB_nat = trA_star.errRGB_ray[i] - trNative.errRGB_ray[i];
        if (dRGB_nat > 1e-12) { ++aStarWorseThanNat_RGB; aStarMaxDeltaNat_RGB = std::max(aStarMaxDeltaNat_RGB, dRGB_nat); }
        double dT_nat = trA_star.errT[i] - trNative.errT[i];
        if (dT_nat > 1e-12) { ++aStarWorseThanNat_T; aStarMaxDeltaNat_T = std::max(aStarMaxDeltaNat_T, dT_nat); }
        
        // A* vs Frozen A
        double dRGB_Af = trA_star.errRGB_ray[i] - trA_frozen.errRGB_ray[i];
        if (dRGB_Af > 1e-12) { ++aStarWorseThanAf_RGB; aStarMaxDeltaAf_RGB = std::max(aStarMaxDeltaAf_RGB, dRGB_Af); }
        double dT_Af = trA_star.errT[i] - trA_frozen.errT[i];
        if (dT_Af > 1e-12) { ++aStarWorseThanAf_T; aStarMaxDeltaAf_T = std::max(aStarMaxDeltaAf_T, dT_Af); }
    }
    
    J report;
    report["scope"] = "Consumer A* (Corrected Pointwise-Projected GL2) & f32 Predicate Preflight";
    report["rays"] = count;
    report["timing_ms"] = {
        {"total", totalMs},
        {"discovery_total", totalDiscoveryMs},
        {"quadrature_total", totalQuadratureMs},
        {"ms_per_ray_total", totalMs / count},
        {"ms_per_ray_discovery", totalDiscoveryMs / count},
        {"ms_per_ray_quadrature", totalQuadratureMs / count}
    };
    
    report["discovery_work"] = {
        {"total_pieces_discovered", totalDiscoveredPieces},
        {"pieces_per_ray", double(totalDiscoveredPieces) / count},
        {"total_definite_pieces", totalDefinitePieces},
        {"definite_retained", defRetained},
        {"definite_rejected_target_sign", defRejTargetSign},
        {"definite_rejected_hi_le_zero", defRejHiLEZero},
        {"definite_undefined_retained", defUndefinedRetained},
        {"total_indefinite_pieces", totalIndefinitePieces},
        {"indefinite_retained", indefRetained},
        {"indefinite_rejected_target_sign", indefRejTargetSign},
        {"indefinite_rejected_hi_le_zero", indefRejHiLEZero},
        {"indefinite_undefined_retained", indefUndefinedRetained},
        {"indefinite_sliver_pct", double(totalIndefinitePieces) * 100.0 / (totalDefinitePieces + totalIndefinitePieces + 1e-9)},
        {"media_box_miss_skips", mediaBoxMissSkips},
        {"evaluator_not_ok_skips", evaluatorNotOkSkips},
        {"total_range_evaluations", totalRangeEvaluations},
        {"range_evaluations_per_ray", double(totalRangeEvaluations) / count},
        {"total_proposed_roots", totalProposedRoots},
        {"proposed_roots_per_ray", double(totalProposedRoots) / count},
        {"budget_exhausted_calls", totalBudgetExhausted}
    };
    
    report["f32_same_cell_census"] = {
        {"total_subpanels", totalSubpanels},
        {"f32_same_cell_pairs", totalF32SameCell},
        {"f32_same_cell_pct", double(totalF32SameCell) * 100.0 / totalSubpanels},
        {"f32_cross_cell_pairs", totalF32CrossCell},
        {"f32_cross_cell_pct", double(totalF32CrossCell) * 100.0 / totalSubpanels},
        {"setups_per_ray_without_fusion", stats(f32SetupsPerRayNoFusion)},
        {"setups_per_ray_with_fusion", stats(f32SetupsPerRayWithFusion)},
        {"setup_reduction_pct", (stats(f32SetupsPerRayNoFusion)["mean"].get<double>() - stats(f32SetupsPerRayWithFusion)["mean"].get<double>()) * 100.0 / stats(f32SetupsPerRayNoFusion)["mean"].get<double>()}
    };
    
    report["subpanel_production_state_census"] = {
        {"both_positive_unfloored", {{"count", countBothPositiveUnfloored}, {"pct", double(countBothPositiveUnfloored) * 100.0 / totalSubpanels}}},
        {"both_floored", {{"count", countBothFloored}, {"pct", double(countBothFloored) * 100.0 / totalSubpanels}}},
        {"mixed_floor_state", {{"count", countMixedFloor}, {"pct", double(countMixedFloor) * 100.0 / totalSubpanels}}},
        {"zero_density_node", {{"count", countZeroDensity}, {"pct", double(countZeroDensity) * 100.0 / totalSubpanels}}}
    };
    
    auto makeTrackReport = [&](const std::string& name, const ErrorTracker& tr) -> J {
        return J{
            {"name", name},
            {"RGB_pooled_channels", stats(tr.errRGB_channel)},
            {"RGB_max_per_ray", stats(tr.errRGB_ray)},
            {"T", stats(tr.errT)}
        };
    };
    
    report["candidates"] = {
        {"current_native", makeTrackReport("Current Native Baseline", trNative)},
        {"consumer_A_frozen", makeTrackReport("Consumer A (Frozen §6bv GL2)", trA_frozen)},
        {"consumer_A_star", makeTrackReport("Consumer A* (Corrected Pointwise-Projected GL2)", trA_star)}
    };
    
    report["regressions_A_star"] = {
        {"A_star_vs_Native", {
            {"RGB_worse_rays", aStarWorseThanNat_RGB},
            {"RGB_worse_pct", double(aStarWorseThanNat_RGB) * 100.0 / count},
            {"RGB_max_delta", aStarMaxDeltaNat_RGB},
            {"T_worse_rays", aStarWorseThanNat_T},
            {"T_worse_pct", double(aStarWorseThanNat_T) * 100.0 / count},
            {"T_max_delta", aStarMaxDeltaNat_T}
        }},
        {"A_star_vs_Frozen_A", {
            {"RGB_worse_rays", aStarWorseThanAf_RGB},
            {"RGB_worse_pct", double(aStarWorseThanAf_RGB) * 100.0 / count},
            {"RGB_max_delta", aStarMaxDeltaAf_RGB},
            {"T_worse_rays", aStarWorseThanAf_T},
            {"T_worse_pct", double(aStarWorseThanAf_T) * 100.0 / count},
            {"T_max_delta", aStarMaxDeltaAf_T}
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
    probe_cpp = Path("/private/tmp/reproducer_consumer_a_star.cpp")
    probe_o = Path("/private/tmp/reproducer_consumer_a_star.o")
    probe_bin = Path("/private/tmp/reproducer_consumer_a_star")
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
    print("Compiled Consumer A* preflight successfully.")
    
    # 3. Execute
    cmd = [str(probe_bin)]
    if args.output:
        cmd.append(str(args.output.resolve()))
    subprocess.run(cmd, cwd=r, check=True)

if __name__ == "__main__":
    main()
