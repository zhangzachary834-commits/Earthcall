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

std::unique_ptr<Node> number(double v) {
    auto n = std::make_unique<Node>();
    n->op = Op::ScalarLeaf;
    n->scalarForm = OntoMath::ScalarForm::constant(v);
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
std::unique_ptr<Node> binary(Op op, std::unique_ptr<Node> a,
                             std::unique_ptr<Node> b) {
    auto n = std::make_unique<Node>();
    n->op = op;
    n->children.push_back(std::move(a));
    n->children.push_back(std::move(b));
    return n;
}
std::unique_ptr<Node> unary(Op op, std::unique_ptr<Node> a) {
    auto n = std::make_unique<Node>();
    n->op = op;
    n->children.push_back(std::move(a));
    return n;
}
std::unique_ptr<Node> vector3(std::unique_ptr<Node> x,
                              std::unique_ptr<Node> y,
                              std::unique_ptr<Node> z) {
    auto n = std::make_unique<Node>();
    n->op = Op::VectorConstruct;
    n->children.push_back(std::move(x));
    n->children.push_back(std::move(y));
    n->children.push_back(std::move(z));
    return n;
}
std::unique_ptr<Node> direction(const char* x, const char* y, const char* z) {
    return vector3(value(x), value(y), value(z));
}
std::unique_ptr<Node> clone(const Node& n) {
    return std::make_unique<Node>(n);
}
std::unique_ptr<Node> maxNode(std::unique_ptr<Node> a,
                              std::unique_ptr<Node> b) {
    // Existing OntoMath Intersection is the generic component/scalar max
    // operation in CPU and WGSL lowering; no Blinn-Phong-specific opcode.
    return binary(Op::Intersection, std::move(a), std::move(b));
}

// Historical direct Blinn-Phong is compatibility DATA, not renderer law.
// wi follows Earthcall transport convention source -> receiver, so the
// traditional receiver -> source light direction is -wi.
std::shared_ptr<OntoMath::Piecewise> legacyDirectResponse() {
    auto lightDir = binary(
        Op::Scale, number(-1.0),
        direction(OntoMath::kWiXVar, OntoMath::kWiYVar, OntoMath::kWiZVar));

    auto diffuseCosine = maxNode(
        binary(Op::Dot, value(OntoMath::kSurfaceNormalVar), clone(*lightDir)),
        number(0.0));

    auto diffuseScalar = binary(
        Op::Scale, bind("diffuse"), clone(*diffuseCosine));
    auto diffuseEnvelope = binary(
        Op::Scale, std::move(diffuseScalar),
        value(OntoMath::kLegacyDiffuseIncidentVar));
    auto diffuseRgb = binary(
        Op::Hadamard, value(OntoMath::kSurfaceColorVar),
        std::move(diffuseEnvelope));

    auto halfVector = unary(
        Op::Normalize,
        binary(Op::Add,
               std::move(lightDir),
               direction(OntoMath::kWoXVar, OntoMath::kWoYVar,
                         OntoMath::kWoZVar)));
    auto specularCosine = maxNode(
        binary(Op::Dot, value(OntoMath::kSurfaceNormalVar),
               std::move(halfVector)),
        number(0.0));
    auto specularExponent = maxNode(bind("shininess"), number(1.0));
    auto specularPower = binary(
        Op::Pow, std::move(specularCosine), std::move(specularExponent));
    auto specularScalar = binary(
        Op::Scale, bind("specular"), std::move(specularPower));
    auto specularRgb = binary(
        Op::Scale, std::move(specularScalar),
        value(OntoMath::kLegacySpecularIncidentVar));

    auto lit = binary(Op::Add, clone(*diffuseRgb), std::move(specularRgb));
    auto specularGate = binary(
        Op::Sub, number(0.0001), clone(*diffuseCosine));

    auto out = std::make_shared<OntoMath::Piecewise>();
    OntoMath::Piecewise::Piece front;
    front.whereLEZero = std::shared_ptr<Node>(specularGate.release());
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
    // Absence means the historical compatibility law, now expressed as data.
    // A Person-authored direct response remains sovereign. If direct response
    // was already authored, do not smuggle legacy environment reflection in.
    const bool hadDirectResponse = bool(material.responseExpr);
    if (!material.responseExpr) {
        material.responseExpr = legacyDirectResponse();
        material.bumpResponseRevision();
    }
    if (!hadDirectResponse && !material.environmentResponseExpr) {
        material.environmentResponseExpr = legacyEnvironmentResponse();
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
            seedLegacyCompatibilityIfUnspecified(*m);
            return;
        }
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
