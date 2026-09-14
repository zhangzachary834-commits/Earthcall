#pragma once

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include <string>
#include <memory>
#include <utility>

class ImageCodecChannel {
public:
    static std::pair<std::shared_ptr<Object>, std::shared_ptr<Material>> ingestPng(
        const std::string& filePath,
        const std::string& slug,
        Zone& zone);

    static std::pair<std::shared_ptr<Object>, std::shared_ptr<Material>> ingestPngFromMemory(
        const unsigned char* buffer,
        int len,
        const std::string& slug,
        Zone& zone);
};
