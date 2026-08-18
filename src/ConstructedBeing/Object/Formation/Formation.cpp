#include "Singularity/Language/Lexeme.hpp"
#include "Formation.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <queue>
#include <set>
#include "Singularity/Screen/Renderer.hpp"

std::string Formation::nextFormationId() {
    static std::atomic<unsigned long long> next{1};
    return "formation-" + std::to_string(next.fetch_add(1));
}

namespace {
glm::vec3 extractScale(const glm::mat4& transform) {
    glm::vec3 scale(glm::length(glm::vec3(transform[0])),
                    glm::length(glm::vec3(transform[1])),
                    glm::length(glm::vec3(transform[2])));
    if (scale.x <= 1e-6f) scale.x = 1.0f;
    if (scale.y <= 1e-6f) scale.y = 1.0f;
    if (scale.z <= 1e-6f) scale.z = 1.0f;
    return scale;
}
}

Formation::Formation(const std::vector<Singular*>& members) {
    for(const auto& member : members) {
        addMember(member);
    }
}



void Formation::addElement(const Singular& s) {
    addMember(const_cast<Singular*>(&s));
}

void Formation::removeElement(const Singular& s) {
    releaseMember(const_cast<Singular*>(&s));
}

// A Formation cannot be its own member. `Formation : public Singular` makes
// `f.addMember(&f)` well-typed, and it used to be accepted in silence — the
// self-ground AUTHORED_CATEGORIES.md §7 says must be refused loudly. Identity
// is checked by pointer AND by identifier, because a being that inherits
// Formation twice over (BodyPart is an Object and a Formation) arrives through
// a different base subobject with the same name.
bool Formation::isSelf(const Singular* s) const {
    if (!s) return false;
    if (s == static_cast<const Singular*>(this)) return true;
    const std::string mine = getIdentifier();
    return !mine.empty() && s->getIdentifier() == mine;
}

bool Formation::admitMember(Singular* s) {
    if (!s) return false;
    if (isSelf(s)) {
        std::fprintf(stderr,
            "Formation '%s': REFUSED to hold itself as a member. A formation "
            "grounded in itself has no ground — that is the infinite regress "
            "the First Mover principle exists to forbid.\n",
            getIdentifier().c_str());
        return false;
    }
    if (hasMember(s)) return false;
    members.push_back(s);
    return true;
}

void Formation::addMember(Singular* s) {
    if (!admitMember(s)) return;
    // Relations may already name this being — that is what loading a save
    // does. Integrate them now rather than never.
    reintegrateRelationsFor(s);
}

bool Formation::setRoot(Singular* s) {
    if (!s) {
        std::fprintf(stderr,
            "Formation '%s': REFUSED a null root. Use clearRoot() to say a "
            "formation is a set rather than a category.\n",
            getIdentifier().c_str());
        return false;
    }
    if (isSelf(s)) {
        std::fprintf(stderr,
            "Formation '%s': REFUSED to be its own root. A category that is "
            "its own ground defines itself by itself (AUTHORED_CATEGORIES.md §7).\n",
            getIdentifier().c_str());
        return false;
    }
    addMember(s);              // the root is always a member
    _root = s;
    return true;
}

bool Formation::hasMember(const Singular* s) const {
    return s && std::find(members.begin(), members.end(), s) != members.end();
}

Singular* Formation::findMemberByIdentifier(const std::string& identifier) const {
    return findMemberByIdentifierAt(identifier, 0);
}

Singular* Formation::findMemberByIdentifierAt(const std::string& identifier, int depth) const {
    if (identifier.empty()) return nullptr;
    for (auto* member : members) {
        if (member && member->getIdentifier() == identifier) {
            return member;
        }
    }
    if (subformations.empty()) return nullptr;
    if (depth + 1 >= kMaxFormationDepth) {
        std::fprintf(stderr,
            "Formation '%s': subformation nesting reached kMaxFormationDepth (%d); "
            "lookup of '%s' stops here. Bounds are doctrine — a formation that "
            "needs a deeper one is in the wrong shape.\n",
            getIdentifier().c_str(), kMaxFormationDepth, identifier.c_str());
        return nullptr;
    }
    for (const auto& sub : subformations) {
        if (!sub) continue;
        if (Singular* found = sub->findMemberByIdentifierAt(identifier, depth + 1)) {
            return found;
        }
    }
    return nullptr;
}

