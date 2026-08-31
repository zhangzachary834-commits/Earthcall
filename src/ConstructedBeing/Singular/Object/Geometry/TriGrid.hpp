#pragma once

// ============================================================================
// TriGrid — a uniform spatial index over a TessMesh, for ray queries.
//
// WHY THIS EXISTS: picking a being under the pointer walked EVERY triangle of
// EVERY object, every frame. `pickSurface` (Tool.cpp) has no broadphase at all,
// and `raycastTessMesh` (ObjectRaycast.cpp) is a linear Möller-Trumbore scan.
// For a cube that is 12 triangles and nobody noticed. For the noise floor —
// one field object meshed at 128 x 24 x 128 marching tets — it is hundreds of
// thousands, scanned per frame whether or not the ray goes anywhere near the
// ground. Zach measured the interaction phase at 40 ms in the Perlin zone
// "even when I'm in the air not touching the ground", which is exactly the
// signature of a pick with no bounding test: being far away costs full price.
// The To-do list had already named InteractionChannel as the likeliest lag
// among the windowed channels; this is that, found.
//
// A UNIFORM GRID rather than a BVH, deliberately. The meshes this indexes come
// out of marching tetrahedra over a regular lattice, so the triangles are
// already near-uniformly sized and near-uniformly spread across the surface —
// the input distribution a uniform grid is optimal on, and the one where a
// BVH's build cost and pointer chasing buy nothing back. Build is O(tris),
// single pass, no sorting; traversal is Amanatides-Woo 3D DDA, which visits
// cells in strict near-to-far order, so the first triangle hit inside a cell
// can end the walk as soon as the hit lies within that cell's slab.
//
// This is Kernel substrate: an acceleration structure over a cache, holding no
// authored state and answering no question a Law can ask. It carries nothing
// that is not derivable from the TessMesh it was built from.
// ============================================================================

#include "SmoothSurface.hpp"   // TessMesh

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace geom {

class TriGrid {
public:
    // Empty until built. A grid over a mesh with no triangles stays empty and
    // `raycast` returns false, which is the same answer the linear scan gives.
    TriGrid() = default;

    // Bin every triangle of `mesh` into a uniform lattice sized so the average
    // cell holds a handful of triangles. Safe to call on an empty mesh.
    void build(const TessMesh& mesh);

    bool empty() const { return _cellStart.empty(); }

    // Nearest forward intersection along `o + t*d`, in the same space the mesh
    // is expressed in. Returns false if the ray misses the mesh's bounds or
    // every triangle in every cell it crosses. `tHit` is written only on a hit.
    //
    // `d` need not be normalised; `tHit` is in units of `d`.
    bool raycast(const TessMesh& mesh, const glm::vec3& o, const glm::vec3& d,
                 float& tHit) const;

    // The mesh bounds this grid was built over. Callers use it to reject a ray
    // before touching the grid at all.
    const glm::vec3& lo() const { return _lo; }
    const glm::vec3& hi() const { return _hi; }

private:
    glm::vec3  _lo{0.0f}, _hi{0.0f};
    glm::vec3  _cellSize{1.0f};
    glm::ivec3 _dim{0};
    // CSR layout: _triIdx[_cellStart[c] .. _cellStart[c+1]) are the triangles
    // overlapping cell c. One allocation for the whole index rather than a
    // vector per cell, which for a 64^3 lattice would be 262 144 allocations.
    std::vector<uint32_t> _cellStart;
    std::vector<uint32_t> _triIdx;

    size_t cellIndex(int x, int y, int z) const {
        return (static_cast<size_t>(z) * _dim.y + y) * _dim.x + x;
    }
};

} // namespace geom
