#include "Law.hpp"

#include "Form/Singular/Property/ComputedProperty.hpp"
#include "Universe.hpp"

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
    law->setAuthorityLevel(j.value("authority", 0));
    law->setActivation(static_cast<Activation>(j.value("activation", 0)));
    law->setScope(static_cast<Scope>(j.value("scope", 0)));
    law->setDrives(j.value("drives", false));
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

    // When the target is itself a Law, this application is a METALAW — and
    // the Singularity-grounded ceiling applies: lower authority may not
    // govern higher. This single check is what keeps the civic order from
    // collapsing into either chaos or tyranny.
    const Law* targetLaw = dynamic_cast<const Law*>(&target);

    if (!_enabled) {
        result = ApplicationResult::Disabled;
    } else if (!isAuthored()) {
        result = ApplicationResult::Unauthored;
    } else if (targetLaw && _authorityLevel < targetLaw->authorityLevel()) {
        result = ApplicationResult::AuthorityDenied;
    } else if (!conditionsSatisfied(target)) {
        result = ApplicationResult::ConditionsFailed;
    } else if (_actions.empty()) {
        result = ApplicationResult::NoAction;
    } else {
        ECA::Event event;
        event.type = "law-apply";
        event.subject = &target;
        event.timestamp = std::time(nullptr);

        // time.sinceApplied context: t=0 is when this law began holding for
        // this subject (continuous edge, or drive-session start); a plain
        // one-shot application begins NOW. Guard clears on every exit path.
        struct OnsetGuard {
            bool armed = false;
            ~OnsetGuard() {
                if (armed) Universe::instance().clearApplicationOnset();
            }
        } guard;
        if (Universe::instance().hasClock()) {
            const std::string subjectId = target.getIdentifier();
            const double onset = hasOnset(subjectId) ? onsetFor(subjectId)
                                                     : Universe::instance().now();
            Universe::instance().setApplicationOnset(onset);
            guard.armed = true;
        }

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

    if (result == ApplicationResult::Applied) {
        // String-typed echo: Person-authored laws bind to "law-applied"
        // without needing a compile-time event type — laws chaining on laws.
        // (Only successful applications echo; refusals are log-only.)
        ECA::Event echo;
        echo.type = "law-applied";
        echo.subject = target;
        echo.timestamp = std::time(nullptr);
        Core::EventBus::instance().publish(echo);
    }
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
        {"authority", _authorityLevel},
        {"activation", static_cast<int>(_activation)},
        {"scope", static_cast<int>(_scope)},
        {"drives", _drives},
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
        case ApplicationResult::AuthorityDenied: return "authority-denied";
    }
    return "unknown";
}

// A Law is a legible Singular: its own governable state addresses through
// PropertyPath like anything else, so a law targeting a law IS a metalaw.
// authorityLevel is deliberately absent — the ceiling is Singularity-granted,
// never law-modifiable.
void Law::buildProperties() {
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Law, bool>>(
        "enabled", this, &Law::propEnabled, &Law::propSetEnabled));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Law, int>>(
        "conditionMode", this, &Law::propConditionMode, &Law::propSetConditionMode));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Law, std::string>>(
        "name", this, &Law::propName, &Law::propSetName));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Law, bool>>(
        "drives", this, &Law::propDrives, &Law::propSetDrives));
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

