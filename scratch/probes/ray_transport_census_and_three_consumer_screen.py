"""Northern Veil — Census of Intra-Subpanel GL2 Dual-Noise Same-Cell Fusion
and Three-Consumer CPU Screen (A: Frozen §6bv, B: Moment-Weighted GL2, C: Barycentric 1-Noise).

Executed by Sparkly Guy (Gemini 3.8 Flash High)
Reviewed by Aetherion / The Firmament (Gemini 4 Argon)
Responding to Original Prism Sun (GPT-5.6 Sol, §§6, 8, 9 of PRISM_REVIEW_6BV_AND_FIRMAMENT_NATIVE_GATE_2026-10-21.md)
Human origin: Zachary Zhang.
Branch: claude/ray-domain-transport-rung0
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
    if (e.empty()) return J{{"mean", 0.0}, {"rms", 0.0}, {"p50", 0.0}, {"p90", 0.0}, {"p99", 0.0}, {"max", 0.0}, {"min", 0.0}};
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
    
    // GL2 nodes
    const double gx[2] = {0.5 * (1.0 - std::sqrt(3.0) / 3.0), 0.5 * (1.0 + std::sqrt(3.0) / 3.0)};
    const double gw[2] = {0.5, 0.5};
    
    // -------------------------------------------------------------
    // PART 1: Intra-Subpanel GL2 Dual-Noise Same-Cell Census
    // -------------------------------------------------------------
    struct CurtainCensus {
        long long totalSubpanels = 0;
        long long sameCellPairs = 0;
        long long crossCellPairs = 0;
        long long crossed1Axis = 0;
        long long crossed2Axes = 0;
        long long crossed3Axes = 0;
        // Floor correlation
        long long same_floorInactive = 0;
        long long same_floorActive = 0;
        long long same_floorCrossing = 0;
        long long cross_floorInactive = 0;
        long long cross_floorActive = 0;
        long long cross_floorCrossing = 0;
    };
    std::vector<CurtainCensus> curtainStats(media.size());
    
    std::vector<double> setupsPerRayNoFusion(count, 0.0);
    std::vector<double> setupsPerRayWithFusion(count, 0.0);
    std::vector<double> subpanelsPerRay(count, 0.0);
    std::vector<double> sameCellPairsPerRay(count, 0.0);
    std::vector<double> crossCellPairsPerRay(count, 0.0);
    
    long long globalSubpanels = 0;
    long long globalSameCell = 0;
    long long globalCrossCell = 0;
    long long globalCrossed1 = 0;
    long long globalCrossed2 = 0;
    long long globalCrossed3 = 0;
    
    // -------------------------------------------------------------
    // PART 2: Three Consumers (A, B, C)
    // -------------------------------------------------------------
    struct ConsumerResult {
        std::vector<double> errRGB;
        std::vector<double> errT;
        long long totalNoiseCalls = 0;
        long long totalEnvelopeCalls = 0;
        long long totalMomentCalls = 0;
        long long totalASTCalls = 0;
        long long totalEvaluations = 0;
        long long admittedSubpanels = 0;
        long long fallbackSubpanels = 0;
        double totalMs = 0.0;
        std::vector<size_t> worsenedRaysRGB;
        std::vector<size_t> worsenedRaysT;
    };
    ConsumerResult resA, resB, resC;
    
    // Helper to evaluate density
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
    
    // Helper to evaluate envelope
    auto evalEnv = [&](int mid, const glm::dvec3& world) -> double {
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
        double E = 0;
        auto v = m.envAST->evaluate(vars);
        if (v && propertyValueToNumber(*v, E) && E > 0) return E;
        return 0.0;
    };
    
    // Helper to evaluate Perlin noise factor w = 0.7 + 0.3 * perlin(nu * p)
    auto evalNoise = [&](int mid, const glm::dvec3& world) -> double {
        const auto& m = media[mid];
        const glm::vec3 pf(world - m.origin);
        return 0.7 + 0.3 * double(glm::perlin(float(m.noiseFreq) * pf));
    };
    
    // Main ray execution loop
    auto tStart = std::chrono::steady_clock::now();
    
    for (size_t i = 0; i < count; ++i) {
        const auto& r = R[i];
        const glm::dvec3 d(r[2], r[3], r[4]);
        const double clipA = r[5], clipB = r[6];
        
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
            
            OntoMath::LineAlgebraicEvaluator ev(m.envForm, bind, true);
            if (!ev.ok()) continue;
            
            const std::vector<std::pair<std::size_t, uint32_t>> target{{0, m.envForm.roots[0]}};
            const double resolution = 1e-4 * (tb - ta);
            const auto pieces = OntoMath::segmentLine({&ev}, ta, tb, resolution, nullptr, 1u << 16, true, nullptr, &target);
            
            for (const auto& p : pieces) {
                if (!p.targetSigns.empty() && p.targetSigns[0] < 0) continue;
                bool undefinedD = false;
                const auto rd = ev.range(m.envForm.roots[0], p.s0, p.s1, p.signs[0], &undefinedD);
                if (!undefinedD && double(rd.hi) <= 0.0) continue;
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
        
        // Per-ray state for Consumer A
        double TA = 1.0; glm::dvec3 LA(0.0);
        // Per-ray state for Consumer B
        double TB = 1.0; glm::dvec3 LB(0.0);
        // Per-ray state for Consumer C
        double TC = 1.0; glm::dvec3 LC(0.0);
        
        long long raySubpanels = 0;
        long long raySameCell = 0;
        long long rayCrossCell = 0;
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
                auto& cs = curtainStats[mid];
                
                int n_sub = std::max(1, int(std::ceil(h / 4.0)));
                double sub_h = h / n_sub;
                
                raySubpanels += n_sub;
                globalSubpanels += n_sub;
                cs.totalSubpanels += n_sub;
                
                for (int sub = 0; sub < n_sub; ++sub) {
                    double sub_s0 = s0 + sub * sub_h;
                    double s_m = sub_s0 + gx[0] * sub_h;
                    double s_p = sub_s0 + gx[1] * sub_h;
                    
                    auto w_m = ro + s_m * d;
                    auto w_p = ro + s_p * d;
                    
                    glm::dvec3 p_m = w_m - m.origin;
                    glm::dvec3 p_p = w_p - m.origin;
                    
                    glm::dvec3 P_m = m.noiseFreq * p_m;
                    glm::dvec3 P_p = m.noiseFreq * p_p;
                    
                    glm::ivec3 c_m(std::floor(P_m.x), std::floor(P_m.y), std::floor(P_m.z));
                    glm::ivec3 c_p(std::floor(P_p.x), std::floor(P_p.y), std::floor(P_p.z));
                    
                    bool sameCell = (c_m == c_p);
                    int axesCrossed = (c_m.x != c_p.x ? 1 : 0) + (c_m.y != c_p.y ? 1 : 0) + (c_m.z != c_p.z ? 1 : 0);
                    
                    raySetupsNoFusion += 2;
                    raySetupsWithFusion += (sameCell ? 1 : 2);
                    
                    if (sameCell) {
                        ++raySameCell;
                        ++globalSameCell;
                        ++cs.sameCellPairs;
                    } else {
                        ++rayCrossCell;
                        ++globalCrossCell;
                        ++cs.crossCellPairs;
                        if (axesCrossed == 1) { ++globalCrossed1; ++cs.crossed1Axis; }
                        else if (axesCrossed == 2) { ++globalCrossed2; ++cs.crossed2Axes; }
                        else if (axesCrossed == 3) { ++globalCrossed3; ++cs.crossed3Axes; }
                    }
                    
                    // Consumer A: direct complete-product GL2
                    double D_m = evalD(mid, w_m);
                    double D_p = evalD(mid, w_p);
                    resA.totalASTCalls += 2;
                    resA.totalEvaluations += 2;
                    resA.totalNoiseCalls += 2;
                    
                    double sigma_avg_A = std::max(m.alpha * (gw[0] * D_m + gw[1] * D_p), 1e-6);
                    double tau_A = sub_h * sigma_avg_A;
                    double atten_A = std::exp(-tau_A);
                    LA += TA * m.color * (1.0 - atten_A);
                    TA *= atten_A;
                    
                    // Extinction floor classification at Gauss nodes
                    double sigma_m = m.alpha * D_m;
                    double sigma_p = m.alpha * D_p;
                    bool m_active = (sigma_m <= 1e-6);
                    bool p_active = (sigma_p <= 1e-6);
                    bool floorInactive = (!m_active && !p_active);
                    bool floorActive = (m_active && p_active);
                    bool floorCrossing = (m_active != p_active);
                    
                    if (sameCell) {
                        if (floorInactive) ++cs.same_floorInactive;
                        else if (floorActive) ++cs.same_floorActive;
                        else ++cs.same_floorCrossing;
                    } else {
                        if (floorInactive) ++cs.cross_floorInactive;
                        else if (floorActive) ++cs.cross_floorActive;
                        else ++cs.cross_floorCrossing;
                    }
                    
                    // Consumer B & C: envelope moments
                    // On this subpanel, evaluate envelope at 3 Gauss points to compute accurate moments (M0, M1)
                    // Gauss-Legendre 3-point on [sub_s0, sub_s0 + sub_h]
                    const double gx3[3] = {0.5 * (1.0 - std::sqrt(0.6)), 0.5, 0.5 * (1.0 + std::sqrt(0.6))};
                    const double gw3[3] = {5.0 / 18.0, 8.0 / 18.0, 5.0 / 18.0};
                    const double gu3[3] = {-std::sqrt(0.6), 0.0, std::sqrt(0.6)};
                    
                    double E3[3];
                    for (int k = 0; k < 3; ++k) {
                        E3[k] = evalEnv(mid, ro + (sub_s0 + gx3[k] * sub_h) * d);
                    }
                    
                    double M0 = 0.0, M1 = 0.0;
                    for (int k = 0; k < 3; ++k) {
                        M0 += gw3[k] * E3[k] * sub_h;
                        M1 += gw3[k] * gu3[k] * E3[k] * sub_h;
                    }
                    
                    // Consumer B: moment-weighted GL2 on floor-inactive subpanels
                    if (floorInactive && M0 > 0.0) {
                        ++resB.admittedSubpanels;
                        resB.totalEnvelopeCalls += 3;
                        resB.totalMomentCalls += 1;
                        
                        double w_noise_m = evalNoise(mid, w_m);
                        double w_noise_p = evalNoise(mid, w_p);
                        resB.totalNoiseCalls += 2;
                        resB.totalEvaluations += 2;
                        
                        double I_lin = M0 * 0.5 * (w_noise_p + w_noise_m) + M1 * (0.5 * std::sqrt(3.0) * (w_noise_p - w_noise_m));
                        double sigma_avg_B = std::max(m.alpha * (I_lin / sub_h), 1e-6);
                        double tau_B = sub_h * sigma_avg_B;
                        double atten_B = std::exp(-tau_B);
                        LB += TB * m.color * (1.0 - atten_B);
                        TB *= atten_B;
                    } else {
                        // Fall back to Consumer A
                        ++resB.fallbackSubpanels;
                        resB.totalASTCalls += 2;
                        resB.totalEvaluations += 2;
                        resB.totalNoiseCalls += 2;
                        
                        double tau_B = sub_h * sigma_avg_A;
                        double atten_B = std::exp(-tau_B);
                        LB += TB * m.color * (1.0 - atten_B);
                        TB *= atten_B;
                    }
                    
                    // Consumer C: barycentric 1-noise on floor-inactive subpanels
                    if (floorInactive && M0 > 1e-12) {
                        ++resC.admittedSubpanels;
                        resC.totalEnvelopeCalls += 3;
                        resC.totalMomentCalls += 1;
                        
                        double u_star = M1 / M0;
                        u_star = std::clamp(u_star, -0.99, 0.99);
                        double s_star = sub_s0 + 0.5 * (1.0 + u_star) * sub_h;
                        
                        double w_noise_star = evalNoise(mid, ro + s_star * d);
                        resC.totalNoiseCalls += 1;
                        resC.totalEvaluations += 1;
                        
                        double I_c = M0 * w_noise_star;
                        double sigma_avg_C = std::max(m.alpha * (I_c / sub_h), 1e-6);
                        double tau_C = sub_h * sigma_avg_C;
                        double atten_C = std::exp(-tau_C);
                        LC += TC * m.color * (1.0 - atten_C);
                        TC *= atten_C;
                    } else {
                        // Fall back to Consumer A
                        ++resC.fallbackSubpanels;
                        resC.totalASTCalls += 2;
                        resC.totalEvaluations += 2;
                        resC.totalNoiseCalls += 2;
                        
                        double tau_C = sub_h * sigma_avg_A;
                        double atten_C = std::exp(-tau_C);
                        LC += TC * m.color * (1.0 - atten_C);
                        TC *= atten_C;
                    }
                }
            } else {
                // Multi-curtain overlap interval: 2 panels of CF4-2 (same for A, B, C)
                int panels = 2;
                double pan_h = h / panels;
                const double a = 0.25 - std::sqrt(3.0) / 6.0, b = 0.5 - a;
                raySetupsNoFusion += 4 * activeMedia.size();
                raySetupsWithFusion += 4 * activeMedia.size();
                
                for (int p_idx = 0; p_idx < panels; ++p_idx) {
                    double p_lo = s0 + p_idx * pan_h;
                    auto u = at(media, activeMedia, ro, d, p_lo + (0.5 - std::sqrt(3.0) / 6.0) * pan_h, clock);
                    auto v = at(media, activeMedia, ro, d, p_lo + (0.5 + std::sqrt(3.0) / 6.0) * pan_h, clock);
                    
                    resA.totalEvaluations += 4; resA.totalASTCalls += 4;
                    resB.totalEvaluations += 4; resB.totalASTCalls += 4;
                    resC.totalEvaluations += 4; resC.totalASTCalls += 4;
                    
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
        
        subpanelsPerRay[i] = raySubpanels;
        sameCellPairsPerRay[i] = raySameCell;
        crossCellPairsPerRay[i] = rayCrossCell;
        setupsPerRayNoFusion[i] = raySetupsNoFusion;
        setupsPerRayWithFusion[i] = raySetupsWithFusion;
        
        const auto& truth = Rows[i];
        double trueT = truth[2].get<double>();
        glm::dvec3 trueRGB(truth[3].get<double>(), truth[4].get<double>(), truth[5].get<double>());
        
        // Consumer A errors
        double errTA = std::abs(TA - trueT);
        double errRGBA = std::max({std::abs(LA.x - trueRGB.x), std::abs(LA.y - trueRGB.y), std::abs(LA.z - trueRGB.z)});
        resA.errT.push_back(errTA);
        resA.errRGB.push_back(errRGBA);
        if (errRGBA > 1.958e-3) resA.worsenedRaysRGB.push_back(i);
        if (errTA > 3.025e-3) resA.worsenedRaysT.push_back(i);
        
        // Consumer B errors
        double errTB = std::abs(TB - trueT);
        double errRGBB = std::max({std::abs(LB.x - trueRGB.x), std::abs(LB.y - trueRGB.y), std::abs(LB.z - trueRGB.z)});
        resB.errT.push_back(errTB);
        resB.errRGB.push_back(errRGBB);
        if (errRGBB > 1.958e-3) resB.worsenedRaysRGB.push_back(i);
        if (errTB > 3.025e-3) resB.worsenedRaysT.push_back(i);
        
        // Consumer C errors
        double errTC = std::abs(TC - trueT);
        double errRGBC = std::max({std::abs(LC.x - trueRGB.x), std::abs(LC.y - trueRGB.y), std::abs(LC.z - trueRGB.z)});
        resC.errT.push_back(errTC);
        resC.errRGB.push_back(errRGBC);
        if (errRGBC > 1.958e-3) resC.worsenedRaysRGB.push_back(i);
        if (errTC > 3.025e-3) resC.worsenedRaysT.push_back(i);
    }
    
    auto tEnd = std::chrono::steady_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    
    // -------------------------------------------------------------
    // Compile JSON Report
    // -------------------------------------------------------------
    J report;
    report["scope"] = "Intra-Subpanel GL2 Dual-Noise Census and Three-Consumer CPU Screen";
    report["rays"] = count;
    report["total_time_ms"] = totalMs;
    report["ms_per_ray"] = totalMs / count;
    
    // Census Section
    J census;
    census["total_subpanels"] = globalSubpanels;
    census["subpanels_per_ray"] = double(globalSubpanels) / count;
    census["same_cell_pairs"] = globalSameCell;
    census["same_cell_pct"] = double(globalSameCell) * 100.0 / globalSubpanels;
    census["cross_cell_pairs"] = globalCrossCell;
    census["cross_cell_pct"] = double(globalCrossCell) * 100.0 / globalSubpanels;
    
    census["axes_crossed"] = {
        {"1_axis", {{"count", globalCrossed1}, {"pct", double(globalCrossed1) * 100.0 / globalCrossCell}}},
        {"2_axes", {{"count", globalCrossed2}, {"pct", double(globalCrossed2) * 100.0 / globalCrossCell}}},
        {"3_axes", {{"count", globalCrossed3}, {"pct", double(globalCrossed3) * 100.0 / globalCrossCell}}}
    };
    
    J byCurtain = J::array();
    for (size_t j = 0; j < media.size(); ++j) {
        const auto& cs = curtainStats[j];
        byCurtain.push_back({
            {"curtain_id", media[j].id},
            {"alpha", alphas[j]},
            {"noise_freq", freqs[j]},
            {"subpanels", cs.totalSubpanels},
            {"same_cell_pct", double(cs.sameCellPairs) * 100.0 / std::max(1LL, cs.totalSubpanels)},
            {"cross_cell_pct", double(cs.crossCellPairs) * 100.0 / std::max(1LL, cs.totalSubpanels)},
            {"axes_crossed", {
                {"1_axis", cs.crossed1Axis},
                {"2_axes", cs.crossed2Axes},
                {"3_axes", cs.crossed3Axes}
            }},
            {"floor_correlation", {
                {"same_cell", {
                    {"inactive", cs.same_floorInactive},
                    {"active", cs.same_floorActive},
                    {"crossing", cs.same_floorCrossing}
                }},
                {"cross_cell", {
                    {"inactive", cs.cross_floorInactive},
                    {"active", cs.cross_floorActive},
                    {"crossing", cs.cross_floorCrossing}
                }}
            }}
        });
    }
    census["by_curtain"] = byCurtain;
    
    census["setups_per_ray"] = {
        {"without_fusion", stats(setupsPerRayNoFusion)},
        {"with_fusion", stats(setupsPerRayWithFusion)},
        {"subpanels", stats(subpanelsPerRay)},
        {"same_cell_pairs", stats(sameCellPairsPerRay)},
        {"cross_cell_pairs", stats(crossCellPairsPerRay)}
    };
    
    double meanNo = census["setups_per_ray"]["without_fusion"]["mean"].get<double>();
    double meanWith = census["setups_per_ray"]["with_fusion"]["mean"].get<double>();
    census["setups_reduction_pct"] = (meanNo - meanWith) * 100.0 / meanNo;
    
    report["census"] = census;
    
    // Screen Section
    auto makeConsumerReport = [&](const std::string& name, const ConsumerResult& cr) -> J {
        return J{
            {"name", name},
            {"evaluations_per_ray", double(cr.totalEvaluations) / count},
            {"noise_calls_per_ray", double(cr.totalNoiseCalls) / count},
            {"envelope_calls_per_ray", double(cr.totalEnvelopeCalls) / count},
            {"moment_calls_per_ray", double(cr.totalMomentCalls) / count},
            {"AST_calls_per_ray", double(cr.totalASTCalls) / count},
            {"admitted_subpanels", cr.admittedSubpanels},
            {"fallback_subpanels", cr.fallbackSubpanels},
            {"admitted_pct", double(cr.admittedSubpanels) * 100.0 / std::max(1LL, cr.admittedSubpanels + cr.fallbackSubpanels)},
            {"RGB", stats(cr.errRGB)},
            {"T", stats(cr.errT)},
            {"worsened_rays_RGB_count", cr.worsenedRaysRGB.size()},
            {"worsened_rays_T_count", cr.worsenedRaysT.size()}
        };
    };
    
    report["screen"] = {
        {"consumer_A", makeConsumerReport("Consumer A (Frozen §6bv GL2 Control)", resA)},
        {"consumer_B", makeConsumerReport("Consumer B (Moment-Weighted GL2)", resB)},
        {"consumer_C", makeConsumerReport("Consumer C (Barycentric 1-Noise)", resC)}
    };
    
    // Target comparisons
    report["native_targets"] = {
        {"RGB_mean", 5.223e-5},
        {"RGB_p99", 7.491e-4},
        {"RGB_max", 1.958e-3},
        {"T_mean", 1.335e-4},
        {"T_p99", 1.430e-3},
        {"T_max", 3.025e-3}
    };
    
    std::cout << report.dump(2) << std::endl;
    
    if (!outputPath.empty()) {
        std::ofstream out(outputPath);
        out << report.dump(2) << "\n";
        std::cout << "Report written to " << outputPath << "\n";
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
    probe_cpp = Path("/private/tmp/reproducer_census_and_screen.cpp")
    probe_o = Path("/private/tmp/reproducer_census_and_screen.o")
    probe_bin = Path("/private/tmp/reproducer_census_and_screen")
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
    print("Compiled census and screen probe successfully.")
    
    # 3. Execute
    cmd = [str(probe_bin)]
    if args.output:
        cmd.append(str(args.output.resolve()))
    subprocess.run(cmd, cwd=r, check=True)

if __name__ == "__main__":
    main()
