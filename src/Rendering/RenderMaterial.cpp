#include "Rendering/RenderMaterial.hpp"

#include "ConstructedBeing/Material/MaterialManager.hpp"

extern MaterialManager materials; // global Material beings (globals.cpp)

RenderMaterial resolveRenderMaterial(const std::string& materialId, const FaceAlbedo& albedo) {
    RenderMaterial rm;
    if (auto m = materials.resolveOrDefault(materialId)) {
        rm.baseColor = m->baseColor;
        rm.opacity   = m->opacity;
        rm.shininess = m->shininess;
        rm.specular  = m->specular;
        rm.ambient   = m->ambient;
        rm.diffuse   = m->diffuse;
    }
    rm.textureId    = albedo.handle;
    rm.albedoPixels = albedo.pixels;
    rm.albedoSize   = albedo.size;
    return rm;
}
