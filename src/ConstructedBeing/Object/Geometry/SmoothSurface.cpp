#include "SmoothSurface.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace geom {

static const float kPI = 3.14159265358979323846f;

// ---------------------------------------------------------------------------
// Quadric algebra
// ---------------------------------------------------------------------------
namespace Quadric {

glm::mat4 sphere(float r) {
    glm::mat4 Q(0.0f);
    Q[0][0] = Q[1][1] = Q[2][2] = 1.0f;
    Q[3][3] = -r * r;
    return Q;
}

glm::mat4 ellipsoid(float a, float b, float c) {
    glm::mat4 Q(0.0f);
    Q[0][0] = 1.0f / (a * a);
    Q[1][1] = 1.0f / (b * b);
    Q[2][2] = 1.0f / (c * c);
    Q[3][3] = -1.0f;
    return Q;
}

glm::mat4 cylinder(float r) {
    glm::mat4 Q(0.0f);
    Q[0][0] = 1.0f / (r * r);
    Q[1][1] = 1.0f / (r * r);
    Q[3][3] = -1.0f; // x² + y² = r²
    return Q;
}

glm::mat4 cone(float k) {
    glm::mat4 Q(0.0f);
    Q[0][0] = 1.0f;
    Q[1][1] = 1.0f;
    Q[2][2] = -k * k; // x² + y² − k²z² = 0
    return Q;
}

glm::mat4 paraboloid(float a) {
    // a(x² + y²) − z = 0. Linear z term lives in the symmetric off-diagonal.
    glm::mat4 Q(0.0f);
    Q[0][0] = a;
    Q[1][1] = a;
    Q[2][3] = Q[3][2] = -0.5f; // contributes −z when expanded as pᵀQp
    return Q;
}

glm::mat4 translate(const glm::mat4& Q, const glm::vec3& t) {
    // Points q satisfy (q−t) on the original surface: Q' = Mᵀ Q M, M = translate(−t).
    glm::mat4 M = glm::translate(glm::mat4(1.0f), -t);
    return glm::transpose(M) * Q * M;
}

OntoMath::ScalarForm toScalarForm(const glm::mat4& Q) {
    OntoMath::ScalarForm form;
    const char* names[3] = {"x", "y", "z"};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            double c = static_cast<double>(Q[j][i]);
            if (c == 0.0) continue;
            OntoMath::Term t(c);
            if (i < 3) t.factors[names[i]] += 1.0;
            if (j < 3) t.factors[names[j]] += 1.0;
            form.terms.push_back(std::move(t));
        }
    }
    return form.normalized();
}

glm::mat4 fromScalarForm(const OntoMath::ScalarForm& form) {
    glm::mat4 Q(0.0f);
    for (const auto& term : form.terms) {
        if (!term.trans.empty()) continue;
        double expX = 0.0, expY = 0.0, expZ = 0.0;
        for (const auto& [var, exp] : term.factors) {
            if (var == "x") expX = exp;
            else if (var == "y") expY = exp;
            else if (var == "z") expZ = exp;
        }
        double totalExp = expX + expY + expZ;
        if (totalExp > 2.0) continue;
        float c = static_cast<float>(term.coefficient);
        if (expX == 2.0) Q[0][0] += c;
        else if (expY == 2.0) Q[1][1] += c;
        else if (expZ == 2.0) Q[2][2] += c;
        else if (expX == 1.0 && expY == 1.0) { Q[0][1] += c * 0.5f; Q[1][0] += c * 0.5f; }
        else if (expX == 1.0 && expZ == 1.0) { Q[0][2] += c * 0.5f; Q[2][0] += c * 0.5f; }
        else if (expY == 1.0 && expZ == 1.0) { Q[1][2] += c * 0.5f; Q[2][1] += c * 0.5f; }
        else if (expX == 1.0 && totalExp == 1.0) { Q[0][3] += c * 0.5f; Q[3][0] += c * 0.5f; }
        else if (expY == 1.0 && totalExp == 1.0) { Q[1][3] += c * 0.5f; Q[3][1] += c * 0.5f; }
        else if (expZ == 1.0 && totalExp == 1.0) { Q[2][3] += c * 0.5f; Q[3][2] += c * 0.5f; }
        else if (totalExp == 0.0) Q[3][3] += c;
    }
    return Q;
}

