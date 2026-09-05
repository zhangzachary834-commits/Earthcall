// Phase 2: an Ourverse root is saved as a semantic record and hydrated after
// its Zone/Formations endpoints exist. No Zone owns the root graph.

#include "Singularity/Storage/Serialization/ZonesOfEarth/OurverseSerialization.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <cassert>
#include <cstdio>
#include <memory>

int main() {
    ZoneManager sourceZones;
    Ourverse source;
    sourceZones.addZone(std::make_shared<Zone>("Sanctum", "strict"));
    sourceZones.addZone(std::make_shared<Zone>("Temple", "strict"));
    Zone& gathering = source.ensureGatheringZone(sourceZones);
    Zone& sanctum = *sourceZones.zones()[0];
    Zone& temple = *sourceZones.zones()[1];
    assert(source.weave(sanctum, temple));
    source.loadConvenesToward("shared-telos");

    Core::Camera camera;
    MouseHandler mouse;
    float currentColor[3] = {1.0f, 1.0f, 1.0f};
    Person person(Soul("Phase 2"), Body("humanoid", "default"), "default");
    LawManager laws;
    double worldTime = 42.0;
    SaveContext saveContext;
    saveContext.camera = &camera;
    saveContext.mouseHandler = &mouse;
    saveContext.currentColor = currentColor;
    saveContext.person = &person;
    saveContext.lawManager = &laws;
    saveContext.ourverse = &source;
    saveContext.worldTime = &worldTime;
    const nlohmann::json session = sourceZones.buildSaveJson(saveContext);
    assert(session.contains("ourverse"));
    assert(session.contains("person"));
    assert(session["person"]["displayName"] == person.getDisplayName());
    assert(session["ourverse"]["gatheringZone"] == gathering.getIdentifier());

    const nlohmann::json saved = ourverseToJson(source);
    assert(saved.value("identifier", std::string{}) == "Ourverse");
    assert(saved["gatheringZone"] == gathering.getIdentifier());
    assert(saved["filaments"].value("identifier", std::string{}) == "ourverse.filaments");
    assert(saved["filaments"]["relations"].size() == 1);

    ZoneManager restoredZones;
    restoredZones.addZone(std::make_shared<Zone>("Sanctum", "strict"));
    restoredZones.addZone(std::make_shared<Zone>("Temple", "strict"));
    auto restoredGathering = std::make_shared<Zone>("Ourverse Gathering", "strict");
    restoredGathering->markOurverseGathering();
    restoredZones.addZone(restoredGathering);

    Ourverse restored;
    auto resolveZone = [&restoredZones](const std::string& id) -> std::shared_ptr<Zone> {
        for (const auto& zone : restoredZones.zones()) {
            if (zone && zone->getIdentifier() == id) return zone;
        }
        return nullptr;
    };
    auto resolveMember = [&restoredZones](const std::string& id) -> Singular* {
        for (const auto& zone : restoredZones.zones()) {
            if (zone && zone->getIdentifier() == id) return zone.get();
        }
        auto& language = Singularity::Language::LanguageSystem::instance();
        if (auto lexeme = language.findById(id)) return lexeme.get();
        if (auto lexeme = language.findBySymbol(id)) return lexeme.get();
        return nullptr;
    };

    assert(ourverseFromJson(restored, saved, resolveZone, resolveMember));
    assert(restored.gatheringZone() == restoredGathering.get());
    assert(restored.joys().satisfiesJoyBounds());
    assert(restored.filaments().getIdentifier() == "ourverse.filaments");
    assert(restored.filaments().getRelationTypeTag() == "filaments");
    assert(restored.propFilamentCount() == 1);
    assert(restored.convenesToward() == "shared-telos");

    std::puts("ourverse_serialization_test: ALL OK");
    return 0;
}
