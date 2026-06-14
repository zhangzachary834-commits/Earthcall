#include <vector>
#include <list>

#include "Person/Person.hpp"
#include "Relation/Relation.hpp"
#include "Form/Object/Formation/Formation.hpp"
#include "Singular.hpp"

class Law extends Object, Relation {
    private:
        Formation* authors;
    public:
        Law(const std::string& name);
        Law(const std::string& name, const std::string& a, const std::string& b, bool directed = false, float weight = 1.0f);
        ~Law();

        void describe() const;
        bool involves(const std::string& entity) const;
        bool involves(const Singular& entity) const;
        bool isBetween(const std::string& a, const std::string& b) const;
        bool isBetween(const Singular& aEntity, const Singular& bEntity) const;
}