bool raycast(const glm::mat4& Q, const glm::vec3& o, const glm::vec3& d, float& t0, float& t1) {
    glm::vec4 P(o, 1.0f);
    glm::vec4 D(d, 0.0f);
    float A = glm::dot(D, Q * D);
    float B = 2.0f * glm::dot(P, Q * D);
    float C = glm::dot(P, Q * P);

    if (std::fabs(A) < 1e-9f) {
        if (std::fabs(B) < 1e-12f) return false;
        t0 = t1 = -C / B;
        return true;
    }
    float disc = B * B - 4.0f * A * C;
    if (disc < 0.0f) return false;
    float s = std::sqrt(disc);
    t0 = (-B - s) / (2.0f * A);
    t1 = (-B + s) / (2.0f * A);
    if (t0 > t1) std::swap(t0, t1);
    return true;
}

glm::vec3 gradient(const glm::mat4& Q, const glm::vec3& p) {
    glm::vec4 g = Q * glm::vec4(p, 1.0f); // spatial part of ½∇(pᵀQp)
    return glm::vec3(g);
}

} // namespace Quadric

OntoMath::ScalarForm SmoothSurfaceData::toScalarForm() const {
    if (model == Model::Quadric) {
        return Quadric::toScalarForm(Q);
    }
    return OntoMath::ScalarForm::constant(0.0);
}

// ---------------------------------------------------------------------------
// Factories
// ---------------------------------------------------------------------------
SmoothSurfaceData makeSphere(float r) {
    SmoothSurfaceData s;
    s.model = SmoothSurfaceData::Model::Quadric;
    s.form = SmoothSurfaceData::QuadricForm::Sphere;
    s.Q = Quadric::sphere(r);
    s.axes = glm::vec3(r);
    s.closed = true; s.orientable = true; s.hasBoundary = false; s.isVolume = true;
    return s;
}

SmoothSurfaceData makeEllipsoid(float a, float b, float c) {
    SmoothSurfaceData s;
    s.model = SmoothSurfaceData::Model::Quadric;
    s.form = SmoothSurfaceData::QuadricForm::Ellipsoid;
    s.Q = Quadric::ellipsoid(a, b, c);
    s.axes = glm::vec3(a, b, c);
    s.closed = true; s.orientable = true; s.hasBoundary = false; s.isVolume = true;
    return s;
}

SmoothSurfaceData makeCylinderSide(float r, float halfH) {
    SmoothSurfaceData s;
    s.model = SmoothSurfaceData::Model::Quadric;
    s.form = SmoothSurfaceData::QuadricForm::CylinderSide;
    s.Q = Quadric::cylinder(r);
    s.axes = glm::vec3(r);
    s.zTrim = glm::vec2(-halfH, halfH);
    s.closed = false; s.orientable = true; s.hasBoundary = true; s.isVolume = false;
    return s;
}

SmoothSurfaceData makeConeSide(float r, float halfH) {
    SmoothSurfaceData s;
    s.model = SmoothSurfaceData::Model::Quadric;
    s.form = SmoothSurfaceData::QuadricForm::ConeSide;
    // Apex at +halfH, base radius r at −halfH → slope k = r / (2·halfH).
    float k = r / (2.0f * halfH);
    s.Q = Quadric::translate(Quadric::cone(k), glm::vec3(0.0f, 0.0f, halfH));
    s.axes = glm::vec3(r);
    s.zTrim = glm::vec2(-halfH, halfH);
    s.closed = false; s.orientable = true; s.hasBoundary = true; s.isVolume = false;
    return s;
}

SmoothSurfaceData makeParaboloid(float a, float halfH) {
    SmoothSurfaceData s;
    s.model = SmoothSurfaceData::Model::Quadric;
    s.form = SmoothSurfaceData::QuadricForm::Paraboloid;
    s.Q = Quadric::paraboloid(a);
    s.zTrim = glm::vec2(0.0f, 2.0f * halfH);
    s.closed = false; s.orientable = true; s.hasBoundary = true; s.isVolume = false;
    return s;
}

SmoothSurfaceData makeTorus(float majorR, float minorR) {
    SmoothSurfaceData s;
    s.model = SmoothSurfaceData::Model::Parametric;
    s.pkind = SmoothSurfaceData::ParametricKind::Torus;
    s.params = {majorR, minorR};
    s.closed = true; s.orientable = true; s.hasBoundary = false; s.isVolume = true;
    return s;
}

