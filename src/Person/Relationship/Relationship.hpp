
// A Relationship is a Relation between two Persons. 
class Relationship extends Relation {
    private:
    public:
        Relationship(const std::string& type, const std::string& a, const std::string& b, bool directed = false, float weight = 1.0f);
        Relationship(const std::string& type, const Singular& aEntity, const Singular& bEntity, bool directed = false, float weight = 1.0f);
        ~Relationship();

        void describe() const;
        bool involves(const std::string& entity) const;
        bool involves(const Singular& entity) const;
        bool isBetween(const std::string& a, const std::string& b) const;
        bool isBetween(const Singular& aEntity, const Singular& bEntity) const;
}