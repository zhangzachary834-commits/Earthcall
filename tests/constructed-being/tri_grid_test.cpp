// TriGrid must return exactly what the exhaustive scan returns.
//
// WHY THIS EXISTS: picking a being walked every triangle of every object, every
// frame — 40 ms in the Perlin zone, and the same 40 ms whether the pointer was
// on the ground or aimed at empty sky, because `pickSurface` has no broadphase
// and `raycastTessMesh` was a linear Möller-Trumbore scan (Bugs.md #16).
// geom::TriGrid indexes the mesh so the pick visits a handful of cells instead.
//
// An index is only worth having if it agrees with the answer it replaced. A
// spatial structure fails in a way that is very easy to miss by eye: it does
// not return garbage, it returns a *slightly further* hit, or none, for rays
// that graze a cell boundary or hit a triangle binned into a cell the walk
// exits before testing. Those rays are a thin set — a hand-picked handful will
// not find them — so this fires thousands of pseudo-random rays at meshes with
// deliberately awkward shapes and demands the grid's answer equal the linear
// scan's to within float tolerance, hit-or-miss included.

#include "ConstructedBeing/Singular/Object/Geometry/TriGrid.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"

#include <glm/glm.hpp>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    std::printf("  %s: %s\n", ok ? "ok" : "FAILED", what.c_str());
    if (!ok) ++g_failures;
}

// The scan TriGrid replaced, reproduced here so the test owns its own ground
// truth rather than trusting the thing under test to hand it over.
bool linearScan(const geom::TessMesh& m, const glm::vec3& o, const glm::vec3& d, float& tHit) {
    float nearest = 1e9f;
    bool found = false;
    for (size_t i = 0; i + 2 < m.tris.size(); i += 3) {
        const glm::vec3& a = m.tris[i].pos;
        const glm::vec3& b = m.tris[i + 1].pos;
        const glm::vec3& c = m.tris[i + 2].pos;
        const glm::vec3 e1 = b - a, e2 = c - a, pv = glm::cross(d, e2);
        const float det = glm::dot(e1, pv);
        if (std::fabs(det) < 1e-8f) continue;
        const float inv = 1.0f / det;
        const glm::vec3 tv = o - a;
        const float u = glm::dot(tv, pv) * inv;
        if (u < 0.0f || u > 1.0f) continue;
        const glm::vec3 qv = glm::cross(tv, e1);
        const float v = glm::dot(d, qv) * inv;
        if (v < 0.0f || u + v > 1.0f) continue;
        const float t = glm::dot(e2, qv) * inv;
        if (t > 1e-4f && t < nearest) { nearest = t; found = true; }
    }
    if (found) tHit = nearest;
    return found;
}

// Deterministic, so a failure is reproducible by rerunning the binary.
struct Rng {
    uint32_t s = 0x1234567u;
    float next() {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return static_cast<float>(s & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu);
    }
    float range(float lo, float hi) { return lo + next() * (hi - lo); }
};

geom::SdfNode leaf(geom::SdfPrim prim, glm::vec3 dims, float p0 = 0.0f) {
    geom::SdfNode n;
    n.op = geom::SdfOp::Leaf;
    n.prim = prim;
    n.dims = dims;
    n.p0 = p0;
    return n;
}

geom::SdfNode expr(const std::string& e) {
    geom::SdfNode n = geom::makeImplicit(e);
    return n;
}