SmoothSurfaceData makeOvoid(float r, float asym) {
    SmoothSurfaceData s;
    s.model = SmoothSurfaceData::Model::Parametric;
    s.pkind = SmoothSurfaceData::ParametricKind::Ovoid;
    s.params = {r, asym};
    s.closed = true; s.orientable = true; s.hasBoundary = false; s.isVolume = true;
    return s;
}

// ---------------------------------------------------------------------------
// Parametric signed-distance fields (for raycast / implicit)
// ---------------------------------------------------------------------------
static float torusSDF(const glm::vec3& p, float R, float r) {
    glm::vec2 q(glm::length(glm::vec2(p.x, p.y)) - R, p.z);
    return glm::length(q) - r;
}

static float ovoidSDF(const glm::vec3& p, float r, float asym) {
    // Egg: a sphere whose radius tapers along +z (smaller top, fatter bottom).
    float taper = 1.0f + asym * (p.z / std::max(1e-3f, r));
    taper = std::max(0.25f, taper);
    glm::vec3 q = p; q.x /= taper; q.y /= taper;
    return glm::length(q) - r;
}

static float parametricSDF(const SmoothSurfaceData& s, const glm::vec3& p) {
    switch (s.pkind) {
        case SmoothSurfaceData::ParametricKind::Torus:
            return torusSDF(p, s.params.size() > 0 ? s.params[0] : 0.35f,
                               s.params.size() > 1 ? s.params[1] : 0.15f);
        case SmoothSurfaceData::ParametricKind::Ovoid:
            return ovoidSDF(p, s.params.size() > 0 ? s.params[0] : 0.5f,
                               s.params.size() > 1 ? s.params[1] : 0.25f);
        default:
            return glm::length(p) - 0.5f; // fallback sphere for exotics (render-only later)
    }
}

static glm::vec3 sdfNormal(const SmoothSurfaceData& s, const glm::vec3& p) {
    const float e = 1e-3f;
    float dx = parametricSDF(s, p + glm::vec3(e, 0, 0)) - parametricSDF(s, p - glm::vec3(e, 0, 0));
    float dy = parametricSDF(s, p + glm::vec3(0, e, 0)) - parametricSDF(s, p - glm::vec3(0, e, 0));
    float dz = parametricSDF(s, p + glm::vec3(0, 0, e)) - parametricSDF(s, p - glm::vec3(0, 0, e));
    glm::vec3 n(dx, dy, dz);
    float len = glm::length(n);
    return len > 1e-8f ? n / len : glm::vec3(0, 1, 0);
}

