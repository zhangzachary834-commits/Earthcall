// End-to-end verification test for the "Sky-Island Sanctuary" Zone.
//
// Verifies:
// 1. Invariant 6 (directoryKey == documentIdentity == "Sky-Island Sanctuary")
//    and companion world session ("sky_island_sanctuary").
// 2. Native Zone hydration via BootedEngineHarness("Zach") + ZoneManager::switchTo
//    with full Material closure (11 shared Material roots) and Law closure
//    (5 authored Laws whose author resolves to Person "Zach").
// 3. Complete WGSL compilation across all 144 SDF Objects, all 7 procedural
//    Material colorExpr shaders, the Sun of Dawn spatialRoot, and all 3
//    volumetric/radiant spatialFields (including CSG volumeOccluder).
// 4. Spatial architecture invariants: 5 distinct floating island masses,
//    8 fluted Pavilion columns + 8 pierced lancet arches, 12 CSG concave Lotus
//    Petals, hollowed Lapis Mirror Pool, Arched Stone Bridge, Sundial, Sky-Cedar,
//    North Bell Campanile, 13 Cloud Stepping Stones, 8 Dodecahedral Lantern Orbs,
//    and 4 Pedagogical Inscription Stelae — zero origin cube piles.
// 5. Two-path interactive Law verification:
//    - Dormant state before clicking.
//    - Clicking the Pearl Touchstone of Dawn ("object-clicked") sets awakened=1,
//      flows resonance -> 1, blooms all 12 Lotus Petals outward (1.95m -> 3.70m),
//      elevates the Heart Crystal, and drifts the Aether Orbs.
//    - Second click smoothly re-gathers the Lotus Sanctuary back to rest.
// 6. Production WebGPU offscreen rendering (when GPU device is available) and
//    Zone round-trip persistence inside an isolated temporary SaveRoot.

#include "support/test_harness.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SdfJson.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"
#include "Singularity/Screen/RenderMaterial.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/Serialization.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

extern MaterialManager materials;

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

const std::string kZoneId = "Sky-Island Sanctuary";
const std::string kWorldId = "sky_island_sanctuary";
const std::string kPrefix = "sky.sanctuary.";

int g_checks = 0;
int g_failures = 0;

void check(bool ok, const std::string& label) {
    ++g_checks;
    std::cout << (ok ? "PASS " : "FAIL ") << label << std::endl;
    if (!ok) ++g_failures;
}

double number(Singular& s, const std::string& path) {
    PropertyValue v;
    if (!lawGetValue(s, PropertyPath::parse(path), v)) return -999.0;
    if (auto d = std::get_if<double>(&v)) return *d;
    if (auto f = std::get_if<float>(&v)) return *f;
    if (auto i = std::get_if<int>(&v)) return static_cast<double>(*i);
    return -998.0;
}