bool Formation::releaseMember(Singular* s) {
    return releaseMemberAt(s, 0);
}

bool Formation::releaseMemberAt(Singular* s, int depth) {
    if (!s) return false;
    bool found = false;
    const auto it = std::remove(members.begin(), members.end(), s);
    if (it != members.end()) {
        members.erase(it, members.end());
        found = true;
    }
    if (_root == s) _root = nullptr;
    if (subformations.empty()) return found;
    if (depth + 1 >= kMaxFormationDepth) {
        std::fprintf(stderr,
            "Formation '%s': subformation nesting reached kMaxFormationDepth (%d); "
            "release stops here and a member may still dangle below.\n",
            getIdentifier().c_str(), kMaxFormationDepth);
        return found;
    }
    for (const auto& sub : subformations) {
        if (!sub) continue;
        found = sub->releaseMemberAt(s, depth + 1) || found;
    }
    return found;
}

void Formation::removeMember(Singular* s) {
    releaseMember(s);
}

void Formation::clearMembers() {
    members.clear();
    subformations.clear();
    _root = nullptr;
}

void Formation::clearRelations() {
    relationMgr = RelationManager{};
    subformations.clear();
}

void Formation::clear() {
    members.clear();
    relationMgr = RelationManager{};
    subformations.clear();
    relationTypeTag.clear();
    _root = nullptr;
}

bool Formation::reachesDirected(const std::string& from, const std::string& to,
                                const std::string& type) const {
    if (from.empty() || to.empty()) return false;
    if (from == to) return true;

    std::map<std::string, std::vector<std::string>> out;
    for (const auto& rel : relationMgr.getAll()) {
        if (!rel || !rel->directed) continue;
        if (rel->type != type) continue;
        out[rel->entityA].push_back(rel->entityB);
    }

    std::set<std::string> seen{from};
    std::vector<std::string> stack{from};
    while (!stack.empty()) {
        const std::string curr = stack.back();
        stack.pop_back();
        auto it = out.find(curr);
        if (it == out.end()) continue;
        for (const auto& next : it->second) {
            if (next == to) return true;
            if (seen.insert(next).second) stack.push_back(next);
        }
    }
    return false;
}

bool Formation::mayAdmitRelation(const std::shared_ptr<Relation>& r) const {
    if (!r) return false;
    if (!r->entityA.empty() && r->entityA == r->entityB) return false;
    if (!r->directed) return true;   // mutual bonds are a set, not a regress
    return !reachesDirected(r->entityB, r->entityA, r->type);
}

bool Formation::addRelation(const std::shared_ptr<Relation>& r) {
    if (!r) return false;
    if (!mayAdmitRelation(r)) {
        if (!r->entityA.empty() && r->entityA == r->entityB) {
            std::fprintf(stderr,
                "Formation '%s': REFUSED relation '%s' from '%s' to itself. "
                "A being is not its own ground.\n",
                getIdentifier().c_str(), r->type.c_str(), r->entityA.c_str());
        } else {
            std::fprintf(stderr,
                "Formation '%s': REFUSED directed relation '%s' %s -> %s: it "
                "closes a cycle among '%s' edges, and a kind that is its own "
                "ancestor has no ground (AUTHORED_CATEGORIES.md §7). The edge is "
                "refused whole; no other edge is dropped to 'break' the cycle, "
                "because that would discard an authorship no one revoked.\n",
                getIdentifier().c_str(), r->type.c_str(), r->entityA.c_str(),
                r->entityB.c_str(), r->type.c_str());
        }
        return false;
    }
    relationMgr.add(r);
    integrateRelationTopology(r);
    return true;
}

