#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SdfJson.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include <glm/glm.hpp>
#include "json.hpp"
#include <array>
#include <functional>
#include <memory>
#include <string>

namespace geom {

// An OntoMath Piecewise, addressable as the JSON it already serializes to.
// Reading gives the whole tree; writing replaces it, and a document that does
// not parse is refused outright so a field is never left half-rewritten.
//
// Writing an AST also selects AST evaluation. Without this transition a Person
// could successfully author field.ast while the Field remained Procedural; the
// authored tree would neither govern evaluation nor be emitted by Field::toJson.
// Keep those two pieces of state coherent at the authoring boundary.
template <typename FieldT>
class AstBridge : public Property {
public:
    explicit AstBridge(std::string name, FieldT* field,
                       std::function<void()> onWritten = {})
        : _name(std::move(name)), _nameId(Earthcall::StringInterner::intern(_name)),
          _field(field), _onWritten(std::move(onWritten)) {}

    std::string name() const override { return _name; }
    Earthcall::StringId nameId() const override { return _nameId; }
    std::string typeName() const override { return "string"; }

    PropertyValue value() const override {
        if (!_field) return PropertyValue(std::string("{}"));
        return PropertyValue(_field->astDefinition.toJson().dump());
    }
    bool setValue(const PropertyValue& v) override {
        if (!_field) return false;
        const std::string* src = std::get_if<std::string>(&v);
        if (!src) return false;
        nlohmann::json parsed = nlohmann::json::parse(*src, nullptr, false);
        if (parsed.is_discarded()) return false;   // malformed: refuse, keep the old AST and mode

        // An identical rewrite (a Law re-asserting the same tree each frame)
        // is not a change; bumping for it would make Screen re-inspect forever.
        if (_field->mode == FieldT::EvaluationMode::AST &&
            parsed == _field->astDefinition.toJson()) return true;
        _field->astDefinition = OntoMath::Piecewise::fromJson(parsed);
        _field->mode = FieldT::EvaluationMode::AST;
        if (_onWritten) _onWritten();
        return true;
    }

private:
    std::string _name;
    Earthcall::StringId _nameId;
    FieldT* _field;
    std::function<void()> _onWritten;
};

// An authored Piecewise that is semantically its own channel rather than a
// ScalarField/VectorField container. Rung 5 uses this for source chroma chi(p,t):
// vec3. The bridge keeps the recursive mathematics reachable by Law without
// pretending RGB is the existing flow/force VectorField merely because both are vec3.
class PiecewiseAstBridge : public Property {
public:
    PiecewiseAstBridge(std::string name, OntoMath::Piecewise* expr,
                       std::function<void()> onWritten = {})
        : _name(std::move(name)), _nameId(Earthcall::StringInterner::intern(_name)),
          _expr(expr), _onWritten(std::move(onWritten)) {}

    std::string name() const override { return _name; }
    Earthcall::StringId nameId() const override { return _nameId; }
    std::string typeName() const override { return "string"; }

    PropertyValue value() const override {
        if (!_expr) return PropertyValue(std::string("{}"));
        return PropertyValue(_expr->toJson().dump());
    }
    bool setValue(const PropertyValue& v) override {
        if (!_expr) return false;
        const std::string* src = std::get_if<std::string>(&v);
        if (!src) return false;
        nlohmann::json parsed = nlohmann::json::parse(*src, nullptr, false);
        if (parsed.is_discarded()) return false;
        if (parsed == _expr->toJson()) return true;   // identical rewrite: no change
        *_expr = OntoMath::Piecewise::fromJson(parsed);
        if (_onWritten) _onWritten();
        return true;
    }

private:
    std::string _name;
    Earthcall::StringId _nameId;
    OntoMath::Piecewise* _expr;
    std::function<void()> _onWritten;
};

class SdfNodeBridge : public Property {
public:
    SdfNodeBridge(std::string name, geom::SdfNode* node,
                  std::function<void()> onWritten = {})
        : _name(std::move(name)), _nameId(Earthcall::StringInterner::intern(_name)),
          _node(node), _onWritten(std::move(onWritten)) {}

