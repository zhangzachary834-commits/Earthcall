#pragma once

#include "Singularity/Core/EventBus.hpp"
#include "Form/Object/Formation/Formation.hpp"
#include "Form/Object/Object.hpp"
#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "Form/Singular/Singular.hpp"
#include "ECA.hpp"
#include "ConditionModel.hpp"
#include "ActionModel.hpp"
#include "json.hpp"

#include <ctime>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// This class should eventually govern Physics. Physics should be extended from
// this class.
//
// Law is identity (an Object); its condition/action models are its essence; the
// compiled ECA closures are its manifestation. Hard-coded closures are for first
// movers only — Person-authored laws arrive as serializable models (see
// LAW_AND_CREATION_SYSTEM.md, Stage 2). The relational aspect of a Law (its
// provenance, its Formation of conditions) is carried by composition rather than
// by also inheriting Relation: Object and Relation are both Singulars, and
// Earthcall models Relation-Objects with member formations, not a diamond.
class Law : public Object {
public:
    // Condition/action vocabulary is the shared ECA language (ECA.hpp), so a
    // Law's pieces are interchangeable with any other event-condition-action
    // carrier in the system.
    using ConditionMode = ECA::ConditionMode;
    using ConditionPredicate = ECA::ConditionPredicate;
    using Condition = ECA::Condition;
    using Action = ECA::Action;

    enum class ApplicationResult {
        Applied,
        Disabled,
        Unauthored,
        NoTarget,
        ConditionsFailed,
        NoAction
    };

    struct ApplicationRecord {
        std::time_t timestamp{0};
        std::string lawId;
        std::string targetId;
        ApplicationResult result{ApplicationResult::NoTarget};
        std::vector<std::string> conditionDescriptions;
        std::vector<std::string> actionDescriptions;

        nlohmann::json toJson() const;
    };

    struct AppliedEvent {
        const Law* law{nullptr};
        Singular* target{nullptr};
        ApplicationResult result{ApplicationResult::NoTarget};
        std::time_t timestamp{0};
    };

    explicit Law(const std::string& name = "Law");
    Law(const std::string& name, const std::vector<Singular*>& authors);

    std::string getIdentifier() const override { return _lawId; }

    const std::string& name() const { return _name; }
    void setName(const std::string& name);

    bool isEnabled() const { return _enabled; }
    void setEnabled(bool enabled) { _enabled = enabled; }

    ConditionMode conditionMode() const { return _conditionMode; }
    void setConditionMode(ConditionMode mode) { _conditionMode = mode; }

    Formation& authors() { return _authors; }
    const Formation& authors() const { return _authors; }
    void addAuthor(Singular& author);
    void setAuthors(const std::vector<Singular*>& authors);
    bool isAuthored() const { return !_authors.getMembers().empty(); }

    Formation& conditions() { return _conditions; }
    const Formation& conditions() const { return _conditions; }
    void addConditionSubject(Singular& subject);
    void addConditionRelation(const std::shared_ptr<Relation>& relation);
    void clearConditionFormation();

    Formation& targets() { return _targets; }
    const Formation& targets() const { return _targets; }
    void addTarget(Singular& target);
    void clearTargets();

    RelationManager& provenance() { return _provenance; }
    const RelationManager& provenance() const { return _provenance; }
    std::shared_ptr<Relation> recordProvenance(const std::string& type,
                                               const Singular& a,
                                               const Singular& b,
                                               bool directed = true,
                                               float weight = 1.0f);

    // First-mover lane: register a hard-coded condition/action closure directly.
    // Person-authored laws will instead carry ConditionModel/ActionModel trees
    // that compile into these same slots (Stage 2).
    void addCondition(const std::string& description,
                      ConditionPredicate predicate,
                      bool required = true);
    void clearConditions();
    bool conditionsSatisfied(const Singular& target) const;

    void addAction(const std::string& description, ECA::ActionExecutor action);
    void clearActions();

    // ------------------------------------------------------------------
    // The law's text (Stage 2): serializable condition/action models.
    // When present, recompile() derives the executable ECA slots from them —
    // the tree is primary, the closure is derived. This is what lets a
    // Person-authored law survive save/load, be synthesized, and be governed.
    // ------------------------------------------------------------------
    bool hasConditionModel() const { return _conditionModel.has_value(); }
    bool hasActionModel() const { return _actionModel.has_value(); }
    const ConditionModel* conditionModel() const {
        return _conditionModel ? &*_conditionModel : nullptr;
    }
    const ActionModel* actionModel() const {
        return _actionModel ? &*_actionModel : nullptr;
    }
    void setConditionModel(ConditionModel model);
    void setActionModel(ActionModel model);
    void recompile();

    // Restore identity + behavior from toJson() output. Authors and targets
    // are world references (identities) — the loader reattaches them; a law
    // without authors stays Unauthored and cannot fire.
    static std::shared_ptr<Law> fromJson(const nlohmann::json& j);

    // Event-typed carrier for this law's ECA loop (which event kind wakes it).
    // Wired to the bus/Rete loop in Stage 3.
    const ECA::Loop& ecaLoop() const { return _ecaLoop; }
    ECA::Loop& ecaLoop() { return _ecaLoop; }

