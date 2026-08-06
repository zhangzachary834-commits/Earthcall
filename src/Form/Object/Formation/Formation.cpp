#include "Singularity/Language/Lexeme.hpp"
#include "Formation.hpp"
#include "Form/Object/Object.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <atomic>
#include <unordered_set>
#include <map>
#include <set>
#include "Rendering/Renderer.hpp"

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
    members.erase(std::remove(members.begin(), members.end(), const_cast<Singular*>(&s)), members.end());
}

void Formation::addMember(Singular* s) {
    if(!s) return;
    if (hasMember(s)) return;
    members.push_back(s);
}

bool Formation::hasMember(const Singular* s) const {
    return s && std::find(members.begin(), members.end(), s) != members.end();
}

Singular* Formation::findMemberByIdentifier(const std::string& identifier) const {
    if (identifier.empty()) return nullptr;
    for (auto* member : members) {
        if (member && member->getIdentifier() == identifier) {
            return member;
        }
    }
    for (const auto& sub : subformations) {
        if (!sub) continue;
        if (Singular* found = sub->findMemberByIdentifier(identifier)) {
            return found;
        }
    }
    return nullptr;
}

void Formation::removeMember(Singular* s) {
    if(!s) return;
    members.erase(std::remove(members.begin(), members.end(), s), members.end());
}

void Formation::clearMembers() {
    members.clear();
    subformations.clear();
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
}

void Formation::addRelation(const std::shared_ptr<Relation>& r) {
    if (!r) return;
    relationMgr.add(r);
    integrateRelationTopology(r);
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
    std::string id = s->getIdentifier();
    for (const auto& rel : relationMgr.getAll()) {
        if (rel && !rel->directed) {
            if (rel->entityA == id && findMemberByIdentifier(rel->entityB)) return true;
            if (rel->entityB == id && findMemberByIdentifier(rel->entityA)) return true;
        }
    }
    return false;
}

std::vector<std::shared_ptr<Formation>> Formation::resolveTopology() {
    std::vector<std::shared_ptr<Formation>> spawnedFormations;
    
    // Step 1: Build undirected adjacency list
    std::map<std::string, std::vector<std::string>> adj;
    for (const auto& rel : relationMgr.getAll()) {
        if (rel && !rel->directed) {
            adj[rel->entityA].push_back(rel->entityB);
            adj[rel->entityB].push_back(rel->entityA);
        }
    }

    // Step 2: Find connected components (undirected)
    std::vector<std::vector<std::string>> components;
    std::set<std::string> visited;

    for (const auto* m : members) {
        if (!m) continue;
        std::string startId = m->getIdentifier();
        if (visited.find(startId) != visited.end()) continue;
        
        // Only consider nodes that have undirected edges
        if (adj.find(startId) == adj.end() || adj[startId].empty()) continue;

        std::vector<std::string> comp;
        std::vector<std::string> stack = {startId};
        visited.insert(startId);

        while (!stack.empty()) {
            std::string curr = stack.back();
            stack.pop_back();
            comp.push_back(curr);

            for (const auto& neighbor : adj[curr]) {
                if (visited.find(neighbor) == visited.end()) {
                    visited.insert(neighbor);
                    stack.push_back(neighbor);
                }
            }
        }
        components.push_back(comp);
    }

    // Step 3: Filter valid cores (>= 3 members)
    std::vector<std::vector<std::string>> validCores;
    for (const auto& comp : components) {
        if (comp.size() >= 3) {
            validCores.push_back(comp);
        }
    }

    if (validCores.empty()) {
        clearMembers();
        relationMgr = RelationManager{};
        return spawnedFormations;
    }

    size_t primaryIndex = 0;
    for (size_t i = 1; i < validCores.size(); ++i) {
        if (validCores[i].size() > validCores[primaryIndex].size()) {
            primaryIndex = i;
        }
    }

    // Step 4: Attach peripheral members
    std::map<std::string, std::vector<std::string>> fullAdj;
    for (const auto& rel : relationMgr.getAll()) {
        if (!rel) continue;
        fullAdj[rel->entityA].push_back(rel->entityB);
        fullAdj[rel->entityB].push_back(rel->entityA); 
    }

    std::vector<std::set<std::string>> coreAndPeripherals(validCores.size());
    for (size_t i = 0; i < validCores.size(); ++i) {
        std::set<std::string>& group = coreAndPeripherals[i];
        std::vector<std::string> stack = validCores[i];
        for (const auto& node : stack) group.insert(node);

        while (!stack.empty()) {
            std::string curr = stack.back();
            stack.pop_back();

            for (const auto& neighbor : fullAdj[curr]) {
                if (group.find(neighbor) == group.end()) {
                    bool inAnotherCore = false;
                    for (size_t j = 0; j < validCores.size(); ++j) {
                        if (i == j) continue;
                        if (std::find(validCores[j].begin(), validCores[j].end(), neighbor) != validCores[j].end()) {
                            inAnotherCore = true;
                            break;
                        }
                    }
                    if (!inAnotherCore) {
                        group.insert(neighbor);
                        stack.push_back(neighbor);
                    }
                }
            }
        }
    }

    auto oldMembers = members;
    auto oldRelations = relationMgr.getAll();
    clearMembers();
    relationMgr = RelationManager{};

    const auto& primaryGroup = coreAndPeripherals[primaryIndex];
    for (auto* m : oldMembers) {
        if (m && primaryGroup.find(m->getIdentifier()) != primaryGroup.end()) {
            addMember(m);
        }
    }
    for (const auto& rel : oldRelations) {
        if (!rel) continue;
        if (primaryGroup.find(rel->entityA) != primaryGroup.end() && primaryGroup.find(rel->entityB) != primaryGroup.end()) {
            relationMgr.add(rel);
        }
    }

    for (size_t i = 0; i < validCores.size(); ++i) {
        if (i == primaryIndex) continue;

        auto spawned = std::make_shared<Formation>();
        const auto& group = coreAndPeripherals[i];
        
        for (auto* m : oldMembers) {
            if (m && group.find(m->getIdentifier()) != group.end()) {
                spawned->addMember(m);
            }
        }
        for (const auto& rel : oldRelations) {
            if (!rel) continue;
            if (group.find(rel->entityA) != group.end() && group.find(rel->entityB) != group.end()) {
                spawned->relations().add(rel);
            }
        }
        spawnedFormations.push_back(spawned);
    }

    return spawnedFormations;
}


