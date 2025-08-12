#include "Formations.hpp"
#include "Form/Object/Object.hpp"
#include <GLFW/glfw3.h>

Formations::Formations(const std::vector<Singular*>& members) {
    for(const auto& member : members) {
        addMember(member);
    }
}


Formations::Formations(const std::vector<Singular*>& members, const glm::vec3& dims) 
    : Form(Form::ShapeType::Cube, dims) {
    for(const auto& member : members) {
        addMember(member);
    }
}

Formations::Formations(const std::vector<Singular*>& members, Form::ShapeType type, const glm::vec3& dims) 
    : Form(type, dims) {
    for(const auto& member : members) {
        addMember(member);
    }
}



void Formations::addElement(const Singular& s) {
    members.push_back(const_cast<Singular*>(&s));
}

void Formations::removeElement(const Singular& s) {
    members.erase(std::remove(members.begin(), members.end(), const_cast<Singular*>(&s)), members.end());
}

void Formations::addMember(Singular* s) {
    if(!s) return;
    members.push_back(s);
}

void Formations::removeMember(Singular* s) {
    if(!s) return;
    members.erase(std::remove(members.begin(), members.end(), s), members.end());
}

// Applies to physical Formation objects
void Formations::draw() const {

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
void Formations::rebuildCompleteGraph() {
    relationMgr = RelationManager{}; // reset
    for (size_t i = 0; i < members.size(); ++i) {
        for (size_t j = i + 1; j < members.size(); ++j) {
            if (!members[i] || !members[j]) continue;
            Relation rel{"member", *members[i], *members[j]};
            relationMgr.add(rel);
        }
    }
}
