#include "Law.hpp"

#include <algorithm>
#include <atomic>

namespace {
std::vector<std::string> formationMemberIds(const Formation& formation) {
    std::vector<std::string> ids;
    for (const auto* member : formation.getMembers()) {
        if (member) ids.push_back(member->getIdentifier());
    }
    return ids;
}
}

nlohmann::json Law::ApplicationRecord::toJson() const {
    return nlohmann::json{
        {"timestamp", timestamp},
        {"lawId", lawId},
        {"targetId", targetId},
        {"result", Law::resultName(result)},
        {"conditions", conditionDescriptions},
        {"actions", actionDescriptions}
    };
}

Law::Law(const std::string& name)
    : _name(name) {
    initializeLawIdentity();
}

Law::Law(const std::string& name, const std::vector<Singular*>& authors)
    : _name(name) {
    initializeLawIdentity();
    setAuthors(authors);
}

void Law::initializeLawIdentity() {
    static std::atomic<unsigned long long> nextLawId{1};
    _lawId = "law-" + std::to_string(nextLawId.fetch_add(1));
    setObjectID(_lawId);
    setPhysicalObject(0);
}

void Law::setName(const std::string& name) {
    _name = name.empty() ? "Law" : name;
}

void Law::addAuthor(Singular& author) {
    _authors.addMember(&author);
    recordProvenance("authored-by", *this, author, true, 1.0f);
}

void Law::setAuthors(const std::vector<Singular*>& authors) {
    _authors.clear();
    for (auto* author : authors) {
        if (author) addAuthor(*author);
    }
}

void Law::addConditionSubject(Singular& subject) {
    _conditions.addMember(&subject);
}

void Law::addConditionRelation(const std::shared_ptr<Relation>& relation) {
    _conditions.addRelation(relation);
}

void Law::clearConditionFormation() {
    _conditions.clear();
}

void Law::addTarget(Singular& target) {
    _targets.addMember(&target);
    recordProvenance("targets", *this, target, true, 1.0f);
}

void Law::clearTargets() {
    _targets.clear();
}

std::shared_ptr<Relation> Law::recordProvenance(const std::string& type,
                                                const Singular& a,
                                                const Singular& b,
                                                bool directed,
                                                float weight) {
    auto relation = std::make_shared<Relation>(type, a, b, directed, weight);
    _provenance.add(relation);
    return relation;
}

void Law::addCondition(const std::string& description,
                       ConditionPredicate predicate,
                       bool required) {
    _conditionPredicates.push_back(Condition{description, std::move(predicate), required});
}

void Law::clearConditions() {
    _conditionPredicates.clear();
}

bool Law::conditionsSatisfied(const Singular& target) const {
    if (_conditionPredicates.empty()) return true;

    ECA::Event event;
    event.type = "law-evaluate";
    event.subject = const_cast<Singular*>(&target);
    event.timestamp = std::time(nullptr);

    bool anySatisfied = false;
    for (const auto& condition : _conditionPredicates) {
        if (!condition.predicate) {
            if (condition.required && _conditionMode == ConditionMode::All) return false;
            continue;
        }

        const bool passed = condition.evaluate(event, target);
        anySatisfied = anySatisfied || passed;

        if (_conditionMode == ConditionMode::All && condition.required && !passed) {
            return false;
        }
        if (_conditionMode == ConditionMode::Any && passed) {
            return true;
        }
    }

    return _conditionMode == ConditionMode::All ? true : anySatisfied;
}

void Law::addAction(const std::string& description, ECA::ActionExecutor action) {
    _actions.push_back(Action{description, std::move(action)});
}

void Law::clearActions() {
    _actions.clear();
}

void Law::setConditionModel(ConditionModel model) {
    _conditionModel = std::move(model);
    recompile();
}

void Law::setActionModel(ActionModel model) {
    _actionModel = std::move(model);
    recompile();
}

// Models are primary; the compiled ECA slots are derived. Only slots owned by
// a model are cleared — first-mover closures registered directly through
// addCondition/addAction survive untouched on model-less laws.
void Law::recompile() {
    if (_conditionModel) {
        _conditionPredicates.clear();
        addCondition(_conditionModel->describe(), _conditionModel->compile());
    }
    if (_actionModel) {
        _actions.clear();
        addAction(_actionModel->describe(), _actionModel->compile());
    }
}

