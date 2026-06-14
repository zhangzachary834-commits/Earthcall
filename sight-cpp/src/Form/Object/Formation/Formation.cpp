#include "Formation.hpp"
#include "Form/Object/Object.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_set>

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


Formation::Formation(const std::vector<Singular*>& members, const glm::vec3& dims) 
    : Form(Form::ShapeType::Cube, dims) {
    for(const auto& member : members) {
        addMember(member);
    }
}

Formation::Formation(const std::vector<Singular*>& members, Form::ShapeType type, const glm::vec3& dims) 
    : Form(type, dims) {
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
    // if hasprimaryform = true, draw the primary form
    // --> Draw the formation shape first (optional visual)
    Form::draw();

    // Iterate through elements, processing each relation. 
    // Call event "draw" to event bus, with priorities on which member of the Formation and which Relations between them are processed first. Need specific algorithm.
    // Run algorithm to determine what algorithm makes this process the lowest O(n). Avoid worst case scenarios if possible.
    // Then draw each contained object
    for (const auto* member : members) {
        if (!member) continue;
        // Only draw if it's an Object
        if (const auto* obj = dynamic_cast<const Object*>(member)) {
            glPushMatrix();
            glMultMatrixf(&obj->getTransform()[0][0]);
            obj->drawObject();
            obj->drawHighlightOutline();
            glPopMatrix();
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
        auto created = std::make_shared<Formation>(Form::ShapeType::Cube, glm::vec3(1.0f));
        created->relationTypeTag = r->type;
        subformations.push_back(created);
        return created;
    }

    auto primary = subformations[matchingIndices.front()];
    for (size_t i = 1; i < matchingIndices.size(); ++i) {
        auto secondary = subformations[matchingIndices[i]];
        if (!secondary || secondary == primary) continue;

        for (auto* member : secondary->getMembers()) {
            primary->addMember(member);
        }
        for (const auto& rel : secondary->relations().getAll()) {
            primary->relations().add(rel);
        }
        subformations.erase(subformations.begin() + static_cast<long>(matchingIndices[i]));
        --i;
        for (size_t j = i + 1; j < matchingIndices.size(); ++j) {
            if (matchingIndices[j] > matchingIndices[i]) {
                --matchingIndices[j];
            }
        }
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
    groupedFormation->relationMgr.add(std::make_shared<Relation>(*r));
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
