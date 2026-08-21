#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Object/Formation/Formation.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include <glm/glm.hpp>

namespace OntoMath {
    class ScalarField;
    class VectorField;
}
namespace geom {
    class FieldNode;
}

class Zone : public Singular
{
public:
    enum class Scope {
        Global,
        World,
        Regional,
        Local,
        UI
    };

    using Qualities = std::unordered_map<std::string, std::string>;
    using Deletability = std::unordered_map<std::string, bool>;

    Zone(const std::string &name, const std::string& foundationSymbol, Scope scope = Scope::Local);

    Zone(const Zone&);
    Zone& operator=(const Zone&);
    Zone(Zone&&) noexcept = default;
    Zone& operator=(Zone&&) noexcept = default;

    size_t current = 0;

    // ------------------------------------------------------------
    // Formation
    Formation& getFormation() { return _formation; }
    const Formation& getFormation() const { return _formation; }
    void addToFormation(Singular* s) { _formation.addMember(s); }
    void removeFromFormation(Singular* s) { _formation.removeMember(s); }
    void addToFormation(const std::vector<Singular*>& members) { 
        for(auto* member : members) {
            _formation.addMember(member);
        }
    }
    void removeFromFormation(const std::vector<Singular*>& members) { 
        for(auto* member : members) {
            _formation.removeMember(member);
        }
    }

    virtual ~Zone();

    void describe() const;

    const std::string& name() const { return _name; }
    const Qualities& getQualities() const { return _qualities; }
    const Deletability& getDeletability() const { return _deletable; }

    std::string propName() const { return _name; }
    std::string scopeName() const;

    const std::string& owner() const { return _ownerId; }
    std::string propOwner() const { return _ownerId; }
    // Gathering Zones (local Ourverse) may not be owned. setOwner is
    // refused loudly and leaves the Zone unowned — OURVERSE.md.
    void setOwner(const std::string& personId);

    static constexpr const char* kGatheringKind = "ourverse-gathering";
    bool isOurverseGathering() const;
    void markOurverseGathering();

    // Scene — objects live on the Zone. `World` was a Singular bag around
    // this list (plus leftover Creative/Survival/Spectator) and has been
    // folded here. Spawn's womb is the Zone. Save JSON still writes the
    // list under `zones[].world.objects` so existing files load.
    void addObject(std::shared_ptr<Object> obj);
    bool removeObject(Object* obj);
    bool removeObjectById(const std::string& identifier);
    const std::vector<std::shared_ptr<Object>>& objects() const { return _objects; }
    const std::vector<std::shared_ptr<Object>>& getOwnedObjects() const { return _objects; }
    std::vector<std::shared_ptr<Object>>& getOwnedObjectsMutable() { return _objects; }
    void update(float dt = 0.016f);

    const std::string& getParentZone() const { return _parentZoneName; }
    void setParentZone(const std::string& pZone) { _parentZoneName = pZone; }

    void setScope(Scope scope) { _scope = scope; }
    Scope scope() const { return _scope; }

    void setQuality(const std::string &key, const std::string &value) { _qualities[key] = value; }
    const std::string &quality(const std::string &key) const { return _qualities.at(key); }
    const Qualities &qualities() const { return _qualities; }

    void setDeletable(const std::string &person, bool flag) { _deletable[person] = flag; }
    bool isDeletable(const std::string &person) const {
        auto it = _deletable.find(person);
        return it != _deletable.end() ? it->second : false;
    }
    const Deletability &deletability() const { return _deletable; }

private:
    void buildProperties() override;

    std::string _name;
    std::string _parentZoneName;
    Scope _scope;
    Qualities _qualities;
    Deletability _deletable;
    Formation _joys;
    std::string _ownerId;
    std::vector<std::shared_ptr<Object>> _objects;
    Formation _formation;
    
    std::shared_ptr<OntoMath::ScalarField> _spatialField;
    std::shared_ptr<OntoMath::VectorField> _spatialVectorField;
    
    std::shared_ptr<geom::FieldNode> _spatialRootObject;

public:
    Formation& formation() { return _formation; }
    const Formation& formation() const { return _formation; }
    Formation& joys() { return _joys; }
    const Formation& joys() const { return _joys; }
    bool satisfiesJoyBounds() const { return _joys.satisfiesJoyBounds(); }
    std::string propJoys() const { return _joys.getIdentifier(); }
    void load();
    void unload();
    void syncFormationMembers(const std::vector<Singular*>& extraMembers = {});
    void applyFormationRelations();

    // Singular interface
    std::string getIdentifier() const override { return _name; }
};
