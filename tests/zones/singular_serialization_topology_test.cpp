// Cross-root proof for the Singular serialization topology. A session is a
// graph of semantic roots, not a Zone-owned object bag.

#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/SessionSemanticRoots.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/HomesOfEarth/Home.hpp"
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>

namespace {

Zone* findZone(ZoneManager& zones, const std::string& id) {
    for (const auto& zone : zones.zones()) {
        if (zone && zone->getIdentifier() == id) return zone.get();
    }
    return nullptr;
}

SaveContext makeContext(Person& person,
                        Core::Camera& camera,
                        MouseHandler& mouse,
                        LawManager& laws,
                        Ourverse& ourverse,
                        float (&color)[3],
                        double& worldTime) {
    SaveContext context;
    context.camera = &camera;
    context.mouseHandler = &mouse;
    context.currentColor = color;
    context.person = &person;
    context.lawManager = &laws;
    context.ourverse = &ourverse;
    context.worldTime = &worldTime;
    return context;
}

} // namespace

int main() {
    const auto sandbox = std::filesystem::temp_directory_path()
        / "earthcall_singular_serialization_topology";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());
    const auto sessionPath = sandbox / "worlds" / "topology.ecform";

    Person sourcePerson(Soul("Topology Witness"), Body("humanoid", "default"), "default");
    sourcePerson.setDisplayName("Topology Witness");
    sourcePerson.getBody().height = 2.25f;
    Core::Camera sourceCamera;
    sourceCamera.pos = {3.0f, 6.0f, 9.0f};
    MouseHandler sourceMouse;
    LawManager sourceLaws;
    Ourverse sourceOurverse;
    float sourceColor[3] = {0.2f, 0.4f, 0.8f};
    double sourceWorldTime = 77.0;
    ZoneManager sourceZones;

    auto home = std::make_shared<Home>("Witness Home", "default");
    home->setOwner(sourcePerson.getIdentifier());
    auto garden = std::make_shared<Zone>("Witness Garden", "default");
    sourceZones.addZone(home);
    sourceZones.addZone(garden);
    Zone& gathering = sourceOurverse.ensureGatheringZone(sourceZones);
    assert(sourceOurverse.weave(*home, *garden));
    sourceOurverse.loadConvenesToward("shared-witness");

    SaveContext sourceContext = makeContext(sourcePerson, sourceCamera, sourceMouse,
                                            sourceLaws, sourceOurverse, sourceColor,
                                            sourceWorldTime);
    sourceZones.saveState(sessionPath.string(), sourceContext);

    {
        std::ifstream in(sessionPath);
        nlohmann::json saved;
        in >> saved;
        assert(saved.contains(kSemanticRootsKey));
        const auto& roots = saved[kSemanticRootsKey];
        assert(roots["format"] == kSemanticRootsFormat);
        assert(roots["version"] == kSemanticRootsVersion);
        assert(roots.contains("person"));
        assert(roots.contains("ourverse"));
        assert(!saved.contains("person"));
        assert(!saved.contains("ourverse"));
        assert(!saved.contains("playerBody"));
        assert(roots["ourverse"]["gatheringZone"] == gathering.getIdentifier());

        // The envelope, not its legacy Zone projection, must be sufficient to
        // hydrate this session. Keep the source file only in the new shape.
        saved.erase("zones");
        saved.erase("zoneRefs");
        std::ofstream out(sessionPath);
        out << saved.dump(2);
    }

    Person restoredPerson(Soul("Temporary"), Body("humanoid", "default"), "default");
    restoredPerson.getBody().height = 1.0f;
    Core::Camera restoredCamera;
    MouseHandler restoredMouse;
    LawManager restoredLaws;
    Ourverse restoredOurverse;
    float restoredColor[3] = {1.0f, 1.0f, 1.0f};
    double restoredWorldTime = 0.0;
    ZoneManager restoredZones;
    SaveContext restoredContext = makeContext(restoredPerson, restoredCamera, restoredMouse,
                                              restoredLaws, restoredOurverse, restoredColor,
                                              restoredWorldTime);
    restoredZones.loadState(sessionPath.string(), restoredContext);

    Zone* restoredHomeZone = findZone(restoredZones, "Witness Home");
    Zone* restoredGarden = findZone(restoredZones, "Witness Garden");
    Zone* restoredGathering = findZone(restoredZones, "Ourverse Gathering");
    assert(restoredHomeZone && restoredHomeZone->isHome());
    assert(restoredGarden);
    assert(restoredGathering && restoredGathering->isOurverseGathering());
    assert(restoredHomeZone->owner() == restoredPerson.getIdentifier());
    assert(std::fabs(restoredPerson.getBody().height - 2.25f) < 1e-3f);
    assert(restoredOurverse.gatheringZone() == restoredGathering);
    assert(restoredOurverse.convenesToward() == "shared-witness");
    assert(restoredOurverse.filaments().getIdentifier() == "ourverse.filaments");
    assert(restoredOurverse.filaments().getRelationTypeTag() == "filaments");
    const auto& filaments = restoredOurverse.filaments().relations().getAll();
    assert(filaments.size() == 1);
    assert(filaments.front()->hasEndpoints());
    assert(filaments.front()->type == Ourverse::kFilamentType);
    assert(filaments.front()->isBetween(*restoredHomeZone, *restoredGarden));

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");
    std::puts("singular_serialization_topology_test: ALL OK");
    return 0;
}
