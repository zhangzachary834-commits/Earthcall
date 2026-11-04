#include "ConstructedBeing/Material/MaterialManager.hpp"

using json = nlohmann::json;

namespace {
// Accept either a full identifier ("material.clay") or a bare name ("clay").
std::string normalizeName(const std::string& s) {
    const std::string prefix = "material.";
    if (s.rfind(prefix, 0) == 0) return s.substr(prefix.size());
    return s;
}

using Node = OntoMath::MathNode;
using Op = OntoMath::MathNode::Op;

std::unique_ptr<Node> value(const char* name) {
    auto n = std::make_unique<Node>();
    n->op = Op::ValueLeaf;
    n->variableName = name;
    return n;
}
std::unique_ptr<Node> bind(const char* path) {
    auto n = std::make_unique<Node>();
    n->op = Op::Bind;
    n->propertyPath = path;
    return n;
}
std::unique_ptr<Node> binary(Op op, std::unique_ptr<Node> a,
                             std::unique_ptr<Node> b) {
    auto n = std::make_unique<Node>();
    n->op = op;
    n->children.push_back(std::move(a));
    n->children.push_back(std::move(b));
    return n;
}

// Historical ambient receiver coefficient, likewise ordinary data. The
// environment incident vector already contains the compatibility source/chroma
// envelope; Material owns how that incident radiance becomes surface RGB.
std::shared_ptr<OntoMath::Piecewise> legacyEnvironmentResponse() {
    auto scaledEnvironment = binary(
        Op::Scale, bind("ambient"), value(OntoMath::kEnvironmentRadianceVar));
    auto reflected = binary(
        Op::Hadamard, value(OntoMath::kSurfaceColorVar),
        std::move(scaledEnvironment));
    return std::make_shared<OntoMath::Piecewise>(
        OntoMath::Piecewise::continuous(
            std::shared_ptr<Node>(reflected.release())));
}

void seedLegacyCompatibilityIfUnspecified(Material& material) {
    // A pre-Rung-9/default Material had neither receiver law. Environment
    // compatibility can migrate exactly because its historical source envelope
    // is already separated from rho/visibility. Direct Blinn-Phong activation
    // waits until source-side diffuse/specular receiver knobs are migrated
    // without changing inhabited worlds.
    if (material.responseExpr || material.environmentResponseExpr) return;
    material.environmentResponseExpr = legacyEnvironmentResponse();
    material.bumpEnvironmentResponseRevision();
}
} // namespace

void MaterialManager::ensureDefault() {
    for (const auto& m : _materials) {
        if (m && m->name() == "default") {
            seedLegacyCompatibilityIfUnspecified(*m);
            return;
        }
    }
    auto material = std::make_shared<Material>("default");
    seedLegacyCompatibilityIfUnspecified(*material);
    _materials.insert(_materials.begin(), std::move(material));
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

void MaterialManager::mergeFromJson(const json& j) {
    if (!j.is_array()) {
        ensureDefault();
        return;
    }
    for (const auto& e : j)
        add(std::make_shared<Material>(Material::fromJson(e)));
    ensureDefault();
}
