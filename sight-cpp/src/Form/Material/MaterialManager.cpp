#include "Form/Material/MaterialManager.hpp"

using json = nlohmann::json;

namespace {
// Accept either a full identifier ("material.clay") or a bare name ("clay").
std::string normalizeName(const std::string& s) {
    const std::string prefix = "material.";
    if (s.rfind(prefix, 0) == 0) return s.substr(prefix.size());
    return s;
}
} // namespace

void MaterialManager::ensureDefault() {
    for (const auto& m : _materials)
        if (m && m->name() == "default") return;
    // Freshly constructed Material already carries the previous global constants.
    _materials.insert(_materials.begin(), std::make_shared<Material>("default"));
}

std::shared_ptr<Material> MaterialManager::create(const std::string& name) {
    std::string n = normalizeName(name);
    if (auto existing = get("material." + n)) return existing;
    auto m = std::make_shared<Material>(n);
    _materials.push_back(m);
    return m;
}

void MaterialManager::add(const std::shared_ptr<Material>& m) {
    if (!m) return;
    // Replace any material of the same identity rather than duplicate it.
    remove(m->getIdentifier());
    _materials.push_back(m);
}

bool MaterialManager::remove(const std::string& identifier) {
    std::string n = normalizeName(identifier);
    if (n == "default") return false; // the fallback must always exist
    for (auto it = _materials.begin(); it != _materials.end(); ++it) {
        if (*it && (*it)->name() == n) { _materials.erase(it); return true; }
    }
    return false;
}

std::shared_ptr<Material> MaterialManager::get(const std::string& identifier) const {
    std::string n = normalizeName(identifier);
    for (const auto& m : _materials)
        if (m && m->name() == n) return m;
    return nullptr;
}

std::shared_ptr<Material> MaterialManager::resolveOrDefault(const std::string& identifier) const {
    if (auto m = get(identifier)) return m;
    return get("material.default");
}

json MaterialManager::toJson() const {
    json arr = json::array();
    for (const auto& m : _materials)
        if (m) arr.push_back(m->toJson());
    return arr;
}

void MaterialManager::loadFromJson(const json& j) {
    _materials.clear();
    if (j.is_array()) {
        for (const auto& e : j)
            _materials.push_back(std::make_shared<Material>(Material::fromJson(e)));
    }
    ensureDefault(); // a save without a default (or an empty save) still resolves
}
