// Carlson rung witness (2026-10-19): OntoMath's symmetric elliptic integrals
// R_F, R_D, R_J, R_C and the first-kind quartic reduction, before any medium
// trusts them. The Original Prism Sun's acceptance criteria, which Zach adopted
// ("LETS GO WITH IT"): comparison against high-precision references, difficult
// parameter regimes, near-degenerate cases, branch continuity, a stated error
// bound, and refusal (the caller's fallback) on unsupported cases.
//
// References (scripts/generate_carlson_references.py): mpmath values stable between 200
// and 400 digits; mpmath's own R_J is wrong at 40 digits for tiny p, which the
// first version of this witness inherited. Integrals: two quadrature methods agreeing.
// Independent of mpmath: Legendre's relation and the arithmetic-geometric mean.
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169.

#include "Singularity/OntoMath/EllipticIntegrals.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

namespace {
int g_checks = 0, g_failures = 0;
void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) ++g_failures;
    std::cout << (condition ? "  ok: " : "  FAILED: ") << description << "\n";
}

#include "ontomath_carlson_references.inc"

constexpr double kUlp = std::numeric_limits<double>::epsilon();

double relative(double got, double want) {
    return want == 0.0 ? std::abs(got) : std::abs(got - want) / std::abs(want);
}
} // namespace

