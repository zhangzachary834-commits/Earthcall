// Live failure, Zone identities contain no 3D placement (Sol/Codex, agent
// intercom "Basic Pixel Changer Zone Identity Bug 9-7-26", 2026-09-09
// 18:11 PDT; full writeup in
// docs/Agenda/Tasks/Specific Tasks/Per_Zone_serialization_pathway/
// Per_Zone_serialization_pathway.md, "Live failure" section).
//
// Zach reported Sanctum of Beginnings booting with every shape stuffed into
// one place, and Synthesis Studio's floor/desk collapsing into one short
// prism at the origin. Root cause, confirmed by reading the code directly:
// commit 946a6240 ("Substrate Split Serialization", 2026-09-01) removed
// `transform`, `center`, `authoritativeAxis`, `targetRotation`, and
// `rotationResponsiveness` from Object::to_json on the theory that these
// were "purely physical" density belonging only to the conglomerate
// .ecmatter sidecar — but Object::from_json never stopped reading them, and
// per-Zone identities (saves/zones/<id>/zone.json) have no matter
// generation of their own at all. Every Object loaded through a bare Zone
// identity — which is most of them, since only a full World load ever
// consults a `.ecmatter` — silently lost its placement to the identity
// transform (translation zero, no rotation, unit scale).
//
// Placement is Person-meaningful and Law-addressable; it is not allowed to
// have zero semantic writer. This test proves the fix two ways: a direct
// Object JSON round-trip, and the actual failing shape — a Zone identity
// hydrated with NO world file and NO matter sidecar at all (the real boot
// path for every Zone that hasn't been Move-to-Zone'd through a
// conglomerate World this session).

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "json.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

bool nearf(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) < eps; }
bool nearVec(const glm::vec3& a, const glm::vec3& b, float eps = 1e-3f) {
    return nearf(a.x, b.x, eps) && nearf(a.y, b.y, eps) && nearf(a.z, b.z, eps);
}
bool nearMat(const glm::mat4& a, const glm::mat4& b, float eps = 1e-3f) {
    const float* pa = &a[0][0];
    const float* pb = &b[0][0];
    for (int i = 0; i < 16; ++i) if (!nearf(pa[i], pb[i], eps)) return false;
    return true;
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running Object semantic pose (placement must have a semantic writer)...\n";
    std::cout << "============================================================\n";

    // ---- Direct round-trip: to_json must write what from_json reads. ----
    {
        Object obj;
        obj.setShape(Object::ShapeKind::Cube);
        obj.setObjectID("pose-test");
        const glm::mat4 transform =
            glm::scale(
                glm::rotate(
                    glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 10.0f, -3.0f)),
                    glm::radians(37.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                glm::vec3(2.0f, 0.5f, 3.0f));
        obj.setTransform(transform);
        obj.setCenter(glm::vec3(0.25f, -0.5f, 1.5f));
        obj.setAuthoritativeAxis(glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
        obj.setTargetRotationEulerDegrees(glm::vec3(10.0f, 20.0f, 30.0f));
        obj.setRotationResponsiveness(4.5f);

        nlohmann::json j;
        to_json(j, obj);

        check(j.contains("transform") && j["transform"].is_array() && j["transform"].size() == 16,
              "to_json writes a 16-element transform array");
        check(j.contains("center") && j.contains("authoritativeAxis") &&
              j.contains("targetRotation") && j.contains("rotationResponsiveness"),
              "to_json writes center, authoritativeAxis, targetRotation, and rotationResponsiveness");

        Object roundTripped;
        from_json(j, roundTripped);
        check(nearMat(roundTripped.getTransform(), transform),
              "transform round-trips through to_json/from_json exactly");
        check(nearVec(roundTripped.getCenter(), obj.getCenter()), "center round-trips");
        check(nearVec(roundTripped.getAuthoritativeAxis(), obj.getAuthoritativeAxis()),
              "authoritativeAxis round-trips");
        check(nearVec(roundTripped.getTargetRotationEulerDegrees(), obj.getTargetRotationEulerDegrees()),
              "targetRotation round-trips");
        check(nearf(roundTripped.getRotationResponsiveness(), obj.getRotationResponsiveness()),
              "rotationResponsiveness round-trips");
    }

    // ---- The actual failing shape: a Zone identity, no World, no matter. ----
    // ---- This is the real boot path for a Zone hydrated straight from   ----
    // ---- saves/zones/<id>/ — exactly what Sanctum/Synthesis Studio/Chess ----
    // ---- go through, and exactly where Zach saw everything collapse to  ----
    // ---- the origin.                                                    ----
    {
        auto sandbox = std::filesystem::temp_directory_path() / "earthcall_object_semantic_pose";
        std::filesystem::remove_all(sandbox);
        std::filesystem::create_directories(sandbox);
        SaveSystem::setSaveRoot(sandbox.string());

        const glm::vec3 floorPosition(0.0f, -0.1f, 0.0f);
        const glm::vec3 floorScale(14.0f, 0.2f, 14.0f);

        {
            ZoneManager writer;
            auto zone = std::make_shared<Zone>("PoseZone", "strict");
            auto floor = std::make_shared<Object>();
            floor->setShape(Object::ShapeKind::Cube);
            floor->setObjectID("studio.platform.floor");
            floor->setTransform(glm::scale(glm::translate(glm::mat4(1.0f), floorPosition), floorScale));
            zone->addObject(floor);
            writer.addZone(zone);
            writer.persistZones(); // writes ONLY saves/zones/PoseZone/zone.json — no World, no .ecmatter
        }

        check(SaveSystem::zoneIdentityExists("PoseZone"),
              "the Zone identity was written with no World and no matter sidecar involved");
        const auto matterPath = sandbox / "zones" / "PoseZone" / "PoseZone.ecmatter";
        check(!std::filesystem::exists(matterPath),
              "control: confirms there really is no matter sidecar for this Zone to fall back on");

        ZoneManager reader;
        reader.hydrateFromZoneStore(); // the real boot path — no loadState, no World, no .ecmatter

        Object* hydratedFloor = nullptr;
        for (const auto& z : reader.zones()) {
            if (!z || z->getIdentifier() != "PoseZone") continue;
            for (const auto& o : z->getOwnedObjects()) {
                if (o && o->getObjectID() == "studio.platform.floor") hydratedFloor = o.get();
            }
        }
        check(hydratedFloor != nullptr, "the Zone-identity-only boot admits the floor object");
        if (hydratedFloor) {
            const glm::vec3 gotPosition(hydratedFloor->getTransform()[3]);
            const glm::vec3 gotScale(
                glm::length(glm::vec3(hydratedFloor->getTransform()[0])),
                glm::length(glm::vec3(hydratedFloor->getTransform()[1])),
                glm::length(glm::vec3(hydratedFloor->getTransform()[2])));
            check(nearVec(gotPosition, floorPosition),
                  "the floor keeps its authored position (0, -0.1, 0) from Zone identity alone — "
                  "was silently collapsing to the origin before this fix");
            check(nearVec(gotScale, floorScale),
                  "the floor keeps its authored scale (14 x 0.2 x 14) from Zone identity alone — "
                  "was silently becoming a unit cube before this fix");
        }

        std::filesystem::remove_all(sandbox);
        SaveSystem::setSaveRoot("");
    }

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "object_semantic_pose_test: FAILED\n";
        return 1;
    }
    std::cout << "object_semantic_pose_test: ALL OK\n";
    return 0;
}
