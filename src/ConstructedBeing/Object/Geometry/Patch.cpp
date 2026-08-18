#include "Patch.hpp"
#include "Singularity/OntoMath/Operations.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <algorithm>
#include <cmath>

namespace geom {

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

PatchForms patchToScalarForms(const BezierPatch& p) {
    PatchForms forms;
    if (!p.valid()) return forms;
    int nu = p.nu(), nv = p.nv();
    std::vector<double> gx(static_cast<size_t>(nu) * nv, 0.0);
    std::vector<double> gy(static_cast<size_t>(nu) * nv, 0.0);
    std::vector<double> gz(static_cast<size_t>(nu) * nv, 0.0);
    for (size_t idx = 0; idx < p.ctrl.size(); ++idx) {
        gx[idx] = static_cast<double>(p.ctrl[idx].x);
        gy[idx] = static_cast<double>(p.ctrl[idx].y);
        gz[idx] = static_cast<double>(p.ctrl[idx].z);
    }
    forms.x = OntoMath::ScalarForm::fromBivariateBernstein(p.du, p.dv, gx, "u", "v");
    forms.y = OntoMath::ScalarForm::fromBivariateBernstein(p.du, p.dv, gy, "u", "v");
    forms.z = OntoMath::ScalarForm::fromBivariateBernstein(p.du, p.dv, gz, "u", "v");
    return forms;
}

BezierPatch scalarFormsToPatch(const PatchForms& forms, int du, int dv) {
    BezierPatch p;
    p.du = du;
    p.dv = dv;
    int nu = du + 1, nv = dv + 1;
    p.ctrl.assign(static_cast<size_t>(nu) * nv, glm::vec3(0.0f));
    auto gx = OntoMath::ScalarForm::toBivariateBernstein(forms.x, du, dv, "u", "v");
    auto gy = OntoMath::ScalarForm::toBivariateBernstein(forms.y, du, dv, "u", "v");
    auto gz = OntoMath::ScalarForm::toBivariateBernstein(forms.z, du, dv, "u", "v");
    for (size_t idx = 0; idx < p.ctrl.size(); ++idx) {
        p.ctrl[idx] = glm::vec3(
            static_cast<float>(gx[idx]),
            static_cast<float>(gy[idx]),
            static_cast<float>(gz[idx])
        );
    }
    return p;
}

glm::vec3 evalBezier(const BezierPatch& p, float u, float v) {
    if (!p.valid()) return glm::vec3(0.0f);
    glm::vec3 sum(0.0f);
    for (int j = 0; j < p.nv(); ++j) {
        float bv = static_cast<float>(OntoMath::Operations::bernstein(p.dv, j, v));
        if (bv == 0.0f) continue;
        for (int i = 0; i < p.nu(); ++i) {
            float bu = static_cast<float>(OntoMath::Operations::bernstein(p.du, i, u));
            sum += (bu * bv) * p.at(i, j);
        }
    }
    return sum;
}

glm::vec3 bezierNormal(const BezierPatch& p, float u, float v) {
    if (!p.valid()) return glm::vec3(0, 0, 1);
    // Exact symbolic derivatives via OntoMath
    PatchForms forms = patchToScalarForms(p);
    auto dx_du = forms.x.derivative("u").evaluate({{"u", u}, {"v", v}}).value_or(0.0);
    auto dy_du = forms.y.derivative("u").evaluate({{"u", u}, {"v", v}}).value_or(0.0);
    auto dz_du = forms.z.derivative("u").evaluate({{"u", u}, {"v", v}}).value_or(0.0);

    auto dx_dv = forms.x.derivative("v").evaluate({{"u", u}, {"v", v}}).value_or(0.0);
    auto dy_dv = forms.y.derivative("v").evaluate({{"u", u}, {"v", v}}).value_or(0.0);
    auto dz_dv = forms.z.derivative("v").evaluate({{"u", u}, {"v", v}}).value_or(0.0);

    glm::vec3 du(static_cast<float>(dx_du), static_cast<float>(dy_du), static_cast<float>(dz_du));
    glm::vec3 dv(static_cast<float>(dx_dv), static_cast<float>(dy_dv), static_cast<float>(dz_dv));
    glm::vec3 n = glm::cross(du, dv);
    float len = glm::length(n);
    if (len > 1e-8f) return n / len;

    // Fallback in degenerate boundary corner
    const float e = 1e-3f;
    glm::vec3 fdu = evalBezier(p, std::min(1.0f, u + e), v) - evalBezier(p, std::max(0.0f, u - e), v);
    glm::vec3 fdv = evalBezier(p, u, std::min(1.0f, v + e)) - evalBezier(p, u, std::max(0.0f, v - e));
    glm::vec3 fn = glm::cross(fdu, fdv);
    float flen = glm::length(fn);
    return flen > 1e-8f ? fn / flen : glm::vec3(0, 0, 1);
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

std::vector<glm::vec3> patchToMonomial(const BezierPatch& p) {
    int nu = p.nu(), nv = p.nv();
    std::vector<glm::vec3> a(static_cast<size_t>(nu) * nv, glm::vec3(0.0f));
    if (!p.valid()) return a;
    PatchForms forms = patchToScalarForms(p);
    auto extractCoeffs = [&](const OntoMath::ScalarForm& form) {
        std::vector<double> coeff(static_cast<size_t>(nu) * nv, 0.0);
        for (const auto& term : form.terms) {
            if (!term.trans.empty()) continue;
            double expU = 0.0, expV = 0.0;
            bool valid = true;
            for (const auto& [var, exp] : term.factors) {
                if (var == "u") expU = exp;
                else if (var == "v") expV = exp;
                else { valid = false; break; }
            }
            if (!valid) continue;
            int k = static_cast<int>(std::round(expU));
            int l = static_cast<int>(std::round(expV));
            if (k >= 0 && k <= p.du && l >= 0 && l <= p.dv) {
                coeff[l * nu + k] += term.coefficient;
            }
        }
        return coeff;
    };
    auto cx = extractCoeffs(forms.x);
    auto cy = extractCoeffs(forms.y);
    auto cz = extractCoeffs(forms.z);
    for (size_t i = 0; i < a.size(); ++i) {
        a[i] = glm::vec3(cx[i], cy[i], cz[i]);
    }
    return a;
}

BezierPatch monomialToPatch(const std::vector<glm::vec3>& coeff, int du, int dv) {
    PatchForms forms;
    int nu = du + 1, nv = dv + 1;
    if (static_cast<int>(coeff.size()) != nu * nv) return makeBezierGrid(du, dv);
    for (int l = 0; l <= dv; ++l) {
        for (int k = 0; k <= du; ++k) {
            const glm::vec3& c = coeff[l * nu + k];
            std::map<std::string, double> factors;
            if (k > 0) factors["u"] = static_cast<double>(k);
            if (l > 0) factors["v"] = static_cast<double>(l);
            if (c.x != 0.0f) forms.x.terms.emplace_back(c.x, factors);
            if (c.y != 0.0f) forms.y.terms.emplace_back(c.y, factors);
            if (c.z != 0.0f) forms.z.terms.emplace_back(c.z, factors);
        }
    }
    forms.x = forms.x.normalized();
    forms.y = forms.y.normalized();
    forms.z = forms.z.normalized();
    return scalarFormsToPatch(forms, du, dv);
}

} // namespace geom