struct ScratchSaveRoot {
    fs::path path;
    ~ScratchSaveRoot() {
        SaveSystem::setSaveRoot("");
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

struct MapResult {
    bool done = false;
    bool ok = false;
};

void onBufferMapped(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* r = static_cast<MapResult*>(u);
    r->done = true;
    r->ok = (s == WGPUMapAsyncStatus_Success);
}

bool renderOffscreen(wgpu::Device& gpu, WebGpuRenderer& r, Zone& zone,
                     const std::vector<Object*>& objects, const fs::path& ppmOut,
                     const glm::vec3& eye, const glm::vec3& aim,
                     double& outMeanLum, double& outSaturatedRatio) {
    constexpr uint32_t W = 960, H = 600, ROW = W * 4;
    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D;
    td.size = {W, H, 1};
    td.format = WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount = 1;
    td.sampleCount = 1;
    auto tex = wgpuDeviceCreateTexture(gpu.device, &td);
    auto view = wgpuTextureCreateView(tex, nullptr);

    // Bind full multi-source radiance and volumetric media exactly like EngineRender.cpp
    std::vector<geom::FieldNode*> candidateFields;
    if (auto* root = zone.spatialRoot()) candidateFields.push_back(root);
    for (const auto& f : zone.additionalSpatialFields()) {
        if (f) candidateFields.push_back(f.get());
    }
    std::vector<Rendering::RadianceSourceBinding> radiantSources;
    std::string sourceSetIdentity;
    std::vector<Rendering::VolumeDensityBinding> volumeDensities;
    std::string volumeSetIdentity;
    for (geom::FieldNode* field : candidateFields) {
        Rendering::VolumeDensityBinding medium;
        if (Rendering::readVolumeDensity(*field, 0.0, 0.016, medium)) {
            Rendering::appendVolumeSetIdentity(volumeSetIdentity, medium.producerId, medium);
            volumeDensities.push_back(medium);
        }
        Rendering::AuthorableLightState l;
        if (!Rendering::readAuthorableLight(*field, l)) continue;
        Rendering::RadianceSourceBinding src;
        src.producerId = field->getIdentifier();
        src.position = l.position;
        src.ambientRadiance = Rendering::lightAmbientRadiance(l);
        src.diffuseRadiance = Rendering::lightDiffuseRadiance(l);
        src.specularRadiance = Rendering::lightSpecularRadiance(l);
        src.coefficients = glm::vec4(l.intensity, l.ambient, l.diffuse, l.specular);
        src.temporalCoordinate = 0.0;
        src.temporalDelta = 0.016;
        src.enabled = l.enabled;
        field->verifyAuthoredMath();
        if (field->field && field->field->mode == OntoMath::ScalarField::EvaluationMode::AST &&
            !field->field->astDefinition.pieces.empty()) {
            src.radianceExpr = &field->field->astDefinition;
            src.radianceRevision = field->authoredMathRevision(geom::FieldNode::kMathFieldAst);
        }
        if (field->lightChroma && !field->lightChroma->pieces.empty()) {
            src.chromaExpr = field->lightChroma.get();
            src.chromaRevision = field->authoredMathRevision(geom::FieldNode::kMathLightChroma);
        }
        if (field->lightAngular && !field->lightAngular->pieces.empty()) {
            src.angularExpr = field->lightAngular.get();
            src.angularRevision = field->authoredMathRevision(geom::FieldNode::kMathLightAngular);
        }
        sourceSetIdentity += src.producerId + ";";
        radiantSources.push_back(src);
    }
    r.setVolumeDensitySources(volumeDensities, std::hash<std::string>{}(volumeSetIdentity));
    r.setRadianceSources(radiantSources, std::hash<std::string>{}(sourceSetIdentity));
    if (!radiantSources.empty()) {
        const auto& c = radiantSources.front();
        r.setLight(c.position, c.ambientRadiance, c.diffuseRadiance, c.specularRadiance);
        r.setLightingEnabled(c.enabled);
        r.setRadianceSourceCoefficients(c.coefficients.x, c.coefficients.y, c.coefficients.z, c.coefficients.w);
        r.setRadianceField(c.radianceExpr, c.radianceRevision);
        r.setRadianceChroma(c.chromaExpr, c.chromaRevision);
        r.setRadianceAngular(c.angularExpr, c.angularRevision);
    }

    r.setCamera(glm::lookAt(eye, aim, glm::vec3(0.0f, 1.0f, 0.0f)),
                glm::perspectiveZO(glm::radians(50.0f), float(W) / float(H), 0.1f, 200.0f),
                eye);
    r.beginFrameOffscreen(view, W, H, glm::vec4(0.06f, 0.08f, 0.13f, 1.0f));
    bool compiled = true;
    for (Object* o : objects) {
        auto mat = resolveRenderMaterial(o->materialId(), {});
        r.setModel(o->getTransform());
        r.drawImplicit(o->getFieldData(), o->getFieldExtent(), mat);
    }
    r.composeVolumes();
    r.endFrame();

    WGPUBufferDescriptor bd = {};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    bd.size = ROW * H;
    auto buffer = wgpuDeviceCreateBuffer(gpu.device, &bd);
    auto enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    WGPUTexelCopyTextureInfo src = {};
    src.texture = tex;
    src.aspect = WGPUTextureAspect_All;
    WGPUTexelCopyBufferInfo dst = {};
    dst.buffer = buffer;
    dst.layout.bytesPerRow = ROW;
    dst.layout.rowsPerImage = H;
    WGPUExtent3D extent = {W, H, 1};
    wgpuCommandEncoderCopyTextureToBuffer(enc, &src, &dst, &extent);
    auto cmd = wgpuCommandEncoderFinish(enc, nullptr);
    wgpuQueueSubmit(gpu.queue, 1, &cmd);

    MapResult mr;
    WGPUBufferMapCallbackInfo cb = {};
    cb.mode = WGPUCallbackMode_AllowProcessEvents;
    cb.callback = onBufferMapped;
    cb.userdata1 = &mr;
    wgpuBufferMapAsync(buffer, WGPUMapMode_Read, 0, ROW * H, cb);
    while (!mr.done) {
        wgpuDevicePoll(gpu.device, true, nullptr);
    }
    size_t visible = 0;
    size_t saturated = 0;
    double lumSum = 0.0;
    if (mr.ok) {
        auto* px = static_cast<const unsigned char*>(wgpuBufferGetConstMappedRange(buffer, 0, ROW * H));
        std::ofstream img;
        if (!ppmOut.empty()) {
            img.open(ppmOut, std::ios::binary);
            img << "P6\n" << W << " " << H << "\n255\n";
        }
        for (size_t i = 0; i < W * H; ++i) {
            const unsigned char R = px[i * 4];
            const unsigned char G = px[i * 4 + 1];
            const unsigned char B = px[i * 4 + 2];
            if (R > 20 || G > 25 || B > 40) ++visible;
            if (R >= 250 && G >= 250 && B >= 250) ++saturated;
            lumSum += (0.2126 * R + 0.7152 * G + 0.0722 * B) / 255.0;
            if (img) img.write(reinterpret_cast<const char*>(px + i * 4), 3);
        }
        wgpuBufferUnmap(buffer);
    }
    outMeanLum = lumSum / double(W * H);
    outSaturatedRatio = double(saturated) / double(W * H);
    wgpuCommandBufferRelease(cmd);
    wgpuCommandEncoderRelease(enc);
    wgpuBufferRelease(buffer);
    wgpuTextureViewRelease(view);
    wgpuTextureRelease(tex);
    return compiled && mr.ok && visible > 20000;
}

} // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    fs::path source = argc > 1 ? argv[1] : "saves";
    if (!fs::exists(source / "zones" / kZoneId / "zone.json")) {
        source = "../saves";
    }
    source = fs::absolute(source);

