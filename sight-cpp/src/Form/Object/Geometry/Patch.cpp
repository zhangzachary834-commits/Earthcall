#include "Patch.hpp"

#include <algorithm>
#include <cmath>

namespace geom {

static double binom(int n, int k) {
    if (k < 0 || k > n) return 0.0;
    double r = 1.0;
    for (int i = 0; i < k; ++i) r = r * (n - i) / (i + 1);
    return r;
}

static float bernstein(int n, int i, float t) {
    return static_cast<float>(binom(n, i)) * std::pow(t, i) * std::pow(1.0f - t, n - i);
}

BezierPatch makeBezierGrid(int du, int dv, float size) {
    BezierPatch p;
    p.du = std::max(1, du);
    p.dv = std::max(1, dv);
    p.ctrl.resize(p.nu() * p.nv());
    for (int j = 0; j < p.nv(); ++j) {
        for (int i = 0; i < p.nu(); ++i) {
            float fx = (p.nu() > 1) ? float(i) / (p.nu() - 1) : 0.5f;
            float fy = (p.nv() > 1) ? float(j) / (p.nv() - 1) : 0.5f;
            p.at(i, j) = glm::vec3(glm::mix(-size, size, fx), glm::mix(-size, size, fy), 0.0f);
        }
    }
    return p;
}

glm::vec3 evalBezier(const BezierPatch& p, float u, float v) {
    if (!p.valid()) return glm::vec3(0.0f);
    glm::vec3 sum(0.0f);
    for (int j = 0; j < p.nv(); ++j) {
        float bv = bernstein(p.dv, j, v);
        if (bv == 0.0f) continue;
        for (int i = 0; i < p.nu(); ++i) {
            float bu = bernstein(p.du, i, u);
            sum += (bu * bv) * p.at(i, j);
        }
    }
    return sum;
}

glm::vec3 bezierNormal(const BezierPatch& p, float u, float v) {
    const float e = 1e-3f;
    glm::vec3 du = evalBezier(p, std::min(1.0f, u + e), v) - evalBezier(p, std::max(0.0f, u - e), v);
    glm::vec3 dv = evalBezier(p, u, std::min(1.0f, v + e)) - evalBezier(p, u, std::max(0.0f, v - e));
    glm::vec3 n = glm::cross(du, dv);
    float len = glm::length(n);
    return len > 1e-8f ? n / len : glm::vec3(0, 0, 1);
}

TessMesh tessellateBezier(const BezierPatch& p, int resU, int resV) {
    TessMesh m;
    if (!p.valid()) return m;
    auto vert = [&](float u, float v) {
        TessVertex tv;
        tv.pos = evalBezier(p, u, v);
        tv.normal = bezierNormal(p, u, v);
        tv.uv = glm::vec2(u, v);
        return tv;
    };
    for (int i = 0; i < resU; ++i) {
        float u0 = float(i) / resU, u1 = float(i + 1) / resU;
        for (int j = 0; j < resV; ++j) {
            float v0 = float(j) / resV, v1 = float(j + 1) / resV;
            TessVertex a = vert(u0, v0), b = vert(u1, v0), c = vert(u1, v1), d = vert(u0, v1);
            m.tris.push_back(a); m.tris.push_back(b); m.tris.push_back(c);
            m.tris.push_back(a); m.tris.push_back(c); m.tris.push_back(d);
        }
    }
    return m;
}

void elevateU(BezierPatch& p) {
    if (!p.valid()) return;
    int n = p.du;
    BezierPatch q; q.du = n + 1; q.dv = p.dv; q.ctrl.resize(q.nu() * q.nv());
    for (int j = 0; j < p.nv(); ++j) {
        q.at(0, j) = p.at(0, j);
        for (int i = 1; i <= n; ++i) {
            float a = float(i) / float(n + 1);
            q.at(i, j) = a * p.at(i - 1, j) + (1.0f - a) * p.at(i, j);
        }
        q.at(n + 1, j) = p.at(n, j);
    }
    p = q;
}

std::vector<glm::vec3> patchToMonomial(const BezierPatch& p) {
    // a_kl = Σ_i Σ_j [ (-1)^(k-i) C(du,k) C(k,i) ] [ (-1)^(l-j) C(dv,l) C(l,j) ] P_ij
    int du = p.du, dv = p.dv, nu = p.nu(), nv = p.nv();
    std::vector<glm::vec3> a(static_cast<size_t>(nu) * nv, glm::vec3(0.0f));
    if (!p.valid()) return a;
    for (int k = 0; k <= du; ++k)
        for (int l = 0; l <= dv; ++l) {
            glm::vec3 sum(0.0f);
            for (int i = 0; i <= k; ++i) {
                double mu = ((k - i) % 2 == 0 ? 1.0 : -1.0) * binom(du, k) * binom(k, i);
                for (int j = 0; j <= l; ++j) {
                    double mv = ((l - j) % 2 == 0 ? 1.0 : -1.0) * binom(dv, l) * binom(l, j);
                    sum += static_cast<float>(mu * mv) * p.at(i, j);
                }
            }
            a[l * nu + k] = sum;
        }
    return a;
}

BezierPatch monomialToPatch(const std::vector<glm::vec3>& coeff, int du, int dv) {
    // P_ij = Σ_k Σ_l [ C(i,k)/C(du,k) ] [ C(j,l)/C(dv,l) ] a_kl
    BezierPatch p; p.du = du; p.dv = dv;
    int nu = du + 1, nv = dv + 1;
    p.ctrl.assign(static_cast<size_t>(nu) * nv, glm::vec3(0.0f));
    if (static_cast<int>(coeff.size()) != nu * nv) return p;
    for (int i = 0; i <= du; ++i)
        for (int j = 0; j <= dv; ++j) {
            glm::vec3 sum(0.0f);
            for (int k = 0; k <= i; ++k) {
                double cu = binom(i, k) / binom(du, k);
                for (int l = 0; l <= j; ++l) {
                    double cv = binom(j, l) / binom(dv, l);
                    sum += static_cast<float>(cu * cv) * coeff[l * nu + k];
                }
            }
            p.at(i, j) = sum;
        }
    return p;
}

void elevateV(BezierPatch& p) {
    if (!p.valid()) return;
    int n = p.dv;
    BezierPatch q; q.du = p.du; q.dv = n + 1; q.ctrl.resize(q.nu() * q.nv());
    for (int i = 0; i < p.nu(); ++i) {
        q.at(i, 0) = p.at(i, 0);
        for (int j = 1; j <= n; ++j) {
            float a = float(j) / float(n + 1);
            q.at(i, j) = a * p.at(i, j - 1) + (1.0f - a) * p.at(i, j);
        }
        q.at(i, n + 1) = p.at(i, n);
    }
    p = q;
}

} // namespace geom