bool Formation::removeRelation(const std::shared_ptr<Relation>& r) {
    bool removed = relationMgr.remove(r);
    for (auto it = subformations.begin(); it != subformations.end();) {
        if (!*it) {
            it = subformations.erase(it);
            continue;
        }
        removed = (*it)->relationMgr.remove(r) || removed;
        if ((*it)->relationMgr.getAll().empty()) {
            it = subformations.erase(it);
        } else {
            ++it;
        }
    }
    return removed;
}

// Applies to physical Formation objects
void Formation::draw() const {

    // Draw the physical members of the formation.
    // Iterate through elements, processing each relation.
    // Call event "draw" to event bus, with priorities on which member of the Formation and which Relations between them are processed first. Need specific algorithm.
    // Run algorithm to determine what algorithm makes this process the lowest O(n). Avoid worst case scenarios if possible.
    // Then draw each contained object
    for (const auto* member : members) {
        if (!member) continue;
        // Only draw if it's an Object
        if (const auto* obj = dynamic_cast<const Object*>(member)) {
            currentRenderer().pushModel(obj->getTransform());
            obj->drawObject();
            obj->drawHighlightOutline();
            currentRenderer().popModel();
        }
    }
}

// -----------------------------------------------------------------------------
// Build complete graph of membership relations between objects
// -----------------------------------------------------------------------------
void Formation::rebuildCompleteGraph() {
    relationMgr = RelationManager{}; // reset
    subformations.clear();
    for (size_t i = 0; i < members.size(); ++i) {
        for (size_t j = i + 1; j < members.size(); ++j) {
            if (!members[i] || !members[j]) continue;
            // Relation rel{"member", *members[i], *members[j]};
            auto rel = std::make_shared<Relation>("member", *members[i], *members[j]);
            relationMgr.add(rel);
        }
    }
}

bool Formation::isCoreMember(const Singular* s) const {
    if (!s) return false;
    if (isRoot(s)) return true;   // the root grounds the category; it is core by definition
    const std::string id = s->getIdentifier();
    for (const auto& rel : relationMgr.getAll()) {
        if (rel && !rel->directed) {
            if (rel->entityA == id && findMemberByIdentifier(rel->entityB)) return true;
            if (rel->entityB == id && findMemberByIdentifier(rel->entityA)) return true;
        }
    }
    return false;
}