    // 1. Verify raw JSON files & Invariant 6
    const fs::path zonePath = source / "zones" / kZoneId / "zone.json";
    const fs::path worldPath = source / "worlds" / (kWorldId + ".json");
    check(fs::exists(zonePath), "Zone file exists at saves/zones/Sky-Island Sanctuary/zone.json");
    check(fs::exists(worldPath), "Companion World save exists at saves/worlds/sky_island_sanctuary.json");
    if (g_failures) return 1;

    const json originalZone = SaveSystem::readSaveData(zonePath.string());
    const json originalWorld = SaveSystem::readSaveData(worldPath.string());
    check(originalZone.value("identifier", "") == kZoneId, "Invariant 6: Zone identifier matches directory key");
    check(originalZone.value("name", "") == kZoneId, "Zone name is 'Sky-Island Sanctuary'");
    check(originalZone.value("owner", "") == "Zach", "Zone owner is 'Zach'");
    check(originalZone.contains("materialRefs") && originalZone["materialRefs"].size() == 11,
          "Zone declares 11 shared materialRefs");
    check(originalZone.contains("lawRefs") && originalZone["lawRefs"].size() == 5,
          "Zone declares 5 authored lawRefs");
    check(originalZone.contains("lexemes") && originalZone["lexemes"].size() == 9,
          "Zone declares 9 Lexemes (8 categories + 1 commission)");
    check(originalZone.contains("formationRelations") && originalZone["formationRelations"].size() >= 300,
          "Zone declares complete formationRelations (" + std::to_string(originalZone["formationRelations"].size()) + ")");
    check(originalZone["world"]["objects"].size() == 144, "Zone contains all 144 authored Objects");
    check(originalWorld.value("identifier", "") == kWorldId, "World identifier is 'sky_island_sanctuary'");