int main() {
    std::cout << "Starting ontomath_carlson_elliptic_test...\n";
    using namespace OntoMath;

    // ---- Against precision-stable references, every regime ----------------------
    // The stated bound: relative error ≤ 16 ulp (Carlson's truncation bound is
    // below one; the rest is double rounding through the duplication steps).
    const double bound = 16.0 * kUlp;
    auto report = [&](const char* name, double worst, const char* worstRegime, int n, int refused) {
        std::cout << "    " << name << ": " << n << " references, worst relative error " << worst / kUlp
                  << " ulp (" << worstRegime << ")" << (refused ? ", " + std::to_string(refused) + " refused" : "") << "\n";
        check(refused == 0 && worst <= bound, std::string(name) + " agrees with precision-stable references within 16 ulp in every regime");
    };
    {
        double worst = 0.0; const char* where = ""; int refused = 0;
        for (const auto& r : kRF) {
            const auto v = carlsonRF(r.x, r.y, r.z);
            if (!v) { ++refused; continue; }
            const double e = relative(*v, r.value);
            if (e > worst) { worst = e; where = r.regime; }
        }
        report("R_F", worst, where, int(std::size(kRF)), refused);
    }
    {
        double worst = 0.0; const char* where = ""; int refused = 0;
        for (const auto& r : kRD) {
            const auto v = carlsonRD(r.x, r.y, r.z);
            if (!v) { ++refused; continue; }
            const double e = relative(*v, r.value);
            if (e > worst) { worst = e; where = r.regime; }
        }
        report("R_D", worst, where, int(std::size(kRD)), refused);
    }
    {
        double worst = 0.0; const char* where = ""; int refused = 0;
        for (const auto& r : kRJ) {
            const auto v = carlsonRJ(r.x, r.y, r.z, r.p);
            if (!v) { ++refused; continue; }
            const double e = relative(*v, r.value);
            if (e > worst) { worst = e; where = r.regime; }
        }
        report("R_J", worst, where, int(std::size(kRJ)), refused);
    }
    {
        double worst = 0.0; const char* where = ""; int refused = 0;
        for (const auto& r : kRC) {
            const auto v = carlsonRC(r.x, r.y);
            if (!v) { ++refused; continue; }
            const double e = relative(*v, r.value);
            if (e > worst) { worst = e; where = r.regime; }
        }
        report("R_C", worst, where, int(std::size(kRC)), refused);
    }
    {
        double worst = 0.0, worstImag = 0.0; int refused = 0;
        for (const auto& r : kComplexRF) {
            const std::complex<double> z(r.zr, r.zi);
            const auto v = carlsonRF(std::complex<double>(r.x, 0.0), z, std::conj(z));
            if (!v) { ++refused; continue; }
            worst = std::max(worst, relative(v->real(), r.value));
            worstImag = std::max(worstImag, std::abs(v->imag()) / std::abs(r.value));
        }
        // Pairs within 0.01π of the cut refuse (double precision cannot resolve
        // the function there); every pair it answers meets the bound.
        std::cout << "    complex R_F (conjugate pairs): " << std::size(kComplexRF) << " references, " << refused
                  << " refused as too near the cut, worst relative error " << worst / kUlp << " ulp\n";
        check(worst <= bound && refused < int(std::size(kComplexRF)), "complex R_F agrees with precision-stable references within 16 ulp wherever it answers");
        check(worstImag <= bound, "a conjugate pair's R_F is real to rounding");
    }

    // ---- Identities, independent of any reference -----------------------------
    {
        bool ok = true;
        for (double x : {1e-200, 1e-7, 0.3, 1.0, 42.0, 1e150}) {
            ok &= relative(*carlsonRF(x, x, x), 1.0 / std::sqrt(x)) <= 4 * kUlp;
            ok &= relative(*carlsonRC(x, x), 1.0 / std::sqrt(x)) <= 4 * kUlp;
            ok &= relative(*carlsonRD(x, x, x), 1.0 / (x * std::sqrt(x))) <= 4 * kUlp;
            ok &= relative(*carlsonRJ(x, x, x, x), 1.0 / (x * std::sqrt(x))) <= 4 * kUlp;
        }
        check(ok, "equal arguments: R_F = R_C = x^(-1/2), R_D = R_J = x^(-3/2)");
        bool homogeneous = true;
        for (double l : {1e-100, 0.5, 7.0, 1e100}) {
            homogeneous &= relative(*carlsonRF(2 * l, 3 * l, 5 * l), *carlsonRF(2, 3, 5) / std::sqrt(l)) <= 8 * kUlp;
            homogeneous &= relative(*carlsonRJ(2 * l, 3 * l, 5 * l, 7 * l), *carlsonRJ(2, 3, 5, 7) / (l * std::sqrt(l))) <= 8 * kUlp;
        }
        check(homogeneous, "homogeneity: R_F(λ·) = λ^(-1/2) R_F, R_J(λ·) = λ^(-3/2) R_J");
    }
    // Legendre's relation E K' + E' K − K K' = π/2 and K by the AGM: complete
    // integrals built from R_F and R_D alone, checked with no reference table.
    {
        auto K = [](double m) { return *carlsonRF(0.0, 1.0 - m, 1.0); };
        auto E = [](double m) { return *carlsonRF(0.0, 1.0 - m, 1.0) - m / 3.0 * *carlsonRD(0.0, 1.0 - m, 1.0); };
        double worstLegendre = 0.0, worstAgm = 0.0;
        for (double m : {1e-12, 0.01, 0.2, 0.5, 0.7, 0.99, 1.0 - 1e-9}) {
            const double mp = 1.0 - m;
            worstLegendre = std::max(worstLegendre, std::abs(E(m) * K(mp) + E(mp) * K(m) - K(m) * K(mp) - M_PI / 2) / (M_PI / 2));
            double a = 1.0, b = std::sqrt(mp);
            for (int i = 0; i < 40 && std::abs(a - b) > 1e-17 * a; ++i) { const double an = 0.5 * (a + b); b = std::sqrt(a * b); a = an; }
            worstAgm = std::max(worstAgm, relative(K(m), M_PI / (2 * a)));
        }
        std::cout << "    Legendre's relation: worst " << worstLegendre / kUlp << " ulp; K against the AGM: worst " << worstAgm / kUlp << " ulp\n";
        check(worstLegendre <= 64 * kUlp, "Legendre's relation holds for complete integrals built from R_F and R_D");
        check(worstAgm <= 16 * kUlp, "K = R_F(0, 1−m, 1) agrees with π/(2·AGM(1, √(1−m)))");
    }

    // ---- Continuity across near-degenerate points ------------------------------
    {
        // R_C(1, y) through y = 1: the series and the iteration meet without a jump,
        // and R_J's p passes through x.
        bool smooth = true;
        double previous = *carlsonRC(1.0, 1.0 - 1e-1);
        for (int k = 1; k <= 15; ++k) {
            const double h = std::pow(10.0, -k);
            const double below = *carlsonRC(1.0, 1.0 - h), above = *carlsonRC(1.0, 1.0 + h);
            // R_C(1, 1 ± h) = 1 ∓ h/3 + h²/5 ∓ h³/7 + h⁴/9 ...: both sides on that series.
            smooth &= std::abs(below - (1.0 + h / 3.0 + h * h / 5.0 + h * h * h / 7.0)) <= std::max(0.2 * h * h * h * h, 4 * kUlp);
            smooth &= std::abs(above - (1.0 - h / 3.0 + h * h / 5.0 - h * h * h / 7.0)) <= std::max(0.2 * h * h * h * h, 4 * kUlp);
            smooth &= below <= previous + 4 * kUlp;
            previous = below;
        }
        check(smooth, "R_C(1, y) is continuous and on its series through y = 1 (h = 10⁻¹ … 10⁻¹⁵)");
        bool pSmooth = true;
        for (int k = 1; k <= 14; ++k) {
            const double h = std::pow(10.0, -k);
            const double lo = *carlsonRJ(1.0, 2.0, 3.0, 2.0 - h), hi = *carlsonRJ(1.0, 2.0, 3.0, 2.0 + h), at = *carlsonRJ(1.0, 2.0, 3.0, 2.0);
            pSmooth &= lo >= at && at >= hi && (lo - hi) <= 4.0 * h * at + 8 * kUlp;   // R_J decreases in p, Lipschitz there
        }
        check(pSmooth, "R_J is monotone and continuous as p passes through an argument");
    }

    // ---- Refusals: the caller keeps its fallback --------------------------------
    {
        const double nan = std::numeric_limits<double>::quiet_NaN(), inf = std::numeric_limits<double>::infinity();
        check(!carlsonRF(-1, 2, 3) && !carlsonRF(0, 0, 3) && !carlsonRF(nan, 1, 2) && !carlsonRF(inf, 1, 2),
              "R_F refuses a negative argument, two zeros, NaN and infinity");
        check(!carlsonRD(0, 0, 1) && !carlsonRD(1, 2, 0) && !carlsonRD(1, 2, -1), "R_D refuses two zeros and z ≤ 0");
        check(!carlsonRJ(1, 2, 3, 0) && !carlsonRJ(1, 2, 3, -1) && !carlsonRJ(0, 0, 3, 1),
              "R_J refuses p ≤ 0 (the principal value is not admitted) and two zeros");
        check(!carlsonRC(1, 0) && !carlsonRC(1, -2) && !carlsonRC(-1, 2), "R_C refuses y ≤ 0 and negative x");
        check(!carlsonRF(std::complex<double>(-1, 0), 2.0, 3.0), "complex R_F refuses the negative real axis (its branch cut)");
    }

    // ---- The first-kind reduction against mpmath quadrature --------------------
    {
        double worst = 0.0; const char* where = ""; int refused = 0;
        for (const auto& r : kTwoQuadratics) {
            const auto v = ellipticFirstKindTwoQuadratics({r.q1[0], r.q1[1], r.q1[2]}, {r.q2[0], r.q2[1], r.q2[2]}, r.y, r.x);
            if (!v) { ++refused; continue; }
            const double e = relative(*v, r.value);
            if (e > worst) { worst = e; where = r.regime; }
        }
        std::cout << "    ∫ dt/√(Q₁Q₂) = 2 R_F(U₁₂², U₁₃², U₁₄²): " << std::size(kTwoQuadratics)
                  << " intervals, worst relative error " << worst << " (" << where << ")"
                  << (refused ? ", " + std::to_string(refused) + " refused" : "") << "\n";
        check(refused == 0 && worst <= 1e-13,
              "the first-kind reduction equals the quadrature reference to 1e-13 (complex-root pairs, real roots outside, far rays)");
        // A quadratic with a root inside the interval is outside the reduction's domain.
        check(!ellipticFirstKindTwoQuadratics({-1.0, 0.0, 1.0}, {1.0, 0.0, 1.0}, 0.0, 2.0),
              "a quadratic vanishing inside the interval refuses (the caller splits there)");
    }

    std::cout << "ontomath_carlson_elliptic_test: " << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