Formation::Topology Formation::resolveTopology() {
    Topology out;

    // Step 1: adjacency over EVERY relation, direction-blind.
    //
    // The old version built adjacency from `!rel->directed` alone. Both category
    // edges are directed (AUTHORED_CATEGORIES.md §2/§8), so a correctly-authored
    // category had zero edges here, zero components, zero valid cores — and was
    // erased. Direction is what a relation MEANS; it is not a statement about
    // whether two beings are held together.
    std::map<std::string, std::vector<std::string>> adj;
    for (const auto& rel : relationMgr.getAll()) {
        if (!rel) continue;
        if (rel->entityA.empty() || rel->entityB.empty()) continue;
        adj[rel->entityA].push_back(rel->entityB);
        adj[rel->entityB].push_back(rel->entityA);
    }

    // Step 2: weakly connected components across the members. A member named by
    // no relation at all is its own component of one.
    std::vector<std::set<std::string>> components;
    std::set<std::string> visited;
    const std::string rootId = _root ? _root->getIdentifier() : std::string{};
    long long rootComponent = -1;

    for (const auto* m : members) {
        if (!m) continue;
        const std::string startId = m->getIdentifier();
        if (startId.empty()) continue;
        if (!visited.insert(startId).second) continue;

        std::set<std::string> comp{startId};
        std::vector<std::string> stack{startId};
        while (!stack.empty()) {
            const std::string curr = stack.back();
            stack.pop_back();
            auto it = adj.find(curr);
            if (it == adj.end()) continue;
            for (const auto& neighbor : it->second) {
                if (comp.insert(neighbor).second) {
                    visited.insert(neighbor);
                    stack.push_back(neighbor);
                }
            }
        }
        if (!rootId.empty() && comp.count(rootId)) rootComponent = static_cast<long long>(components.size());
        components.push_back(std::move(comp));
    }

    // Step 3: a component is a valid core when it holds the root — a rooted
    // Formation is a category and its root grounds it at any size — or when it
    // binds at least three participants.
    std::vector<size_t> validCores;
    for (size_t i = 0; i < components.size(); ++i) {
        const bool rooted = (static_cast<long long>(i) == rootComponent);
        if (rooted || components[i].size() >= 3) validCores.push_back(i);
    }

    if (validCores.empty()) {
        // Report; do NOT enact. This used to run `clearMembers(); relationMgr =
        // RelationManager{};` — asking a formation a question destroyed it, and
        // the caller was told nothing. An empty result is an answer, not a licence.
        out.applied = false;
        for (auto* m : members) if (m) out.orphaned.push_back(m);
        return out;
    }

    // The root's component is always primary: a category does not get
    // re-parented into whichever unrelated group happens to be larger.
    size_t primary = validCores.front();
    if (rootComponent >= 0) {
        primary = static_cast<size_t>(rootComponent);
        out.rooted = true;
    } else {
        for (size_t idx : validCores) {
            if (components[idx].size() > components[primary].size()) primary = idx;
        }
    }

    const std::vector<Singular*> oldMembers = members;
    const std::vector<std::shared_ptr<Relation>> oldRelations = relationMgr.getAll();
    Singular* oldRoot = _root;

    members.clear();
    relationMgr = RelationManager{};
    subformations.clear();
    _root = nullptr;

    const std::set<std::string>& primaryGroup = components[primary];
    for (auto* m : oldMembers) {
        if (m && primaryGroup.count(m->getIdentifier())) admitMember(m);
    }
    for (const auto& rel : oldRelations) {
        if (!rel) continue;
        if (primaryGroup.count(rel->entityA) && primaryGroup.count(rel->entityB)) {
            // Re-admitted, not re-authored: these relations were already
            // admitted once, so they are not put back through the §7 guard.
            relationMgr.add(rel);
            integrateRelationTopology(rel);
        }
    }
    if (oldRoot && hasMember(oldRoot)) _root = oldRoot;

    for (size_t idx : validCores) {
        if (idx == primary) continue;

        auto spawned = std::make_shared<Formation>();
        const std::set<std::string>& group = components[idx];

        for (auto* m : oldMembers) {
            if (m && group.count(m->getIdentifier())) spawned->admitMember(m);
        }
        for (const auto& rel : oldRelations) {
            if (!rel) continue;
            if (group.count(rel->entityA) && group.count(rel->entityB)) {
                spawned->relationMgr.add(rel);
                spawned->integrateRelationTopology(rel);
            }
        }
        out.spawned.push_back(spawned);
    }

    for (auto* m : oldMembers) {
        if (!m) continue;
        bool kept = false;
        for (size_t idx : validCores) {
            if (components[idx].count(m->getIdentifier())) { kept = true; break; }
        }
        if (!kept) out.orphaned.push_back(m);
    }
    out.applied = true;

    if (!out.orphaned.empty()) {
        std::fprintf(stderr,
            "Formation '%s': resolveTopology released %zu member(s) held by no "
            "valid core. They are returned in Topology::orphaned — nothing was "
            "destroyed silently.\n",
            getIdentifier().c_str(), out.orphaned.size());
    }
    return out;
}


