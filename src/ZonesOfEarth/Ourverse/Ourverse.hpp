#pragma once
#include <vector>
#include <string>
#include <memory>
#include <ctime>

#include "../Zone/Zone.hpp"
#include "Relation/Relation.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Formation/Formation.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"

class ZoneManager;
class Community;
class LawManager;

// Ourverse is the vessel of unity in Christ — an ordering principle that
// channels Zones toward shared Joys. It is a Singular, not a Zone.
// See docs/architecture/ourverse/OURVERSE.md.
//
// The Engine still parks a leftover object list and camera here (Game bag).
// That list is not the Ourverse's meaning and is not registered.
class Ourverse : public Singular {
public:
    Ourverse();

    static constexpr const char* kFilamentType = "filament";
    static constexpr const char* kGathersType  = "gathers";
    static constexpr const char* kHostsType    = "hosts";
    static constexpr const char* kConvenesType = "convenes-toward";

    void display() const;
    void renderModeUI();

    void updateObjectCollisions(glm::vec3& position, const Object& obj, const glm::mat4& transform) const;
    void onUpdate(float deltaTime = 0.016f);

    void setCamera(glm::vec3* cam) { cameraPos = cam; }
    glm::vec3* getCamera() const { return cameraPos; }

    void addOwnedObject(std::shared_ptr<Object> obj) { ownedObjects.push_back(std::move(obj)); }
    const std::vector<std::shared_ptr<Object>>& getOwnedObjects() const { return ownedObjects; }
    std::vector<std::shared_ptr<Object>>& getOwnedObjectsMutable() { return ownedObjects; }
    void clearDynamicObjects();

    std::string getIdentifier() const override { return "Ourverse"; }

    // --- liturgical surface ---
    Zone* gatheringZone() const { return _gatheringZone.get(); }
    std::shared_ptr<Zone> getPrimaryGatheringZone() const { return _gatheringZone; }
    void setPrimaryGatheringZone(std::shared_ptr<Zone> zone);

    Formation& joys() { return _joys; }
    const Formation& joys() const { return _joys; }
    Formation& filaments() { return _filaments; }
    const Formation& filaments() const { return _filaments; }
    Formation& getLaws() { return _metalaws; }
    const Formation& getLaws() const { return _metalaws; }

    const std::string& convenesToward() const { return _convenesToward; }

    // Mint (or reclaim) the unowned gathering Zone. All may participate;
    // no one owns it.
    Zone& ensureGatheringZone(ZoneManager& zones);

    // Relate the gathering Zone to a Community (equal standing, not ownership).
    bool ensureCommunityGathering(Community& community);

    // Undirected filament between two Zones. Directed edges are refused —
    // interweaving is mutual; a one-way filament would seat one Zone over
    // another.
    bool weave(Zone& a, Zone& b);

    bool mayWeave(const Zone& a, const Zone& b) const;

    // First-mover metalaws onto this Ourverse's law Formation / LawManager.
    void registerMetalaws(LawManager& laws);

    std::string propGatheringZone() const;
    std::string propJoys() const { return _joys.getIdentifier(); }
    int propFilamentCount() const;
    std::string propMetalaws() const { return _metalaws.getIdentifier(); }
    std::string propConvenesToward() const { return _convenesToward; }

private:
    void buildProperties() override;

    // BENEATH THE ONTOLOGY (Engine-bag debt, not Ourverse's meaning):
    // leftover Game world list and camera the render/physics loop still
    // reads. Named so they are not mistaken for "what Ourverse is."
    // Retire with World → Zone (near-term 6).
    glm::vec3* cameraPos = nullptr;
    std::vector<std::shared_ptr<Object>> ownedObjects;

    std::shared_ptr<Zone> _gatheringZone;
    Formation _joys;
    Formation _filaments;
    Formation _metalaws;
    std::string _convenesToward;
};

struct InteractionEvent {
    std::string description;
    std::time_t timestamp;
    Object* other;
};