void ReteNetwork::retractFirst(std::size_t count) {
    if (count >= _facts.size()) {
        _facts.clear();
    } else {
        _facts.erase(_facts.begin(), _facts.begin() + static_cast<std::ptrdiff_t>(count));
    }
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

void ReteNetwork::unbindLaw(const std::string& lawId) {
    for (auto& binding : _alphaLawBindings) {
        auto& laws = binding.second;
        laws.erase(std::remove(laws.begin(), laws.end(), lawId), laws.end());
    }
    for (auto& binding : _betaLawBindings) {
        auto& laws = binding.second;
        laws.erase(std::remove(laws.begin(), laws.end(), lawId), laws.end());
    }
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

    ECA::Event echo;
    echo.type = "law-registered";
    echo.subject = law.get();
    echo.timestamp = std::time(nullptr);
    Core::EventBus::instance().publish(echo);
}

void LawManager::connectToEventBus() {
    if (_connected) return;
    _connected = true;
    Core::EventBus::instance().subscribe<ECA::Event>([this](const ECA::Event& e) {
        ReteFact fact;
        fact.type = e.type;
        fact.subject = e.subject;
        fact.subjectId = e.subject ? e.subject->getIdentifier() : "";
        _rete.assertFact(fact);
        _dirty = true;
    });
}

std::vector<Law::ApplicationRecord> LawManager::tick() {
    std::vector<Law::ApplicationRecord> records;
    for (int round = 0; round < kMaxChainRounds && _dirty; ++round) {
        _dirty = false;
        // Facts asserted before this round are consumed by it; facts asserted
        // DURING it (laws firing events from applyTo) survive into the next
        // round — that's how law chains resolve, bounded by kMaxChainRounds.
        const std::size_t consumed = _rete.facts().size();
        _rete.evaluate();
        std::vector<ReteActivation> agenda = _rete.drainAgenda();
        for (const auto& activation : agenda) {
            Law* law = find(activation.lawId);
            if (!law) continue;
            Singular* subject = activation.token.facts.empty()
                                    ? nullptr
                                    : activation.token.facts.front().subject;

            if (law->scope() == Law::Scope::Everyone) {
                // The event is the OCCASION; the application sweeps every
                // being (targets, or the Universe) that satisfies the
                // conditions — "every instance of the category".
                std::vector<Singular*> subjects;
                const auto& targets = law->targets().getMembers();
                if (!targets.empty()) subjects.assign(targets.begin(), targets.end());
                else subjects = Universe::instance().beings();
                for (Singular* being : subjects) {
                    if (!being || !law->conditionsSatisfied(*being)) continue;
                    // A live drive session OWNS the process: a re-firing
                    // event (a block resting in constant collision) is
                    // absorbed, not stacked — one process, one clock, per
                    // law-and-subject. Retrigger-restarts would be a future
                    // authored choice, never an accident.
                    if (law->drives() &&
                        hasDriveSession(law->getIdentifier(), being->getIdentifier())) {
                        continue;
                    }
                    if (law->applyTo(*being) == Law::ApplicationResult::Applied) {
                        maybeStartDriveSession(*law, *being);
                    }
                    if (!law->applicationLog().empty()) {
                        records.push_back(law->applicationLog().back());
                    }
                }
                continue;
            }

            if (!subject) continue;
            if (law->drives() &&
                hasDriveSession(law->getIdentifier(), subject->getIdentifier())) {
                continue;   // the session owns the process (see above)
            }
            if (law->applyTo(*subject) == Law::ApplicationResult::Applied) {
                maybeStartDriveSession(*law, *subject);
            }
            if (!law->applicationLog().empty()) {
                records.push_back(law->applicationLog().back());
            }
        }
        _rete.retractFirst(consumed);
    }

    // ------------------------------------------------------------------
    // Continuous pass: level-triggered laws don't wait for events — their
    // condition phase monitors the program every tick. Subjects come from
    // the law's targets Formation when present, otherwise from the whole
    // Universe of beings. (Events a continuous application fires — the
    // law-applied echo — become facts for the NEXT tick's event rounds.)
    // ------------------------------------------------------------------
    for (const auto& law : _laws) {
        if (!law || law->activation() == Law::Activation::OnEvent) continue;
        if (!law->isEnabled() || !law->isAuthored()) continue;

        std::vector<Singular*> subjects;
        const auto& targets = law->targets().getMembers();
        if (!targets.empty()) {
            subjects.assign(targets.begin(), targets.end());
        } else {
            subjects = Universe::instance().beings();
        }

        for (Singular* subject : subjects) {
            if (!subject) continue;
            const bool holds = law->conditionsSatisfied(*subject);
            const std::string subjectId = subject->getIdentifier();
            const bool wasHolding = law->lastConditionState(subjectId);
            law->rememberConditionState(subjectId, holds);

            // The false->true edge is t=0 for this subject's change-over-time
            // clock (time.sinceApplied); release re-arms it.
            if (holds && !wasHolding && Universe::instance().hasClock()) {
                law->rememberOnset(subjectId, Universe::instance().now());
            } else if (!holds && wasHolding) {
                law->forgetOnset(subjectId);
            }

            const bool fire = law->activation() == Law::Activation::WhileTrue
                                  ? holds
                                  : (holds && !wasHolding);   // the false->true edge
            if (!fire) continue;
            if (law->drives() &&
                hasDriveSession(law->getIdentifier(), subjectId)) {
                continue;   // a re-edge while the launched process still
                            // runs is absorbed — the session owns it
            }
            if (law->applyTo(*subject) == Law::ApplicationResult::Applied) {
                // An OnBecomeTrue law that drives launches its process at
                // the edge and runs it to the end of its authored bounds.
                maybeStartDriveSession(*law, *subject);
            }
            if (!law->applicationLog().empty()) {
                records.push_back(law->applicationLog().back());
            }
        }
    }

    runDriveSessions(records);
    return records;
}

void LawManager::maybeStartDriveSession(Law& law, Singular& subject) {
    // Driving is the law's AUTHORED choice, not an inference from what it
    // reads. WhileTrue needs no session — it re-applies on its own.
    if (!law.drives()) return;
    if (law.activation() == Law::Activation::WhileTrue) return;
    if (!Universe::instance().hasClock()) return;

    const std::string subjectId = subject.getIdentifier();
    for (const auto& session : _driveSessions) {
        if (session.lawId == law.getIdentifier() && session.subjectId == subjectId) {
            return;   // already driving this subject
        }
    }
    const double onset = Universe::instance().now();
    law.rememberOnset(subjectId, onset);
    _driveSessions.push_back({law.getIdentifier(), subjectId, onset});
}

void LawManager::runDriveSessions(std::vector<Law::ApplicationRecord>& records) {
    if (_driveSessions.empty() || !Universe::instance().hasClock()) return;
    const double now = Universe::instance().now();

    for (auto it = _driveSessions.begin(); it != _driveSessions.end();) {
        Law* law = find(it->lawId);
        Singular* subject = nullptr;
        if (law) {
            for (Singular* being : Universe::instance().beings()) {
                if (being && being->getIdentifier() == it->subjectId) {
                    subject = being;
                    break;
                }
            }
            if (!subject) {
                for (Singular* target : law->targets().getMembers()) {
                    if (target && target->getIdentifier() == it->subjectId) {
                        subject = target;
                        break;
                    }
                }
            }
        }

        // A law or being that left the world ends its sessions silently.
        if (!law || !subject || !law->isEnabled()) {
            if (law) law->forgetOnset(it->subjectId);
            it = _driveSessions.erase(it);
            continue;
        }

        // The authored bounds ARE the duration — and ANY bound variable may
        // cut them (time, another being's position, the subject's own
        // state). The drive lives while the function is still defined for
        // the subject; checked in the law's application context so
        // time.sinceApplied resolves like any other input variable. A law
        // whose action has no bounded function drives until disabled.
        bool alive = true;
        if (law->hasActionModel()) {
            Universe::instance().setApplicationOnset(it->onset);
            alive = law->actionModel()->definedFor(*subject);
            Universe::instance().clearApplicationOnset();
        }
        if (!alive && now > it->onset) {
            law->forgetOnset(it->subjectId);
            Core::EventBus::instance().publish(
                ECA::Event{"law-drive-finished", subject, nullptr, std::time(nullptr)});
            it = _driveSessions.erase(it);
            continue;
        }

        // The starting tick already applied the law (t=0) when it fired.
        if (now > it->onset) {
            // The session owns this drive's t=0 — reassert it so applyTo's
            // context matches even if the law's edge memory moved meanwhile.
            law->rememberOnset(it->subjectId, it->onset);
            law->applyTo(*subject);
            if (!law->applicationLog().empty()) {
                records.push_back(law->applicationLog().back());
            }
        }
        ++it;
    }
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
