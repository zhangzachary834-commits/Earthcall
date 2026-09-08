// The Basic Pixel Changer canvas rendered red instead of its authored white
// even after the Zone-identity JSON path was fixed to round-trip faceColors
// correctly (see Zone_identity_store_field_level_merge.md). Found by Sol
// (GPT-5.6, session 01a0707e) on the agent intercom 2026-09-08: a SEPARATE
// binary sidecar, the physical-matter FlatBuffer (.ecmatter), was applied
// AFTER the semantic JSON zone load in ZoneManager::loadState and
// unconditionally overwrote materialId/faceTextures/faceColors from whatever
// FlatBuffer Entity matched an object's bare identifier — resolved with no
// owning-Zone disambiguation, so a stale duplicate entity (or, more subtly,
// just an old buffer written before some later re-authoring) could silently
// clobber a correctly-loaded semantic value with a stale one, always
// finishing last regardless of what the JSON path got right.
//
// The fix: materialId/faceTextures/faceColors are semantic/Material state —
// Material::toJson already round-trips all three — so applyMatterFlatBuffer
// no longer applies them at all, and buildMatterFlatBuffer no longer writes
// real values into those FlatBuffer slots (left as empty/0 offsets rather
// than removed from the schema, so an old buffer that still carries real
// data in those fields stays readable, just inert).
//
// This test proves the precedence directly: an object whose semantic state
// says white, fed a hand-built LEGACY matter buffer whose Entity for the
// same object id says red, must finish white — the exact shape of what
// Zach was seeing.

#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Storage/Schema/Earthcall_generated.h"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

extern MaterialManager materials;

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++g_failures;
        std::printf("  FAILED: %s\n", what.c_str());
        return;
    }
    std::printf("  ok: %s\n", what.c_str());
}

bool nearf(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) < eps; }

// Mirrors the shape object_roundtrip_test.cpp already builds by hand — one
// Entity, only id + face_colors populated, matching what an OLD
// buildMatterFlatBuffer (before this fix) would have written for an object
// whose faceColors happened to be the legacy cube-face default at save time.
std::vector<uint8_t> buildLegacyRedMatterBuffer(const std::string& objectId) {
    flatbuffers::FlatBufferBuilder builder(512);
    auto id_str = builder.CreateString(objectId);
    auto name_str = builder.CreateString(std::string("Shape2D"));

    std::vector<Earthcall::Schema::Vec3> fbs_colors;
    fbs_colors.push_back(Earthcall::Schema::Vec3(1.0f, 0.0f, 0.0f));
    fbs_colors.push_back(Earthcall::Schema::Vec3(1.0f, 0.0f, 0.0f));
    fbs_colors.push_back(Earthcall::Schema::Vec3(0.0f, 1.0f, 0.0f));
    fbs_colors.push_back(Earthcall::Schema::Vec3(0.0f, 1.0f, 0.0f));
    fbs_colors.push_back(Earthcall::Schema::Vec3(0.0f, 0.0f, 1.0f));
    fbs_colors.push_back(Earthcall::Schema::Vec3(0.0f, 0.0f, 1.0f));
    auto fbs_colors_vec = builder.CreateVectorOfStructs(fbs_colors);

    std::vector<float> identity16 = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    auto tf_vec = builder.CreateVector(identity16);

    auto entity = Earthcall::Schema::CreateEntity(
        builder,
        id_str,
        name_str,
        tf_vec,
        0, // polyhedron
        0, // patch
        0, // smooth
        0, // field
        0, // face_textures
        fbs_colors_vec,
        0, // sdf_nodes
        0, // laws
        0, // material_id
        nullptr, nullptr, nullptr,
        1.0f
    );
    std::vector<flatbuffers::Offset<Earthcall::Schema::Entity>> ents = {entity};
    auto chunk = Earthcall::Schema::CreateSaveChunk(
        builder, builder.CreateString("legacy_matter"), builder.CreateVector(ents));
    builder.Finish(chunk);
    const uint8_t* buf = builder.GetBufferPointer();
    return std::vector<uint8_t>(buf, buf + builder.GetSize());
}

} // namespace

int main() {
    std::printf("=== Matter/semantic precedence: legacy .ecmatter must not overwrite authored color ===\n");

    ZoneManager mgr;
    auto zone = std::make_shared<Zone>("MatterPrecedence", "strict");
    auto canvas = std::make_shared<Object>("precedence-canvas");
    canvas->setFaceColor(0, 1.0f, 1.0f, 1.0f); // authored white, the semantic value
    zone->addObject(canvas);
    mgr.addZone(zone);

    check(nearf(canvas->faceColors[0][0], 1.0f) && nearf(canvas->faceColors[0][1], 1.0f) &&
              nearf(canvas->faceColors[0][2], 1.0f),
          "canvas starts authored white");

    const auto legacyMatter = buildLegacyRedMatterBuffer("precedence-canvas");
    mgr.applyMatterFlatBuffer(legacyMatter);

    check(nearf(canvas->faceColors[0][0], 1.0f) && nearf(canvas->faceColors[0][1], 1.0f) &&
              nearf(canvas->faceColors[0][2], 1.0f),
          "a legacy matter buffer's face_colors does not overwrite the semantic value — "
          "this is the exact Basic Pixel Changer red-canvas mechanism Sol found");

    const auto freshMatter = mgr.buildMatterFlatBuffer();
    const auto* chunk = Earthcall::Schema::GetSaveChunk(freshMatter.data());
    bool foundEmptyFaceColors = false;
    bool foundEntity = false;
    if (chunk && chunk->entities()) {
        for (const auto* entity : *chunk->entities()) {
            if (!entity || !entity->id() || entity->id()->str() != "precedence-canvas") continue;
            foundEntity = true;
            foundEmptyFaceColors = (entity->face_colors() == nullptr);
        }
    }
    check(foundEntity, "the object is present in a freshly built matter buffer");
    check(foundEmptyFaceColors,
          "buildMatterFlatBuffer no longer writes real face_colors data — the write side "
          "of the same fix, so a NEW .ecmatter cannot reintroduce this bug");

    std::printf("%s (%d failure%s)\n", g_failures ? "FAIL" : "PASS", g_failures,
                g_failures == 1 ? "" : "s");
    return g_failures == 0 ? 0 : 1;
}