    std::string name() const override { return _name; }
    Earthcall::StringId nameId() const override { return _nameId; }
    std::string typeName() const override { return "string"; }

    PropertyValue value() const override {
        if (!_node || !geom::isSdfActive(_node)) return PropertyValue(std::string("{}"));
        return PropertyValue(geom::sdfToJson(*_node).dump());
    }
    bool setValue(const PropertyValue& v) override {
        if (!_node) return false;
        const std::string* src = std::get_if<std::string>(&v);
        if (!src) return false;
        nlohmann::json parsed = nlohmann::json::parse(*src, nullptr, false);
        if (parsed.is_discarded() || !parsed.is_object()) return false;
        if (geom::isSdfActive(_node) && parsed == geom::sdfToJson(*_node)) return true;
        *_node = geom::sdfFromJson(parsed);
        if (_onWritten) _onWritten();
        return true;
    }

private:
    std::string _name;
    Earthcall::StringId _nameId;
    geom::SdfNode* _node;
    std::function<void()> _onWritten;
};

// A FieldNode represents the spatial placement of an OntoMath Field within the scene.
// By inheriting from Singular, it maps the field's mathematical variables into the 
// PropertyPath system, allowing the Law system to modulate the field dynamically.
class FieldNode : public Singular {
private:
    std::string _id;

public:
    FieldNode(std::string id = "field_node") 
        : _id(std::move(id)), 
          field(std::make_shared<OntoMath::ScalarField>()),
          vectorField(std::make_shared<OntoMath::VectorField>()),
          volumeDensity(std::make_shared<OntoMath::Piecewise>()),
          volumeExtinction(std::make_shared<OntoMath::Piecewise>()),
          volumeScattering(std::make_shared<OntoMath::Piecewise>()),
          volumeChroma(std::make_shared<OntoMath::Piecewise>()),
          volumePhase(std::make_shared<OntoMath::Piecewise>()),
          volumeEmission(std::make_shared<OntoMath::Piecewise>()),
          volumeOccluder(std::make_shared<geom::SdfNode>()),
          lightChroma(std::make_shared<OntoMath::Piecewise>()),
          lightAngular(std::make_shared<OntoMath::Piecewise>()) {
        volumeOccluder->dims = glm::vec3(0.0f);
        noteAuthoredMathWritten();
    }

    std::string getIdentifier() const override { return _id; }

    // Spatial transform
    glm::vec3 origin{0.0f};
    glm::vec3 scale{1.0f};

    // The pure mathematical field definition
    // Const pointer ensures the property registry doesn't dangle
    const std::shared_ptr<OntoMath::ScalarField> field;
    const std::shared_ptr<OntoMath::VectorField> vectorField;

    // V0 participating-medium density D(p,t) -> scalar. This is deliberately
    // independent from the generic scalar field and from source radiance rho.
    // Empty means no explicitly authored volume-density channel; compatibility
    // migration, where required, is resolved outside this storage boundary.
    const std::shared_ptr<OntoMath::Piecewise> volumeDensity;

    // V1 participating-medium extinction sigma_t(p,t) -> scalar. Empty means
    // compatibility extinction (0.5 * D) rather than absence of the medium.
    // This channel is authored independently from D: equal density fields may
    // intentionally transmit light very differently.
    const std::shared_ptr<OntoMath::Piecewise> volumeExtinction;

    // V2 participating-medium scattering sigma_s(p,t) -> scalar. Empty means
    // exact compatibility sigma_s=D; presence is sole scattering authority.
    const std::shared_ptr<OntoMath::Piecewise> volumeScattering;

    // V2 participating-medium chroma C_v(p,t) -> vec3. Empty means neutral
    // white compatibility. This is medium truth, not source/light chroma.
    const std::shared_ptr<OntoMath::Piecewise> volumeChroma;

