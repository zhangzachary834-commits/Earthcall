#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Material/MaterialResponseDefaults.hpp"

using json = nlohmann::json;

namespace {
// Accept either a full identifier ("material.clay") or a bare name ("clay").
std::string normalizeName(const std::string& s) {
    const std::string prefix = "material.";
    if (s.rfind(prefix, 0) == 0) return s.substr(prefix.size());
    return s;
}

void seedLegacyCompatibilityIfUnspecified(Material& material) {
    const bool hadDirectResponse = bool(material.responseExpr);
    if (!material.responseExpr) {
        material.responseExpr =
            MaterialResponseDefaults::makeLegacyDirectResponse();
        material.bumpResponseRevision();
    }
    if (!hadDirectResponse && !material.environmentResponseExpr) {
        material.environmentResponseExpr =
            MaterialResponseDefaults::makeLegacyEnvironmentResponse();
        material.bumpEnvironmentResponseRevision();
    }
}

void seedCurrentDefaultIfUnspecified(Material& material) {
    if (!material.responseExpr) {
        material.responseExpr =
            MaterialResponseDefaults::makeCurrentDefaultDirectResponse();
        material.bumpResponseRevision();
    }
    if (!material.environmentResponseExpr) {
        material.environmentResponseExpr =
            MaterialResponseDefaults::makeCurrentDefaultEnvironmentResponse();
        material.bumpEnvironmentResponseRevision();
    }
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
    for (const auto& m : _materials) {
        if (m && m->name() == "default") {
            return;
        }
    }
    auto material = std::make_shared<Material>("default");
    seedCurrentDefaultIfUnspecified(*material);
    _materials.insert(_materials.begin(), std::move(material));
    rebuildIndex();
}

void MaterialManager::ensureLegacyDefaultForSnapshot() {
    for (const auto& m : _materials) {
        if (m && m->name() == "default") return;
    }
    auto material = std::make_shared<Material>("default");
    seedLegacyCompatibilityIfUnspecified(*material);
    _materials.insert(_materials.begin(), std::move(material));
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
    // material.default is externally undeletable, but merge/upsert must still
    // replace its state rather than create two beings with one identity.
    if (m->name() == "default") {
        auto it = _materialIndexByName.find("default");
        if (it != _materialIndexByName.end() && it->second < _materials.size()) {
            _materials[it->second] = m;
            return;
        }
    }
    // Replace any non-default material of the same identity rather than duplicate it.
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
            _materials.push_back(
                std::make_shared<Material>(Material::fromJson(e)));
    }
    // Snapshot restore with no persisted default predates the Gate B current
    // default contract. Preserve the old world's fallback appearance by
    // materializing explicit legacy response data.
    ensureLegacyDefaultForSnapshot();
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
