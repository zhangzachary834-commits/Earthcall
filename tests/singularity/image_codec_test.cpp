#include "support/test_harness.hpp"
#include "Singularity/Screen/ImageCodecChannel.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include <cassert>
#include <iostream>

int main() {
    // A simple 2x2 white PNG image
    const unsigned char png_data[] = {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xb6, 0x0d, 0x24, 0x00, 0x00, 0x00, 0x14, 0x49, 0x44, 0x41, 0x54, 0x08, 0x5b, 0x63, 0xfc, 0xcf, 0xc0, 0x00, 0x44, 0x0c, 0x0c, 0x8c, 0x30, 0x06, 0xb2, 0x00, 0x00, 0x39, 0x01, 0x01, 0xfd, 0xef, 0x3a, 0x63, 0x93, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
    };

    TestSupport::BootedEngineHarness harness; // Initialize globals like materials
    Zone zone("test_zone", "zone.test");

    auto [obj, mat] = ImageCodecChannel::ingestPngFromMemory(png_data, sizeof(png_data), "test_image", zone);

    assert(obj != nullptr);
    assert(mat != nullptr);

    assert(obj->getIdentifier() == "image.test_image");
    assert(mat->getIdentifier() == "material.image.test_image");

    PropertyValue w, h, ar, cs;
    assert(obj->getDynamicProperty("image.pixelWidth", w));
    assert(std::get<double>(w) == 2.0);

    assert(obj->getDynamicProperty("image.pixelHeight", h));
    assert(std::get<double>(h) == 2.0);

    assert(obj->getDynamicProperty("image.aspectRatio", ar));
    assert(std::get<double>(ar) == 1.0);

    assert(obj->getDynamicProperty("image.colorSpace", cs));
    assert(std::get<std::string>(cs) == "sRGB");

    assert(mat->faceTextures.size() == 1);
    assert(mat->faceTextures[0].width == 2);
    assert(mat->faceTextures[0].height == 2);
    
    // Check if the pixel is white (as per the PNG)
    assert(mat->faceTextures[0].pixels[0] == 255);

    std::cout << "ImageCodecTest passed!" << std::endl;
    return 0;
}
