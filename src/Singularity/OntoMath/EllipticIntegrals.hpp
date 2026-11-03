#pragma once

// ============================================================================
// Carlson's symmetric elliptic integrals — OntoMath's special-function layer
// for exact reductions of integrals no elementary antiderivative holds.
//
// Human origin: Zachary Zhang approved Carlson evaluation as exact
// ("LETS GO WITH IT", 2026-10-19), on the Original Prism Sun's verdict that an
// exact reduction to R_F/R_D/R_J is exact mathematics whose finite-precision
// evaluation is an execution question, like sqrt or sin. The Sun's hierarchy:
//   elementary closed form  !=  exact special-function reduction  !=  quadrature.
// These functions are the middle layer. A reduction that lands here is exact;
// evaluating it carries the error stated below, and is never quadrature.
//
//   R_F(x, y, z)    = ½ ∫₀^∞ dt / √((t+x)(t+y)(t+z))
//   R_D(x, y, z)    = 3/2 ∫₀^∞ dt / ((t+z) √((t+x)(t+y)(t+z)))
//   R_J(x, y, z, p) = 3/2 ∫₀^∞ dt / ((t+p) √((t+x)(t+y)(t+z)))
//   R_C(x, y)       = R_F(x, y, y)
//
// Evaluation: Carlson's duplication theorem (B. C. Carlson, "Numerical
// computation of real or complex elliptic integrals", Numer. Algorithms 10
// (1995) 13–26), iterated until his truncation bound guarantees relative error
// below r = 2⁻⁵³, then his fifth/seventh-order series. Total error is that bound
// plus double rounding; `ontomath_carlson_elliptic_test` measures it against
// 40-digit mpmath references across hard regimes and states the worst case.
//
// Domains (anything else answers nullopt, never a guess):
//   R_F: x, y, z ≥ 0, at most one zero.     R_D: x, y ≥ 0 (at most one zero), z > 0.
//   R_J: x, y, z ≥ 0 (at most one zero), p > 0 (the Cauchy principal value for
//        p < 0 is not admitted).           R_C: x ≥ 0, y > 0.
//   Complex R_F: arguments at least 0.01π from the cut along the negative real
//        axis (nearer, the value is not resolved in double precision), at most
//        one zero — the conjugate pairs real reductions produce.
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 · 2026-10-19.
// ============================================================================

#include <array>
#include <complex>
#include <optional>

namespace OntoMath {

std::optional<double> carlsonRF(double x, double y, double z);
std::optional<double> carlsonRD(double x, double y, double z);
std::optional<double> carlsonRJ(double x, double y, double z, double p);
std::optional<double> carlsonRC(double x, double y);
// R_J's Cauchy principal value for p < 0 (x, y, z ≥ 0, at most one zero), by
// DLMF 19.20.14: with x ≤ y ≤ z, q = −p, w = (z − x)/(z + q), p' = w·y + (1 − w)·z,
//   (q + z)·PV R_J(x, y, z, −q) = (p' − z)·R_J(x, y, z, p') − 3·R_F(x, y, z)
//                                + 3·√(xyz/(xy + p'q))·R_C(xy + p'q, p'q).
// VERIFIED, not recalled: a remembered form with y and z exchanged was off by
// 1e-2. The references agree three ways (the reduction, mpmath's own R_J at
// negative p, and a direct principal-value quadrature) before the double
// implementation is measured. A separate name from carlsonRJ, which still
// refuses p ≤ 0: a principal value is a different object, chosen explicitly.
// Needed by the second- and third-kind reductions (the crescent; plan §6v).
std::optional<double> carlsonRJPrincipalValue(double x, double y, double z, double p);
std::optional<std::complex<double>> carlsonRF(std::complex<double> x, std::complex<double> y, std::complex<double> z);

// The first-kind reduction, Carlson's quartic case (his tables of elliptic
// integrals, Math. Comp. 1987–1992; verified here against mpmath quadrature,
// not taken on trust):
//   ∫_y^x Π_{i=1..4} (a_i + b_i t)^(−1/2) dt = 2 R_F(U₁₂², U₁₃², U₁₄²),
//   U_ij = (X_i X_j Y_k Y_l + Y_i Y_j X_k X_l)/(x − y),  X_i = √(a_i + b_i x),
//   Y_i = √(a_i + b_i y),  {i, j, k, l} = {1, 2, 3, 4},
// for y < x with every factor's product positive on [y, x]. Factors may be
// complex in conjugate pairs (1, 2) and (3, 4): a quadratic with no real root
// is A(t − ρ)(t − ρ̄). Real factors must each be positive on [y, x] (the caller
// orients them). nullopt outside that domain.
// For a conjugate pair, U₁₃ = 2|X₁X₃||Y₁Y₃|·cos(φ(x) − φ(y))/(x − y) with
// φ = ½ Σ arg of the pair's factors: the formula holds only while the complex
// factors sweep less than π of argument in total (beyond that U₁₃ turns
// negative and 2R_F(U²…) is a different branch — found by this rung's witness).
// The interval is split, exactly by additivity, until each piece sweeps ≤ 0.9π.
std::optional<double> ellipticFirstKindQuartic(const std::array<std::complex<double>, 4>& a,
                                               const std::array<std::complex<double>, 4>& b,
                                               double y, double x);
// ∫_y^x dt / √(Q₁(t)·Q₂(t)), Q_k = A_k t² + B_k t + C_k (coefficients low to
// high: {C, B, A}), each positive on [y, x]: the first-kind integral of two
// quadratic factors, through ellipticFirstKindQuartic.
std::optional<double> ellipticFirstKindTwoQuadratics(const std::array<double, 3>& q1, const std::array<double, 3>& q2,
                                                     double y, double x);

} // namespace OntoMath
