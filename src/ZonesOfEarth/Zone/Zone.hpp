#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "../World/World.hpp"
#include "ConstructedBeing/Object/Formation/Formation.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include <glm/glm.hpp>

class World; // forward decl
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

    Zone(const std::string &name, const std::string& joyOrdering, Scope scope = Scope::Local);

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
    void setOwner(const std::string& personId) {
        _ownerId = personId;
        if (!personId.empty()) _deletable[personId] = true;
    }

    World& world() { return *_world; }
    const World& world() const { return *_world; }

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
    std::string _joyOrdering;
    std::string _ownerId;
    std::unique_ptr<World> _world;
    Formation _formation;
    
    std::shared_ptr<OntoMath::ScalarField> _spatialField;
    std::shared_ptr<OntoMath::VectorField> _spatialVectorField;
    
    std::shared_ptr<geom::FieldNode> _spatialRootObject;

public:
    Formation& formation() { return _formation; }
    const Formation& formation() const { return _formation; }
    void load();
    void unload();
    void syncFormationMembers(const std::vector<Singular*>& extraMembers = {});
    void applyFormationRelations();

    // Singular interface
    std::string getIdentifier() const override { return _name; }
};
