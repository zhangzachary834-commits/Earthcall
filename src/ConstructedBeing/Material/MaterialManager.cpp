#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Material/MaterialResponseDefaults.hpp"
#include "ConstructedBeing/Material/MaterialPersistenceMigration.hpp"

using json = nlohmann::json;

namespace {
// Accept either a full identifier ("material.clay") or a bare name ("clay").
std::string normalizeName(const std::string& s) {
    const std::string prefix = "material.";
    if (s.rfind(prefix, 0) == 0) return s.substr(prefix.size());
    return s;
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

    // Gate B safety rail at the object boundary, not only JSON ingestion.
    // Shared Material roots and stored-Singular restoration may hand this
    // registry an already-deserialized Material directly. The canonical global
    // default may therefore be replaced only by explicit CURRENT authored
    // response state. A response-less or exact historical compatibility default
    // belongs to a document scope and must be transplanted to a named
    // compatibility Material before registry admission.
    if (m->name() == "default") {
        const bool exactLegacy =
            m->responseExpr &&
            m->responseExpr->toJson() ==
                MaterialPersistenceMigration::legacyDirectJson();
        if (!m->responseExpr || exactLegacy) {
            ensureDefault();
            return;
        }

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
    _materialIndexByName.clear();
    if (j.is_array()) {
        for (const auto& e : j) {
            // Gate B safety rail: a legacy saved default has document-local
            // appearance semantics. It must be scoped/remapped by the
            // persistence boundary before entering this global registry.
            if (MaterialPersistenceMigration::isLegacyDefaultRecord(e)) {
                continue;
            }
            // Snapshot replacement still passes through the same object-level
            // admission law as shared roots/stored Singulars. A current-marker
            // response-less default is not a valid canonical default either:
            // material.default cannot inherit itself.
            add(std::make_shared<Material>(Material::fromJson(e)));
        }
    }
    // The canonical fallback is always CURRENT. Old worlds retain historical
    // appearance through scoped explicit compatibility Materials, never by
    // changing what new response omission means globally.
    ensureDefault();
    rebuildIndex();
}

void MaterialManager::mergeFromJson(const json& j) {
    if (!j.is_array()) {
        ensureDefault();
        return;
    }
    for (const auto& e : j) {
        if (MaterialPersistenceMigration::isLegacyDefaultRecord(e)) {
            continue;
        }
        add(std::make_shared<Material>(Material::fromJson(e)));
    }
    ensureDefault();
    rebuildIndex();
}
