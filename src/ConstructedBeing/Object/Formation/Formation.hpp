#pragma once

#include <vector>
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "Relation/RelationManager.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"


// A Formation is how a *set* of beings, and how a *category* of beings, are the
// same mechanism seen from two sides:
//
//   "A Formation without a root is a set; a Formation with one is a category."
//                                    — AUTHORED_CATEGORIES.md §3
//
// The root is the being the Formation is ABOUT — the authored Object that holds
// the kind's Material and its shared properties. Membership and taxonomy are
// carried as Relations (`instance-of`, `subcategory-of`), which are DIRECTED
// (§2), so nothing here may treat a directed edge as an absence of structure.
class Formation : public Singular {

public:
    // Bounds are doctrine, not limits. The same 32 that bounds OntoMath call
    // depth bounds Formation's recursive traversals; if a design needs it
    // raised, the design is in the wrong shape (ALGORITHMS_AS_LAW.md §3).
    static constexpr int kMaxFormationDepth = 32;

    // The outcome of resolveTopology(). Resolution NEVER dissolves a Formation
    // as a silent side effect: when no valid core is found, NOTHING is applied,
    // `applied` is false, and every member is listed in `orphaned` for the
    // caller to act on. An empty result is reported, not enacted.
    struct Topology {
        bool applied = false;                             // membership was rewritten
        bool rooted  = false;                             // the root's component grounded it
        std::vector<std::shared_ptr<Formation>> spawned;  // disconnected valid cores
        std::vector<Singular*> orphaned;                  // members held by no valid core
    };

    // How fromJson turns a saved identifier back into a being. Formation holds
    // NON-OWNING pointers and has no registry of its own, so deserialization
    // cannot invent members: the caller supplies the world they live in.
    using MemberResolver = std::function<Singular*(const std::string& identifier)>;

    // Constructor
    Formation() = default;
    Formation(const std::vector<Singular*>& members);

    // Some Formations are purely self-referencing Relations (i.e. formations of 2 singulars and the relation between singulars and the relation of relations), which could themselves be modeled as Relations between singulars and relations.

    // Every Formation is a discrete being: identity is unique per instance,
    // and a COPY mints a fresh identity — a copied formation is a new being,
    // not an alias (the SdfNode deep-copy rule applied to identity). That rule
    // governs the CONTENTS too: `subformations` are owned handles, so they are
    // deep-copied. A shallow copy left two "distinct" beings sharing the same
    // subformation objects, and a merge in one erased subformations the other
    // still held — an alias wearing a fresh name. Keep the member lists below
    // in sync with the data members.
    Formation(const Formation& o)
        : Singular(o), members(o.members), relationMgr(o.relationMgr),
          subformations(cloneSubformations(o.subformations, 0)),
          relationTypeTag(o.relationTypeTag), _root(o._root) {}
    Formation& operator=(const Formation& o) {
        if (this != &o) {
            Singular::operator=(o);
            members = o.members;
            relationMgr = o.relationMgr;
            subformations = cloneSubformations(o.subformations, 0);
            relationTypeTag = o.relationTypeTag;
            _root = o._root;
            // _formationId intentionally kept: assignment replaces content,
            // not identity.
        }
        return *this;
    }

    // Destructor
    ~Formation() = default;

    // ------ Generic membership helpers (any Singular) ------------
    virtual void addMember(Singular* s);
    bool hasMember(const Singular* s) const;
    Singular* findMemberByIdentifier(const std::string& identifier) const;

    // Release a being from this Formation AND from every subformation beneath
    // it, and drop it as root if it was one. This is the removal path a caller
    // must use when the being is about to be destroyed: Formations hold raw,
    // non-owning `Singular*`, so a member left behind is a dangling pointer
    // that findMemberByIdentifier dereferences on the next sweep.
    // Returns true if the being was found anywhere.
    bool releaseMember(Singular* s);
    void removeMember(Singular* s);   // alias for releaseMember, result discarded
    void clearMembers();
    void clearRelations();
    void clear();