std::shared_ptr<Formation> Formation::findOrCreateRelationFormation(const std::shared_ptr<Relation>& r) {
    if (!r) return nullptr;

    std::vector<size_t> matchingIndices;
    for (size_t i = 0; i < subformations.size(); ++i) {
        const auto& sub = subformations[i];
        if (!sub) continue;
        if (sub->relationTypeTag != r->type) continue;
        // A subformation that ALREADY HOLDS this relation is its home, even if
        // it has no members yet. Matching on members alone meant a subformation
        // born before its members (which is what loading a save does) could
        // never match anything again, and stayed empty for the session.
        bool holdsRelation = false;
        for (const auto& held : sub->relationMgr.getAll()) {
            if (held == r) { holdsRelation = true; break; }
        }
        if (holdsRelation ||
            sub->findMemberByIdentifier(r->entityA) ||
            sub->findMemberByIdentifier(r->entityB)) {
            matchingIndices.push_back(i);
        }
    }

    if (matchingIndices.empty()) {
        auto created = std::make_shared<Formation>();
        created->relationTypeTag = r->type;
        subformations.push_back(created);
        return created;
    }

    auto primary = subformations[matchingIndices.front()];

    // Take owning handles to every match before touching the vector. Merging
    // by index meant each erase shifted the ones after it, and an unrelated
    // subformation could slide into a slot the loop still had to visit —
    // absorbing a set that never matched the relation at all.
    std::vector<std::shared_ptr<Formation>> absorbed;
    for (size_t i = 1; i < matchingIndices.size(); ++i) {
        const auto& secondary = subformations[matchingIndices[i]];
        if (!secondary || secondary == primary) continue;
        absorbed.push_back(secondary);
    }

    for (const auto& secondary : absorbed) {
        for (auto* member : secondary->getMembers()) {
            primary->admitMember(member);
        }
        for (const auto& rel : secondary->relations().getAll()) {
            primary->relations().add(rel);
        }
    }

    if (!absorbed.empty()) {
        subformations.erase(
            std::remove_if(subformations.begin(), subformations.end(),
                           [&absorbed](const std::shared_ptr<Formation>& sub) {
                               return std::find(absorbed.begin(), absorbed.end(), sub) != absorbed.end();
                           }),
            subformations.end());
    }
    return primary;
}

void Formation::integrateRelationTopology(const std::shared_ptr<Relation>& r) {
    if (!r) return;

    Singular* memberA = findMemberByIdentifier(r->entityA);
    Singular* memberB = findMemberByIdentifier(r->entityB);
    if (memberA) admitMember(memberA);
    if (memberB) admitMember(memberB);

    auto groupedFormation = findOrCreateRelationFormation(r);
    if (!groupedFormation) return;

    // admitMember, not addMember: a subformation is a derived view, and letting
    // it re-integrate would grow subformations of its own.
    if (memberA) groupedFormation->admitMember(memberA);
    if (memberB) groupedFormation->admitMember(memberB);
    // Share the relation rather than copying it: a subformation is a view onto
    // the same bond, not a second bond. Copies drifted whenever the original's
    // attachment was re-measured, and merges then carried stale duplicates.
    groupedFormation->relationMgr.add(r);
}

void Formation::reintegrateRelationsFor(Singular* s) {
    if (!s || _integrating) return;
    const std::string id = s->getIdentifier();
    if (id.empty()) return;

    std::vector<std::shared_ptr<Relation>> naming;
    for (const auto& rel : relationMgr.getAll()) {
        if (rel && (rel->entityA == id || rel->entityB == id)) naming.push_back(rel);
    }
    if (naming.empty()) return;

    _integrating = true;
    for (const auto& rel : naming) integrateRelationTopology(rel);
    _integrating = false;
}

