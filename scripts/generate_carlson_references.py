#!/usr/bin/env python3
"""Reference values for OntoMath's Carlson elliptic integrals.

Writes tests/singularity/ontomath_carlson_references.inc: precision-stable values
of R_F, R_D, R_J, R_C (and complex R_F on conjugate pairs) across hard regimes,
and of the first-kind two-quadratic integral by mpmath quadrature, for
ontomath_carlson_elliptic_test. Requires mpmath (pip install mpmath==1.3.0).
Deterministic (fixed seed). Run from the repository root.

Human origin: Zachary Zhang approved Carlson evaluation as exact on the Original
Prism Sun's criteria, which ask for comparison against high-precision references
in difficult and near-degenerate regimes (2026-10-19).
Claude Code / Claude Opus 5.5 / session f3af84cf-bfe3-429d-b417-2d533119f169.
"""
import random
import mpmath as mp

mp.mp.dps = 40
rng = random.Random(20261019)

# A reference must be stable under precision, not merely produced by mpmath:
# mpmath's Carlson iterations lose about as many digits as the arguments'
# spread (elliprj(2, 3, 4, 1e-200) answers 67.1 at 40 digits, 80.5 at 50 and
# 141.13295926608468879 from 200 on, which R_J's defining integral confirms).
# Each value is computed at 200 and at 400 digits and written only if the two
# agree to 1e-40.


def stable(fn, *args):
    with mp.workdps(200):
        a = fn(*args)
    with mp.workdps(400):
        b = fn(*args)
    assert abs(a - b) <= mp.mpf(10) ** -40 * abs(b), ("reference unstable under precision", fn.__name__, args)
    return b


def logu(lo, hi):
    return 10 ** rng.uniform(lo, hi)


def num(v):
    return mp.nstr(mp.mpf(v), 21, min_fixed=-mp.inf, max_fixed=mp.inf) if v != 0 else "0.0"


rf, rd, rj, rc, crf, quad = [], [], [], [], [], []

# Carlson (1995) published test values, then regimes.
for x, y, z in [(1, 2, 0), (2, 3, 4), (0.5, 1, 0), (1, 1, 1), (1e-300, 1, 2), (1, 1, 1 + 1e-12),
                (1e-200, 1e-200, 1), (1e100, 1e-100, 1), (3, 3, 1e-150), (1e300, 1e300, 1e-300)]:
    rf.append((x, y, z, "regime"))
for _ in range(60):
    rf.append((logu(-100, 100), logu(-100, 100), logu(-100, 100), "random"))
for _ in range(20):
    rf.append((0.0, logu(-50, 50), logu(-50, 50), "one zero"))
for _ in range(20):
    c = logu(-5, 5)
    rf.append((c, c * (1 + rng.uniform(-1e-9, 1e-9)), c * (1 + rng.uniform(-1e-9, 1e-9)), "near equal"))

for x, y, z in [(0, 2, 1), (2, 3, 4), (1, 1, 1), (0, 1, 1e-200), (1e-200, 1, 1), (1e100, 1e100, 1e-100)]:
    rd.append((x, y, z, "regime"))
for _ in range(60):
    rd.append((logu(-100, 100), logu(-100, 100), logu(-100, 100), "random"))
for _ in range(20):
    rd.append((0.0, logu(-50, 50), logu(-50, 50), "one zero"))

for x, y, z, p in [(0, 1, 2, 3), (2, 3, 4, 5), (1, 1, 1, 1), (2, 3, 4, 1e-200), (2, 3, 4, 1e200),
                   (1, 1, 1, 1 + 1e-12), (0, 1, 1, 1e-100)]:
    rj.append((x, y, z, p, "regime"))
for _ in range(60):
    rj.append((logu(-80, 80), logu(-80, 80), logu(-80, 80), logu(-80, 80), "random"))
for _ in range(20):
    rj.append((0.0, logu(-30, 30), logu(-30, 30), logu(-30, 30), "one zero"))

for x, y in [(0, 0.25), (2.25, 2), (1, 1), (1, 1 + 1e-12), (1 + 1e-12, 1), (1e-300, 1), (1e100, 1e-100)]:
    rc.append((x, y, "regime"))
for _ in range(40):
    rc.append((logu(-100, 100), logu(-100, 100), "random"))

for _ in range(40):
    x = logu(-20, 20)
    z = mp.mpc(rng.uniform(-1, 1) * logu(-20, 20), logu(-20, 20))
    crf.append((x, z, "conjugate pair"))