    // ------ The root: what makes this Formation a category ---------------
    // Refuses a null root, and refuses the self-ground (a Formation rooted in
    // itself is the infinite regress the First Mover principle exists to
    // forbid — AUTHORED_CATEGORIES.md §7). Returns false on refusal, loudly.
    bool setRoot(Singular* s);
    void clearRoot() { _root = nullptr; }
    Singular* root() const { return _root; }
    bool hasRoot() const { return _root != nullptr; }
    bool isRoot(const Singular* s) const { return s && s == _root; }

    // Since members are Singulars*, that means a Relation can be part of a Formation.
    const std::vector<Singular*>& getMembers() const { return members; }

    // If we want to get the "hard" members, we use this.
    // Need to change the code to reflect this.
    const std::vector<Singular*>& getNonRelationMembers() const { return members; }

    // -----------------------------------------------------------------
    // Relation management helpers
    // -----------------------------------------------------------------
    RelationManager& relations() { return relationMgr; }
    const RelationManager& relations() const { return relationMgr; }

    // Admit a relation. Returns false — loudly — when the relation is refused:
    //   * a self-loop (entityA == entityB): a being grounded in itself;
    //   * a directed edge that closes a directed cycle among relations OF THE
    //     SAME TYPE. That is §7's `mayAddSubcategoryEdge` reachability check,
    //     generalized to every directed type. The cycle is refused, never
    //     "broken" by dropping some other edge — that would silently discard
    //     an authorship no one asked to revoke.
    // Undirected relations are unaffected: a complete graph of mutual bonds is
    // a set, not a regress.
    bool addRelation(const std::shared_ptr<Relation>& r);
    bool removeRelation(const std::shared_ptr<Relation>& r);

    // Add a relation directly (alias for addRelation)
    bool add(const std::shared_ptr<Relation>& r) { return addRelation(r); }

    // Would addRelation admit this relation? Same answer, no side effects and
    // no complaint printed — for callers that want to ask before they author.
    bool mayAdmitRelation(const std::shared_ptr<Relation>& r) const;

    // Build a simple fully-connected graph between all objects currently
    // in this formation (undirected, weight 1.0, type="member")
    void rebuildCompleteGraph();
    void applyAttachmentRelations();
    const std::vector<std::shared_ptr<Formation>>& getSubformations() const { return subformations; }
    std::string getRelationTypeTag() const { return relationTypeTag; }
    void setRelationTypeTag(const std::string& type) { relationTypeTag = type; }

    // Render the formation and its constituent objects
    void draw() const;

    // (De)Serialization Helpers (Logos Phase 2)
    // NOTE: no save file persists a whole Formation today (ZoneManager writes
    // `formation().relations().toJson()` only), so the on-disk format is not
    // touched by anything here.
    nlohmann::json toJson() const;
    // Rebuilds a Formation from `toJson` output. Members are looked up through
    // `resolve` — WITHOUT a resolver this refuses loudly and returns nullptr
    // rather than handing back a plausible-looking empty Formation.
    static std::shared_ptr<Formation> fromJson(const nlohmann::json& json,
                                               const MemberResolver& resolve = {});


    // Add methods to manipulate formations, such as adding or removing elements,
    // checking relationships, etc.

    // Add a Singular element to the formation
    void addElement(const Singular& s);

    // Remove a Singular element from the formation
    void removeElement(const Singular& s);

    // Other methods can be added as needed for functionality

    // Resolve the internal topology into cores and the beings held by them.
    //
    // Connectivity is DIRECTION-BLIND — a directed `subcategory-of` edge holds a
    // category together exactly as an undirected bond holds a set together, and
    // building adjacency from undirected relations alone dissolved every
    // correctly-authored category (AUTHORED_CATEGORIES.md §2/§8 make both
    // category edges directed). Direction is meaning, not connection.
    //
    // A component is a valid core when it contains the root (a rooted Formation
    // is grounded by its root, at any size) or when it holds >= 3 participants.
    // If no component qualifies, NOTHING is applied — see Topology.
    Topology resolveTopology();

    // True when this being holds a MUTUAL (undirected) bond with another member,
    // or is the root. Directed edges make a being a participant, not a core:
    // an instance points at its category without the category pointing back.
    bool isCoreMember(const Singular* s) const;

