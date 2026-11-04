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

std::unique_ptr<Node> number(double value) {
    auto n = std::make_unique<Node>();
    n->op = Op::ScalarLeaf;
    n->scalarForm = OntoMath::ScalarForm::constant(value);
    return n;
}
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
std::unique_ptr<Node> unary(Op op, std::unique_ptr<Node> a) {
    auto n = std::make_unique<Node>();
    n->op = op;
    n->children.push_back(std::move(a));
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
std::unique_ptr<Node> ternary(Op op, std::unique_ptr<Node> a,
                              std::unique_ptr<Node> b,
                              std::unique_ptr<Node> c) {
    auto n = std::make_unique<Node>();
    n->op = op;
    n->children.push_back(std::move(a));
    n->children.push_back(std::move(b));
    n->children.push_back(std::move(c));
    return n;
}
std::unique_ptr<Node> clone(const Node& n) {
    return std::make_unique<Node>(n);
}
std::unique_ptr<Node> vector3(std::unique_ptr<Node> x,
                              std::unique_ptr<Node> y,
                              std::unique_ptr<Node> z) {
    return ternary(Op::VectorConstruct, std::move(x), std::move(y), std::move(z));
}
std::unique_ptr<Node> direction(const char* x, const char* y, const char* z) {
    return vector3(value(x), value(y), value(z));
}

// Historical direct Blinn-Phong, now ordinary authored mathematics. There is
// deliberately no BlinnPhong/BRDF operation: only generic OntoMath plus Bind.
std::shared_ptr<OntoMath::Piecewise> legacyDirectResponse() {
    auto wi = direction(OntoMath::kWiXVar, OntoMath::kWiYVar, OntoMath::kWiZVar);
    auto lightDir = binary(Op::Scale, number(-1.0), std::move(wi));

    auto diffuseCosine = ternary(
        Op::Clamp,
        binary(Op::Dot, value(OntoMath::kSurfaceNormalVar), clone(*lightDir)),
        number(0.0), number(1.0));

    auto diffuseScale = binary(
        Op::Scale, bind("diffuse"), clone(*diffuseCosine));
    auto diffuseRgb = binary(
        Op::Scale, value(OntoMath::kSurfaceColorVar), std::move(diffuseScale));

    auto wo = direction(OntoMath::kWoXVar, OntoMath::kWoYVar, OntoMath::kWoZVar);
    auto halfVector = unary(
        Op::Normalize,
        binary(Op::Add, clone(*lightDir), std::move(wo)));
    auto specularCosine = ternary(
        Op::Clamp,
        binary(Op::Dot, value(OntoMath::kSurfaceNormalVar), std::move(halfVector)),
        number(0.0), number(1.0));
    auto specularPower = binary(
        Op::Pow, std::move(specularCosine), bind("shininess"));
    auto specularShape = binary(
        Op::Scale, bind("specular"), std::move(specularPower));

    auto specularRgb = vector3(
        clone(*specularShape), clone(*specularShape), std::move(specularShape));
    auto lit = binary(Op::Add, clone(*diffuseRgb), std::move(specularRgb));

    auto gate = binary(
        Op::Sub, number(0.0001), clone(*diffuseCosine));

    auto out = std::make_shared<OntoMath::Piecewise>();
    OntoMath::Piecewise::Piece front;
    front.whereLEZero = std::shared_ptr<Node>(gate.release());
    front.mathNode = std::shared_ptr<Node>(lit.release());
    out->pieces.push_back(std::move(front));

    OntoMath::Piecewise::Piece fallback;
    fallback.mathNode = std::shared_ptr<Node>(diffuseRgb.release());
    out->pieces.push_back(std::move(fallback));
    return out;
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
    // Both absent identifies the pre-authorable default. A partially authored
    // pair is intentional state; never smuggle the compatibility half back in.
    if (material.responseExpr || material.environmentResponseExpr) return;
    material.responseExpr = legacyDirectResponse();
    material.environmentResponseExpr = legacyEnvironmentResponse();
    material.bumpResponseRevision();
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
