// ontomath_veil_transport_feasibility_test — can Northern Veil's REAL curtains
// take the certified transport route (plan §6ar)? A measurement before gate 3.
//
// For every medium in saves/zones/Northern Veil (read only), at t = 0:
//   σ_t  = the authored extinction, or 0.5·D (the compatibility law);
//   ε_c  = max(emission_c, 0) per channel — the marcher's own clamp — with the
//          view direction ω certified over [−1, 1]³ so one certificate covers
//          every pixel's direction;
// certify both (OntoMath::certifyLineQuadrature under kVolumeExecutionModel),
// then plan 12 rays crossing the curtain's authored box (origin ± |scale|),
// entry to exit, at ±1e-2 and ±1e-3 (absolute, emission units). Printed: what
// certifies, σ ≥ 0, admitted fractions and cells. ASSERTED: only soundness —
// on every admitted ray, the double march at the plan's cells lies inside the
// plan's mathematical part against a cumulative long-double reference.
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 · 2026-10-20.

#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/OntoMath/LineRestriction.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/VolumeRayTransport.hpp"

#include "json.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace {
int g_checks = 0, g_failures = 0;
void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) ++g_failures;
    std::cout << (condition ? "  ok: " : "  FAILED: ") << description << "\n";
}
using OntoMath::MathNode;
using OntoMath::ScalarForm;
using Op = MathNode::Op;
using Node = std::unique_ptr<MathNode>;
Node clone(const MathNode& n) { return MathNode::fromJson(n.toJson()); }
Node constant(double c) { auto n = std::make_unique<MathNode>(); n->op = Op::ScalarLeaf; n->scalarForm = ScalarForm::constant(c); return n; }
template <class... Kids> Node node(Op op, Kids... kids) {
    auto n = std::make_unique<MathNode>(); n->op = op; (n->children.push_back(std::move(kids)), ...); return n;
}
const MathNode* singlePiece(const OntoMath::Piecewise* pw) {
    if (!pw || pw->pieces.size() != 1) return nullptr;
    const auto& piece = pw->pieces[0];
    if (piece.guard || piece.call || piece.fold || piece.hasLo || piece.hasHi || piece.whereLEZero) return nullptr;
    return piece.mathNode.get();
}
} // namespace