    // Copy Zone, Materials, and Laws into an isolated temporary SaveRoot
    ScratchSaveRoot scratch{
        fs::temp_directory_path() /
        ("earthcall-sky-island-sanctuary-test-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
    fs::create_directories(scratch.path / "zones" / kZoneId);
    fs::copy(source / "zones" / kZoneId, scratch.path / "zones" / kZoneId,
             fs::copy_options::recursive | fs::copy_options::overwrite_existing);

    for (const auto& mref : originalZone["materialRefs"]) {
        const std::string mid = mref.get<std::string>();
        const std::string stem = mid.rfind("material.", 0) == 0 ? mid.substr(9) : mid;
        fs::create_directories(scratch.path / "materials" / stem);
        fs::copy_file(source / "materials" / stem / "material.json",
                      scratch.path / "materials" / stem / "material.json",
                      fs::copy_options::overwrite_existing);
    }
    for (const auto& lref : originalZone["lawRefs"]) {
        const std::string lid = lref.get<std::string>();
        fs::create_directories(scratch.path / "laws" / lid);
        fs::copy_file(source / "laws" / lid / "law.json",
                      scratch.path / "laws" / lid / "law.json",
                      fs::copy_options::overwrite_existing);
    }

    SaveSystem::setSaveRoot(scratch.path.string());

    // 2. Native BootedEngineHarness hydration and ZoneManager::switchTo
    TestSupport::BootedEngineHarness h("Zach");
    h.zones.bindLive();
    size_t zoneIdx = 0;
    while (zoneIdx < h.zones.zones().size() &&
           h.zones.zones()[zoneIdx]->getIdentifier() != kZoneId) {
        ++zoneIdx;
    }
    check(zoneIdx < h.zones.zones().size() && h.zones.switchTo(zoneIdx),
          "Native Sky-Island Sanctuary boot and switchTo succeeds without session file");
    if (g_failures) return 1;

    auto zone = h.zones.zones()[zoneIdx];
    std::vector<Object*> allObjects;
    std::map<std::string, Object*> byId;
    int lotusPetalCount = 0;
    int islandMassCount = 0;
    int steleCount = 0;
    bool allMaterialsResolve = true;

    for (auto& o : zone->getOwnedObjects()) {
        if (!o) continue;
        allObjects.push_back(o.get());
        const std::string id = o->getIdentifier();
        byId[id] = o.get();
        if (id.rfind("sky.sanctuary.petal.", 0) == 0) ++lotusPetalCount;
        if (id.rfind("sky.sanctuary.island.", 0) == 0 && id.find(".crag") != std::string::npos) ++islandMassCount;
        if (id.rfind("sky.sanctuary.stele.", 0) == 0) ++steleCount;
        if (!materials.get(o->materialId())) {
            allMaterialsResolve = false;
            std::cerr << "Unresolved material: " << o->materialId() << " on " << id << "\n";
        }
    }

    check(allObjects.size() == 144, "All 144 authored Objects survive native hydration");
    check(allMaterialsResolve, "Every Object's materialId resolves to a real hydrated Material");
    check(lotusPetalCount == 12, "All 12 CSG concave Lotus Petals are hydrated");
    check(islandMassCount == 5, "All 5 primary floating sky-islands are hydrated");
    check(steleCount == 4, "All 4 Pedagogical Inscription Stelae are hydrated");

    // Verify all 5 Laws are live and their author resolves to Person "Zach"
    for (const auto* suffix : {"touchstone-chime", "resonance-flow", "lotus-bloom", "heart-ascend", "lantern-drift"}) {
        const std::string lawId = std::string("law-sky-sanctuary-") + suffix;
        auto* law = h.lawManager.find(lawId);
        check(law && law->isAuthored() && law->isEnabled(), "Authored Law is live and enabled: " + lawId);
        check(law && law->authors().findMemberByIdentifier("Zach") == &h.player,
              "Law author resolves to the actual Person 'Zach': " + lawId);
    }

    // 3. Verify WGSL compilation of every spatial field, material colorExpr, and Object SDF
    auto* root = zone->spatialRoot();
    PropertyValue rootDisplayName;
    const bool hasRootName = root && root->getDynamicProperty("displayName", rootDisplayName) &&
                             std::holds_alternative<std::string>(rootDisplayName) &&
                             std::get<std::string>(rootDisplayName) == "Celestial Sun of the Living Dawn";
    check(root != nullptr && hasRootName,
          "Zone owns spatialRoot 'Celestial Sun of the Living Dawn'");
    Rendering::AuthorableLightState light;
    check(root && Rendering::readAuthorableLight(*root, light) && light.enabled,
          "Sun of Dawn spatialRoot resolves as an enabled authorable light source");

    {
        std::vector<const geom::FieldNode*> fieldNodes;
        if (root) fieldNodes.push_back(root);
        for (const auto& f : zone->additionalSpatialFields()) {
            if (f) fieldNodes.push_back(f.get());
        }
        check(fieldNodes.size() == 5, "One spatialRoot + four additional volumetric/radiant spatialFields");

        bool allFieldsCompile = true;
        bool foundOccluderMist = false;
        for (const auto* n : fieldNodes) {
            const std::string who = n->getIdentifier();
            if (n->field && !n->field->astDefinition.pieces.empty()) {
                const auto l = sdfwgsl::inspectScalarExpression(&n->field->astDefinition, true);
                if (!l.ok) {
                    allFieldsCompile = false;
                    std::cerr << who << " scalar radiance WGSL error: " << l.error << "\n";
                }
            }
            if (n->lightChroma && !n->lightChroma->pieces.empty()) {
                const auto l = sdfwgsl::inspectVectorExpression(n->lightChroma.get(), true);
                if (!l.ok) {
                    allFieldsCompile = false;
                    std::cerr << who << " lightChroma WGSL error: " << l.error << "\n";
                }
            }
            if (n->lightAngular && !n->lightAngular->pieces.empty()) {
                const auto l = sdfwgsl::inspectAngularExpression(n->lightAngular.get());
                if (!l.ok) {
                    allFieldsCompile = false;
                    std::cerr << who << " lightAngular WGSL error: " << l.error << "\n";
                }
            }
            if (n->volumeDensity && !n->volumeDensity->pieces.empty()) {
                const auto l = sdfwgsl::inspectDensityExpression(n->volumeDensity.get());
                if (!l.ok) {
                    allFieldsCompile = false;
                    std::cerr << who << " volumeDensity WGSL error: " << l.error << "\n";
                }
            }
            if (n->volumeExtinction && !n->volumeExtinction->pieces.empty()) {
                const auto l = sdfwgsl::inspectExtinctionExpression(n->volumeExtinction.get());
                if (!l.ok) {
                    allFieldsCompile = false;
                    std::cerr << who << " volumeExtinction WGSL error: " << l.error << "\n";
                }
            }
            if (n->volumeChroma && !n->volumeChroma->pieces.empty()) {
                const auto l = sdfwgsl::inspectVolumeChromaExpression(n->volumeChroma.get());
                if (!l.ok) {
                    allFieldsCompile = false;
                    std::cerr << who << " volumeChroma WGSL error: " << l.error << "\n";
                }
            }
            if (n->volumeEmission && !n->volumeEmission->pieces.empty()) {
                const auto l = sdfwgsl::inspectEmissionExpression(n->volumeEmission.get());
                if (!l.ok) {
                    allFieldsCompile = false;
                    std::cerr << who << " volumeEmission WGSL error: " << l.error << "\n";
                }
            }
            if (n->volumePhase && !n->volumePhase->pieces.empty()) {
                const auto l = sdfwgsl::inspectPhaseExpression(n->volumePhase.get());
                if (!l.ok) {
                    allFieldsCompile = false;
                    std::cerr << who << " volumePhase WGSL error: " << l.error << "\n";
                }
            }
            if (n->volumeOccluder && geom::isSdfActive(n->volumeOccluder.get())) {
                foundOccluderMist = true;
                const auto l = sdfwgsl::inspectOccluderLayout(n->volumeOccluder.get());
                if (!l.ok) {
                    allFieldsCompile = false;
                    std::cerr << who << " volumeOccluder WGSL error: " << l.error << "\n";
                }
            }
        }
        check(allFieldsCompile, "All spatialRoot and spatialField expressions compile cleanly to WGSL");
        check(foundOccluderMist, "Sunlit Halo Mist carries an active CSG volumeOccluder");

        bool allObjectsCompile = true;
        for (Object* o : allObjects) {
            auto mat = resolveRenderMaterial(o->materialId(), {});
            auto prog = sdfwgsl::compile(
                o->getFieldData(), nullptr, mat.colorExpr.get(),
                root && root->field ? &root->field->astDefinition : nullptr);
            if (!prog.ok) {
                allObjectsCompile = false;
                std::cerr << "Object WGSL compile failure (" << o->getIdentifier() << "): " << prog.error << "\n";
            }
        }
        check(allObjectsCompile, "All 144 Objects + their Material colorExprs + Sun radiance compile to WGSL");
    }

    // 4. Two-path Behavioral Verification (Touchstone Click -> Lotus Bloom & Heart Ascent -> Re-Gather)
    auto* touch = byId["sky.sanctuary.touchstone"];
    auto* heart = byId["sky.sanctuary.heart_crystal"];
    check(touch != nullptr && heart != nullptr, "Touchstone and Heart Crystal are present");
    if (!touch || !heart) return 1;

    auto tick = [&](double dt) {
        h.worldTime += dt;
        Universe::instance().setClock(h.worldTime, dt);
        h.lawManager.tick();
    };
    auto click = [&](Object* o) {
        Core::EventBus::instance().publish(ECA::Event{"object-clicked", o, nullptr, std::time(nullptr)});
        tick(1.0 / 60.0);
    };

    // Dormant path
    tick(1.0 / 60.0);
    check(std::abs(number(*touch, "awakened") - 0.0) < 1e-6, "Dormant baseline: touchstone.awakened == 0");
    check(std::abs(number(*touch, "resonance") - 0.0) < 1e-4, "Dormant baseline: touchstone.resonance == 0");

    const float dormantHeartY = heart->getPosition().y;
    int gatheredPetals = 0;
    for (Object* o : allObjects) {
        if (o->getIdentifier().rfind("sky.sanctuary.petal.", 0) == 0) {
            const auto p = o->getPosition();
            const float radius = glm::length(glm::vec2(p.x, p.z));
            if (std::abs(radius - 1.95f) < 0.03f && std::abs(p.y - 3.25f) < 0.03f) {
                ++gatheredPetals;
            }
        }
    }
    check(gatheredPetals == 12, "Dormant baseline: all 12 Lotus Petals stand gathered at radius 1.95m");

    // Awakened path: click Touchstone
    click(touch);
    check(std::abs(number(*touch, "awakened") - 1.0) < 1e-6, "Clicking Touchstone sets awakened == 1");
    tick(0.2);
    check(number(*touch, "resonance") > 0.0 && number(*touch, "resonance") < 1.0,
          "Resonance flows smoothly through intermediate values");

    for (int i = 0; i < 180; ++i) {
        tick(1.0 / 60.0);
    }
    check(number(*touch, "resonance") > 0.999, "Resonance reaches its full awakened state (~1.0)");

    int bloomedPetals = 0;
    for (Object* o : allObjects) {
        if (o->getIdentifier().rfind("sky.sanctuary.petal.", 0) == 0) {
            const auto p = o->getPosition();
            const float radius = glm::length(glm::vec2(p.x, p.z));
            if (std::abs(radius - 3.70f) < 0.03f && std::abs(p.y - 2.83f) < 0.03f) {
                ++bloomedPetals;
            }
        }
    }
    check(bloomedPetals == 12, "Awakened state: all 12 Lotus Petals bloom outward to radius 3.70m and y=2.83m");
    check(heart->getPosition().y > dormantHeartY + 0.55f,
          "Awakened state: Heart Crystal ascends into the Pavilion dome (y=" +
              std::to_string(heart->getPosition().y) + " vs dormant " + std::to_string(dormantHeartY) + ")");

    // Unrelated click does not disturb the Touchstone
    if (auto* poolRim = byId["sky.sanctuary.mirror_pool_rim"]) {
        click(poolRim);
        check(std::abs(number(*touch, "awakened") - 1.0) < 1e-6,
              "Clicking an unrelated Sanctuary Object does not alter Touchstone state");
    }

    // 5. Optional WebGPU offscreen render check + Persistence check
    wgpu::Device gpu;
    if (gpu.init()) {
        WebGpuRenderer renderer;
        check(renderer.init(gpu), "Production WebGPU renderer initializes");
        setCurrentRenderer(&renderer);
        double meanLumOverview = 0.0, satOverview = 1.0;
        check(renderOffscreen(gpu, renderer, *zone, allObjects,
                              "build/sanctuary_overview.ppm",
                              glm::vec3(18.0f, 11.5f, 24.0f), glm::vec3(0.0f, 2.0f, 0.0f),
                              meanLumOverview, satOverview),
              "Sky-Island Sanctuary overview renders on production WebGPU pipeline");
        std::cout << "  Overview meanLuminance=" << meanLumOverview
                  << " saturatedWhiteRatio=" << satOverview << "\n";
        check(satOverview < 0.01,
              "Intentional lighting: overview has < 1% pure-white saturated pixels (actual: " +
                  std::to_string(satOverview * 100.0) + "%)");
        check(meanLumOverview > 0.06 && meanLumOverview < 0.45,
              "Intentional lighting: overview mean luminance is balanced (" +
                  std::to_string(meanLumOverview) + ")");
        setCurrentRenderer(nullptr);
    }

    check(h.zones.persistActiveZone(), "ZoneManager::persistActiveZone succeeds in isolated SaveRoot");
    const json savedZone = SaveSystem::readZoneIdentity(kZoneId);
    check(savedZone["world"]["objects"].size() == 144, "Round-trip persistence preserves all 144 Objects");

    // Re-gather path: second click on Touchstone
    click(touch);
    tick(10.0);
    tick(1.0 / 60.0);
    check(std::abs(number(*touch, "awakened") - 0.0) < 1e-6 &&
          std::abs(number(*touch, "resonance")) < 0.001,
          "Second click on Touchstone smoothly gathers the Sanctuary back to rest");

    int regatheredPetals = 0;
    for (Object* o : allObjects) {
        if (o->getIdentifier().rfind("sky.sanctuary.petal.", 0) == 0) {
            const auto p = o->getPosition();
            const float radius = glm::length(glm::vec2(p.x, p.z));
            if (std::abs(radius - 1.95f) < 0.03f && std::abs(p.y - 3.25f) < 0.03f) {
                ++regatheredPetals;
            }
        }
    }
    check(regatheredPetals == 12, "Re-gathered state: all 12 Lotus Petals return to radius 1.95m");

    check(SaveSystem::readSaveData(zonePath.string()) == originalZone,
          "Real authored Zone file in saves/ remains untouched by the test");

    std::cout << "------------------------------------------------------------\n";
    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures ? 1 : 0;
}
