// Smooth-surface tessellation cache & GC test (Phase 4 follow-up)
//
// Verifies:
// 1. Objects with identical SmoothSurfaceData share the cached TessMesh (s_smoothCache).
// 2. gcSmoothTessellationCache() retains cached meshes while in active use (use_count > 1).
// 3. gcSmoothTessellationCache() evicts meshes when no live Object holds them (use_count <= 1).
// 4. Multiple distinct smooth shapes are tracked, shared, and independently evicted.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <cassert>
#include <cstdio>
#include <vector>
#include <memory>

int main() {
    std::printf("=== Smooth Tessellation Cache & GC Test ===\n");

    // Clear any previous state
    Object::clearSmoothTessellationCache();
    assert(Object::smoothTessellationCacheSize() == 0);
    std::printf("  [1] Initial cache cleared (size = 0)\n");

    // Scope A: Create two spheres with identical parameters
    {
        auto obj1 = std::make_unique<Object>("sphere-1");
        Object::ShapeParams p1; p1.r = 0.5f;
        obj1->setShape(Object::ShapeKind::Sphere, p1);
        assert(obj1->hasSmoothSurface());

        size_t sizeAfterFirst = Object::smoothTessellationCacheSize();
        assert(sizeAfterFirst == 1);
        std::printf("  [2] First sphere cached (cache size = 1)\n");

        auto obj2 = std::make_unique<Object>("sphere-2");
        obj2->setShape(Object::ShapeKind::Sphere, p1);
        assert(obj2->hasSmoothSurface());

        size_t sizeAfterSecond = Object::smoothTessellationCacheSize();
        assert(sizeAfterSecond == 1); // Shared cached mesh, no second insertion
        std::printf("  [3] Second sphere shared cache entry (cache size = 1)\n");

        // GC while both are alive should evict 0 items
        size_t evicted = Object::gcSmoothTessellationCache();
        assert(evicted == 0);
        assert(Object::smoothTessellationCacheSize() == 1);
        std::printf("  [4] GC with 2 live references evicts 0 items\n");

        // Add a torus (different shape data)
        auto obj3 = std::make_unique<Object>("torus-1");
        Object::ShapeParams pTorus; pTorus.majorR = 0.7f; pTorus.minorR = 0.2f;
        obj3->setShape(Object::ShapeKind::Torus, pTorus);
        assert(obj3->hasSmoothSurface());
        assert(Object::smoothTessellationCacheSize() == 2);
        std::printf("  [5] Torus inserted (cache size = 2)\n");

        // Destroy obj1 (sphere-1)
        obj1.reset();
        evicted = Object::gcSmoothTessellationCache();
        assert(evicted == 0); // obj2 still holds the sphere mesh
        assert(Object::smoothTessellationCacheSize() == 2);
        std::printf("  [6] Destroyed 1 sphere; GC evicts 0 because 1 sphere remains\n");

        // Destroy obj2 (sphere-2)
        obj2.reset();
        evicted = Object::gcSmoothTessellationCache();
        assert(evicted == 1); // Sphere mesh should now be evicted
        assert(Object::smoothTessellationCacheSize() == 1); // Torus remains
        std::printf("  [7] Destroyed 2nd sphere; GC evicts 1 (cache size = 1, torus alive)\n");

        // Change obj3 from Torus to Cube
        obj3->setShapeKind(Object::ShapeKind::Cube);
        assert(!obj3->hasSmoothSurface());
        evicted = Object::gcSmoothTessellationCache();
        assert(evicted == 1); // Torus mesh should now be evicted
        assert(Object::smoothTessellationCacheSize() == 0);
        std::printf("  [8] Torus changed to Cube; GC evicts torus (cache size = 0)\n");
    }

    // Verification complete
    std::printf("RESULT: smooth_tessellation_cache_test: ALL OK (8/8 checks passed)\n");
    return 0;
}
