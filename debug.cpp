#include "support/test_harness.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/Screen/ImageCodecChannel.hpp"
#include <iostream>

extern MaterialManager materials;

int main() {
    TestSupport::BootedEngineHarness harness;
    auto activeZone = harness.zones.zones()[harness.zones.currentIndex()];
    
    const unsigned char png_data[] = {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xb6, 0x0d, 0x24, 0x00, 0x00, 0x00, 0x14, 0x49, 0x44, 0x41, 0x54, 0x08, 0x5b, 0x63, 0xfc, 0xcf, 0xc0, 0x00, 0x44, 0x0c, 0x0c, 0x8c, 0x30, 0x06, 0xb2, 0x00, 0x00, 0x39, 0x01, 0x01, 0xfd, 0xef, 0x3a, 0x63, 0x93, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
    };

    auto [obj, mat] = ImageCodecChannel::ingestPngFromMemory(png_data, sizeof(png_data), "witness_img", *activeZone);
    
    std::cout << "Object Material ID: " << obj->materialId() << std::endl;
    std::cout << "Material in manager: " << (materials.get(obj->materialId()) != nullptr) << std::endl;
    std::cout << "Material num faces: " << mat->faceTextures.size() << std::endl;
    
    PropertyValue val;
    auto result = PropertyPath::parse("regionA").getValue(*obj, val);
    std::cout << "Result of reading regionA: " << (result == PropertyPath::PathResult::Ok) << std::endl;
    
    return 0;
}
