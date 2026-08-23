#pragma once
#include "../Zone/Zone.hpp"
#include <string>
#include <vector>

// A Home is a Zone whose telos is dwelling.
//
// Constitutive — named in Singularity::Earthcall beside Person and Zone —
// not a domain noun. Community Home is still this class: shared stakes are
// the Community that owns it, not a CommunityHome type.
//
// Memory: Home carries dwelling state Zone does not have. sizeof(Home) is
// not sizeof(Zone). That state is also a separate identity file
// (saves/homes/<id>/home.json), not a Zone snapshot.
//
//   Ownership  — _primary is a kernel bit; a claimed primary Home cannot
//                be transferred. Extra Homes are still Homes.
//   Stakes     — _stakes / _stakeIds: who holds a share in this dwelling
//                (Person, Relationship, Community).
//   Governance — _entryRequiresWill, _cannotForceStay: kernel, read-only.
//                Inhabitants are recorded here, not on Zone.
class Home : public Zone {
public:
    Home(const std::string& name, const std::string& foundationSymbol,
         Scope scope = Scope::Local);

    bool isHome() const override { return true; }
    bool isPrimaryHome() const override;
    void markPrimaryHome() override;
    void markCommunityHome() override;

    void setOwner(const std::string& ownerId) override;
    void setOwner(const std::string& ownerId, const std::string& ownerKind) override;
    void setQuality(const std::string& key, const std::string& value) override;
    void setDeletable(const std::string& person, bool flag) override;
    bool isDeletable(const std::string& person) const override;

    bool entryRequiresWill() const { return _entryRequiresWill; }
    bool cannotForceStay() const { return _cannotForceStay; }
    bool propEntryRequiresWill() const { return _entryRequiresWill; }
    bool propCannotForceStay() const { return _cannotForceStay; }

    Formation& stakes() { return _stakes; }
    const Formation& stakes() const { return _stakes; }
    std::string propStakes() const { return _stakes.getIdentifier(); }
    int propStakeCount() const { return static_cast<int>(_stakeIds.size()); }
    const std::vector<std::string>& stakeIds() const { return _stakeIds; }
    bool hasStake(const std::string& beingId) const;
    void addStake(const std::string& beingId);
    void loadStakeIds(std::vector<std::string> ids);

    const std::vector<std::string>& inhabitantIds() const { return _inhabitants; }
    int propInhabitantCount() const { return static_cast<int>(_inhabitants.size()); }
    // Owner is always admitted. Anyone else needs will — manifesto:
    // nobody forces themselves into another's Home apart from will.
    bool admitInhabitant(const std::string& personId, bool will);
    // The inhabitant may always leave. Others cannot expel them while
    // cannotForceStay holds.
    bool releaseInhabitant(const std::string& personId, bool bySelf);
    void loadInhabitantIds(std::vector<std::string> ids);

    void welcome() const;

protected:
    void buildProperties() override;

private:
    bool _primary = false;
    bool _entryRequiresWill = true;
    bool _cannotForceStay = true;
    Formation _stakes;
    std::vector<std::string> _stakeIds;
    std::vector<std::string> _inhabitants;
};