std::shared_ptr<Formation> Formation::findOrCreateRelationFormation(const std::shared_ptr<Relation>& r) {
    if (!r) return nullptr;

    std::vector<size_t> matchingIndices;
    for (size_t i = 0; i < subformations.size(); ++i) {
        const auto& sub = subformations[i];
        if (!sub) continue;
        if (sub->relationTypeTag != r->type) continue;
        if (sub->findMemberByIdentifier(r->entityA) || sub->findMemberByIdentifier(r->entityB)) {
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
            primary->addMember(member);
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
    if (memberA) addMember(memberA);
    if (memberB) addMember(memberB);

    auto groupedFormation = findOrCreateRelationFormation(r);
    if (!groupedFormation) return;

    if (memberA) groupedFormation->addMember(memberA);
    if (memberB) groupedFormation->addMember(memberB);
    // Share the relation rather than copying it: a subformation is a view onto
    // the same bond, not a second bond. Copies drifted whenever the original's
    // attachment was re-measured, and merges then carried stale duplicates.
    groupedFormation->relationMgr.add(r);
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

nlohmann::json Formation::toJson() const {
    nlohmann::json j = nlohmann::json::object();
    // Serialize members (Lexemes, etc.)
    nlohmann::json jMembers = nlohmann::json::array();
    for (auto* m : members) {
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
    
    // Subformations
    nlohmann::json jSub = nlohmann::json::array();
    for (const auto& sub : subformations) {
        jSub.push_back(sub->toJson());
    }
    j["subformations"] = jSub;
    
    // Relations
    j["relations"] = relationMgr.toJson();
    return j;
}

std::shared_ptr<Formation> Formation::fromJson(const nlohmann::json& json) {
    auto f = std::make_shared<Formation>(std::vector<Singular*>{});
    // This is a stub for the full recursive traversal. 
    // In a real system we'd look up the Singulars from the engine by ID or create new Lexemes
    // using LanguageSystem::instance().resolve(sym);
    return f;
}
