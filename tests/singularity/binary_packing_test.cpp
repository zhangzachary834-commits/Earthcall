#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "json.hpp"
#include "Singularity/Storage/BinaryPack.hpp"
#include "ConstructedBeing/Object/Geometry/Patch.hpp"

using json = nlohmann::json;

void test_stroke_binary_packing() {
    // Simulate a Zone with heavy strokes
    json j;
    
    // Create some fake stroke data
    std::vector<float> points(10000); // 10k points
    for(size_t i=0; i<points.size(); ++i) points[i] = i * 0.1f;
    
    BinaryPack::Writer bw;
    bw.write(static_cast<uint32_t>(1)); // 1 stroke
    bw.write(0.5f); bw.write(0.6f); bw.write(0.7f); bw.write(2.0f); // r, g, b, lineWidth
    bw.writeArray(points);
    
    j["strokesBinary"] = bw.toBinaryJson();
    
    // Serialize to msgpack
    std::vector<uint8_t> packed = json::to_msgpack(j);
    
    // Verify it deserializes
    json loaded = json::from_msgpack(packed);
    assert(loaded.contains("strokesBinary"));
    
    BinaryPack::Reader br(loaded["strokesBinary"].get_binary());
    uint32_t numStrokes = br.read<uint32_t>();
    assert(numStrokes == 1);
    
    float r = br.read<float>();
    float g = br.read<float>();
    float b = br.read<float>();
    float w = br.read<float>();
    assert(r == 0.5f && g == 0.6f && b == 0.7f && w == 2.0f);
    
    std::vector<float> readPoints;
    br.readArray(readPoints);
    assert(readPoints.size() == 10000);
    assert(readPoints[0] == 0.0f);
    assert(readPoints[9999] == 9999 * 0.1f);
    
    std::cout << "Stroke binary packing successful! Size in msgpack: " << packed.size() << " bytes\n";
    
    // Compare to standard JSON array
    json legacyJ;
    json strokesJ = json::array();
    json sj; sj["color"] = {0.5f, 0.6f, 0.7f}; sj["points"] = points;
    strokesJ.push_back(sj);
    legacyJ["strokes"] = strokesJ;
    
    std::vector<uint8_t> legacyPacked = json::to_msgpack(legacyJ);
    std::cout << "Legacy msgpack size (no zero-copy): " << legacyPacked.size() << " bytes\n";
    
    assert(packed.size() < legacyPacked.size()); // Zero-copy packing should be dramatically smaller and faster
}

void test_patch_binary_packing() {
    geom::BezierPatch p;
    p.du = 10; p.dv = 10;
    p.ctrl.resize((p.du+1)*(p.dv+1));
    for (size_t i = 0; i < p.ctrl.size(); ++i) {
        p.ctrl[i] = glm::vec3(i, i+1, i+2);
    }
    
    json j;
    j["du"] = p.du; j["dv"] = p.dv;
    
    BinaryPack::Writer bw;
    bw.writeArray(p.ctrl);
    j["ctrlBinary"] = bw.toBinaryJson();
    
    std::vector<uint8_t> packed = json::to_msgpack(j);
    
    json loaded = json::from_msgpack(packed);
    assert(loaded.contains("ctrlBinary"));
    
    geom::BezierPatch p2;
    p2.du = loaded["du"];
    p2.dv = loaded["dv"];
    BinaryPack::Reader br(loaded["ctrlBinary"].get_binary());
    br.readArray(p2.ctrl);
    
    assert(p2.ctrl.size() == p.ctrl.size());
    assert(p2.ctrl[0].x == p.ctrl[0].x);
    assert(p2.ctrl[p.ctrl.size()-1].z == p.ctrl[p.ctrl.size()-1].z);
    
    std::cout << "Patch binary packing successful! Size in msgpack: " << packed.size() << " bytes\n";
}

int main() {
    std::cout << "Testing Zero-Dependency Binary Packing...\n";
    test_stroke_binary_packing();
    test_patch_binary_packing();
    std::cout << "All binary packing tests passed!\n";
    return 0;
}