void Formation::applyAttachmentRelations() {
    std::vector<std::shared_ptr<Relation>> attachments;
    std::unordered_set<std::string> seenRelations;
    for (const auto& rel : relationMgr.getAll()) {
        if (rel && rel->isAttachment() && seenRelations.insert(rel->getIdentifier()).second) attachments.push_back(rel);
    }
    for (const auto& sub : subformations) {
        if (!sub) continue;
        for (const auto& rel : sub->relations().getAll()) {
            if (rel && rel->isAttachment() && seenRelations.insert(rel->getIdentifier()).second) attachments.push_back(rel);
        }
    }

    const size_t maxIterations = attachments.size() + 1;
    for (size_t iteration = 0; iteration < maxIterations; ++iteration) {
        bool progress = false;
        for (const auto& rel : attachments) {
            if (!rel || !rel->attachment.enabled) continue;

            auto* parent = dynamic_cast<Object*>(findMemberByIdentifier(rel->entityA));
            auto* child  = dynamic_cast<Object*>(findMemberByIdentifier(rel->entityB));
            if (!parent || !child) continue;

            glm::mat4 parentTransform = parent->getTransform();
            glm::mat4 childTransform = child->getTransform();
            glm::mat4 nextTransform = rel->attachment.localOffset;

            if (rel->attachment.inheritTranslation || rel->attachment.inheritRotation || rel->attachment.inheritScale) {
                if (rel->attachment.inheritRotation && rel->attachment.inheritScale && rel->attachment.inheritTranslation) {
                    nextTransform = parentTransform * rel->attachment.localOffset;
                } else {
                    glm::vec3 translation = rel->attachment.inheritTranslation
                        ? glm::vec3(parentTransform * glm::vec4(glm::vec3(rel->attachment.localOffset[3]), 1.0f))
                        : glm::vec3(childTransform[3]);

                    glm::mat4 rebuilt = glm::translate(glm::mat4(1.0f), translation);
                    if (rel->attachment.inheritRotation) {
                        glm::vec3 parentX = glm::normalize(glm::vec3(parentTransform[0]));
                        glm::vec3 parentY = glm::normalize(glm::vec3(parentTransform[1]));
                        glm::vec3 parentZ = glm::normalize(glm::vec3(parentTransform[2]));
                        glm::mat4 rotationOnly(1.0f);
                        rotationOnly[0] = glm::vec4(parentX, 0.0f);
                        rotationOnly[1] = glm::vec4(parentY, 0.0f);
                        rotationOnly[2] = glm::vec4(parentZ, 0.0f);
                        rebuilt *= rotationOnly;
                    } else {
                        glm::vec3 childX = glm::normalize(glm::vec3(childTransform[0]));
                        glm::vec3 childY = glm::normalize(glm::vec3(childTransform[1]));
                        glm::vec3 childZ = glm::normalize(glm::vec3(childTransform[2]));
                        rebuilt[0] = glm::vec4(childX, 0.0f);
                        rebuilt[1] = glm::vec4(childY, 0.0f);
                        rebuilt[2] = glm::vec4(childZ, 0.0f);
                    }

                    glm::vec3 scale = rel->attachment.inheritScale
                        ? extractScale(parentTransform * rel->attachment.localOffset)
                        : extractScale(childTransform);
                    rebuilt = glm::scale(rebuilt, scale);
                    nextTransform = rebuilt;
                }
            }

            child->setTransform(nextTransform);
            progress = true;
        }
        if (!progress) break;
    }
}

std::vector<std::shared_ptr<Formation>> Formation::cloneSubformations(
    const std::vector<std::shared_ptr<Formation>>& src, int depth) {
    std::vector<std::shared_ptr<Formation>> copies;
    if (src.empty()) return copies;
    if (depth >= kMaxFormationDepth) {
        std::fprintf(stderr,
            "Formation: subformation nesting reached kMaxFormationDepth (%d) while "
            "copying; the copy is truncated here rather than recursing forever.\n",
            kMaxFormationDepth);
        return copies;
    }
    copies.reserve(src.size());
    for (const auto& sub : src) {
        if (!sub) { copies.push_back(nullptr); continue; }
        // Built field-by-field rather than through the copy constructor so the
        // depth bound survives the recursion. A fresh Formation mints a fresh
        // identity, which is the whole point: a copy is a new being.
        auto copy = std::make_shared<Formation>();
        copy->members = sub->members;
        copy->relationMgr = sub->relationMgr;
        copy->relationTypeTag = sub->relationTypeTag;
        copy->_root = sub->_root;
        copy->subformations = cloneSubformations(sub->subformations, depth + 1);
        copies.push_back(std::move(copy));
    }
    return copies;
}

nlohmann::json Formation::toJson() const {
    std::vector<const Formation*> seen;
    return toJsonAt(0, seen);
}

