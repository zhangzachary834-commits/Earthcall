#include <iostream>
#include "Singularity/Storage/Serialization.hpp"
#include "ZonesOfEarth/Zone.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

int main() {
    std::ifstream f("saves/zones/Perlin Noise Floor Zone/zone.json");
    if (!f.is_open()) { std::cout << "no file" << std::endl; return 1; }
    nlohmann::json j;
    f >> j;
    Zone z("test");
    applyZoneJson(z, j, true);
    for (const auto& obj : z.getOwnedObjects()) {
        if (obj->getSpatialKind() == Object::SpatialKind::Field) {
            obj->setRenderMode(Object::RenderMode::Mesh);
            obj->rebuildGeometryCaches();
            obj->rebuildFieldMesh();
            std::cout << "Triangles: " << obj->getFieldMesh().tris.size() / 3 << std::endl;
        }
    }
    return 0;
}