    // V3 participating-medium phase Phi(p,wi,wo,t) -> scalar. Empty means the
    // exact V2 compatibility identity Phi=1. This is medium angular-scattering
    // truth and never aliases source angular emission.
    const std::shared_ptr<OntoMath::Piecewise> volumePhase;

    // V4 participating-medium emission E_v(p,omega,t) -> vec3. Empty means
    // exact pre-V4 compatibility: the medium contributes no self-emitted
    // radiance. This is independent from D, sigma_t, sigma_s, C_v, Phi and
    // every source-side rho/chi/alpha channel.
    const std::shared_ptr<OntoMath::Piecewise> volumeEmission;

    // Optional participating-medium occluder geometry S(p) -> signed distance.
    // When present, volumetric transport evaluates path visibility between the
    // medium sample and the radiant source, carving radiance into volumetric beams.
    const std::shared_ptr<geom::SdfNode> volumeOccluder;

    // Optional source-side chroma chi(p,t) -> vec3. Empty means ABSENT, in which
    // case the historical authored light.color remains the constant chroma.
    // This is deliberately not VectorField: that existing vessel means flow/force.
    const std::shared_ptr<OntoMath::Piecewise> lightChroma;

    // Optional source-side angular factor alpha(p,omega,t) -> scalar. Empty is
    // exactly the multiplicative identity alpha=1. Direction is bound by the
    // consuming radiance channel, never stored here as a renderer preset.
    const std::shared_ptr<OntoMath::Piecewise> lightAngular;

    nlohmann::json toJson() const;
    void applyJson(const nlohmann::json& j);
    static std::shared_ptr<FieldNode> fromJson(const nlohmann::json& j);