std::shared_ptr<Law> Law::fromJson(const nlohmann::json& j) {
    auto law = std::make_shared<Law>(j.value("name", std::string("Law")));
    // Preserve saved identity so provenance/Rete bindings keep meaning across
    // loads. (Guarding the fresh-id counter against restored ids is the world
    // loader's concern, as is reattaching authors/targets by identifier —
    // until authors are reattached the law stays Unauthored and cannot fire.)
    if (j.contains("id")) {
        law->_lawId = j["id"].get<std::string>();
        law->setObjectID(law->_lawId);
    }
    law->setEnabled(j.value("enabled", true));
    law->setConditionMode(j.value("conditionMode", std::string("all")) == "any"
                              ? ConditionMode::Any
                              : ConditionMode::All);
    if (j.contains("conditionModel")) {
        law->setConditionModel(ConditionNode::fromJson(j["conditionModel"]));
    }
    if (j.contains("actionModel")) {
        law->setActionModel(ActionNode::fromJson(j["actionModel"]));
    }
    return law;
}

Law::ApplicationResult Law::applyTo(Singular& target) {
    ApplicationResult result = ApplicationResult::Applied;

    if (!_enabled) {
        result = ApplicationResult::Disabled;
    } else if (!isAuthored()) {
        result = ApplicationResult::Unauthored;
    } else if (!conditionsSatisfied(target)) {
        result = ApplicationResult::ConditionsFailed;
    } else if (_actions.empty()) {
        result = ApplicationResult::NoAction;
    } else {
        ECA::Event event;
        event.type = "law-apply";
        event.subject = &target;
        event.timestamp = std::time(nullptr);

        for (const auto& action : _actions) {
            action.run(event, target);
        }
    }

    _applicationLog.push_back(makeRecord(&target, result));
    publishAppliedEvent(&target, result);
    return result;
}

std::vector<Law::ApplicationRecord> Law::applyToTargets() {
    std::vector<ApplicationRecord> records;
    for (auto* target : _targets.getMembers()) {
        if (!target) continue;
        applyTo(*target);
        records.push_back(_applicationLog.back());
    }

    if (records.empty()) {
        auto record = makeRecord(nullptr, ApplicationResult::NoTarget);
        _applicationLog.push_back(record);
        publishAppliedEvent(nullptr, ApplicationResult::NoTarget);
        records.push_back(std::move(record));
    }
    return records;
}

Law::ApplicationRecord Law::makeRecord(Singular* target, ApplicationResult result) const {
    ApplicationRecord record;
    record.timestamp = std::time(nullptr);
    record.lawId = _lawId;
    record.targetId = target ? target->getIdentifier() : "";
    record.result = result;

    for (const auto& condition : _conditionPredicates) {
        record.conditionDescriptions.push_back(condition.description);
    }
    for (const auto& action : _actions) {
        record.actionDescriptions.push_back(action.description);
    }
    return record;
}

void Law::publishAppliedEvent(Singular* target, ApplicationResult result) const {
    AppliedEvent event{this, target, result, std::time(nullptr)};
    Core::EventBus::instance().publish(event);
}

nlohmann::json Law::toJson() const {
    nlohmann::json log = nlohmann::json::array();
    for (const auto& record : _applicationLog) {
        log.push_back(record.toJson());
    }

    std::vector<std::string> conditionDescriptions;
    for (const auto& condition : _conditionPredicates) {
        conditionDescriptions.push_back(condition.description);
    }

    std::vector<std::string> actionDescriptions;
    for (const auto& action : _actions) {
        actionDescriptions.push_back(action.description);
    }

    nlohmann::json j{
        {"id", _lawId},
        {"name", _name},
        {"enabled", _enabled},
        {"conditionMode", _conditionMode == ConditionMode::All ? "all" : "any"},
        {"authors", formationMemberIds(_authors)},
        {"conditionSubjects", formationMemberIds(_conditions)},
        {"targets", formationMemberIds(_targets)},
        {"provenance", _provenance.toJson()},
        {"conditionDescriptions", conditionDescriptions},
        {"actionDescriptions", actionDescriptions},
        {"applicationLog", log}
    };
    // The law's text — the part that makes behavior (not just descriptions)
    // survive save/load.
    if (_conditionModel) j["conditionModel"] = _conditionModel->toJson();
    if (_actionModel) j["actionModel"] = _actionModel->toJson();
    return j;
}

const char* Law::resultName(ApplicationResult result) {
    switch (result) {
        case ApplicationResult::Applied: return "applied";
        case ApplicationResult::Disabled: return "disabled";
        case ApplicationResult::Unauthored: return "unauthored";
        case ApplicationResult::NoTarget: return "no-target";
        case ApplicationResult::ConditionsFailed: return "conditions-failed";
        case ApplicationResult::NoAction: return "no-action";
    }
    return "unknown";
}

