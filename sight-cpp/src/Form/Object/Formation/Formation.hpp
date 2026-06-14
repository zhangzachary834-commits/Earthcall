#pragma once

#include <vector>
#include <algorithm>
#include <memory>
#include "Form/Form.hpp"
#include <glm/glm.hpp>
#include "Relation/RelationManager.hpp"
#include "Singular.hpp"


class Formation : public Form, public Singular {

public:
    // Constructor
    Formation(const std::vector<Singular*>& members);
    Formation(const std::vector<Singular*>& members, const glm::vec3& dims);
    Formation(const std::vector<Singular*>& members, Form::ShapeType type, const glm::vec3& dims);

    // Legacy constructor for fallback
    Formation(Form::ShapeType type, const glm::vec3& dims = {1.0f,1.0f,1.0f}) : Form(type, dims) {}
    
    // Destructor
    ~Formation() = default;

    // ------ Generic membership helpers (any Singular) ------------
    void addMember(Singular* s);
    bool hasMember(const Singular* s) const;
    Singular* findMemberByIdentifier(const std::string& identifier) const;

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
    void addRelation(const std::shared_ptr<Relation>& r);
    bool removeRelation(const std::shared_ptr<Relation>& r);
    
    // Add a relation directly (alias for addRelation)
    void add(const std::shared_ptr<Relation>& r) { addRelation(r); }
    
    // Build a simple fully-connected graph between all objects currently
    // in this formation (undirected, weight 1.0, type="member")
    void rebuildCompleteGraph();
    void applyAttachmentRelations();
    const std::vector<std::shared_ptr<Formation>>& getSubformations() const { return subformations; }
    std::string getRelationTypeTag() const { return relationTypeTag; }
    void setRelationTypeTag(const std::string& type) { relationTypeTag = type; }

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
    std::string getIdentifier() const override { return "Formation"; }

private:
    void integrateRelationTopology(const std::shared_ptr<Relation>& r);
    std::shared_ptr<Formation> findOrCreateRelationFormation(const std::shared_ptr<Relation>& r);

    std::vector<Singular*> members;
    RelationManager relationMgr;
    std::vector<std::shared_ptr<Formation>> subformations;
    std::string relationTypeTag;
};