    // Authored-math revision: how Screen tells that any of this node's authored
    // expressions (field.ast, the seven volume channels, light chroma/angular)
    // changed, without serializing them. Every rendered frame used to dump all
    // of them to JSON and hash the text -- 24 dumps, ~7 ms/frame in Northern
    // Veil -- to answer a question whose answer is almost always "no".
    //
    // The value comes from one process-wide sequence that never repeats, so a
    // node rebuilt at a recycled address can never present a revision some
    // renderer cache has already seen (the SourceRho producer-rebinding hazard).
    //
    // Writers: the property bridges and applyJson() call
    // noteAuthoredMathWritten() themselves. Anything that assigns a channel in
    // place (the MCP author_volume path does) must call it too. If one is ever
    // missed, verifiedAuthoredMathRevision() still catches it: each call
    // re-hashes ONE channel's content round-robin and, on an unrevisioned
    // change, bumps the revision and says so on stderr. A missed writer is a
    // bug that heals within a few frames; it can never stay stale silently.
    // Claude Opus 5.5, 2026-10-09 -- Zach asked for the serialization gone.
    uint64_t authoredMathRevision() const { return _authoredMathRevision; }
    uint64_t verifiedAuthoredMathRevision() const;
    void noteAuthoredMathWritten();

private:
    // BENEATH THE KERNEL: change-detection bookkeeping for the Screen channel's
    // caches. Not the being's state -- the machine's way of noticing that state
    // moved. A Person can mean nothing by a sequence number; the authored
    // mathematics it tracks is registered above as field.ast / volume.*.ast /
    // volume.occluder.sdf / light.*.ast.
    static constexpr std::size_t kAuthoredMathChannels = 10;
    mutable uint64_t _authoredMathRevision = 0;   // mutable: the verifier may heal it
    mutable std::size_t _verifyCursor = 0;
    mutable std::array<uint64_t, kAuthoredMathChannels> _verifiedContentHash{};
    mutable std::array<uint64_t, kAuthoredMathChannels> _verifiedAtRevision{};

protected:
    void buildProperties() override {
        // Expose spatial transform
        registerProperty(std::make_unique<PropertyRef<FieldNode, glm::vec3>>("origin", this, &FieldNode::origin));
        registerProperty(std::make_unique<PropertyRef<FieldNode, glm::vec3>>("scale", this, &FieldNode::scale));

        // Expose mathematical configuration from the underlying OntoMath field
        if (field) {
            registerProperty(std::make_unique<PropertyRef<OntoMath::ScalarField, float>>("field.baseDensity", field.get(), &OntoMath::ScalarField::baseDensity));
            registerProperty(std::make_unique<PropertyRef<OntoMath::ScalarField, float>>("field.frequency", field.get(), &OntoMath::ScalarField::frequency));
            registerProperty(std::make_unique<PropertyRef<OntoMath::ScalarField, float>>("field.amplitude", field.get(), &OntoMath::ScalarField::amplitude));
            
            // The AST itself, as its own serialized form. It used to reach no
            // property path at all -- the note here said the Law system could
            // rewrite it "via specialized OntoMath endpoints or over the
            // network", which is to say: not by law. That is the black box
            // refusal #6 forbids, and the mathematics of a field is exactly
            // the state a Person most needs to reach.
            //
            // A Piecewise is a recursive tree, so it is exposed the way the
            // rest of the engine exposes recursive state -- as the JSON it
            // already round-trips through. Readable in full; writable, with a
            // malformed document REFUSED rather than half-applied.
            registerProperty(std::make_unique<AstBridge<OntoMath::ScalarField>>(
                "field.ast", field.get(),
                [this] { noteAuthoredMathWritten(); }));
        }

        if (vectorField) {
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.baseFlowX", vectorField.get(), &OntoMath::VectorField::baseFlowX));
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.baseFlowY", vectorField.get(), &OntoMath::VectorField::baseFlowY));
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.baseFlowZ", vectorField.get(), &OntoMath::VectorField::baseFlowZ));
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.frequency", vectorField.get(), &OntoMath::VectorField::frequency));
            registerProperty(std::make_unique<PropertyRef<OntoMath::VectorField, float>>("vectorField.amplitude", vectorField.get(), &OntoMath::VectorField::amplitude));
            registerProperty(std::make_unique<AstBridge<OntoMath::VectorField>>(
                "vectorField.ast", vectorField.get(),
                [this] { noteAuthoredMathWritten(); }));
        }

        if (volumeDensity) {
            registerProperty(std::make_unique<PiecewiseAstBridge>(
                "volume.density.ast", volumeDensity.get(),
                [this] { noteAuthoredMathWritten(); }));
        }
        if (volumeExtinction) {
            registerProperty(std::make_unique<PiecewiseAstBridge>(
                "volume.extinction.ast", volumeExtinction.get(),
                [this] { noteAuthoredMathWritten(); }));
        }
        if (volumeScattering) {
            registerProperty(std::make_unique<PiecewiseAstBridge>(
                "volume.scattering.ast", volumeScattering.get(),
                [this] { noteAuthoredMathWritten(); }));
        }
        if (volumeChroma) {
            registerProperty(std::make_unique<PiecewiseAstBridge>(
                "volume.chroma.ast", volumeChroma.get(),
                [this] { noteAuthoredMathWritten(); }));
        }
        if (volumePhase) {
            registerProperty(std::make_unique<PiecewiseAstBridge>(
                "volume.phase.ast", volumePhase.get(),
                [this] { noteAuthoredMathWritten(); }));
        }
        if (volumeEmission) {
            registerProperty(std::make_unique<PiecewiseAstBridge>(
                "volume.emission.ast", volumeEmission.get(),
                [this] { noteAuthoredMathWritten(); }));
        }
        if (volumeOccluder) {
            registerProperty(std::make_unique<SdfNodeBridge>(
                "volume.occluder.sdf", volumeOccluder.get(),
                [this] { noteAuthoredMathWritten(); }));
        }

        if (lightChroma) {
            registerProperty(std::make_unique<PiecewiseAstBridge>(
                "light.chroma.ast", lightChroma.get(),
                [this] { noteAuthoredMathWritten(); }));
        }
        if (lightAngular) {
            registerProperty(std::make_unique<PiecewiseAstBridge>(
                "light.angular.ast", lightAngular.get(),
                [this] { noteAuthoredMathWritten(); }));
        }
    }
};

} // namespace geom
