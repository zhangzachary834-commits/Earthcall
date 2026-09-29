#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Formation/Formation.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include <glm/glm.hpp>
#include <map>

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
    // Display is presentation-only and MAY diverge from identity (Zach,
    // 2026-09-09: "Zone should absolutely have a real identifier/name
    // split. It's a Singular."). A Zone constructed the ordinary way keeps
    // _identifier == _name (every existing call site, unchanged); only a
    // record whose JSON "name" and "identifier" fields actually differ
    // ever calls this — see makeZoneFromJson. Never touches _identifier:
    // renaming a Zone's display must never silently re-key its identity.
    void setName(const std::string& displayName) { _name = displayName; }
    const Qualities& getQualities() const { return _qualities; }
    const Deletability& getDeletability() const { return _deletable; }

    std::string propName() const { return _name; }
    std::string propIdentifier() const { return _identifier; }
    std::string scopeName() const;

    const std::string& owner() const { return _ownerId; }
    std::string propOwner() const { return _ownerId; }
    // Owner is a being identifier: a Person, a Relationship, or a
    // Community. Gathering Zones refuse every owner. Dwelling-specific
    // locks (primary Home transfer, Community Home stakes) live on Home,
    // which overrides these.
    virtual void setOwner(const std::string& ownerId);
    virtual void setOwner(const std::string& ownerId, const std::string& ownerKind);

    static constexpr const char* kGatheringKind = "ourverse-gathering";
    static constexpr const char* kHomeKind = "home";
    static constexpr const char* kCommunityHomeKind = "community-home";
    static constexpr const char* kCommunityZoneKind = "community-zone";
    static constexpr const char* kOwnerKindPerson = "person";
    static constexpr const char* kOwnerKindRelationship = "relationship";
    static constexpr const char* kOwnerKindCommunity = "community";

    bool isOurverseGathering() const;
    void markOurverseGathering();

    // Home is a Zone whose telos is dwelling — a C++ kind, like Person, not
    // a domain noun. Qualities remain for serialization dual-read of files
    // written before the class was restored. Virtuals dispatch to Home.
    virtual bool isHome() const;
    bool isPersonalHome() const;
    bool isCommunityHome() const;
    bool isCommunityZone() const;
    virtual bool isPrimaryHome() const;
    virtual void markPrimaryHome();
    virtual void markCommunityHome();
    void markCommunityZone();

    std::string propKind() const;
    bool propPrimary() const;
    std::string propOwnerKind() const;

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
    struct UpdateTiming {
        double groundScanMs = 0.0;
        double rotationMs = 0.0;
        double automationMs = 0.0;
        double physicsMs = 0.0;
        double totalMs = 0.0;
        int substeps = 0;
    };
    void update(float dt = 0.016f, UpdateTiming* out = nullptr);
    const UpdateTiming& lastUpdateTiming() const { return _lastUpdateTiming; }

    const std::string& getParentZone() const { return _parentZoneName; }
    void setParentZone(const std::string& pZone) { _parentZoneName = pZone; }

    // ------------------------------------------------------------
    // Zones as mathematical bounds (docs/plans/ZONES_AS_MATHEMATICAL_BOUNDS_PLAN_2026-09-23.md).
    // Zach, 2026-09-23: a Zone is not "one 3D world here, another there" but
    // a bound in a continuum, and the continuum is itself a Zone — "a
    // Dimensional Zone which basically represents an entire continuum of a
    // dimension upon which values could be had (like 1D, 2D, 3D space)."
    //
    // All of it is authored data on the Zone, not C++ kinds:
    //   within             the containing Zone (the historical parentZone)
    //   dimension.<axis>   text: the property path a being's coordinate on
    //                      that axis is read from ("position.x"). Any axis ⇒
    //                      this Zone is Dimensional. No enum of dimensions.
    //   placement.<axis>   number: this Zone's frame origin in its parent's
    //                      frame (translation; rotation/scale are Rung 5)
    //   extent             ScalarField (OntoMath Piecewise over axis names)
    //   extent.lo / .hi    optional numbers. Inside ⇔ f defined ∧ lo ≤ f ≤ hi,
    //                      absent side open — ConditionNode::Kind::Zone's own
    //                      semantics. Signed-distance extents set hi = 0.
    // Where a being IS is derived from these (ZoneManager::locate), never
    // stored: ownership stays a Relation and residence stays the store.
    static constexpr const char* kDimensionPrefix = "dimension.";
    static constexpr const char* kPlacementPrefix = "placement.";
    static constexpr const char* kExtentLo = "extent.lo";
    static constexpr const char* kExtentHi = "extent.hi";

    // Axis name -> property path, sorted by axis name.
    std::vector<std::pair<std::string, std::string>> dimensionAxes() const;
    bool isDimensional() const { return !dimensionAxes().empty(); }
    // Placement along an axis of the parent's frame; 0 when unauthored.
    double placementAlong(const std::string& axis) const;
    const std::shared_ptr<OntoMath::ScalarField>& extent() const { return _extent; }
    void setExtent(std::shared_ptr<OntoMath::ScalarField> field) { _extent = std::move(field); }
    // Does the authored extent hold at `coords` (axis -> value, this Zone's
    // own frame)? No extent = unbounded = true. Undefined math = false.
    bool extentHolds(const std::map<std::string, PropertyValue>& coords) const;

    void setScope(Scope scope) { _scope = scope; }
    Scope scope() const { return _scope; }

    virtual void setQuality(const std::string &key, const std::string &value);
    const std::string &quality(const std::string &key) const { return _qualities.at(key); }
    const Qualities &qualities() const { return _qualities; }

    virtual void setDeletable(const std::string &person, bool flag);
    virtual bool isDeletable(const std::string &person) const;
    const Deletability &deletability() const { return _deletable; }

    // Every Zone already owns one continuous mathematical FieldNode and keeps
    // it in the Zone Formation. It is world substrate, not an Object wrapper.
    // Expose the existing being so persistence and rendering bridges can read
    // its authored state without inventing a domain-specific C++ Light class.
    geom::FieldNode* spatialRoot() { return _spatialRootObject.get(); }
    const geom::FieldNode* spatialRoot() const { return _spatialRootObject.get(); }

    // Rung 7 world composition. The historical spatialRoot remains the Zone's
    // canonical field and the exact one-source compatibility source. Additional
    // FieldNodes are ordinary authored beings owned by the Zone, admitted to its
    // Formation, persisted, and Law-reachable. Rendering can therefore discover
    // potential radiant sources from this tiny direct index rather than scanning
    // every Object in the world.
    const std::vector<std::shared_ptr<geom::FieldNode>>& additionalSpatialFields() const {
        return _additionalSpatialFields;
    }
    void addSpatialField(std::shared_ptr<geom::FieldNode> field);
    void clearAdditionalSpatialFields();

