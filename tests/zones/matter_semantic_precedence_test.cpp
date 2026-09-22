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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

std::vector<float> translationMatrix(float x, float y, float z) {
    return {1,0,0,0, 0,1,0,0, 0,0,1,0, x,y,z,1};
}

glm::vec3 translationOf(const glm::mat4& m) { return glm::vec3(m[3][0], m[3][1], m[3][2]); }

// One spec per Entity a test wants in a hand-built matter buffer.
// ownerIdentifier empty ("") means an ownerless legacy record — the shape
// every real pre-fix .ecmatter entity has, since owner_identifier did not
// exist before Sol's Invariant 2 (2026-09-08).
struct EntitySpec {
    std::string id;
    std::string ownerIdentifier;
    std::vector<float> transform16;
    bool redFaceColors = false;
};

flatbuffers::Offset<Earthcall::Schema::Entity> buildEntity(
        flatbuffers::FlatBufferBuilder& builder, const EntitySpec& spec) {
    auto id_str = builder.CreateString(spec.id);
    auto name_str = builder.CreateString(std::string("Shape2D"));
    auto tf_vec = builder.CreateVector(spec.transform16);
    flatbuffers::Offset<flatbuffers::String> owner_str = 0;
    if (!spec.ownerIdentifier.empty()) owner_str = builder.CreateString(spec.ownerIdentifier);
    flatbuffers::Offset<flatbuffers::Vector<const Earthcall::Schema::Vec3*>> colors_vec = 0;
    if (spec.redFaceColors) {
        std::vector<Earthcall::Schema::Vec3> fbs_colors(6, Earthcall::Schema::Vec3(1.0f, 0.0f, 0.0f));
        colors_vec = builder.CreateVectorOfStructs(fbs_colors);
    }
    return Earthcall::Schema::CreateEntity(
        builder, id_str, name_str, tf_vec,
        0, 0, 0, 0,          // polyhedron, patch, smooth, field
        0,                   // face_textures
        colors_vec,          // face_colors
        0, 0,                // sdf_nodes, laws
        0,                   // material_id
        nullptr, nullptr, nullptr,
        1.0f,
        owner_str);
}

std::vector<uint8_t> buildMatterBuffer(const std::vector<EntitySpec>& specs) {
    flatbuffers::FlatBufferBuilder builder(1024);
    std::vector<flatbuffers::Offset<Earthcall::Schema::Entity>> ents;
    ents.reserve(specs.size());
    for (const auto& spec : specs) ents.push_back(buildEntity(builder, spec));
    auto chunk = Earthcall::Schema::CreateSaveChunk(
        builder, builder.CreateString("test_matter"), builder.CreateVector(ents));
    builder.Finish(chunk);
    const uint8_t* buf = builder.GetBufferPointer();
    return std::vector<uint8_t>(buf, buf + builder.GetSize());
}

