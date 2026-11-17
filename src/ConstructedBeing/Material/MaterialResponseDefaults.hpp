#pragma once

#include "Singularity/OntoMath/ScalarForm.hpp"

#include <memory>

namespace MaterialResponseDefaults {

using Node = OntoMath::MathNode;
using Op = OntoMath::MathNode::Op;

// Persistence metadata only. This key is not a Material ontology property and
// must never become a renderer branch.
inline constexpr const char* kOmissionSemanticsKey =
    "responseOmissionSemantics";
inline constexpr const char* kInheritDefaultV1 =
    "inherit-material-default-v1";

inline std::unique_ptr<Node> number(double v) {
    auto n = std::make_unique<Node>();
    n->op = Op::ScalarLeaf;
    n->scalarForm = OntoMath::ScalarForm::constant(v);
    return n;
}

inline std::unique_ptr<Node> value(const char* name) {
    auto n = std::make_unique<Node>();
    n->op = Op::ValueLeaf;
    n->variableName = name;
    return n;
}

inline std::unique_ptr<Node> bind(const char* path) {
    auto n = std::make_unique<Node>();
    n->op = Op::Bind;
    n->propertyPath = path;
    return n;
}

inline std::unique_ptr<Node> binary(
    Op op, std::unique_ptr<Node> a, std::unique_ptr<Node> b) {
    auto n = std::make_unique<Node>();
    n->op = op;
    n->children.push_back(std::move(a));
    n->children.push_back(std::move(b));
    return n;
}

inline std::unique_ptr<Node> unary(Op op, std::unique_ptr<Node> a) {
    auto n = std::make_unique<Node>();
    n->op = op;
    n->children.push_back(std::move(a));
    return n;
}

inline std::unique_ptr<Node> vector3(
    std::unique_ptr<Node> x,
    std::unique_ptr<Node> y,
    std::unique_ptr<Node> z) {
    auto n = std::make_unique<Node>();
    n->op = Op::VectorConstruct;
    n->children.push_back(std::move(x));
    n->children.push_back(std::move(y));
    n->children.push_back(std::move(z));
    return n;
}

inline std::unique_ptr<Node> direction(
    const char* x, const char* y, const char* z) {
    return vector3(value(x), value(y), value(z));
}

inline std::unique_ptr<Node> clone(const Node& n) {
    return std::make_unique<Node>(n);
}

inline std::unique_ptr<Node> maxNode(
    std::unique_ptr<Node> a, std::unique_ptr<Node> b) {
    // OntoMath Intersection is generic scalar/component max.
    return binary(Op::Intersection, std::move(a), std::move(b));
}

// Historical direct Blinn-Phong compatibility expressed entirely as authored
// OntoMath. This survives only as migration/compatibility DATA.
inline std::shared_ptr<OntoMath::Piecewise> makeLegacyDirectResponse() {
    auto lightDir = binary(
        Op::Scale, number(-1.0),
        direction(OntoMath::kWiXVar, OntoMath::kWiYVar, OntoMath::kWiZVar));

    auto diffuseCosine = maxNode(
        binary(Op::Dot, value(OntoMath::kSurfaceNormalVar), clone(*lightDir)),
        number(0.0));

    auto diffuseScalar =
        binary(Op::Scale, bind("diffuse"), clone(*diffuseCosine));
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
    auto specularPower =
        binary(Op::Pow, std::move(specularCosine), std::move(specularExponent));
    auto specularScalar =
        binary(Op::Scale, bind("specular"), std::move(specularPower));
    auto specularRgb = binary(
        Op::Scale, std::move(specularScalar),
        value(OntoMath::kLegacySpecularIncidentVar));

    auto lit = binary(Op::Add, clone(*diffuseRgb), std::move(specularRgb));
    auto specularGate =
        binary(Op::Sub, number(0.0001), clone(*diffuseCosine));

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

inline std::shared_ptr<OntoMath::Piecewise> makeLegacyEnvironmentResponse() {
    auto scaledEnvironment = binary(
        Op::Scale, bind("ambient"), value(OntoMath::kEnvironmentRadianceVar));
    auto reflected = binary(
        Op::Hadamard, value(OntoMath::kSurfaceColorVar),
        std::move(scaledEnvironment));
    return std::make_shared<OntoMath::Piecewise>(
        OntoMath::Piecewise::continuous(
            std::shared_ptr<Node>(reflected.release())));
}

// Gate B current default: restrained matte direct reflection. Earthcall's wi
// convention is source -> receiver, so receiver -> source is -wi.
inline std::shared_ptr<OntoMath::Piecewise> makeCurrentDefaultDirectResponse() {
    auto lightDir = binary(
        Op::Scale, number(-1.0),
        direction(OntoMath::kWiXVar, OntoMath::kWiYVar, OntoMath::kWiZVar));
    auto cosine = maxNode(
        binary(Op::Dot, value(OntoMath::kSurfaceNormalVar),
               std::move(lightDir)),
        number(0.0));
    auto coloredIncident = binary(
        Op::Hadamard, value(OntoMath::kSurfaceColorVar),
        value(OntoMath::kIncidentRadianceVar));
    auto reflected =
        binary(Op::Scale, std::move(cosine), std::move(coloredIncident));
    return std::make_shared<OntoMath::Piecewise>(
        OntoMath::Piecewise::continuous(
            std::shared_ptr<Node>(reflected.release())));
}

inline std::shared_ptr<OntoMath::Piecewise>
makeCurrentDefaultEnvironmentResponse() {
    auto reflected = binary(
        Op::Hadamard, value(OntoMath::kSurfaceColorVar),
        value(OntoMath::kEnvironmentRadianceVar));
    return std::make_shared<OntoMath::Piecewise>(
        OntoMath::Piecewise::continuous(
            std::shared_ptr<Node>(reflected.release())));
}

} // namespace MaterialResponseDefaults
