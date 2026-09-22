#include "TriGrid.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace geom {

namespace {

// Möller-Trumbore, identical in arithmetic to the linear scan this replaces —
// same epsilons, same one-sided t > 1e-4 rejection — so switching a shape onto
// the grid cannot move a hit point. That mattered enough to duplicate eighteen
// lines rather than share them across a header boundary and let one drift.
bool rayTri(const glm::vec3& o, const glm::vec3& d,
            const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
            float& t) {
    const glm::vec3 e1 = b - a, e2 = c - a, pv = glm::cross(d, e2);
    const float det = glm::dot(e1, pv);
    if (std::fabs(det) < 1e-8f) return false;
    const float inv = 1.0f / det;
    const glm::vec3 tv = o - a;
    const float u = glm::dot(tv, pv) * inv;
    if (u < 0.0f || u > 1.0f) return false;
    const glm::vec3 qv = glm::cross(tv, e1);
    const float v = glm::dot(d, qv) * inv;
    if (v < 0.0f || u + v > 1.0f) return false;
    const float tt = glm::dot(e2, qv) * inv;
    if (tt <= 1e-4f) return false;
    t = tt;
    return true;
}

} // namespace

void TriGrid::build(const TessMesh& mesh) {
    _cellStart.clear();
    _triIdx.clear();
    _dim = glm::ivec3(0);

    const size_t triCount = mesh.tris.size() / 3;
    if (triCount == 0) return;

    _lo = glm::vec3(std::numeric_limits<float>::max());
    _hi = glm::vec3(-std::numeric_limits<float>::max());
    for (const auto& v : mesh.tris) {
        _lo = glm::min(_lo, v.pos);
        _hi = glm::max(_hi, v.pos);
    }
    // A degenerate axis (a perfectly flat mesh) would divide by zero. Give every
    // axis a floor so the lattice stays well formed rather than producing NaN
    // cell coordinates that silently index nothing.
    const glm::vec3 span = glm::max(_hi - _lo, glm::vec3(1e-4f));
    _hi = _lo + span;

    // Aim for ~2 triangles per occupied cell. Only cells ON the surface are
    // occupied, and a marching-tets surface is two-dimensional inside a
    // three-dimensional lattice, so resolution goes as the square root of the
    // triangle count, not the cube root — asking for cbrt here builds a lattice
    // far too coarse and leaves the scan nearly linear.
    const float target = std::sqrt(static_cast<float>(triCount) / 2.0f);
    const float maxAxis = std::max(span.x, std::max(span.y, span.z));
    int nx = 1, ny = 1, nz = 1;
    if (maxAxis > 0.0f) {
        // Cells stay cubic: an axis gets resolution in proportion to its extent,
        // so a 1000 x 30 x 1000 box does not get 64 cells across its thin axis.
        const float cells = std::clamp(target, 1.0f, 256.0f);
        nx = std::clamp(static_cast<int>(cells * span.x / maxAxis), 1, 256);
        ny = std::clamp(static_cast<int>(cells * span.y / maxAxis), 1, 256);
        nz = std::clamp(static_cast<int>(cells * span.z / maxAxis), 1, 256);
    }
    _dim = glm::ivec3(nx, ny, nz);
    _cellSize = span / glm::vec3(_dim);

    const size_t cells = static_cast<size_t>(nx) * ny * nz;

    auto cellRangeOf = [&](size_t tri, glm::ivec3& c0, glm::ivec3& c1) {
        const glm::vec3& a = mesh.tris[tri * 3 + 0].pos;
        const glm::vec3& b = mesh.tris[tri * 3 + 1].pos;
        const glm::vec3& c = mesh.tris[tri * 3 + 2].pos;
        const glm::vec3 tlo = glm::min(a, glm::min(b, c));
        const glm::vec3 thi = glm::max(a, glm::max(b, c));
        c0 = glm::clamp(glm::ivec3((tlo - _lo) / _cellSize), glm::ivec3(0), _dim - 1);
        c1 = glm::clamp(glm::ivec3((thi - _lo) / _cellSize), glm::ivec3(0), _dim - 1);
    };

    // Two passes, counting then filling, so the CSR arrays are each allocated
    // exactly once at their final size.
    std::vector<uint32_t> counts(cells + 1, 0);
    for (size_t tri = 0; tri < triCount; ++tri) {
        glm::ivec3 c0, c1;
        cellRangeOf(tri, c0, c1);
        for (int z = c0.z; z <= c1.z; ++z)
            for (int y = c0.y; y <= c1.y; ++y)
                for (int x = c0.x; x <= c1.x; ++x)
                    ++counts[cellIndex(x, y, z) + 1];
    }
    for (size_t i = 1; i <= cells; ++i) counts[i] += counts[i - 1];

    _cellStart = counts;                 // prefix sums; counts is reused as a cursor
    _triIdx.resize(_cellStart[cells]);
    for (size_t tri = 0; tri < triCount; ++tri) {
        glm::ivec3 c0, c1;
        cellRangeOf(tri, c0, c1);
        for (int z = c0.z; z <= c1.z; ++z)
            for (int y = c0.y; y <= c1.y; ++y)
                for (int x = c0.x; x <= c1.x; ++x)
                    _triIdx[counts[cellIndex(x, y, z)]++] = static_cast<uint32_t>(tri);
    }
}

