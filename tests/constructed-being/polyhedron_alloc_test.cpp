#include "ConstructedBeing/Singular/Object/Object/PolyhedronData.hpp"
#include <iostream>
#include <new>
#include <cstdlib>
#include <cassert>
#include <exception>

// The task involves testing `faceNormals.reserve(faces.size())` within `computeNormals()`.
// This allocates `faces.size() * sizeof(glm::vec3)` bytes.
// Since testing memory allocation failures robustly in C++ unit tests is difficult without custom allocators,
// we selectively replace `operator new` only during the test execution block using a thread-local or static flag.

static bool g_simulateAllocFailure = false;
static int g_allocCount = 0;
static int g_targetAllocFailCount = 0;

// Note on ODR: In standard testing frameworks, overriding `operator new` locally in a test file can cause ODR violations
// if the test files are globbed into a single binary. However, as memory specifies, Earthcall C++ tests are compiled
// as standalone executables with their own `main()` function, isolating this override to just this test executable.
void* operator new(std::size_t size) {
    if (g_simulateAllocFailure) {
        g_allocCount++;
        if (g_allocCount == g_targetAllocFailCount) {
            throw std::bad_alloc();
        }
    }
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}

void operator delete(void* p) noexcept {
    std::free(p);
}

void operator delete(void* p, std::size_t) noexcept {
    std::free(p);
}

int main() {
    std::cout << "Running PolyhedronData allocator test..." << std::endl;

    PolyhedronData data;
    // We add enough items to ensure reserve() has to allocate.
    for (int i = 0; i < 100; i++) {
        data.faces.push_back({0, 1, 2});
    }
    for (int i = 0; i < 100; i++) {
        data.vertices.push_back({0,0,0});
    }

    // Try computing normals and see how many allocations happen before throwing.
    // In computeNormals(), `faceNormals.clear()` happens, then `faceNormals.reserve(...)`.
    // The `reserve` should be the very first allocation in `computeNormals()`.

    g_simulateAllocFailure = true;
    g_allocCount = 0;
    g_targetAllocFailCount = 1;

    bool computeNormalsRanWithoutThrowing = false;
    try {
        data.computeNormals();
        computeNormalsRanWithoutThrowing = true;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: Exception leaked from computeNormals: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "FAIL: unknown exception leaked from computeNormals!" << std::endl;
        return 1;
    }

    assert(computeNormalsRanWithoutThrowing);

    if (g_allocCount < g_targetAllocFailCount) {
        std::cerr << "FAIL: Allocation was not requested!" << std::endl;
        return 1;
    }

    std::cout << "Successfully caught exception in computeNormals!" << std::endl;
    g_simulateAllocFailure = false;

    std::cout << "All allocator tests passed." << std::endl;
    return 0;
}
