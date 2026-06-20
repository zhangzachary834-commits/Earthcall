#pragma once

#include "Core/EventBus.hpp"
#include "Form/Object/Formation/Formation.hpp"
#include "Form/Object/Object.hpp"
#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "Form/Singular/Singular.hpp"
#include "json.hpp"

#include <ctime>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Law : public Object {
public:
    enum class ConditionMode { All, Any };
    enum class ApplicationResult {
        Applied,
        Disabled,
        Unauthored,
        NoTarget,
        ConditionsFailed,
        NoAction
    };

    using ConditionPredicate = std::function<bool(const Singular&)>;
    using Action = std::function<void(Singular&)>;

    struct Condition {
        std::string description;
        ConditionPredicate predicate;
        bool required = true;
    };

    struct ActionStep {
        std::string description;
        Action action;
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

    void addCondition(const std::string& description,
                      ConditionPredicate predicate,
                      bool required = true);
    void clearConditions();
    bool conditionsSatisfied(const Singular& target) const;

    void addAction(const std::string& description, Action action);
    void clearActions();

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

    std::vector<Condition> _conditionPredicates;
    std::vector<ActionStep> _actions;
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