    // Create algorithm to use ordinary tools to create conditions (projection
    // mode — the shape tools emit ConditionNodes instead of world objects).

    ApplicationResult applyTo(Singular& target);
    std::vector<ApplicationRecord> applyToTargets();

    const std::vector<ApplicationRecord>& applicationLog() const { return _applicationLog; }
    nlohmann::json toJson() const;

    static const char* resultName(ApplicationResult result);

private:
    void initializeLawIdentity();
    ApplicationRecord makeRecord(Singular* target, ApplicationResult result) const;
    void publishAppliedEvent(Singular* target, ApplicationResult result) const;

    std::string _lawId;
    std::string _name;
    bool _enabled = true;
    ConditionMode _conditionMode = ConditionMode::All;

    Formation _authors{Form::ShapeType::Cube, glm::vec3(1.0f)};
    Formation _conditions{Form::ShapeType::Cube, glm::vec3(1.0f)};
    Formation _targets{Form::ShapeType::Cube, glm::vec3(1.0f)};
    RelationManager _provenance;

    ECA::Loop _ecaLoop;
    std::optional<ConditionModel> _conditionModel;
    std::optional<ActionModel> _actionModel;
    std::vector<Condition> _conditionPredicates;
    std::vector<Action> _actions;
    std::vector<ApplicationRecord> _applicationLog;
};

struct LawRegisteredEvent {
    std::shared_ptr<const Law> law;
    std::time_t timestamp{0};
};

struct ReteFact {
    std::string id;
    std::string type;
    std::string subjectId;
    std::string attribute;
    nlohmann::json value;
    Singular* subject{nullptr};
};

struct ReteToken {
    std::vector<ReteFact> facts;
    std::unordered_map<std::string, std::string> bindings;
};

struct ReteActivation {
    std::string lawId;
    ReteToken token;
    std::time_t timestamp{0};
};

class ReteNetwork {
public:
    using AlphaPredicate = std::function<bool(const ReteFact&)>;
    using BetaJoin = std::function<bool(const ReteToken&, const ReteFact&)>;

    struct AlphaNode {
        std::size_t id{0};
        std::string description;
        AlphaPredicate predicate;
        std::vector<ReteFact> memory;
    };

    struct BetaNode {
        std::size_t id{0};
        std::string description;
        std::size_t leftAlphaId{0};
        std::size_t rightAlphaId{0};
        BetaJoin join;
        std::vector<ReteToken> memory;
    };

    std::string assertFact(ReteFact fact);
    bool retractFact(const std::string& factId);
    void clearFacts();
    const std::vector<ReteFact>& facts() const { return _facts; }

    std::size_t addAlphaNode(const std::string& description, AlphaPredicate predicate);
    std::size_t addBetaNode(const std::string& description,
                            std::size_t leftAlphaId,
                            std::size_t rightAlphaId,
                            BetaJoin join = {});

    void bindLawToAlpha(const std::string& lawId, std::size_t alphaNodeId);
    void bindLawToBeta(const std::string& lawId, std::size_t betaNodeId);

    const std::vector<ReteActivation>& evaluate();
    std::vector<ReteActivation> drainAgenda();
    const std::vector<ReteActivation>& agenda() const { return _agenda; }

    nlohmann::json toJson() const;

private:
    const AlphaNode* findAlpha(std::size_t id) const;
    AlphaNode* findAlpha(std::size_t id);

    std::vector<ReteFact> _facts;
    std::vector<AlphaNode> _alphaNodes;
    std::vector<BetaNode> _betaNodes;
    std::vector<ReteActivation> _agenda;
    std::unordered_map<std::size_t, std::vector<std::string>> _alphaLawBindings;
    std::unordered_map<std::size_t, std::vector<std::string>> _betaLawBindings;
    std::size_t _nextAlphaId{1};
    std::size_t _nextBetaId{1};
};

class LawManager {
public:
    std::shared_ptr<Law> createLaw(const std::string& name,
                                   const std::vector<Singular*>& authors = {});
    void add(const std::shared_ptr<Law>& law);
    bool remove(const std::string& lawId);

    Law* find(const std::string& lawId) const;
    std::vector<std::shared_ptr<Law>> getByAuthor(const std::string& authorId) const;
    const std::vector<std::shared_ptr<Law>>& getAll() const { return _laws; }

    Formation& formation() { return _lawFormation; }
    const Formation& formation() const { return _lawFormation; }

    std::vector<Law::ApplicationRecord> applyAllTo(Singular& target);
    std::vector<Law::ApplicationRecord> applyAllToTargets();

    ReteNetwork& rete() { return _rete; }
    const ReteNetwork& rete() const { return _rete; }
    const std::vector<ReteActivation>& evaluateRete() { return _rete.evaluate(); }

    nlohmann::json toJson() const;

private:
    std::vector<std::shared_ptr<Law>> _laws;
    Formation _lawFormation{Form::ShapeType::Cube, glm::vec3(1.0f)};
    ReteNetwork _rete;
};
