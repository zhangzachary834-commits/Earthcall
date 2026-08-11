#pragma once

#include "ConstructedBeing/Material/Material.hpp"
#include "json.hpp"

#include <memory>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Owns the Material beings, the way RelationManager owns Relations and
// ZoneManager owns Zones. Objects hold a material *identifier*; this manager
// resolves it to the being. There is always a "material.default" whose values
// reproduce the previous global shading, so an object with no material assigned
// still renders and never dangles.
// ---------------------------------------------------------------------------
class MaterialManager {
public:
    MaterialManager() { ensureDefault(); }

    // Create a named material (or return the existing one of that name).
    std::shared_ptr<Material> create(const std::string& name);
    void add(const std::shared_ptr<Material>& m);
    bool remove(const std::string& identifier); // never removes material.default

    // Resolve by full identifier ("material.clay") or bare name ("clay").
    // Returns nullptr if absent — callers that must draw use resolveOrDefault.
    std::shared_ptr<Material> get(const std::string& identifier) const;
    std::shared_ptr<Material> resolveOrDefault(const std::string& identifier) const;
    std::shared_ptr<Material> defaultMaterial() const { return get("material.default"); }

    const std::vector<std::shared_ptr<Material>>& getAll() const { return _materials; }

    nlohmann::json toJson() const;
    void loadFromJson(const nlohmann::json& j); // reinstates material.default afterward

private:
    void ensureDefault();
    std::vector<std::shared_ptr<Material>> _materials;
};