std::string ReteNetwork::assertFact(ReteFact fact) {
    static std::atomic<unsigned long long> nextFactId{1};
    if (fact.id.empty()) {
        fact.id = "fact-" + std::to_string(nextFactId.fetch_add(1));
    }
    if (fact.subject && fact.subjectId.empty()) {
        fact.subjectId = fact.subject->getIdentifier();
    }
    _facts.push_back(fact);
    return _facts.back().id;
}

bool ReteNetwork::retractFact(const std::string& factId) {
    auto oldSize = _facts.size();
    _facts.erase(std::remove_if(_facts.begin(), _facts.end(), [&](const ReteFact& fact) {
        return fact.id == factId;
    }), _facts.end());
    return _facts.size() != oldSize;
}

void ReteNetwork::clearFacts() {
    _facts.clear();
    _agenda.clear();
    for (auto& alpha : _alphaNodes) alpha.memory.clear();
    for (auto& beta : _betaNodes) beta.memory.clear();
}

std::size_t ReteNetwork::addAlphaNode(const std::string& description, AlphaPredicate predicate) {
    AlphaNode node;
    node.id = _nextAlphaId++;
    node.description = description;
    node.predicate = std::move(predicate);
    _alphaNodes.push_back(std::move(node));
    return _alphaNodes.back().id;
}

std::size_t ReteNetwork::addBetaNode(const std::string& description,
                                     std::size_t leftAlphaId,
                                     std::size_t rightAlphaId,
                                     BetaJoin join) {
    BetaNode node;
    node.id = _nextBetaId++;
    node.description = description;
    node.leftAlphaId = leftAlphaId;
    node.rightAlphaId = rightAlphaId;
    node.join = std::move(join);
    _betaNodes.push_back(std::move(node));
    return _betaNodes.back().id;
}

void ReteNetwork::bindLawToAlpha(const std::string& lawId, std::size_t alphaNodeId) {
    if (!lawId.empty()) _alphaLawBindings[alphaNodeId].push_back(lawId);
}

void ReteNetwork::bindLawToBeta(const std::string& lawId, std::size_t betaNodeId) {
    if (!lawId.empty()) _betaLawBindings[betaNodeId].push_back(lawId);
}

const std::vector<ReteActivation>& ReteNetwork::evaluate() {
    _agenda.clear();

    for (auto& alpha : _alphaNodes) {
        alpha.memory.clear();
        for (const auto& fact : _facts) {
            if (!alpha.predicate || alpha.predicate(fact)) {
                alpha.memory.push_back(fact);
            }
        }

        auto bindingIt = _alphaLawBindings.find(alpha.id);
        if (bindingIt == _alphaLawBindings.end()) continue;
        for (const auto& lawId : bindingIt->second) {
            for (const auto& fact : alpha.memory) {
                ReteToken token;
                token.facts.push_back(fact);
                if (!fact.subjectId.empty()) token.bindings["subject"] = fact.subjectId;
                _agenda.push_back(ReteActivation{lawId, token, std::time(nullptr)});
            }
        }
    }

    for (auto& beta : _betaNodes) {
        beta.memory.clear();
        const AlphaNode* left = findAlpha(beta.leftAlphaId);
        const AlphaNode* right = findAlpha(beta.rightAlphaId);
        if (!left || !right) continue;

        for (const auto& leftFact : left->memory) {
            ReteToken seed;
            seed.facts.push_back(leftFact);
            if (!leftFact.subjectId.empty()) seed.bindings["left"] = leftFact.subjectId;

            for (const auto& rightFact : right->memory) {
                const bool joined = beta.join ? beta.join(seed, rightFact)
                                              : leftFact.subjectId == rightFact.subjectId;
                if (!joined) continue;

                ReteToken token = seed;
                token.facts.push_back(rightFact);
                if (!rightFact.subjectId.empty()) token.bindings["right"] = rightFact.subjectId;
                beta.memory.push_back(token);
            }
        }

        auto bindingIt = _betaLawBindings.find(beta.id);
        if (bindingIt == _betaLawBindings.end()) continue;
        for (const auto& lawId : bindingIt->second) {
            for (const auto& token : beta.memory) {
                _agenda.push_back(ReteActivation{lawId, token, std::time(nullptr)});
            }
        }
    }

    return _agenda;
}

std::vector<ReteActivation> ReteNetwork::drainAgenda() {
    std::vector<ReteActivation> drained = std::move(_agenda);
    _agenda.clear();
    return drained;
}

