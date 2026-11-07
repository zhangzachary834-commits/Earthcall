#include "ConstructedBeing/Material/MaterialManager.hpp"

using json = nlohmann::json;

namespace {
// Accept either a full identifier ("material.clay") or a bare name ("clay").
std::string normalizeName(const std::string& s) {
    const std::string prefix = "material.";
    if (s.rfind(prefix, 0) == 0) return s.substr(prefix.size());
    return s;
}
} // namespace

void MaterialManager::rebuildIndex() {
    _materialIndexByName.clear();
    for (size_t i = 0; i < _materials.size(); ++i) {
        if (_materials[i]) {
            _materialIndexByName[_materials[i]->name()] = i;
        }
    }
}

void MaterialManager::ensureDefault() {
    for (const auto& m : _materials)
        if (m && m->name() == "default") return;
    // Freshly constructed Material already carries the previous global constants.
    _materials.insert(_materials.begin(), std::make_shared<Material>("default"));
    rebuildIndex();
}

std::shared_ptr<Material> MaterialManager::create(const std::string& name) {
    std::string n = normalizeName(name);
    if (auto existing = get("material." + n)) return existing;
    auto m = std::make_shared<Material>(n);
    _materials.push_back(m);
    _materialIndexByName[n] = _materials.size() - 1;
    return m;
}

void MaterialManager::add(const std::shared_ptr<Material>& m) {
    if (!m) return;
    // Replace any material of the same identity rather than duplicate it.
    remove(m->getIdentifier());
    _materials.push_back(m);
    _materialIndexByName[m->name()] = _materials.size() - 1;
}

bool MaterialManager::remove(const std::string& identifier) {
    std::string n = normalizeName(identifier);
    if (n == "default") return false; // the fallback must always exist
    auto itIdx = _materialIndexByName.find(n);
    if (itIdx != _materialIndexByName.end()) {
        size_t idx = itIdx->second;
        if (idx < _materials.size() && _materials[idx] && _materials[idx]->name() == n) {
            _materials.erase(_materials.begin() + idx);
            rebuildIndex();
            return true;
        }
    }
    for (auto it = _materials.begin(); it != _materials.end(); ++it) {
        if (*it && (*it)->name() == n) {
            _materials.erase(it);
            rebuildIndex();
            return true;
        }
    }
    return false;
}

std::shared_ptr<Material> MaterialManager::get(const std::string& identifier) const {
    std::string n = normalizeName(identifier);
    auto it = _materialIndexByName.find(n);
    if (it != _materialIndexByName.end()) {
        const size_t idx = it->second;
        if (idx < _materials.size() && _materials[idx] && _materials[idx]->name() == n) {
            return _materials[idx];
        }
    }
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
    rebuildIndex();
}

void MaterialManager::mergeFromJson(const json& j) {
    if (!j.is_array()) {
        ensureDefault();
        return;
    }
    for (const auto& e : j)
        add(std::make_shared<Material>(Material::fromJson(e)));
    ensureDefault();
    rebuildIndex();
}