nlohmann::json Formation::toJsonAt(int depth, std::vector<const Formation*>& seen) const {
    nlohmann::json j = nlohmann::json::object();
    // Serialize members (Lexemes, etc.)
    nlohmann::json jMembers = nlohmann::json::array();
    for (auto* m : members) {
        if (!m) continue;
        if (auto* lexeme = dynamic_cast<Singularity::Language::Lexeme*>(m)) {
            jMembers.push_back({
                {"type", "Lexeme"},
                {"symbol", lexeme->getSymbol()},
                {"id", lexeme->getIdentifier()}
            });
        } else {
            // Stub for other singulars
            jMembers.push_back({
                {"type", "Singular"},
                {"id", m->getIdentifier()}
            });
        }
    }
    j["members"] = jMembers;

    // The root, when there is one: a Formation with a root is a category, and
    // which being it is about is not derivable from the members.
    if (_root) j["root"] = _root->getIdentifier();

    // Subformations — bounded by depth AND by a visited set, because a handle
    // graph that ever became cyclic would otherwise recurse until the stack ran out.
    seen.push_back(this);
    nlohmann::json jSub = nlohmann::json::array();
    if (depth + 1 >= kMaxFormationDepth) {
        if (!subformations.empty()) {
            std::fprintf(stderr,
                "Formation '%s': subformation nesting reached kMaxFormationDepth (%d); "
                "serialization is truncated here.\n",
                getIdentifier().c_str(), kMaxFormationDepth);
        }
    } else {
        for (const auto& sub : subformations) {
            if (!sub) continue;
            if (std::find(seen.begin(), seen.end(), sub.get()) != seen.end()) {
                std::fprintf(stderr,
                    "Formation '%s': subformation cycle detected during "
                    "serialization; the repeated formation is emitted once.\n",
                    getIdentifier().c_str());
                continue;
            }
            jSub.push_back(sub->toJsonAt(depth + 1, seen));
        }
    }
    seen.pop_back();
    j["subformations"] = jSub;

    // Relations
    j["relations"] = relationMgr.toJson();
    return j;
}

std::shared_ptr<Formation> Formation::fromJson(const nlohmann::json& json,
                                               const MemberResolver& resolve) {
    if (!resolve) {
        // The old body constructed an empty Formation and threw the JSON away.
        // A Formation holds NON-OWNING pointers to beings it did not create, so
        // without a resolver there is no honest reconstruction — say so instead
        // of handing back something that looks loaded and is not.
        std::fprintf(stderr,
            "Formation::fromJson REFUSED: no MemberResolver supplied. A Formation "
            "cannot invent its members; pass a resolver that maps an identifier to "
            "a being in the world being loaded.\n");
        return nullptr;
    }

    auto f = std::make_shared<Formation>();

    if (json.contains("members") && json["members"].is_array()) {
        for (const auto& m : json["members"]) {
            if (!m.is_object()) continue;
            const std::string id = m.value("id", std::string{});
            if (id.empty()) continue;
            Singular* being = resolve(id);
            if (!being) {
                std::fprintf(stderr,
                    "Formation::fromJson: no being named '%s' in this world; the "
                    "member is dropped rather than faked.\n", id.c_str());
                continue;
            }
            f->admitMember(being);
        }
    }

    if (json.contains("relations")) {
        RelationManager loaded;
        loaded.loadFromJson(json["relations"]);
        for (const auto& rel : loaded.getAll()) f->addRelation(rel);
    }

    // "subformations" is deliberately NOT parsed. Subformations are DERIVED —
    // findOrCreateRelationFormation builds them from the relations above, and
    // they share the parent's Relation objects rather than owning copies.
    // Parsing them would mint a second set of Relations for the same bonds and
    // break that sharing; integrating is the faithful reconstruction. This is
    // also why fromJson has no recursion to bound.

    const std::string rootId = json.value("root", std::string{});
    if (!rootId.empty()) {
        if (Singular* rootBeing = f->findMemberByIdentifier(rootId)) {
            f->setRoot(rootBeing);
        } else {
            std::fprintf(stderr,
                "Formation::fromJson: root '%s' is not among the resolved members; "
                "the formation loads as a set, not a category.\n", rootId.c_str());
        }
    }

    return f;
}

