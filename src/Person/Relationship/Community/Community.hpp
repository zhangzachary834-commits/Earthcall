class Community extends Formations {

    private:
    public:
        Community(const std::string& name);
        ~Community();

        void describe() const;
        bool involves(const std::string& entity) const;
        bool involves(const Singular& entity) const;
        bool isBetween(const std::string& a, const std::string& b) const;
        bool isBetween(const Singular& aEntity, const Singular& bEntity) const;
}