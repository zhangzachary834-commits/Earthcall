#include "Singularity/OntoMath/EllipticIntegrals.hpp"

#include <algorithm>
#include <cmath>

// Carlson's duplication algorithms (Numer. Algorithms 10 (1995) 13–26). Each
// loop runs until 4⁻ᵐ·Q ≤ |A_m|, his condition for the series that follows to
// truncate with relative error below r; then one series evaluation. See the
// header for domains and for where the error is measured.

namespace OntoMath {

namespace {
constexpr double kTolerance = 0x1p-53;   // r: Carlson's truncation bound target

bool finiteNonnegative(double v) { return std::isfinite(v) && v >= 0.0; }
int zeros(double x, double y, double z) { return (x == 0.0) + (y == 0.0) + (z == 0.0); }
} // namespace

std::optional<double> carlsonRC(double x, double y) {
    if (!finiteNonnegative(x) || !(std::isfinite(y) && y > 0.0)) return std::nullopt;
    const double a0 = (x + 2.0 * y) / 3.0;
    double q = std::pow(3.0 * kTolerance, -1.0 / 8.0) * std::abs(a0 - x);
    double a = a0, xm = x, ym = y, scale = 1.0;   // scale = 4⁻ᵐ
    while (scale * q > std::abs(a)) {
        const double lambda = 2.0 * std::sqrt(xm) * std::sqrt(ym) + ym;
        xm = 0.25 * (xm + lambda);
        ym = 0.25 * (ym + lambda);
        a = 0.25 * (a + lambda);
        scale *= 0.25;
    }
    const double s = (y - a0) * scale / a;
    const double s2 = s * s;
    return (1.0 + s2 * (3.0 / 10.0 + s * (1.0 / 7.0 + s * (3.0 / 8.0 + s * (9.0 / 22.0 + s * (159.0 / 208.0 + s * (9.0 / 8.0)))))))
           / std::sqrt(a);
}

std::optional<double> carlsonRF(double x, double y, double z) {
    if (!finiteNonnegative(x) || !finiteNonnegative(y) || !finiteNonnegative(z) || zeros(x, y, z) > 1) return std::nullopt;
    const double a0 = (x + y + z) / 3.0;
    const double q = std::pow(3.0 * kTolerance, -1.0 / 6.0) * std::max({std::abs(a0 - x), std::abs(a0 - y), std::abs(a0 - z)});
    double a = a0, xm = x, ym = y, zm = z, scale = 1.0;
    while (scale * q > std::abs(a)) {
        const double sx = std::sqrt(xm), sy = std::sqrt(ym), sz = std::sqrt(zm);
        const double lambda = sx * sy + sy * sz + sz * sx;
        xm = 0.25 * (xm + lambda);
        ym = 0.25 * (ym + lambda);
        zm = 0.25 * (zm + lambda);
        a = 0.25 * (a + lambda);
        scale *= 0.25;
    }
    const double X = (a0 - x) * scale / a, Y = (a0 - y) * scale / a, Z = -(X + Y);
    const double e2 = X * Y - Z * Z, e3 = X * Y * Z;
    return (1.0 - e2 / 10.0 + e3 / 14.0 + e2 * e2 / 24.0 - 3.0 * e2 * e3 / 44.0) / std::sqrt(a);
}

std::optional<std::complex<double>> carlsonRF(std::complex<double> x, std::complex<double> y, std::complex<double> z) {
    using C = std::complex<double>;
    // Within ~1.8° of the cut the value is not resolved in double precision
    // (the function jumps across it): refuse rather than answer.
    auto admissible = [](C v) {
        return std::isfinite(v.real()) && std::isfinite(v.imag()) && (v == C(0.0) || std::abs(std::arg(v)) <= 0.99 * M_PI);
    };
    if (!admissible(x) || !admissible(y) || !admissible(z)) return std::nullopt;
    if ((x == C(0.0)) + (y == C(0.0)) + (z == C(0.0)) > 1) return std::nullopt;
    const C a0 = (x + y + z) / 3.0;
    const double q = std::pow(3.0 * kTolerance, -1.0 / 6.0) * std::max({std::abs(a0 - x), std::abs(a0 - y), std::abs(a0 - z)});
    C a = a0, xm = x, ym = y, zm = z;
    double scale = 1.0;
    while (scale * q > std::abs(a)) {
        const C sx = std::sqrt(xm), sy = std::sqrt(ym), sz = std::sqrt(zm);
        const C lambda = sx * sy + sy * sz + sz * sx;
        xm = 0.25 * (xm + lambda);
        ym = 0.25 * (ym + lambda);
        zm = 0.25 * (zm + lambda);
        a = 0.25 * (a + lambda);
        scale *= 0.25;
    }
    const C X = (a0 - x) * scale / a, Y = (a0 - y) * scale / a, Z = -(X + Y);
    const C e2 = X * Y - Z * Z, e3 = X * Y * Z;
    return (1.0 - e2 / 10.0 + e3 / 14.0 + e2 * e2 / 24.0 - 3.0 * e2 * e3 / 44.0) / std::sqrt(a);
}

std::optional<double> carlsonRD(double x, double y, double z) {
    if (!finiteNonnegative(x) || !finiteNonnegative(y) || !(std::isfinite(z) && z > 0.0) || (x == 0.0 && y == 0.0))
        return std::nullopt;
    const double a0 = (x + y + 3.0 * z) / 5.0;
    const double q = std::pow(0.25 * kTolerance, -1.0 / 6.0) * std::max({std::abs(a0 - x), std::abs(a0 - y), std::abs(a0 - z)});
    double a = a0, xm = x, ym = y, zm = z, scale = 1.0, sum = 0.0;
    while (scale * q > std::abs(a)) {
        const double sx = std::sqrt(xm), sy = std::sqrt(ym), sz = std::sqrt(zm);
        const double lambda = sx * sy + sy * sz + sz * sx;
        sum += scale / (sz * (zm + lambda));
        xm = 0.25 * (xm + lambda);
        ym = 0.25 * (ym + lambda);
        zm = 0.25 * (zm + lambda);
        a = 0.25 * (a + lambda);
        scale *= 0.25;
    }
    const double X = (a0 - x) * scale / a, Y = (a0 - y) * scale / a, Z = -(X + Y) / 3.0;
    const double xy = X * Y, z2 = Z * Z;
    const double e2 = xy - 6.0 * z2, e3 = (3.0 * xy - 8.0 * z2) * Z, e4 = 3.0 * (xy - z2) * z2, e5 = xy * z2 * Z;
    const double series = 1.0 - 3.0 * e2 / 14.0 + e3 / 6.0 + 9.0 * e2 * e2 / 88.0 - 3.0 * e4 / 22.0 - 9.0 * e2 * e3 / 52.0 + 3.0 * e5 / 26.0;
    return scale * series / (a * std::sqrt(a)) + 3.0 * sum;
}

std::optional<double> carlsonRJ(double x, double y, double z, double p) {
    if (!finiteNonnegative(x) || !finiteNonnegative(y) || !finiteNonnegative(z) || zeros(x, y, z) > 1 ||
        !(std::isfinite(p) && p > 0.0))
        return std::nullopt;
    const double a0 = (x + y + z + 2.0 * p) / 5.0;
    const double q = std::pow(0.25 * kTolerance, -1.0 / 6.0) *
                     std::max({std::abs(a0 - x), std::abs(a0 - y), std::abs(a0 - z), std::abs(a0 - p)});
    double a = a0, xm = x, ym = y, zm = z, pm = p, scale = 1.0, sum = 0.0;
    while (scale * q > std::abs(a)) {
        const double sx = std::sqrt(xm), sy = std::sqrt(ym), sz = std::sqrt(zm), sp = std::sqrt(pm);
        const double lambda = sx * sy + sy * sz + sz * sx;
        const double d = (sp + sx) * (sp + sy) * (sp + sz);
        // Carlson's e_m = 4⁻³ᵐ·δ/d_m² with δ = (p − x)(p − y)(p − z) at m = 0, as a
        // product of three factors f_i = 4⁻ᵐ(p₀ − x_i)/(√p_m + √x_i,m)², each in
        // (−1, 1): δ itself overflows (p = 1e200), and √p_m − √x_m would cancel as
        // the iterates converge (why Carlson keeps δ). 1 + e cancels when e → −1,
        // which happens whenever p lies far from the arguments on either side; so
        // each factor is f_i = σ_i(1 − δ_i) with δ_i the small one of 1 ∓ f_i,
        // both exact without subtraction (1 − f = 2√x/(√p + √x), 1 + f =
        // 2√p/(√p + √x)), and 1 + e is a sum of non-negative terms either way.
        double sigma = 1.0, dl[3];
        const double sqs[3] = {sx, sy, sz}, args[3] = {x, y, z};
        for (int i = 0; i < 3; ++i) {
            const double f = scale * (p - args[i]) / ((sp + sqs[i]) * (sp + sqs[i]));
            if (f >= 0.0) dl[i] = 2.0 * sqs[i] / (sp + sqs[i]);
            else { sigma = -sigma; dl[i] = 2.0 * sp / (sp + sqs[i]); }
        }
        const double onePlusE = sigma < 0.0 ? dl[0] + (1.0 - dl[0]) * (dl[1] + (1.0 - dl[1]) * dl[2])
                                            : 1.0 + (1.0 - dl[0]) * (1.0 - dl[1]) * (1.0 - dl[2]);
        const auto rc = carlsonRC(1.0, onePlusE);
        if (!rc) return std::nullopt;
        sum += scale * *rc / d;
        xm = 0.25 * (xm + lambda);
        ym = 0.25 * (ym + lambda);
        zm = 0.25 * (zm + lambda);
        pm = 0.25 * (pm + lambda);
        a = 0.25 * (a + lambda);
        scale *= 0.25;
    }
    const double X = (a0 - x) * scale / a, Y = (a0 - y) * scale / a, Z = (a0 - z) * scale / a;
    const double P = -(X + Y + Z) / 2.0;
    const double e2 = X * Y + X * Z + Y * Z - 3.0 * P * P;
    const double e3 = X * Y * Z + 2.0 * e2 * P + 4.0 * P * P * P;
    const double e4 = (2.0 * X * Y * Z + e2 * P + 3.0 * P * P * P) * P;
    const double e5 = X * Y * Z * P * P;
    const double series = 1.0 - 3.0 * e2 / 14.0 + e3 / 6.0 + 9.0 * e2 * e2 / 88.0 - 3.0 * e4 / 22.0 - 9.0 * e2 * e3 / 52.0 + 3.0 * e5 / 26.0;
    return scale * series / (a * std::sqrt(a)) + 6.0 * sum;
}

std::optional<double> carlsonRJPrincipalValue(double x, double y, double z, double p) {
    if (!(p < 0.0) || !std::isfinite(p) || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return std::nullopt;
    std::array<double, 3> v{x, y, z};
    std::sort(v.begin(), v.end());
    const double lo = v[0], mid = v[1], hi = v[2];
    if (lo < 0.0 || !(mid > 0.0)) return std::nullopt;   // R_F's domain: at most one zero
    // DLMF 19.20.14 with 19.20.16, p' = w·y + (1 − w)·z, w = (z − x)/(z + q),
    // so p' − z = w·(y − z) is formed without cancelling z(x + y + q) − xy.
    const double q = -p;
    const double w = (hi - lo) / (hi + q), oneMinusW = (lo + q) / (hi + q);   // never 1 − w: it rounds to 0
    const double pp = w * mid + oneMinusW * hi;
    const double xy = lo * mid, pq = pp * q;
    const auto rj = carlsonRJ(lo, mid, hi, pp);
    const auto rf = carlsonRF(lo, mid, hi);
    const auto rc = carlsonRC(xy + pq, pq);
    if (!rj || !rf || !rc) return std::nullopt;
    const double root = std::sqrt(lo) * std::sqrt(mid) * std::sqrt(hi) / std::sqrt(xy + pq);
    const double value = (w * (mid - hi) * *rj - 3.0 * *rf + 3.0 * root * *rc) / (q + hi);
    if (!std::isfinite(value)) return std::nullopt;
    return value;
}

namespace {
// One interval of the first-kind reduction, valid while the complex factors
// sweep less than π of argument in total (see ellipticFirstKindQuartic).
std::optional<double> firstKindPiece(const std::array<std::complex<double>, 4>& a,
                                     const std::array<std::complex<double>, 4>& b, double y, double x, int depth) {
    using C = std::complex<double>;
    double swept = 0.0;
    for (int i = 0; i < 4; ++i)
        if (a[i].imag() != 0.0 || b[i].imag() != 0.0) swept += std::abs(std::arg(a[i] + b[i] * x) - std::arg(a[i] + b[i] * y));
    if (swept > 0.9 * M_PI) {
        if (depth > 40) return std::nullopt;
        const double m = 0.5 * (x + y);
        const auto lo = firstKindPiece(a, b, y, m, depth + 1), hi = firstKindPiece(a, b, m, x, depth + 1);
        if (!lo || !hi) return std::nullopt;
        return *lo + *hi;
    }
    std::array<C, 4> X, Y;
    for (int i = 0; i < 4; ++i) {
        X[i] = std::sqrt(a[i] + b[i] * x);
        Y[i] = std::sqrt(a[i] + b[i] * y);
    }
    const double w = x - y;
    const C u12 = (X[0] * X[1] * Y[2] * Y[3] + Y[0] * Y[1] * X[2] * X[3]) / w;
    const C u13 = (X[0] * X[2] * Y[1] * Y[3] + Y[0] * Y[2] * X[1] * X[3]) / w;
    const C u14 = (X[0] * X[3] * Y[1] * Y[2] + Y[0] * Y[3] * X[1] * X[2]) / w;
    if (!(u12.real() > 0.0 && u13.real() > 0.0 && u14.real() > 0.0)) return std::nullopt;
    const auto rf = carlsonRF(u12 * u12, u13 * u13, u14 * u14);
    if (!rf) return std::nullopt;
    // Conjugate pairing makes the value real; a residue beyond rounding means the
    // factors were not paired or not positive on the interval.
    if (std::abs(rf->imag()) > 1e-9 * std::abs(rf->real())) return std::nullopt;
    return 2.0 * rf->real();
}
} // namespace

std::optional<double> ellipticFirstKindQuartic(const std::array<std::complex<double>, 4>& a,
                                               const std::array<std::complex<double>, 4>& b, double y, double x) {
    using C = std::complex<double>;
    if (!(std::isfinite(x) && std::isfinite(y) && x > y)) return std::nullopt;
    for (int i = 0; i < 4; ++i) {
        const C ax = a[i] + b[i] * x, ay = a[i] + b[i] * y;
        // A real factor must be positive on the interval (the caller orients it).
        if (a[i].imag() == 0.0 && b[i].imag() == 0.0 && !(ax.real() > 0.0 && ay.real() > 0.0)) return std::nullopt;
    }
    return firstKindPiece(a, b, y, x, 0);
}

std::optional<double> ellipticFirstKindTwoQuadratics(const std::array<double, 3>& q1, const std::array<double, 3>& q2,
                                                     double y, double x) {
    using C = std::complex<double>;
    std::array<C, 4> a, b;
    // Each quadratic as two linear factors positive on [y, x]: a conjugate pair
    // √|A|(t − ρ), √|A|(t − ρ̄) when it has no real root, otherwise two real
    // factors oriented so each is positive on the interval.
    auto split = [&](const std::array<double, 3>& q, int at) -> bool {
        const double A = q[2], B = q[1], Cc = q[0];
        if (!(A != 0.0) || !std::isfinite(A) || !std::isfinite(B) || !std::isfinite(Cc)) return false;
        auto value = [&](double t) { return (A * t + B) * t + Cc; };
        if (!(value(y) > 0.0 && value(x) > 0.0)) return false;
        const double disc = B * B - 4.0 * A * Cc;
        const double r = std::sqrt(std::abs(A));
        if (disc < 0.0) {
            const C rho(-B / (2.0 * A), std::sqrt(-disc) / (2.0 * std::abs(A)));
            if (A < 0.0) return false;   // A < 0 with no real root is negative everywhere
            a[at] = -r * rho; b[at] = r;
            a[at + 1] = -r * std::conj(rho); b[at + 1] = r;
            return true;
        }
        // Real roots r1 ≤ r2 (stable form), neither inside [y, x].
        const double sq = std::sqrt(disc);
        const double qq = -0.5 * (B + (B >= 0.0 ? sq : -sq));
        double r1 = qq / A, r2 = qq != 0.0 ? Cc / qq : r1;
        if (r1 > r2) std::swap(r1, r2);
        for (int k = 0; k < 2; ++k) {
            const double root = k == 0 ? r1 : r2;
            if (root > y && root < x) return false;
            // (t − root) or (root − t), whichever is positive on the interval.
            const double sign = (0.5 * (x + y) - root) > 0.0 ? 1.0 : -1.0;
            a[at + k] = -sign * r * root;
            b[at + k] = sign * r;
        }
        // The product of the two oriented factors is |A|·(t − r1)(t − r2)·(±1);
        // it must equal Q (positive), which holds iff the orientation signs
        // multiply to sign(A).
        const double s1 = b[at].real() > 0 ? 1.0 : -1.0, s2 = b[at + 1].real() > 0 ? 1.0 : -1.0;
        return s1 * s2 == (A > 0.0 ? 1.0 : -1.0);
    };
    if (!split(q1, 0) || !split(q2, 2)) return std::nullopt;
    return ellipticFirstKindQuartic(a, b, y, x);
}

} // namespace OntoMath