bool Formation::satisfiesJoyBounds() const {
    if (!isJoyHierarchy()) return false;
    if (!hasRoot()) return false;
    if (!dynamic_cast<const Singularity::Language::Lexeme*>(_root)) return false;
    for (const Singular* member : members) {
        if (!member) continue;
        if (!dynamic_cast<const Singularity::Language::Lexeme*>(member)) return false;
    }
    return true;
}

int Formation::rankOf(const std::string& identifier) const {
    if (identifier.empty() || !hasRoot()) return -1;

    std::unordered_map<std::string, std::vector<std::string>> children;
    for (const auto& rel : relationMgr.getAll()) {
        if (!rel || !rel->directed) continue;
        if (rel->type != kGroundsType) continue;
        if (rel->entityA.empty() || rel->entityB.empty()) continue;
        children[rel->entityA].push_back(rel->entityB);
    }

    const std::string rootId = _root->getIdentifier();
    std::unordered_map<std::string, int> rank;
    std::queue<std::string> walk;
    rank[rootId] = 0;
    walk.push(rootId);
    int depth = 0;
    while (!walk.empty() && depth < kMaxFormationDepth) {
        const size_t layer = walk.size();
        for (size_t i = 0; i < layer; ++i) {
            const std::string curr = walk.front();
            walk.pop();
            auto it = children.find(curr);
            if (it == children.end()) continue;
            for (const auto& child : it->second) {
                if (rank.count(child)) continue;
                rank[child] = rank[curr] + 1;
                walk.push(child);
            }
        }
        ++depth;
    }

    auto found = rank.find(identifier);
    return found == rank.end() ? -1 : found->second;
}

int Formation::rankOf(const Singular& being) const {
    const std::string& telos = being.telosId();
    if (!telos.empty()) return rankOf(telos);
    return rankOf(being.getIdentifier());
}

std::vector<Singular*> Formation::membersOrderedByTelos() const {
    std::vector<Singular*> ordered = members;
    std::stable_sort(ordered.begin(), ordered.end(),
        [this](const Singular* a, const Singular* b) {
            const int ra = a ? rankOf(*a) : -1;
            const int rb = b ? rankOf(*b) : -1;
            const int ka = ra < 0 ? 100000 : ra;
            const int kb = rb < 0 ? 100000 : rb;
            return ka < kb;
        });
    return ordered;
}

std::vector<std::shared_ptr<Relation>> Formation::relationsOrderedByTelos() const {
    auto ordered = relationMgr.getAll();
    std::stable_sort(ordered.begin(), ordered.end(),
        [this](const std::shared_ptr<Relation>& a, const std::shared_ptr<Relation>& b) {
            if (!a || !b) return bool(a) && !b;
            const int aMin = [&] {
                const int ra = rankOf(a->entityA);
                const int rb = rankOf(a->entityB);
                const int ka = ra < 0 ? 100000 : ra;
                const int kb = rb < 0 ? 100000 : rb;
                return std::min(ka, kb);
            }();
            const int bMin = [&] {
                const int ra = rankOf(b->entityA);
                const int rb = rankOf(b->entityB);
                const int ka = ra < 0 ? 100000 : ra;
                const int kb = rb < 0 ? 100000 : rb;
                return std::min(ka, kb);
            }();
            if (aMin != bMin) return aMin < bMin;
            const int aSpan = std::abs((rankOf(a->entityA) < 0 ? 100000 : rankOf(a->entityA))
                                     - (rankOf(a->entityB) < 0 ? 100000 : rankOf(a->entityB)));
            const int bSpan = std::abs((rankOf(b->entityA) < 0 ? 100000 : rankOf(b->entityA))
                                     - (rankOf(b->entityB) < 0 ? 100000 : rankOf(b->entityB)));
            return aSpan < bSpan;
        });
    return ordered;
}
