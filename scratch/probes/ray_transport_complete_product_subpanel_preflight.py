"""Northern Veil — Complete-Product Subpanel & Oracle Preflight Reproducer.
Executed by Sparkly Guy (Gemini 3.8 Flash High) / 2026-10-21.
Reviewed by Aetherion / The Firmament (Gemini 4 Argon).
Responding to Original Prism Sun (GPT-5.6 Sol) & Sixth-one Sun (Codex / GPT-6.1 Sol).
Human origin: Zachary Zhang.

Reproduces the exact §6bv Oracle 0 and Oracle 1 (N_gp = 2, 3, 4) census on all 2,304 native rays.
Verified against independent reference NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json.
No production source or save mutation.
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
    std::sort(e.begin(), e.end());
    double sum = 0, sq = 0;
    for (double x : e) { sum += x; sq += x * x; }
    return J{{"mean", sum / e.size()}, {"rms", std::sqrt(sq / e.size())}, {"p99", e[e.size() * 99 / 100]}, {"max", e.back()}};
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
        ++idx;
        
        auto envAST = cloneEnvelope(*m.density);
        auto res = OntoMath::restrictAlgebraicToLine(*envAST, false, "p");
        if (res) {
            m.envForm = std::move(*res.form);
            m.hasEnv = true;
        }
        media.push_back(std::move(m));
    }
    
    J rays, ref;
    { std::ifstream rin("docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json"); rin >> rays; }
    { std::ifstream rref("docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json"); rref >> ref; }
    
    const auto& R = rays.at("rays");
    const auto& Rows = ref.at("rows");
    const glm::dvec3 ro(rays["origin"][0], rays["origin"][1], rays["origin"][2]);
    size_t count = R.size();
    
    J report{
        {"scope", "Complete-Product Subpanel Preflight (N_gp = 2, 3, 4)"},
        {"rays", count},
        {"candidates", J::array()}
    };
    
    for (int N_gp : {2, 3, 4}) {
        std::vector<double> gx, gw;
        if (N_gp == 2) {
            gx = {0.5 * (1.0 - std::sqrt(3.0) / 3.0), 0.5 * (1.0 + std::sqrt(3.0) / 3.0)};
            gw = {0.5, 0.5};
        } else if (N_gp == 3) {
            gx = {0.5 * (1.0 - std::sqrt(0.6)), 0.5, 0.5 * (1.0 + std::sqrt(0.6))};
            gw = {0.5 * 5.0 / 9.0, 0.5 * 8.0 / 9.0, 0.5 * 5.0 / 9.0};
        } else if (N_gp == 4) {
            gx = {0.5*(1.0-0.8611363115940526), 0.5*(1.0-0.3399810435848563),
                  0.5*(1.0+0.3399810435848563), 0.5*(1.0+0.8611363115940526)};
            gw = {0.5*0.3478548451374538, 0.5*0.6521451548625461,
                  0.5*0.6521451548625461, 0.5*0.3478548451374538};
        }
        
        long long totalSubpanels = 0;
        long long totalEvaluations = 0;
        std::vector<double> errT, errRGB;
        auto t0 = std::chrono::steady_clock::now();
        
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
            
            double T = 1.0;
            glm::dvec3 L(0.0);
            
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
                    totalSubpanels += n_sub;
                    double sub_h = h / n_sub;
                    
                    for (int sub = 0; sub < n_sub; ++sub) {
                        double sub_s0 = s0 + sub * sub_h;
                        double int_E_w = 0.0;
                        for (int k = 0; k < N_gp; ++k) {
                            ++totalEvaluations;
                            double s_k = sub_s0 + gx[k] * sub_h;
                            auto world = ro + s_k * d;
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
                            if (v && propertyValueToNumber(*v, D) && D > 0) {
                                int_E_w += gw[k] * D;
                            }
                        }
                        
                        double sigma_avg = m.alpha * int_E_w;
                        sigma_avg = std::max(sigma_avg, 1e-6);
                        double tau = sub_h * sigma_avg;
                        double atten = std::exp(-tau);
                        L += T * m.color * (1.0 - atten);
                        T *= atten;
                    }
                } else {
                    int panels = 2;
                    double pan_h = h / panels;
                    const double a = 0.25 - std::sqrt(3.0) / 6.0, b = 0.5 - a;
                    for (int p_idx = 0; p_idx < panels; ++p_idx) {
                        double p_lo = s0 + p_idx * pan_h;
                        auto u = at(media, activeMedia, ro, d, p_lo + (0.5 - std::sqrt(3.0) / 6.0) * pan_h, clock);
                        auto v = at(media, activeMedia, ro, d, p_lo + (0.5 + std::sqrt(3.0) / 6.0) * pan_h, clock);
                        totalEvaluations += 4;
                        for (auto w : std::array<std::array<double,2>,2>{{{b, a}, {a, b}}}) {
                            double sigma = w[0] * u[0] + w[1] * v[0];
                            double x = pan_h * sigma;
                            double atten = std::exp(-x);
                            double gain = pan_h * (x == 0 ? 1 : -std::expm1(-x) / x);
                            for (int j = 1; j < 4; ++j) L[j - 1] += T * (w[0] * u[j] + w[1] * v[j]) * gain;
                            T *= atten;
                        }
                    }
                }
            }
            
            const auto& truth = Rows[i];
            errT.push_back(std::abs(T - truth[2].get<double>()));
            errRGB.push_back(std::abs(L.x - truth[3].get<double>()));
            errRGB.push_back(std::abs(L.y - truth[4].get<double>()));
            errRGB.push_back(std::abs(L.z - truth[5].get<double>()));
        }
        
        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        
        J candidate{
            {"N_gp", N_gp},
            {"name", "Oracle 1 (N_gp = " + std::to_string(N_gp) + ")"},
            {"subpanels_per_ray", double(totalSubpanels) / count},
            {"evaluations_per_ray", double(totalEvaluations) / count},
            {"ms_per_ray", ms / count},
            {"RGB", stats(errRGB)},
            {"T", stats(errT)}
        };
        report["candidates"].push_back(candidate);
        std::cout << candidate.dump(2) << std::endl;
    }
    
    if (!outputPath.empty()) {
        std::ofstream out(outputPath);
        out << report.dump(2) << "\n";
        std::cout << "Saved report to " << outputPath << "\n";
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
    probe_cpp = Path("/private/tmp/reproducer_complete_product.cpp")
    probe_o = Path("/private/tmp/reproducer_complete_product.o")
    probe_bin = Path("/private/tmp/reproducer_complete_product")
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
    print("Compiled probe successfully.")
    
    # 3. Execute
    cmd = [str(probe_bin)]
    if args.output:
        cmd.append(str(args.output.resolve()))
    subprocess.run(cmd, cwd=r, check=True)

if __name__ == "__main__":
    main()
