#pragma once

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "json.hpp"

#include <memory>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Owns Category beings, which are authored extra-spatial Objects.
// They act as templates or classifications. 
// For example, "category.tool.brush" is a category.
// ---------------------------------------------------------------------------
class CategoryManager {
public:
    CategoryManager() { ensureDefaults(); }

    // Create a named category (or return the existing one of that name).
    std::shared_ptr<Object> create(const std::string& name);
    void add(const std::shared_ptr<Object>& cat);
    bool remove(const std::string& identifier); 

    // Resolve by full identifier
    std::shared_ptr<Object> get(const std::string& identifier) const;
    std::shared_ptr<Object> resolveOrDefault(const std::string& identifier) const;

    const std::vector<std::shared_ptr<Object>>& getAll() const { return _categories; }

    nlohmann::json toJson() const;
    void loadFromJson(const nlohmann::json& j); 

private:
    void ensureDefaults();
    std::vector<std::shared_ptr<Object>> _categories;
};