# ∫_y^x dt/√(Q1 Q2): complex-root pairs, real roots outside the interval, far rays.
def quadratic_case(kind):
    while True:
        if kind == "complex":
            def q():
                c = rng.uniform(-3, 3); w = logu(-2, 1); a = logu(-1, 1)
                return (a * (c * c + w * w), -2 * a * c, a)   # a((t-c)^2 + w^2)
            q1, q2 = q(), q()
            y = rng.uniform(-5, 5); x = y + logu(-3, 1)
        elif kind == "real roots":
            def q():
                r1 = rng.uniform(-10, 10); r2 = r1 + logu(-1, 1); a = rng.choice([-1, 1]) * logu(-1, 1)
                return (a * r1 * r2, -a * (r1 + r2), a), (r1, r2), a
            (q1, rr1, a1), (q2, rr2, a2) = q(), q()
            y = rng.uniform(-12, 12); x = y + logu(-2, 0.5)
            ok = True
            for (rr, a) in ((rr1, a1), (rr2, a2)):
                if any(y < r < x for r in rr):
                    ok = False
                f = lambda t, rr=rr, a=a: a * (t - rr[0]) * (t - rr[1])
                if not (f(y) > 0 and f(x) > 0):
                    ok = False
            if not ok:
                continue
        else:  # far from the origin, as along a long ray
            def q():
                c = rng.uniform(480, 520); w = logu(-1, 1)
                return (c * c + w * w, -2 * c, 1.0)
            q1, q2 = q(), q()
            y = rng.uniform(470, 500); x = y + logu(-1, 1.3)
        f = lambda t: 1 / mp.sqrt((q1[2] * t * t + q1[1] * t + q1[0]) * (q2[2] * t * t + q2[1] * t + q2[0]))
        # A complex root near the interval makes a spike of width Im ρ: grade the
        # breakpoints around each root's real part, and confirm by a second,
        # different quadrature (the plain one was off by 3.5e-10 on such a spike).
        pts = {mp.mpf(y), mp.mpf(x)}
        for q in (q1, q2):
            A, B, C = (mp.mpf(v) for v in (q[2], q[1], q[0]))
            c = -B / (2 * A)
            w = mp.sqrt(abs(B * B - 4 * A * C)) / (2 * abs(A))
            for k in (0, 1, 3, 10, 30, 100, 300):
                for sgn in (-1, 1):
                    t = c + sgn * k * w
                    if y < t < x:
                        pts.add(t)
        pts = sorted(pts)
        # Two different methods on two different panelings must agree.
        mid = [(pts[i] + pts[i + 1]) / 2 for i in range(len(pts) - 1)]
        v1 = mp.quad(f, pts, method="tanh-sinh")
        v2 = mp.quad(f, sorted(pts + mid), method="gauss-legendre")
        assert abs(v1 - v2) <= mp.mpf(10) ** -25 * v1, ("reduction reference unconverged", q1, q2, y, x, v1, v2)
        return q1, q2, y, x, v1, kind


for _ in range(25):
    quad.append(quadratic_case("complex"))
for _ in range(15):
    quad.append(quadratic_case("real roots"))
for _ in range(10):
    quad.append(quadratic_case("far"))

out = ["// Generated by scripts/generate_carlson_references.py (mpmath " + mp.__version__ + "; special functions stable"]
out.append("// between 200 and 400 digits, integrals agreeing between two quadrature methods). Do not edit.")
out.append("struct RefF { double x, y, z; const char* regime; double value; };")
out.append("static const RefF kRF[] = {")
out += ["    {%s, %s, %s, \"%s\", %s}," % (num(x), num(y), num(z), r, num(stable(mp.elliprf, x, y, z))) for x, y, z, r in rf]
out.append("};")
out.append("static const RefF kRD[] = {")
out += ["    {%s, %s, %s, \"%s\", %s}," % (num(x), num(y), num(z), r, num(stable(mp.elliprd, x, y, z))) for x, y, z, r in rd]
out.append("};")
out.append("struct RefJ { double x, y, z, p; const char* regime; double value; };")
out.append("static const RefJ kRJ[] = {")
out += ["    {%s, %s, %s, %s, \"%s\", %s}," % (num(x), num(y), num(z), num(p), r, num(stable(mp.elliprj, x, y, z, p))) for x, y, z, p, r in rj]
out.append("};")
out.append("struct RefC { double x, y; const char* regime; double value; };")
out.append("static const RefC kRC[] = {")
out += ["    {%s, %s, \"%s\", %s}," % (num(x), num(y), r, num(stable(mp.elliprc, x, y))) for x, y, r in rc]
out.append("};")
out.append("struct RefCF { double x, zr, zi; const char* regime; double value; };")
out.append("static const RefCF kComplexRF[] = {")
for x, z, r in crf:
    v = stable(mp.elliprf, x, z, mp.conj(z))
    out.append("    {%s, %s, %s, \"%s\", %s}," % (num(x), num(z.real), num(z.imag), r, num(v.real)))
out.append("};")
out.append("struct RefQ { double q1[3], q2[3], y, x; double value; const char* regime; };")
out.append("static const RefQ kTwoQuadratics[] = {")
for q1, q2, y, x, v, r in quad:
    out.append("    {{%s, %s, %s}, {%s, %s, %s}, %s, %s, %s, \"%s\"}," % (
        num(q1[0]), num(q1[1]), num(q1[2]), num(q2[0]), num(q2[1]), num(q2[2]), num(y), num(x), num(v), r))
out.append("};")
open("tests/singularity/ontomath_carlson_references.inc", "w").write("\n".join(out) + "\n")
print(len(rf), len(rd), len(rj), len(rc), len(crf), len(quad))
