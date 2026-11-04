#include "Singularity/Screen/RenderMaterial.hpp"

#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"

#include <set>

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
        rm.colorExpr = m->colorExpr;
        rm.colorRevision = m->getRevision();
        rm.responseExpr = m->responseExpr;
        rm.responseRevision = m->getResponseRevision();
        rm.environmentResponseExpr = m->environmentResponseExpr;
        rm.environmentResponseRevision = m->getEnvironmentResponseRevision();

        if (rm.responseExpr || rm.environmentResponseExpr) {
            std::set<std::string> paths;
            const auto collectBindings = [&](const std::shared_ptr<OntoMath::Piecewise>& expr) {
                if (!expr) return;
                for (const auto& piece : expr->pieces) {
                    if (piece.mathNode) piece.mathNode->collectPropertyBindings(paths);
                    if (piece.whereLEZero) piece.whereLEZero->collectPropertyBindings(paths);
                }
            };
            collectBindings(rm.responseExpr);
            collectBindings(rm.environmentResponseExpr);

            // Resolve only authored dependencies. Missing paths stay absent:
            // the GPU compiler names and refuses the unresolved Bind rather than
            // manufacturing a value. The fingerprint includes that absence.
            std::string fingerprint;
            for (const auto& path : paths) {
                fingerprint += std::to_string(path.size()) + ":" + path + "=";
                PropertyValue value;
                if (lawGetValue(*m, PropertyPath::parse(path), value)) {
                    rm.responseBindings.emplace(path, value);
                    try {
                        const std::string encoded = propertyValueToJson(value).dump();
                        fingerprint += std::to_string(encoded.size()) + ":" + encoded;
                    } catch (...) {
                        fingerprint += "<unserializable>";
                    }
                } else {
                    fingerprint += "<unresolved>";
                }
                fingerprint += ";";
            }
            rm.responseBindingIdentity = std::move(fingerprint);
        }
    }
    rm.textureId    = albedo.handle;
    rm.albedoPixels = albedo.pixels;
    rm.albedoWidth  = albedo.width;
    rm.albedoHeight = albedo.height;
    return rm;
}