// Fire rays from all directions and demand the two agree on every one.
void agreesOn(const char* name, const geom::SdfNode& field,
              const glm::vec3& extent, const glm::ivec3& res, int rays) {
    const geom::TessMesh mesh = geom::tessellateSdf(field, extent, res);
    geom::TriGrid grid;
    grid.build(mesh);

    if (mesh.tris.empty()) {
        check(false, std::string(name) + ": tessellated to nothing, the case proves nothing");
        return;
    }

    Rng rng;
    int disagreements = 0, hits = 0;
    float worstDelta = 0.0f;
    // Origins spread over a box a little larger than the shape. Much wider and
    // most rays miss everything, which passes trivially and tests nothing.
    const float reach = glm::length(extent) * 1.2f;

    for (int i = 0; i < rays; ++i) {
        // Origins both outside and INSIDE the volume: a ray starting inside
        // enters the lattice at t = 0 rather than at a face, which is its own
        // branch in the DDA setup.
        const glm::vec3 o = (i % 4 == 0)
            ? glm::vec3(rng.range(-extent.x, extent.x),
                        rng.range(-extent.y, extent.y),
                        rng.range(-extent.z, extent.z))
            : glm::vec3(rng.range(-reach, reach),
                        rng.range(-reach, reach),
                        rng.range(-reach, reach));
        glm::vec3 d(rng.range(-1.0f, 1.0f), rng.range(-1.0f, 1.0f), rng.range(-1.0f, 1.0f));
        // Every few rays, aim straight down an axis — the degenerate-direction
        // path the slab test and the DDA both special-case.
        if (i % 7 == 0) d = glm::vec3(0.0f, -1.0f, 0.0f);
        if (i % 11 == 0) d = glm::vec3(1.0f, 0.0f, 0.0f);
        if (glm::dot(d, d) < 1e-8f) continue;
        d = glm::normalize(d);

        float tLin = 0.0f, tGrid = 0.0f;
        const bool hLin  = linearScan(mesh, o, d, tLin);
        const bool hGrid = grid.raycast(mesh, o, d, tGrid);

        if (hLin) ++hits;
        if (hLin != hGrid) { ++disagreements; continue; }
        if (hLin) {
            const float delta = std::fabs(tLin - tGrid);
            worstDelta = std::max(worstDelta, delta);
            if (delta > 1e-3f) ++disagreements;
        }
    }

    check(hits > rays / 20,
          std::string(name) + ": the ray set actually hits the mesh (" +
          std::to_string(hits) + " of " + std::to_string(rays) + ")");
    check(disagreements == 0,
          std::string(name) + ": grid agrees with the exhaustive scan on every ray");
    if (disagreements != 0)
        std::printf("    (%d disagreements, worst |dt| = %.6f, %zu triangles)\n",
                    disagreements, worstDelta, mesh.tris.size() / 3);
}

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("=== TriGrid agrees with the scan it replaced ===\n");

    agreesOn("sphere", leaf(geom::SdfPrim::Sphere, glm::vec3(0.7f)),
             glm::vec3(1.0f), glm::ivec3(24), 4000);

    agreesOn("torus", leaf(geom::SdfPrim::Torus, glm::vec3(0.55f, 0.2f, 0.0f)),
             glm::vec3(1.0f), glm::ivec3(28), 4000);

    // A LOPSIDED box, which is the shape the noise floor actually is and the one
    // that breaks a lattice sized by a single cube root: 1000 x 30 x 1000 in
    // miniature. Cells must stay cubic or the thin axis gets one cell.
    agreesOn("lopsided heightfield", expr("y - 0.3*sin(3.0*x)*cos(3.0*z)"),
             glm::vec3(10.0f, 1.0f, 10.0f), glm::ivec3(40, 12, 40), 4000);

    // Two disjoint pieces, so the walk must survive crossing empty cells
    // between them rather than stopping at the first gap.
    {
        auto a = std::make_shared<geom::SdfNode>(leaf(geom::SdfPrim::Sphere, glm::vec3(0.5f)));
        a->offset = glm::vec3(-0.62f, 0.0f, 0.0f);
        auto b = std::make_shared<geom::SdfNode>(leaf(geom::SdfPrim::Sphere, glm::vec3(0.5f)));
        b->offset = glm::vec3(0.62f, 0.0f, 0.0f);
        geom::SdfNode both;
        both.op = geom::SdfOp::Union;
        both.children = { a, b };
        agreesOn("two disjoint spheres", both, glm::vec3(1.2f), glm::ivec3(32), 4000);
    }

    // --- The empty mesh is not a crash -------------------------------------
    {
        geom::TessMesh empty;
        geom::TriGrid g;
        g.build(empty);
        float t = 0.0f;
        check(g.empty(), "a grid over an empty mesh is empty");
        check(!g.raycast(empty, glm::vec3(0, 5, 0), glm::vec3(0, -1, 0), t),
              "and raycasting it misses rather than reading out of bounds");
    }

    // --- Rebuilding replaces, never accumulates ----------------------------
    // The grid is rebuilt in place every time the field mesh is, so a build that
    // appended to the previous index would grow without bound and slowly return
    // hits belonging to a shape the object no longer has.
    {
        const geom::TessMesh sphere =
            geom::tessellateSdf(leaf(geom::SdfPrim::Sphere, glm::vec3(0.7f)), glm::vec3(1.0f), glm::ivec3(20));
        const geom::TessMesh box =
            geom::tessellateSdf(leaf(geom::SdfPrim::Box, glm::vec3(0.4f)), glm::vec3(1.0f), glm::ivec3(20));
        geom::TriGrid g;
        g.build(sphere);
        g.build(box);          // second build over a DIFFERENT mesh
        float tGrid = 0.0f, tLin = 0.0f;
        const glm::vec3 o(0.0f, 3.0f, 0.0f), d(0.0f, -1.0f, 0.0f);
        const bool hGrid = g.raycast(box, o, d, tGrid);
        const bool hLin  = linearScan(box, o, d, tLin);
        check(hGrid == hLin && (!hLin || std::fabs(tGrid - tLin) < 1e-3f),
              "a rebuilt grid answers for the new mesh, not the old one");
    }

    std::printf(g_failures == 0 ? "tri_grid_test: ALL OK\n" : "tri_grid_test: FAILURES\n");
    return g_failures > 0 ? 1 : 0;
}