protected:
    void buildProperties() override;

private:
    std::string _name;
    // Stable identity, distinct from _name (display). Always initialized
    // equal to the constructor's `name` argument — every pre-existing call
    // site is unaffected — and diverges only when a JSON record's own
    // "identifier"/"name" fields differ (makeZoneFromJson via setName()).
    std::string _identifier;
    std::string _parentZoneName;
    std::shared_ptr<OntoMath::ScalarField> _extent;
    std::string propWithin() const { return _parentZoneName; }
    void propSetWithin(const std::string& id) { _parentZoneName = id; }
    bool propDimensional() const { return isDimensional(); }
    // The bound fields above live partly in dynamic properties, which
    // Zone's hand-written copy does not inherit (Singular base is
    // default-constructed there). Copied explicitly.
    void copyBoundsFrom(const Zone& other);
    Scope _scope;
    Qualities _qualities;
    Deletability _deletable;
    Formation _joys;
    std::string _ownerId;
    std::vector<std::shared_ptr<Object>> _objects;
    float _accumulator = 0.0f;
    // Kernel performance instrumentation — sub-phase tick telemetry beneath the Law system.
    UpdateTiming _lastUpdateTiming;
    Formation _formation;
    
    std::shared_ptr<OntoMath::ScalarField> _spatialField;
    std::shared_ptr<OntoMath::VectorField> _spatialVectorField;
    
    std::shared_ptr<geom::FieldNode> _spatialRootObject;
    std::vector<std::shared_ptr<geom::FieldNode>> _additionalSpatialFields;

public:
    Formation& formation() { return _formation; }
    const Formation& formation() const { return _formation; }
    Formation& joys() { return _joys; }
    const Formation& joys() const { return _joys; }
    bool satisfiesJoyBounds() const { return _joys.satisfiesJoyBounds(); }
    std::string propJoys() const { return _joys.getIdentifier(); }
    virtual void load();
    void unload();
    void syncFormationMembers(const std::vector<Singular*>& extraMembers = {});
    void applyFormationRelations();

    // Singular interface. Identity, not display — see _identifier's comment.
    std::string getIdentifier() const override { return _identifier; }
};