bool TriGrid::raycast(const TessMesh& mesh, const glm::vec3& o, const glm::vec3& d,
                      float& tHit) const {
    if (empty()) return false;

    // Slab test against the mesh bounds. This alone is what makes standing in
    // the air over a terrain cost nothing: the ray misses the box and no
    // triangle is ever touched.
    float tEnter = 0.0f, tExit = std::numeric_limits<float>::max();
    for (int a = 0; a < 3; ++a) {
        if (std::fabs(d[a]) < 1e-12f) {
            if (o[a] < _lo[a] || o[a] > _hi[a]) return false;
            continue;
        }
        const float inv = 1.0f / d[a];
        float t0 = (_lo[a] - o[a]) * inv;
        float t1 = (_hi[a] - o[a]) * inv;
        if (t0 > t1) std::swap(t0, t1);
        tEnter = std::max(tEnter, t0);
        tExit  = std::min(tExit,  t1);
        if (tEnter > tExit) return false;
    }
    if (tExit < 0.0f) return false;

    // Amanatides & Woo: step cell to cell in near-to-far order.
    const glm::vec3 entry = o + d * tEnter;
    glm::ivec3 cell = glm::clamp(glm::ivec3((entry - _lo) / _cellSize),
                                 glm::ivec3(0), _dim - 1);

    glm::ivec3 stepDir(0);
    glm::vec3  tMax(std::numeric_limits<float>::max());
    glm::vec3  tDelta(std::numeric_limits<float>::max());
    for (int a = 0; a < 3; ++a) {
        if (d[a] > 1e-12f) {
            stepDir[a] = 1;
            tMax[a]   = tEnter + ((_lo[a] + (cell[a] + 1) * _cellSize[a]) - entry[a]) / d[a];
            tDelta[a] = _cellSize[a] / d[a];
        } else if (d[a] < -1e-12f) {
            stepDir[a] = -1;
            tMax[a]   = tEnter + ((_lo[a] + cell[a] * _cellSize[a]) - entry[a]) / d[a];
            tDelta[a] = -_cellSize[a] / d[a];
        }
    }

    float best = std::numeric_limits<float>::max();
    bool  found = false;

    while (true) {
        const size_t c = cellIndex(cell.x, cell.y, cell.z);
        for (uint32_t i = _cellStart[c]; i < _cellStart[c + 1]; ++i) {
            const uint32_t tri = _triIdx[i];
            float t;
            if (rayTri(o, d,
                       mesh.tris[tri * 3 + 0].pos,
                       mesh.tris[tri * 3 + 1].pos,
                       mesh.tris[tri * 3 + 2].pos, t) && t < best) {
                best = t;
                found = true;
            }
        }

        // The exit parameter of the cell just tested. A hit at or before it is
        // inside this cell, and cells are visited near to far, so nothing
        // further along can beat it. A hit BEYOND it belongs to a triangle that
        // merely overlaps this cell — keep walking, or the answer would depend
        // on which cell a triangle happened to be binned into.
        const float tCellExit = std::min(tMax.x, std::min(tMax.y, tMax.z));
        if (found && best <= tCellExit) break;
        if (tCellExit > tExit) break;

        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            cell.x += stepDir.x; if (cell.x < 0 || cell.x >= _dim.x) break; tMax.x += tDelta.x;
        } else if (tMax.y < tMax.z) {
            cell.y += stepDir.y; if (cell.y < 0 || cell.y >= _dim.y) break; tMax.y += tDelta.y;
        } else {
            cell.z += stepDir.z; if (cell.z < 0 || cell.z >= _dim.z) break; tMax.z += tDelta.z;
        }
    }

    if (found) tHit = best;
    return found;
}

} // namespace geom