int main() {
    std::cout << "Starting ontomath_veil_transport_feasibility_test...\n";
    fs::path dir;
    for (const char* candidate : {"saves/zones/Northern Veil", "../saves/zones/Northern Veil"})
        if (fs::exists(fs::path(candidate) / "zone.json") || fs::exists(fs::path(candidate) / "zone.ecform")) { dir = candidate; break; }
    if (dir.empty()) { std::cout << "UNAVAILABLE: Northern Veil's zone was not found from this directory\n"; return 77; }
    nlohmann::json zone;
    if (fs::exists(dir / "zone.json")) {
        std::ifstream in(dir / "zone.json");
        zone = nlohmann::json::parse(in);
    } else {
        std::ifstream in(dir / "zone.ecform", std::ios::binary);
        const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        zone = nlohmann::json::parse(nlohmann::json::from_msgpack(bytes).at("MigrationRoot").get<std::string>());
    }
    const auto& model = Rendering::kVolumeExecutionModel;
    const std::map<std::string, std::pair<double, double>> symbols{
        {OntoMath::kTimeVar, {0.0, 0.0}}, {OntoMath::kOmegaXVar, {-1.0, 1.0}},
        {OntoMath::kOmegaYVar, {-1.0, 1.0}}, {OntoMath::kOmegaZVar, {-1.0, 1.0}}};
    int media = 0, soundRays = 0, admittedRays = 0;
    for (const auto& fj : zone.at("spatialFields")) {
        auto field = geom::FieldNode::fromJson(fj);
        Rendering::VolumeDensityBinding b;
        if (!field || !Rendering::readVolumeDensity(*field, 0.0, 0.0, b) || !b.densityExpr) continue;
        ++media;
        std::printf("  -- %s: box centre (%.3g, %.3g, %.3g) half-span (%.3g, %.3g, %.3g)\n", b.producerId.c_str(), b.origin.x, b.origin.y,
                    b.origin.z, std::abs(b.scale.x), std::abs(b.scale.y), std::abs(b.scale.z));
        const MathNode* ext = singlePiece(b.extinctionExpr);
        const MathNode* den = singlePiece(b.densityExpr);
        const MathNode* emi = singlePiece(b.emissionExpr);
        if ((!ext && !den) || !emi) {
            std::printf("     not a single unguarded piece (extinction/density %s, emission %s): not attempted\n",
                        (ext || den) ? "ok" : "no", emi ? "ok" : "no");
            continue;
        }
        if (std::getenv("EC_PRINT_VEIL")) std::printf("     σ: %s\n     ε: %s\n", (ext ? ext : den)->print().c_str(), emi->print().c_str());
        Node sigma = ext ? clone(*ext) : node(Op::Scale, constant(0.5), clone(*den));
        std::string why;
        const auto sc = OntoMath::certifyLineQuadrature(*sigma, model, 20, &why, symbols);
        if (!sc) { std::printf("     σ_t does not certify: %s\n", why.c_str()); continue; }
        // The band covering the curtain's box.
        const double reach = std::max({std::abs(b.origin.x) + std::abs(b.scale.x), std::abs(b.origin.y) + std::abs(b.scale.y),
                                       std::abs(b.origin.z) + std::abs(b.scale.z)});
        const OntoMath::LineQuadratureBand* sb = nullptr;
        for (const auto& band : sc->bands) if (reach <= band.cap) { sb = &band; break; }
        if (!sb) { std::printf("     no σ band reaches the box (|p| up to %.3g)\n", reach); continue; }
        std::printf("     σ_t band ≤ %g: range [%.3g, %.3g], L %.3g, H %s, device error %.3g → σ ≥ 0 %s\n", sb->cap, sb->low, sb->high,
                    sb->lipschitz, std::isfinite(sb->hessian) ? std::to_string(sb->hessian).c_str() : "unbounded", sb->execution,
                    sb->low >= 0.0 ? "PROVED" : "NOT proved");
        const char* axes[3] = {"x", "y", "z"};
        for (int c = 0; c < 3; ++c) {
            auto component = node(Op::Component, clone(*emi));
            component->stringArg = axes[c];
            // max(ε_c, 0): Intersection is max for scalars (a Clamp with a huge
            // literal upper bound charged that literal's f32 rounding, 1.5e25).
            auto epsilon = node(Op::Intersection, std::move(component), constant(0.0));
            const auto ec = OntoMath::certifyLineQuadrature(*epsilon, model, 20, &why, symbols);
            if (!ec) { std::printf("     ε_%s does not certify: %s\n", axes[c], why.c_str()); continue; }
            const OntoMath::LineQuadratureBand* eb = nullptr;
            for (const auto& band : ec->bands) if (reach <= band.cap) { eb = &band; break; }
            std::mt19937_64 rng(1000 + c);
            std::uniform_real_distribution<double> un(-1.0, 1.0);
            int admitted[2] = {0, 0}, sound = 0;
            double cells[2] = {0.0, 0.0};
            std::string refusal[2];
            for (int r = 0; r < 12; ++r) {
                double o[3], d[3], n2 = 0.0;
                const double centre[3] = {b.origin.x, b.origin.y, b.origin.z}, half[3] = {std::abs(b.scale.x), std::abs(b.scale.y), std::abs(b.scale.z)};
                for (int k = 0; k < 3; ++k) { o[k] = centre[k] + half[k] * 0.9 * un(rng); d[k] = un(rng); n2 += d[k] * d[k]; }
                for (int k = 0; k < 3; ++k) d[k] /= std::sqrt(n2);
                double t0 = -1e300, t1 = 1e300;   // the box's entry and exit along the line
                for (int k = 0; k < 3; ++k) {
                    if (std::abs(d[k]) < 1e-12) continue;
                    const double a = (centre[k] - half[k] - o[k]) / d[k], bb = (centre[k] + half[k] - o[k]) / d[k];
                    t0 = std::max(t0, std::min(a, bb));
                    t1 = std::min(t1, std::max(a, bb));
                }
                auto at = [&](const MathNode& f, long double s) {
                    const glm::vec3 p(float(o[0] + s * d[0]), float(o[1] + s * d[1]), float(o[2] + s * d[2]));
                    const auto v = f.evaluate({{"p", PropertyValue(p)}, {"x", PropertyValue(double(p.x))}, {"y", PropertyValue(double(p.y))},
                                               {"z", PropertyValue(double(p.z))}, {OntoMath::kTimeVar, PropertyValue(0.0)},
                                               {OntoMath::kOmegaXVar, PropertyValue(-d[0])}, {OntoMath::kOmegaYVar, PropertyValue(-d[1])},
                                               {OntoMath::kOmegaZVar, PropertyValue(-d[2])}});
                    double dd = NAN; if (v) propertyValueToNumber(*v, dd); return (long double)dd;
                };
                const int M = 2000;
                const long double hh = ((long double)t1 - t0) / M;
                std::vector<long double> tau(M + 1, 0.0L);
                for (int i = 2; i <= M; i += 2)
                    tau[i] = tau[i - 2] + hh / 3.0L * (at(*sigma, t0 + (i - 2) * hh) + 4.0L * at(*sigma, t0 + (i - 1) * hh) + at(*sigma, t0 + i * hh));
                for (int i = 1; i < M; i += 2)
                    tau[i] = tau[i - 1] + hh * (5.0L * at(*sigma, t0 + (i - 1) * hh) + 8.0L * at(*sigma, t0 + i * hh) - at(*sigma, t0 + (i + 1) * hh)) / 12.0L;
                long double sum = 0.0L;
                for (int i = 0; i <= M; ++i) sum += (i == 0 || i == M ? 1.0L : (i % 2 ? 4.0L : 2.0L)) * std::exp(-tau[i]) * at(*epsilon, t0 + i * hh);
                const double ref = double(sum * hh / 3.0L);
                for (int k = 0; k < 2; ++k) {
                    const double tol = k == 0 ? 1e-2 : 1e-3;
                    std::string pw;
                    const auto plan = OntoMath::planLineTransport(*sc, *ec, o, d, t0, t1, tol, 4096, &pw);
                    if (!plan) { refusal[k] = pw; continue; }
                    ++admitted[k];
                    cells[k] += plan->cells;
                    // The march in double at the plan's cells, against the reference.
                    const int n = int(plan->cells);
                    const double h = (t1 - t0) / n;
                    double tauA = 0.0, sigA = double(at(*sigma, t0)), gA = double(at(*epsilon, t0)), L = 0.0;
                    for (int j = 0; j < n; j += 2) {
                        const double sm = t0 + (j + 1) * h, sbb = t0 + (j + 2) * h;
                        const double sigM = double(at(*sigma, sm)), sigB = double(at(*sigma, sbb));
                        const double tauM = tauA + h * (5.0 * sigA + 8.0 * sigM - sigB) / 12.0, tauB = tauA + h / 3.0 * (sigA + 4.0 * sigM + sigB);
                        const double gM = std::exp(-tauM) * double(at(*epsilon, sm)), gB = std::exp(-tauB) * double(at(*epsilon, sbb));
                        L += h / 3.0 * (gA + 4.0 * gM + gB);
                        tauA = tauB; sigA = sigB; gA = gB;
                    }
                    sound += std::abs(L - ref) <= plan->mathematical;
                    ++admittedRays;
                }
            }
            soundRays += sound;
            std::printf("     ε_%s (band sup %.3g, L %.3g, H %s, device error %.3g): ±1e-2 %d/12 (mean %.0f cells)%s; ±1e-3 %d/12 (mean %.0f cells)%s\n",
                        axes[c], eb ? eb->sup : NAN, eb ? eb->lipschitz : NAN,
                        eb && std::isfinite(eb->hessian) ? std::to_string(eb->hessian).c_str() : "unbounded", eb ? eb->execution : NAN,
                        admitted[0], admitted[0] ? cells[0] / admitted[0] : 0.0, admitted[0] < 12 ? (" — " + refusal[0]).c_str() : "",
                        admitted[1], admitted[1] ? cells[1] / admitted[1] : 0.0, admitted[1] < 12 ? (" — " + refusal[1]).c_str() : "");
        }
    }
    std::printf("  %d media read; %d admitted (ray, channel, tolerance) plans\n", media, admittedRays);
    check(media > 0, "Northern Veil's media were read");
    check(soundRays == admittedRays, "on every admitted plan, the double march lies inside the mathematical part (" +
                                         std::to_string(soundRays) + "/" + std::to_string(admittedRays) + ")");
    std::cout << "ontomath_veil_transport_feasibility_test: " << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