static bool raycastParametric(const SmoothSurfaceData& s, const glm::vec3& o, const glm::vec3& d,
                              float& tHit, glm::vec3& nrm, glm::vec2& uv) {
    float t = 1e-3f;
    const float maxT = 100.0f;
    for (int i = 0; i < 160 && t < maxT; ++i) {
        glm::vec3 p = o + d * t;
        float dist = parametricSDF(s, p);
        if (dist < 1e-4f) {
            tHit = t;
            nrm = sdfNormal(s, p);
            uv = glm::vec2(0.5f + std::atan2(p.y, p.x) / (2.0f * kPI),
                           0.5f + std::atan2(p.z, glm::length(glm::vec2(p.x, p.y))) / (2.0f * kPI));
            return true;
        }
        t += std::max(dist, 1e-4f);
    }
    return false;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------
static glm::vec2 quadricUV(const SmoothSurfaceData& s, const glm::vec3& p) {
    switch (s.form) {
        case SmoothSurfaceData::QuadricForm::Sphere:
        case SmoothSurfaceData::QuadricForm::Ellipsoid: {
            float u = 0.5f + std::atan2(p.y, p.x) / (2.0f * kPI);
            float rad = std::max(1e-4f, s.axes.z);
            float v = 0.5f - std::asin(std::clamp(p.z / rad, -1.0f, 1.0f)) / kPI;
            return glm::vec2(u, v);
        }
        default: {
            float u = 0.5f + std::atan2(p.y, p.x) / (2.0f * kPI);
            float span = std::max(1e-4f, s.zTrim.y - s.zTrim.x);
            float v = std::clamp((p.z - s.zTrim.x) / span, 0.0f, 1.0f);
            return glm::vec2(u, v);
        }
    }
}

bool raycastSmooth(const SmoothSurfaceData& s, const glm::vec3& o, const glm::vec3& d,
                   float& tHit, glm::vec3& nrm, glm::vec2& uv) {
    if (s.model == SmoothSurfaceData::Model::Parametric) {
        return raycastParametric(s, o, d, tHit, nrm, uv);
    }

    float t0, t1;
    if (!Quadric::raycast(s.Q, o, d, t0, t1)) return false;

    const bool trimZ = (s.form == SmoothSurfaceData::QuadricForm::CylinderSide ||
                        s.form == SmoothSurfaceData::QuadricForm::ConeSide ||
                        s.form == SmoothSurfaceData::QuadricForm::Paraboloid);
    for (float t : {t0, t1}) {
        if (t <= 1e-4f) continue;
        glm::vec3 p = o + d * t;
        if (trimZ && (p.z < s.zTrim.x || p.z > s.zTrim.y)) continue;
        tHit = t;
        glm::vec3 g = Quadric::gradient(s.Q, p);
        float len = glm::length(g);
        nrm = len > 1e-8f ? g / len : -d;
        if (glm::dot(nrm, d) > 0.0f) nrm = -nrm; // face the ray
        uv = quadricUV(s, p);
        return true;
    }
    return false;
}

bool isConvex(const SmoothSurfaceData& s) {
    // Torus is the only non-convex smooth surface we model here.
    return !(s.model == SmoothSurfaceData::Model::Parametric &&
             s.pkind == SmoothSurfaceData::ParametricKind::Torus);
}

glm::vec3 supportPoint(const SmoothSurfaceData& s, const glm::vec3& dir, bool& ok) {
    ok = false;
    if (s.model == SmoothSurfaceData::Model::Quadric &&
        (s.form == SmoothSurfaceData::QuadricForm::Sphere ||
         s.form == SmoothSurfaceData::QuadricForm::Ellipsoid)) {
        // Ellipsoid support: p = (a²dx, b²dy, c²dz) / ||(a dx, b dy, c dz)||.
        glm::vec3 a = s.axes;
        glm::vec3 num(a.x * a.x * dir.x, a.y * a.y * dir.y, a.z * a.z * dir.z);
        float den = std::sqrt(a.x * a.x * dir.x * dir.x +
                              a.y * a.y * dir.y * dir.y +
                              a.z * a.z * dir.z * dir.z);
        if (den > 1e-8f) { ok = true; return num / den; }
    }
    return glm::vec3(0.0f); // caller falls back to a vertex cloud
}

float implicitSmooth(const SmoothSurfaceData& s, const glm::vec3& p) {
    if (s.model == SmoothSurfaceData::Model::Parametric) {
        return parametricSDF(s, p);
    }
    glm::vec4 P(p, 1.0f);
    return glm::dot(P, s.Q * P); // negative inside for closed quadrics
}

// ---------------------------------------------------------------------------
// Tessellation
// ---------------------------------------------------------------------------
static void pushTri(TessMesh& m, const TessVertex& a, const TessVertex& b, const TessVertex& c) {
    m.tris.push_back(a); m.tris.push_back(b); m.tris.push_back(c);
}

static TessVertex sv(const glm::vec3& p, const glm::vec3& n, const glm::vec2& uv) {
    TessVertex v; v.pos = p; v.normal = glm::normalize(n); v.uv = uv; return v;
}

static TessMesh tessQuadricSurface(const SmoothSurfaceData& s, int slices, int stacks) {
    TessMesh m;
    auto eval = [&](float u, float v) -> glm::vec3 {
        float theta = u * 2.0f * kPI;
        switch (s.form) {
            case SmoothSurfaceData::QuadricForm::Sphere:
            case SmoothSurfaceData::QuadricForm::Ellipsoid: {
                float phi = v * kPI;
                return glm::vec3(s.axes.x * std::sin(phi) * std::cos(theta),
                                 s.axes.y * std::sin(phi) * std::sin(theta),
                                 s.axes.z * std::cos(phi));
            }
            case SmoothSurfaceData::QuadricForm::CylinderSide: {
                float z = glm::mix(s.zTrim.x, s.zTrim.y, v);
                return glm::vec3(s.axes.x * std::cos(theta), s.axes.x * std::sin(theta), z);
            }
            case SmoothSurfaceData::QuadricForm::ConeSide: {
                float z = glm::mix(s.zTrim.x, s.zTrim.y, v);
                float k = s.axes.x / std::max(1e-4f, s.zTrim.y - s.zTrim.x);
                float rad = k * (s.zTrim.y - z); // 0 at apex (+halfH)
                return glm::vec3(rad * std::cos(theta), rad * std::sin(theta), z);
            }
            case SmoothSurfaceData::QuadricForm::Paraboloid: {
                float z = glm::mix(s.zTrim.x, s.zTrim.y, v);
                float a = (s.Q[0][0] > 1e-6f) ? s.Q[0][0] : 2.0f;
                float rad = std::sqrt(std::max(0.0f, z / a));
                return glm::vec3(rad * std::cos(theta), rad * std::sin(theta), z);
            }
        }
        return glm::vec3(0.0f);
    };

    for (int i = 0; i < slices; ++i) {
        float u0 = float(i) / slices, u1 = float(i + 1) / slices;
        for (int j = 0; j < stacks; ++j) {
            float v0 = float(j) / stacks, v1 = float(j + 1) / stacks;
            glm::vec3 p00 = eval(u0, v0), p10 = eval(u1, v0);
            glm::vec3 p01 = eval(u0, v1), p11 = eval(u1, v1);
            auto N = [&](const glm::vec3& p) { return Quadric::gradient(s.Q, p); };
            TessVertex a = sv(p00, N(p00), glm::vec2(u0, v0));
            TessVertex b = sv(p10, N(p10), glm::vec2(u1, v0));
            TessVertex c = sv(p11, N(p11), glm::vec2(u1, v1));
            TessVertex d = sv(p01, N(p01), glm::vec2(u0, v1));
            pushTri(m, a, b, c);
            pushTri(m, a, c, d);
        }
    }
    return m;
}

static TessMesh tessParametricSurface(const SmoothSurfaceData& s, int slices, int stacks) {
    TessMesh m;
    if (s.pkind != SmoothSurfaceData::ParametricKind::Torus) {
        // Exotics / ovoid: approximate via the SDF normal over a UV sphere shell.
        SmoothSurfaceData shell = makeSphere(s.params.size() > 0 ? s.params[0] : 0.5f);
        TessMesh base = tessQuadricSurface(shell, slices, stacks);
        for (auto& v : base.tris) {
            // Project the sphere sample onto the SDF surface by a few march steps.
            glm::vec3 dir = glm::normalize(v.pos);
            glm::vec3 p = v.pos;
            for (int it = 0; it < 8; ++it) p -= dir * parametricSDF(s, p);
            v.pos = p;
            v.normal = sdfNormal(s, p);
        }
        return base;
    }
    float R = s.params[0], r = s.params[1];
    for (int i = 0; i < slices; ++i) {
        float u0 = float(i) / slices * 2.0f * kPI, u1 = float(i + 1) / slices * 2.0f * kPI;
        for (int j = 0; j < stacks; ++j) {
            float v0 = float(j) / stacks * 2.0f * kPI, v1 = float(j + 1) / stacks * 2.0f * kPI;
            auto pt = [&](float u, float v) {
                glm::vec3 p((R + r * std::cos(v)) * std::cos(u),
                            (R + r * std::cos(v)) * std::sin(u),
                            r * std::sin(v));
                glm::vec3 c(R * std::cos(u), R * std::sin(u), 0.0f);
                return sv(p, p - c, glm::vec2(u / (2 * kPI), v / (2 * kPI)));
            };
            TessVertex a = pt(u0, v0), b = pt(u1, v0), c = pt(u1, v1), d = pt(u0, v1);
            pushTri(m, a, b, c);
            pushTri(m, a, c, d);
        }
    }
    return m;
}

TessMesh tessellateSmooth(const SmoothSurfaceData& s, int slices, int stacks) {
    return (s.model == SmoothSurfaceData::Model::Parametric)
               ? tessParametricSurface(s, slices, stacks)
               : tessQuadricSurface(s, slices, stacks);
}

} // namespace geom
