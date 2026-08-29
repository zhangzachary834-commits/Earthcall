#!/bin/bash
cat << 'CXX' > tests/scratch/mesh_test.cpp
#include <iostream>
#include <chrono>
#include "Singularity/Storage/Serialization.hpp"
#include "ZonesOfEarth/Zone.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

int main() {
    std::ifstream f("saves/zones/Perlin Noise Floor Zone/zone.json");
    nlohmann::json j;
    f >> j;
    Zone z("test");
    applyZoneJson(z, j, true);
    for (const auto& obj : z.getOwnedObjects()) {
        if (obj->getSpatialKind() == Object::SpatialKind::Field) {
            std::cout << "Building mesh..." << std::endl;
            auto start = std::chrono::high_resolution_clock::now();
            obj->rebuildGeometryCaches();
            obj->rebuildFieldMesh();
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
            std::cout << "Triangles: " << obj->getFieldMesh().tris.size() / 3 << std::endl;
        }
    }
    return 0;
}
CXX

cat << 'CMAKE' >> CMakeLists.txt
add_executable(mesh_test tests/scratch/mesh_test.cpp)
target_link_libraries(mesh_test PRIVATE earthcall_core)
CMAKE

cmake --build build --target mesh_test -j8
./build/mesh_test