    // Hierarchy of Joys — this Formation IS the hierarchy when tagged so.
    // Members are Lexemes; directed `grounds` edges are the order
    // (A grounds B: A is more foundational). No new C++ kind.
    // See docs/architecture/HIERARCHY_OF_JOYS.md.
    static constexpr const char* kGroundsType      = "grounds";
    static constexpr const char* kJoyHierarchyTag  = "hierarchy-of-joys";

    bool isJoyHierarchy() const {
        return relationTypeTag == kJoyHierarchyTag;
    }
    void markJoyHierarchy() { relationTypeTag = kJoyHierarchyTag; }

    // Rooted, tagged, every member is a Lexeme. Orphans are not invented.
    bool satisfiesJoyBounds() const;

    // Rank of a Lexeme (or of a being's telos) in this hierarchy.
    // Root = 0. Unranked / not in the graph = -1. Depth-bounded.
    int rankOf(const std::string& identifier) const;
    int rankOf(const Singular& being) const;

    // Views — membership and the relation list are not rewritten.
    std::vector<Singular*> membersOrderedByTelos() const;
    std::vector<std::shared_ptr<Relation>> relationsOrderedByTelos() const;

    // Implement the pure virtual method from Singular. Unique per instance —
    // identity-keyed systems (Rete bindings, provenance) depend on it.
    std::string getIdentifier() const override { return _formationId; }

    // Name this Formation with a STABLE slug instead of the generated
    // `formation-N`, which changes between runs and so cannot appear in law
    // text. A Formation that belongs to a named being — a Law's authors, its
    // targets — should be addressable as `<owner>.authors`, not as whichever
    // number it happened to draw this session. Refuses an empty name rather
    // than leaving the being unnamed; returns false when it does.
    bool setIdentifier(const std::string& id) {
        if (id.empty()) return false;
        _formationId = id;
        return true;
    }

private:
    static std::string nextFormationId();
    std::string _formationId = nextFormationId();

    void buildProperties() override {}

    // Raw admission: the checks and the push_back, with no relation
    // re-integration. Used for subformations, which are derived views and must
    // not spawn subformations of their own. Returns true if newly added.
    bool admitMember(Singular* s);
    // Refuses a Formation being made a member of itself — the self-ground §7
    // says must be refused loudly rather than accepted in silence.
    bool isSelf(const Singular* s) const;

    // When a member arrives AFTER the relations that name it (which is exactly
    // what loading a save does), the relations that were left unintegrated get
    // integrated now. Without this, a save produced one empty, permanently
    // unmatchable subformation per relation.
    void reintegrateRelationsFor(Singular* s);

    void integrateRelationTopology(const std::shared_ptr<Relation>& r);
    std::shared_ptr<Formation> findOrCreateRelationFormation(const std::shared_ptr<Relation>& r);

    // Depth-bounded recursive helpers. The depth bound alone guarantees
    // termination even on a cyclic subformation graph, which is why the hot
    // lookup path does not pay for a visited set; the copy and serialization
    // paths, which are cold, carry one as well.
    Singular* findMemberByIdentifierAt(const std::string& identifier, int depth) const;
    nlohmann::json toJsonAt(int depth, std::vector<const Formation*>& seen) const;
    bool releaseMemberAt(Singular* s, int depth);
    static std::vector<std::shared_ptr<Formation>> cloneSubformations(
        const std::vector<std::shared_ptr<Formation>>& src, int depth);

    // Directed reachability over relations of one type, used by the §7
    // acyclicity check. Bounded by the number of relations.
    bool reachesDirected(const std::string& from, const std::string& to,
                         const std::string& type) const;

    std::vector<Singular*> members;
    RelationManager relationMgr;
    std::vector<std::shared_ptr<Formation>> subformations;
    std::string relationTypeTag;

    // The being this Formation is ABOUT. Non-owning, like every member, and
    // always also a member. Null for a Formation that is merely a set.
    Singular* _root = nullptr;

    // Guards reintegrateRelationsFor against re-entering itself while
    // integration is adding the very members it is reacting to.
    bool _integrating = false;
};