// Mirrors the shape object_roundtrip_test.cpp already builds by hand — one
// ownerless Entity, only id + face_colors populated, matching what an OLD
// buildMatterFlatBuffer (before this fix) would have written for an object
// whose faceColors happened to be the legacy cube-face default at save time.
std::vector<uint8_t> buildLegacyRedMatterBuffer(const std::string& objectId) {
    return buildMatterBuffer({EntitySpec{objectId, "", translationMatrix(0, 0, 0), true}});
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

    {
        // Sol's Invariant 2, direct test: the same bare id in two different
        // Zones must each receive only their OWN physical state, addressed
        // by (owner Zone id, object id) — not whichever the old bare-id map
        // happened to visit last.
        std::printf("=== Composite address: same bare id in two Zones stays independent ===\n");
        ZoneManager m;
        auto zoneA = std::make_shared<Zone>("ZoneA", "strict");
        auto zoneB = std::make_shared<Zone>("ZoneB", "strict");
        auto objA = std::make_shared<Object>("shared-id");
        auto objB = std::make_shared<Object>("shared-id");
        zoneA->addObject(objA);
        zoneB->addObject(objB);
        m.addZone(zoneA);
        m.addZone(zoneB);

        const auto buf = buildMatterBuffer({
            EntitySpec{"shared-id", "ZoneA", translationMatrix(1, 0, 0)},
            EntitySpec{"shared-id", "ZoneB", translationMatrix(9, 0, 0)},
        });
        m.applyMatterFlatBuffer(buf);

        check(nearf(translationOf(objA->getTransform()).x, 1.0f),
              "ZoneA's 'shared-id' received its own (owner-addressed) transform");
        check(nearf(translationOf(objB->getTransform()).x, 9.0f),
              "ZoneB's 'shared-id' received its own (owner-addressed) transform, "
              "not ZoneA's — this is the exact cross-Zone collision Sol found "
              "(382 duplicate bare ids in the real basic_pixel_changer.ecmatter)");
    }

    {
        // Sol's Invariant 3, direct test: a literal duplicate composite key
        // within ONE buffer must refuse both records, not apply whichever
        // is last. This is the real basic_pixel_changer.ecmatter's actual
        // shape — two legacy (ownerless) entities for "basic-pixel-canvas".
        std::printf("=== Duplicate composite key refuses, does not last-write-win ===\n");
        ZoneManager m;
        auto zone = std::make_shared<Zone>("DupZone", "strict");
        auto obj = std::make_shared<Object>("dup-id");
        obj->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(5, 5, 5)));
        zone->addObject(obj);
        m.addZone(zone);

        const auto buf = buildMatterBuffer({
            EntitySpec{"dup-id", "DupZone", translationMatrix(1, 0, 0)},
            EntitySpec{"dup-id", "DupZone", translationMatrix(2, 0, 0)},
        });
        m.applyMatterFlatBuffer(buf);

        const auto pos = translationOf(obj->getTransform());
        check(nearf(pos.x, 5.0f) && nearf(pos.y, 5.0f) && nearf(pos.z, 5.0f),
              "a duplicate composite key in one buffer changes nothing — the object keeps "
              "its pre-load transform rather than adopting either duplicate record");
    }

    {
        // Regression (2026-09-08): the first implementation of composite
        // addressing broke test_observation_load_test — loadTestObservation
        // deliberately re-parents a dump's objects into a freshly-named
        // "test.<stem>" Zone, different from whatever Zone owned them when
        // the .ecmatter was written, so a legitimately-moved object's
        // owner_identifier can never match post-move. An owner_identifier
        // that names no live Zone holding that bare id must fall through to
        // the same unambiguous-bare-id resolution an ownerless legacy
        // record gets, not refuse outright — owner_identifier disambiguates
        // a real collision, it does not veto an otherwise-safe resolution.
        std::printf("=== Stale owner_identifier (object legitimately moved) still resolves when unambiguous ===\n");
        ZoneManager m;
        auto zone = std::make_shared<Zone>("test.renamed-zone", "strict");
        auto obj = std::make_shared<Object>("moved-id");
        zone->addObject(obj);
        m.addZone(zone);

        // owner_identifier names the Zone this object lived in AT SAVE
        // TIME ("original-zone"), which no longer exists — obj now lives in
        // "test.renamed-zone" instead, exactly as loadTestObservation does.
        const auto buf = buildMatterBuffer({
            EntitySpec{"moved-id", "original-zone", translationMatrix(6, 0, 0)},
        });
        m.applyMatterFlatBuffer(buf);

        check(nearf(translationOf(obj->getTransform()).x, 6.0f),
              "a stale owner_identifier does not block resolution when the bare id is "
              "still unambiguous among currently-live objects");
    }

    {
        // Sol's third test shape: an ambiguous ownerless legacy record must
        // refuse (matches this test's earlier single-object case, which
        // already proves the unique-ownerless-legacy path still applies).
        std::printf("=== Ambiguous ownerless legacy record refuses ===\n");
        ZoneManager m;
        auto zoneA = std::make_shared<Zone>("AmbigA", "strict");
        auto zoneB = std::make_shared<Zone>("AmbigB", "strict");
        auto objA = std::make_shared<Object>("ambiguous-id");
        auto objB = std::make_shared<Object>("ambiguous-id");
        objA->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(3, 0, 0)));
        objB->setTransform(glm::translate(glm::mat4(1.0f), glm::vec3(4, 0, 0)));
        zoneA->addObject(objA);
        zoneB->addObject(objB);
        m.addZone(zoneA);
        m.addZone(zoneB);

        // No ownerIdentifier: this is exactly what every pre-Invariant-2
        // .ecmatter record looks like.
        const auto buf = buildMatterBuffer({
            EntitySpec{"ambiguous-id", "", translationMatrix(7, 7, 7)},
        });
        m.applyMatterFlatBuffer(buf);

        const auto posA = translationOf(objA->getTransform());
        const auto posB = translationOf(objB->getTransform());
        check(nearf(posA.x, 3.0f) && nearf(posB.x, 4.0f),
              "an ownerless legacy record matching more than one live object is refused "
              "entirely — neither ZoneA's nor ZoneB's object is guessed at");
    }

    std::printf("%s (%d failure%s)\n", g_failures ? "FAIL" : "PASS", g_failures,
                g_failures == 1 ? "" : "s");
    return g_failures == 0 ? 0 : 1;
}