nlohmann::json ReteNetwork::toJson() const {
    nlohmann::json factsJson = nlohmann::json::array();
    for (const auto& fact : _facts) {
        factsJson.push_back({
            {"id", fact.id},
            {"type", fact.type},
            {"subjectId", fact.subjectId},
            {"attribute", fact.attribute},
            {"value", fact.value}
        });
    }

    nlohmann::json alphaJson = nlohmann::json::array();
    for (const auto& alpha : _alphaNodes) {
        alphaJson.push_back({
            {"id", alpha.id},
            {"description", alpha.description},
            {"memorySize", alpha.memory.size()}
        });
    }

    nlohmann::json betaJson = nlohmann::json::array();
    for (const auto& beta : _betaNodes) {
        betaJson.push_back({
            {"id", beta.id},
            {"description", beta.description},
            {"leftAlphaId", beta.leftAlphaId},
            {"rightAlphaId", beta.rightAlphaId},
            {"memorySize", beta.memory.size()}
        });
    }

    nlohmann::json agendaJson = nlohmann::json::array();
    for (const auto& activation : _agenda) {
        nlohmann::json factIds = nlohmann::json::array();
        for (const auto& fact : activation.token.facts) factIds.push_back(fact.id);
        agendaJson.push_back({
            {"lawId", activation.lawId},
            {"timestamp", activation.timestamp},
            {"factIds", factIds},
            {"bindings", activation.token.bindings}
        });
    }

    return nlohmann::json{
        {"facts", factsJson},
        {"alphaNodes", alphaJson},
        {"betaNodes", betaJson},
        {"agenda", agendaJson}
    };
}

const ReteNetwork::AlphaNode* ReteNetwork::findAlpha(std::size_t id) const {
    for (const auto& alpha : _alphaNodes) {
        if (alpha.id == id) return &alpha;
    }
    return nullptr;
}

ReteNetwork::AlphaNode* ReteNetwork::findAlpha(std::size_t id) {
    for (auto& alpha : _alphaNodes) {
        if (alpha.id == id) return &alpha;
    }
    return nullptr;
}

std::shared_ptr<Law> LawManager::createLaw(const std::string& name,
                                           const std::vector<Singular*>& authors) {
    auto law = std::make_shared<Law>(name, authors);
    add(law);
    return law;
}

void LawManager::add(const std::shared_ptr<Law>& law) {
    if (!law) return;
    const std::string id = law->getIdentifier();
    auto existing = std::find_if(_laws.begin(), _laws.end(), [&](const std::shared_ptr<Law>& candidate) {
        return candidate && candidate->getIdentifier() == id;
    });
    if (existing != _laws.end()) return;

    _laws.push_back(law);
    _lawFormation.addMember(law.get());

    LawRegisteredEvent event{law, std::time(nullptr)};
    Core::EventBus::instance().publish(event);
}

bool LawManager::remove(const std::string& lawId) {
    auto it = std::find_if(_laws.begin(), _laws.end(), [&](const std::shared_ptr<Law>& law) {
        return law && law->getIdentifier() == lawId;
    });
    if (it == _laws.end()) return false;

    _lawFormation.removeMember(it->get());
    _laws.erase(it);
    return true;
}

Law* LawManager::find(const std::string& lawId) const {
    for (const auto& law : _laws) {
        if (law && law->getIdentifier() == lawId) return law.get();
    }
    return nullptr;
}

std::vector<std::shared_ptr<Law>> LawManager::getByAuthor(const std::string& authorId) const {
    std::vector<std::shared_ptr<Law>> result;
    for (const auto& law : _laws) {
        if (!law) continue;
        for (auto* author : law->authors().getMembers()) {
            if (author && author->getIdentifier() == authorId) {
                result.push_back(law);
                break;
            }
        }
    }
    return result;
}

std::vector<Law::ApplicationRecord> LawManager::applyAllTo(Singular& target) {
    std::vector<Law::ApplicationRecord> records;
    for (const auto& law : _laws) {
        if (!law) continue;
        law->applyTo(target);
        if (!law->applicationLog().empty()) {
            records.push_back(law->applicationLog().back());
        }
    }
    return records;
}

std::vector<Law::ApplicationRecord> LawManager::applyAllToTargets() {
    std::vector<Law::ApplicationRecord> records;
    for (const auto& law : _laws) {
        if (!law) continue;
        auto lawRecords = law->applyToTargets();
        records.insert(records.end(), lawRecords.begin(), lawRecords.end());
    }
    return records;
}

nlohmann::json LawManager::toJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& law : _laws) {
        if (law) arr.push_back(law->toJson());
    }
    return nlohmann::json{
        {"laws", arr},
        {"formationMembers", formationMemberIds(_lawFormation)},
        {"rete", _rete.toJson()}
    };
}
