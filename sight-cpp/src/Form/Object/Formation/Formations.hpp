#pragma once

#include <vector>
#include <algorithm>
#include "Form/Form.hpp"
#include <glm/glm.hpp>
#include "Relation/RelationManager.hpp"
#include "Singular.hpp"


class Formations : public Form, public Singular {

public:
    // Constructor
    Formations(const std::vector<Singular*>& members);
    Formations(const std::vector<Singular*>& members, const glm::vec3& dims);
    Formations(const std::vector<Singular*>& members, Form::ShapeType type, const glm::vec3& dims);

    // Legacy constructor for fallback
    Formations(Form::ShapeType type, const glm::vec3& dims = {1.0f,1.0f,1.0f}) : Form(type, dims) {}
    
    // Destructor
    ~Formations() = default;

    // ------ Generic membership helpers (any Singular) ------------
    void addMember(Singular* s);

    void removeMember(Singular* s);

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

    // Convenience wrapper to add a relation directly
    void addRelation(const Relation& r) { relationMgr.add(r); }
    
    // Add a relation directly (alias for addRelation)
    void add(const Relation& r) { relationMgr.add(r); }
    
    // Build a simple fully-connected graph between all objects currently
    // in this formation (undirected, weight 1.0, type="member")
    void rebuildCompleteGraph();

    // Render the formation and its constituent objects
    void draw() const;

    // Add methods to manipulate formations, such as adding or removing elements,
    // checking relationships, etc.

    // Add a Singular element to the formation
    void addElement(const Singular& s);
    
    // Remove a Singular element from the formation
    void removeElement(const Singular& s);

    // Other methods can be added as needed for functionality
    
    // Implement the pure virtual method from Singular
    std::string getIdentifier() const override { return "Formations"; }

private:
    std::vector<Singular*> members;
    RelationManager relationMgr;
};