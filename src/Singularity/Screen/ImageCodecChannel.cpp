#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Screen/ImageCodecChannel.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../../../third_party/stb/stb_image.h"

extern MaterialManager materials;

std::pair<std::shared_ptr<Object>, std::shared_ptr<Material>> ImageCodecChannel::ingestPngFromMemory(
    const unsigned char* buffer,
    int len,
    const std::string& slug,
    Zone& zone)
{
    int w, h, channels;
    unsigned char* data = stbi_load_from_memory(buffer, len, &w, &h, &channels, 4);
    if (!data) return {nullptr, nullptr};

    std::string matName = "material.image." + slug;
    auto mat = materials.create(matName);
    mat->initFaceTextures(1, w, h);
    FaceTexture& ft = mat->faceTextures[0];
    const size_t bytes = static_cast<size_t>(w) * h * 4;
    ft.pixels.assign(data, data + bytes);
    stbi_image_free(data);

    auto obj = std::make_shared<Object>("image." + slug);
    obj->setShape(ObjectTypes::ShapeKind::Shape2D);
    obj->setMaterialId(mat->getIdentifier());
    
    obj->setDynamicProperty("image.pixelWidth", PropertyValue(static_cast<double>(w)));
    obj->setDynamicProperty("image.pixelHeight", PropertyValue(static_cast<double>(h)));
    obj->setDynamicProperty("image.aspectRatio", PropertyValue(static_cast<double>(w) / static_cast<double>(h)));
    obj->setDynamicProperty("image.colorSpace", PropertyValue(std::string("sRGB")));
    obj->setDynamicProperty("image.regions", PropertyValue(std::make_shared<PropertyDict>()));
    PropertyPath::parse("shape.width2D").setValue(*obj, PropertyValue(static_cast<double>(w)));
    PropertyPath::parse("shape.height2D").setValue(*obj, PropertyValue(static_cast<double>(h)));
    
    zone.addObject(obj);
    
    return {obj, mat};
}

std::pair<std::shared_ptr<Object>, std::shared_ptr<Material>> ImageCodecChannel::ingestPng(
    const std::string& filePath,
    const std::string& slug,
    Zone& zone)
{
    int w, h, channels;
    unsigned char* data = stbi_load(filePath.c_str(), &w, &h, &channels, 4);
    if (!data) return {nullptr, nullptr};
    
    std::string matName = "material.image." + slug;
    auto mat = materials.create(matName);
    mat->initFaceTextures(1, w, h);
    FaceTexture& ft = mat->faceTextures[0];
    const size_t bytes = static_cast<size_t>(w) * h * 4;
    ft.pixels.assign(data, data + bytes);
    stbi_image_free(data);
    
    auto obj = std::make_shared<Object>("image." + slug);
    obj->setShape(ObjectTypes::ShapeKind::Shape2D);
    obj->setMaterialId(mat->getIdentifier());
    
    obj->setDynamicProperty("image.pixelWidth", PropertyValue(static_cast<double>(w)));
    obj->setDynamicProperty("image.pixelHeight", PropertyValue(static_cast<double>(h)));
    obj->setDynamicProperty("image.aspectRatio", PropertyValue(static_cast<double>(w) / static_cast<double>(h)));
    obj->setDynamicProperty("image.colorSpace", PropertyValue(std::string("sRGB")));
    obj->setDynamicProperty("image.regions", PropertyValue(std::make_shared<PropertyDict>()));
    PropertyPath::parse("shape.width2D").setValue(*obj, PropertyValue(static_cast<double>(w)));
    PropertyPath::parse("shape.height2D").setValue(*obj, PropertyValue(static_cast<double>(h)));
    
    zone.addObject(obj);
    
    return {obj, mat};
}
