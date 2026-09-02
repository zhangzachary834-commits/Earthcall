#include "Law.hpp"

#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "Universe.hpp"
#include "LawAuditLogger.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <GLFW/glfw3.h>  // at top of file

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <iterator>
#include <optional>
#include <unordered_set>

#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "Person/Person.hpp"

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
    // The node trace travels with the record: "Applied" says the branch was
    // reached, and this says what came of it. A reader that sees Applied with
    // every node's `wrote` false is looking at a law that did nothing, and
    // now it can tell.
    nlohmann::json nodes = nlohmann::json::array();
    for (const auto& node : trace.nodes) {
        nlohmann::json entry{{"action", node.actionName}, {"wrote", node.wrote}};
        if (!node.path.empty()) entry["path"] = node.path;
        if (!node.wrote) {
            entry["reason"] = node.note.empty()
                                  ? std::string(ActionNode::reasonName(node.reason))
                                  : node.note;
        }
        nodes.push_back(std::move(entry));
    }
    return nlohmann::json{
        {"timestamp", timestamp},
        {"lawId", lawId},
        {"targetId", targetId},
        {"result", Law::resultName(result)},
        {"changed", changedSomething()},
        {"conditions", conditionDescriptions},
        {"actions", actionDescriptions},
        {"nodes", nodes}
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

namespace {
std::atomic<unsigned long long> g_nextLawId{1};

// A restored law id advances the fresh counter past itself — otherwise the
// next authored law collides with a loaded one and LawManager::add silently
// discards it (the register would "blur").
void claimLawIdAtLeast(const std::string& id) {
    const std::string prefix = "law-";
    if (id.rfind(prefix, 0) != 0) return;
    const unsigned long long n = std::strtoull(id.c_str() + prefix.size(), nullptr, 10);
    unsigned long long current = g_nextLawId.load();
    while (n + 1 > current && !g_nextLawId.compare_exchange_weak(current, n + 1)) {
    }
}
} // namespace

void Law::initializeLawIdentity() {
    _lawId = "law-" + std::to_string(g_nextLawId.fetch_add(1));
    nameGroupFormations();
}

void Law::setLawIdentifier(const std::string& id) {
    if (id.empty()) return;
    _lawId = id;
    claimLawIdAtLeast(_lawId);
    nameGroupFormations();
}

// The three groups are beings, so they need names a Person can write, not the
// generated `formation-N` that changes every run. Called from all three places
// the law id is decided — construction, load, and setLawIdentifier — because
// an id decided anywhere must carry its formations' names with it.
void Law::nameGroupFormations() {
    _authors.setIdentifier(_lawId + ".authors");
    _conditions.setIdentifier(_lawId + ".conditions");
    _targets.setIdentifier(_lawId + ".targets");
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

std::uint64_t Law::s_textRevision = 0;

void Law::setConditionModel(ConditionModel model) {
    _conditionModel = std::move(model);
    ++_conditionRevision;
    bumpTextRevision();
    recompile();
}

void Law::setActionModel(ActionModel model) {
    _actionModel = std::move(model);
    bumpTextRevision();
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
    rebuildRequiredProperties();
}

// The law's text names the vocabulary it needs; this reads it off once, at
// the same moment the closures are derived, so the per-tick sweep never pays
// to re-derive it.
void Law::rebuildRequiredProperties() {
    _requiredProperties.clear();

    std::vector<PropertyPath> paths;
    if (_conditionModel) _conditionModel->collectPaths(paths);
    if (_actionModel) _actionModel->collectPaths(paths);

    for (const auto& path : paths) {
        if (path.segments.empty()) continue;
        const std::string& root = path.segments.front();
        // Qualified roots address someone else; reserved time paths address
        // the clock. Neither is a claim about the subject.
        if (root.empty() || root[0] == '@' || root == "time") continue;
        if (std::find(_requiredProperties.begin(), _requiredProperties.end(), root) ==
            _requiredProperties.end()) {
            _requiredProperties.push_back(root);
        }
    }
}

bool Law::couldApplyTo(Singular& being) const {
    // No stated requirements = no filter. A law of pure kind-tests or pure
    // quantification really is about every being, and must keep sweeping.
    for (const std::string& name : _requiredProperties) {
        if (being.findProperty(name)) continue;
        // Authored properties are as real as first-mover ones: a law that
        // reads a granted `warmth` must still reach the beings a previous
        // law granted it to.
        PropertyValue ignored;
        if (being.getDynamicProperty(name, ignored)) continue;
        return false;
    }
    return true;
}

std::shared_ptr<Law> Law::fromJson(const nlohmann::json& j) {
    auto law = std::make_shared<Law>(j.value("name", std::string("Law")));
    // Preserve saved identity so provenance/Rete bindings keep meaning across
    // loads. (Guarding the fresh-id counter against restored ids is the world
    // loader's concern, as is reattaching authors/targets by identifier —
    // until authors are reattached the law stays Unauthored and cannot fire.)
    if (j.contains("id")) {
        law->_lawId = j["id"].get<std::string>();
        law->nameGroupFormations();
        claimLawIdAtLeast(law->_lawId);   // fresh ids stay fresh after loads
    }
    law->setEnabled(j.value("enabled", true));
    // CLAMPED, deliberately. A save file is authored text like any other, and
    // the authority ceiling is the one thing authored text may never raise —
    // otherwise the anti-tyranny guarantee is a single hand-edited integer
    // away from meaningless. First movers receive their level through
    // grantAuthority at construction, in code, and never through here.
    law->setAuthorityLevel(j.value("authority", 0));
    law->setActivation(static_cast<Activation>(j.value("activation", 0)));
    law->setScope(static_cast<Scope>(j.value("scope", 0)));
    law->setDrives(j.value("drives", false));
    law->setRetrigger(static_cast<Retrigger>(j.value("retrigger", 0)));
    law->setConditionMode(j.value("conditionMode", std::string("all")) == "any"
                              ? ConditionMode::Any
                              : ConditionMode::All);
    if (j.contains("conditionModel")) {
        law->setConditionModel(ConditionNode::fromJson(j["conditionModel"]));
    }
    if (j.contains("actionModel")) {
        law->setActionModel(ActionNode::fromJson(j["actionModel"]));
    }
    // The law's descent, restored. `toJson` has always written provenance and
    // this never read it back, so "synthesized-from" — the whole record of
    // which laws a higher law was made out of — survived exactly until the
    // next load. Authors and targets are still reattached by the world loader
    // (they are live members, not recorded edges); this is the ANCESTRY, and
    // it is recorded by identifier, so it restores here on its own.
    if (j.contains("provenance")) {
        law->_provenance.loadFromJson(j["provenance"], [law](const std::string& id) -> Singular* {
            if (id == law->getIdentifier()) return law.get();
            for (Singular* being : Universe::instance().beings()) {
                if (being && being->getIdentifier() == id) return being;
            }
            return nullptr;
        });
    }
    return law;
}

Law::ApplicationResult Law::applyTo(Singular& target) {
    ApplicationResult result = ApplicationResult::Applied;
    ActionNode::Trace trace;

    // When the target is itself a Law, this application is a METALAW — and
    // the Singularity-grounded ceiling applies: lower authority may not
    // govern higher. This single check is what keeps the civic order from
    // collapsing into either chaos or tyranny.
    const Law* targetLaw = dynamic_cast<const Law*>(&target);
    const Person* targetPerson = dynamic_cast<const Person*>(&target);
    const Zone* targetZone = dynamic_cast<const Zone*>(&target);

    bool violatesKernelBoundary = false;
    if (_actionModel) {
        std::vector<PropertyPath> paths;
        _actionModel->collectPaths(paths);
        
        bool isSelfAuthored = false;
        for (auto* author : _authors.getMembers()) {
            if (author == &target) {
                isSelfAuthored = true;
                break;
            }
        }

        for (const auto& p : paths) {
            if (p.segments.empty()) continue;
            const std::string& root = p.segments.front();
            
            // 1. Person Guard: Nobody else can move your body against your will.
            if (targetPerson && !isSelfAuthored) {
                if (root == "position" || root == "velocity" || root == "acceleration") {
                    violatesKernelBoundary = true;
                    break;
                }
            }
            
            // 3. Zone Exit Lock Rejection: Nobody can be locked in a zone against their will.
            if (targetZone) {
                if (root == "canExit") {
                    violatesKernelBoundary = true;
                    break;
                }
                // Local Ourverse gathering: no one may own it. Owner is
                // already read-only on Zone; this refuses any write path
                // that would seat a Singular over the gathering place.
                if (targetZone->isOurverseGathering() && root == "owner") {
                    violatesKernelBoundary = true;
                    break;
                }
            }
        }
    }

    if (!_enabled) {
        result = ApplicationResult::Disabled;
    } else if (!isAuthored()) {
        result = ApplicationResult::Unauthored;
    } else if (targetLaw && _authorityLevel < targetLaw->authorityLevel()) {
        result = ApplicationResult::AuthorityDenied;
    } else if (violatesKernelBoundary) {
        result = ApplicationResult::AuthorityDenied;
    } else if (_jurisdiction && !_jurisdiction->getFormation().hasMember(&target)) {
        result = ApplicationResult::AuthorityDenied;
    } else if (!conditionsSatisfied(target)) {
        result = ApplicationResult::ConditionsFailed;
        ECA::LawAuditLogger::instance().log("LAW", "Law \"" + getIdentifier() + "\" applied to \"" + target.getIdentifier() + "\" - CONDITIONS FAILED", {
            {"lawId", getIdentifier()},
            {"targetId", target.getIdentifier()},
            {"result", "ConditionsFailed"}
        });
    } else if (_actions.empty()) {
        result = ApplicationResult::NoAction;
        if (ECA::LawAuditLogger::instance().level() == ECA::LawAuditLogger::Level::Verbose) {
            ECA::LawAuditLogger::instance().log("LAW", "Law \"" + getIdentifier() + "\" applied to \"" + target.getIdentifier() + "\" - NO ACTIONS", {
                {"lawId", getIdentifier()},
                {"targetId", target.getIdentifier()},
                {"result", "NoAction"}
            });
        }
    } else {
        ECA::Event event;
        event.type = "law-apply";
        event.subject = &target;
        event.timestamp = std::time(nullptr);

        // time.sinceApplied context: t=0 is when this law began holding for
        // this subject (continuous edge, or drive-session start); a plain
        // one-shot application begins NOW. The scope RESTORES rather than
        // clears, so a law applied from within another law's action hands
        // back the outer onset instead of erasing it.
        std::optional<Universe::OnsetScope> onsetScope;
        if (Universe::instance().hasClock()) {
            const std::string subjectId = target.getIdentifier();
            onsetScope.emplace(hasOnset(subjectId) ? onsetFor(subjectId)
                                                   : Universe::instance().now());
        }

        // Arm the trace: every action node reports into it, and the record
        // carries it out. This is where "did anything actually happen" is
        // answered — the application result only says the branch was reached.
        ActionNode::TraceScope traceScope;
        for (const auto& action : _actions) {
            action.run(event, target);
        }
        trace = traceScope.trace();

        // A law whose actions are hard-coded CLOSURES (the first-mover lane,
        // registered through addAction rather than compiled from a model) has
        // nothing to report into the trace — the closure is opaque. Silence
        // from an unobservable action is not evidence it did nothing, so it
        // is recorded as fired-and-effective. Only model-derived actions can
        // honestly claim to have written nothing.
        if (!_actionModel && !_actions.empty() && trace.nodes.empty()) {
            trace.nodes.push_back(ActionNode::NodeOutcome{
                "FirstMover", {}, true, PropertyPath::PathResult::Ok, {}});
        }
    }

    _applicationLog.push_back(makeRecord(&target, result));
    _applicationLog.back().trace = trace;

    if (result == ApplicationResult::Applied) {
        // Report what the NODES did, not merely that we got here. A law whose
        // every write failed used to log SUCCESS; now it says so, names the
        // first reason, and the record carries the rest.
        const bool wrote = trace.anyWrote();
        std::string message = "Law \"" + getIdentifier() + "\" applied to \"" +
                              target.getIdentifier() + "\" - " +
                              (trace.fired() ? (wrote ? "SUCCESS" : "NO EFFECT") : "NO NODES");
        if (!wrote && trace.fired()) {
            const auto& first = trace.nodes.front();
            message += " (" + first.actionName + ": " +
                       (first.note.empty() ? std::string(ActionNode::reasonName(first.reason))
                                           : first.note) + ")";
        }
        if (wrote) {
            for (const auto& node : trace.nodes) {
                if (node.wrote && !node.path.empty()) {
                    for (auto* author : _authors.getMembers()) {
                        if (author) {
                            target.addStakeholder(node.path, author->getIdentifier(), getIdentifier(), std::time(nullptr));
                        }
                    }
                }
            }
        }

        // WhileTrue that wrote nothing is a level, not an event. Logging it
        // every tick is the disk stall after loading a world of servos.
        const bool skipAudit = _activation == Activation::WhileTrue && !wrote;
        if (!skipAudit) {
            nlohmann::json nodesJson = nlohmann::json::array();
            for (const auto& node : trace.nodes) {
                nodesJson.push_back({{"action", node.actionName},
                                     {"path", node.path},
                                     {"wrote", node.wrote},
                                     {"reason", node.wrote ? "" : (node.note.empty()
                                         ? std::string(ActionNode::reasonName(node.reason))
                                         : node.note)}});
            }
            ECA::LawAuditLogger::instance().log("LAW", message, {
                {"lawId", getIdentifier()},
                {"targetId", target.getIdentifier()},
                {"result", "Applied"},
                {"changed", wrote},
                {"nodes", nodesJson}
            });
        }
    }
    // Bounded memory: a WhileTrue law applies every tick — the log is a
    // window onto recent history, not an infinite ledger.
    constexpr std::size_t kMaxLogEntries = 256;
    if (_applicationLog.size() > kMaxLogEntries) {
        _applicationLog.erase(_applicationLog.begin(),
                              _applicationLog.end() - kMaxLogEntries);
    }
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

    // String-typed echo: Person-authored laws bind to "law-applied" without
    // needing a compile-time event type — laws chaining on laws. (Only
    // successful applications echo; refusals are log-only.)
    //
    // Published ONLY when a law is actually bound to it. Every echo becomes a
    // fact, and every fact marks the network dirty — so an unheard echo cost
    // a whole extra evaluation round per tick, over every alpha node, for
    // nobody. Silence when nobody is listening is not a lost signal; it is
    // the absence of one.
    if (result == ApplicationResult::Applied &&
        Universe::instance().anyoneHears("law-applied")) {
        ECA::Event echo;
        echo.type = "law-applied";
        echo.subject = target;
        echo.timestamp = std::time(nullptr);
        Core::EventBus::instance().publish(echo);
    }
}

nlohmann::json Law::toJson() const {
    // Only the recent tail persists — the law's TEXT is what must survive;
    // its full history bloated saves by megabytes.
    constexpr std::size_t kMaxSavedLogEntries = 16;
    nlohmann::json log = nlohmann::json::array();
    const std::size_t start = _applicationLog.size() > kMaxSavedLogEntries
                                  ? _applicationLog.size() - kMaxSavedLogEntries
                                  : 0;
    for (std::size_t i = start; i < _applicationLog.size(); ++i) {
        log.push_back(_applicationLog[i].toJson());
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
        {"retrigger", static_cast<int>(_retrigger)},
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
void Law::registerEnabledProperty() {
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Law, bool>>(
        "enabled", this, &Law::propEnabled, &Law::propSetEnabled));
}

void Law::buildProperties() {
    registerEnabledProperty();
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Law, int>>(
        "conditionMode", this, &Law::propConditionMode, &Law::propSetConditionMode));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Law, std::string>>(
        "name", this, &Law::propName, &Law::propSetName));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Law, bool>>(
        "drives", this, &Law::propDrives, &Law::propSetDrives));
}

ReteToken ReteNetwork::alphaToken(const FactPtr& fact) {
    ReteToken token;
    token.facts.push_back(fact);
    if (fact->subject && !fact->subject->getIdentifier().empty()) token.bindings["subject"] = fact->subject->getIdentifier();
    else if (!fact->subjectId.empty()) token.bindings["subject"] = fact->subjectId;
    return token;
}

ReteToken ReteNetwork::joinSeed(const FactPtr& left) {
    ReteToken seed;
    seed.facts.push_back(left);
    if (left->subject && !left->subject->getIdentifier().empty()) seed.bindings["left"] = left->subject->getIdentifier();
    else if (!left->subjectId.empty()) seed.bindings["left"] = left->subjectId;
    return seed;
}

ReteToken ReteNetwork::joinedToken(const FactPtr& left, const FactPtr& right) {
    ReteToken token = joinSeed(left);
    token.facts.push_back(right);
    if (right->subject && !right->subject->getIdentifier().empty()) token.bindings["right"] = right->subject->getIdentifier();
    else if (!right->subjectId.empty()) token.bindings["right"] = right->subjectId;
    return token;
}

ReteToken ReteNetwork::joinedToken(const ReteToken& left, const FactPtr& right) {
    ReteToken token = left;
    token.facts.push_back(right);
    if (right->subject && !right->subject->getIdentifier().empty()) token.bindings["right"] = right->subject->getIdentifier();
    else if (!right->subjectId.empty()) token.bindings["right"] = right->subjectId;
    return token;
}

bool ReteNetwork::alphaFeedsAnyBeta(std::size_t alphaId) const {
    for (const auto& beta : _betaNodes) {
        if ((!beta.leftIsBeta && beta.leftId == alphaId) || beta.rightAlphaId == alphaId) return true;
    }
    return false;
}

bool ReteNetwork::alphaIsRead(std::size_t alphaId) const {
    auto binding = _alphaLawBindings.find(alphaId);
    if (binding != _alphaLawBindings.end() && !binding->second.empty()) return true;
    return alphaFeedsAnyBeta(alphaId);
}

void ReteNetwork::refillAlphaMemory(AlphaNode& alpha) {
    alpha.memory.clear();
    for (const auto& fact : _facts) {
        if (!alpha.predicate || alpha.predicate(fact)) alpha.memory.push_back(fact);
    }
}

void ReteNetwork::refillBetaMemory(BetaNode& beta) {
    beta.memory.clear();
    const AlphaNode* right = findAlpha(beta.rightAlphaId);
    if (!right) return;
    
    if (beta.leftIsBeta) {
        auto it = std::find_if(_betaNodes.begin(), _betaNodes.end(), [&](const BetaNode& b) { return b.id == beta.leftId; });
        if (it == _betaNodes.end()) return;
        const BetaNode* left = &(*it);
        
        for (const auto& leftToken : left->memory) {
            for (const auto& rightFact : right->memory) {
                const bool joined = beta.join ? beta.join(leftToken, rightFact)
                                              : (leftToken.facts.empty() ? false : (leftToken.facts[0]->subjectId == rightFact->subjectId));
                if (joined) beta.memory.push_back(joinedToken(leftToken, rightFact));
            }
        }
    } else {
        const AlphaNode* left = findAlpha(beta.leftId);
        if (!left) return;
        for (const auto& leftFact : left->memory) {
            const ReteToken seed = joinSeed(leftFact);
            for (const auto& rightFact : right->memory) {
                const bool joined = beta.join ? beta.join(seed, rightFact)
                                              : leftFact->subjectId == rightFact->subjectId;
                if (joined) beta.memory.push_back(joinedToken(leftFact, rightFact));
            }
        }
    }
}

std::string ReteNetwork::assertFact(FactPtr fact) {
    static std::atomic<unsigned long long> nextFactId{1};
    if (fact->id.empty()) {
        fact->id = "fact-" + std::to_string(nextFactId.fetch_add(1));
    }
    if (fact->subject && fact->subjectId.empty()) {
        fact->subjectId = fact->subject->getIdentifier();
    }
    _facts.push_back(fact);
    const FactPtr& f = fact;

    std::vector<std::size_t> activatedAlphas;
    for (auto& alpha : _alphaNodes) {
        auto bindingIt = _alphaLawBindings.find(alpha.id);
        const bool boundToLaw = bindingIt != _alphaLawBindings.end() && !bindingIt->second.empty();
        if (!boundToLaw && !alphaFeedsAnyBeta(alpha.id)) continue;

        if (!alpha.predicate || alpha.predicate(f)) {
            alpha.memory.push_back(f);
            activatedAlphas.push_back(alpha.id);

            if (boundToLaw) {
                for (const auto& lawId : bindingIt->second) {
                    _agenda.push_back(
                        ReteActivation{lawId, alphaToken(f), std::time(nullptr)});
                }
            }
        }
    }

    std::vector<std::size_t> activatedBetas;
    std::unordered_map<std::size_t, std::vector<ReteToken>> newBetaTokens;
    
    for (auto& beta : _betaNodes) {
        bool inLeftAlpha = !beta.leftIsBeta && std::find(activatedAlphas.begin(), activatedAlphas.end(), beta.leftId) != activatedAlphas.end();
        bool inLeftBeta = beta.leftIsBeta && std::find(activatedBetas.begin(), activatedBetas.end(), beta.leftId) != activatedBetas.end();
        bool inRight = std::find(activatedAlphas.begin(), activatedAlphas.end(), beta.rightAlphaId) != activatedAlphas.end();
        if (!inLeftAlpha && !inLeftBeta && !inRight) continue;

        const AlphaNode* right = findAlpha(beta.rightAlphaId);
        if (!right) continue;

        auto bindingIt = _betaLawBindings.find(beta.id);
        const bool hasLaw = bindingIt != _betaLawBindings.end() && !bindingIt->second.empty();
        bool betaActivated = false;

        if (beta.leftIsBeta) {
            auto it = std::find_if(_betaNodes.begin(), _betaNodes.end(), [&](const BetaNode& b) { return b.id == beta.leftId; });
            if (it == _betaNodes.end()) continue;
            const BetaNode* left = &(*it);
            
            if (inLeftBeta) {
                for (const auto& leftToken : newBetaTokens[beta.leftId]) {
                    for (const auto& rightFact : right->memory) {
                        const bool joined = beta.join ? beta.join(leftToken, rightFact)
                                                      : (leftToken.facts.empty() ? false : (leftToken.facts[0]->subjectId == rightFact->subjectId));
                        if (!joined) continue;
                        const ReteToken token = joinedToken(leftToken, rightFact);
                        beta.memory.push_back(token);
                        newBetaTokens[beta.id].push_back(token);
                        betaActivated = true;
                        if (hasLaw) {
                            for (const auto& lawId : bindingIt->second) {
                                _agenda.push_back(ReteActivation{lawId, token, std::time(nullptr)});
                            }
                        }
                    }
                }
            }
            if (inRight) {
                for (const auto& leftToken : left->memory) {
                    const bool joined = beta.join ? beta.join(leftToken, f)
                                                  : (leftToken.facts.empty() ? false : (leftToken.facts[0]->subjectId == f->subjectId));
                    if (!joined) continue;
                    const ReteToken token = joinedToken(leftToken, f);
                    
                    bool isDuplicate = false;
                    if (inLeftBeta) {
                        for (const auto& nt : newBetaTokens[beta.id]) {
                            if (nt.facts == token.facts) { isDuplicate = true; break; }
                        }
                    }
                    if (isDuplicate) continue;
                    
                    beta.memory.push_back(token);
                    newBetaTokens[beta.id].push_back(token);
                    betaActivated = true;
                    if (hasLaw) {
                        for (const auto& lawId : bindingIt->second) {
                            _agenda.push_back(ReteActivation{lawId, token, std::time(nullptr)});
                        }
                    }
                }
            }
        } else {
            const AlphaNode* left = findAlpha(beta.leftId);
            if (!left) continue;
            
            if (inLeftAlpha) {
                for (const auto& rightFact : right->memory) {
                    const bool joined = beta.join ? beta.join(joinSeed(f), rightFact)
                                                  : f->subjectId == rightFact->subjectId;
                    if (!joined) continue;
                    const ReteToken token = joinedToken(f, rightFact);
                    beta.memory.push_back(token);
                    // Also announce it downstream. A beta node whose LEFT is
                    // this one reads newBetaTokens[leftId], not the memory —
                    // so a token that only landed in memory was invisible to
                    // the next join, and a three-clause All() dropped matches
                    // depending on which order its facts happened to arrive in.
                    newBetaTokens[beta.id].push_back(token);
                    betaActivated = true;
                    if (hasLaw) {
                        for (const auto& lawId : bindingIt->second) {
                            _agenda.push_back(ReteActivation{lawId, token, std::time(nullptr)});
                        }
                    }
                }
            }
            if (inRight) {
                for (const auto& leftFact : left->memory) {
                    if (inLeftAlpha && leftFact->id == f->id) continue;
                    const bool joined = beta.join ? beta.join(joinSeed(leftFact), f)
                                                  : leftFact->subjectId == f->subjectId;
                    if (!joined) continue;
                    const ReteToken token = joinedToken(leftFact, f);
                    beta.memory.push_back(token);
                    newBetaTokens[beta.id].push_back(token);   // see above
                    betaActivated = true;
                    if (hasLaw) {
                        for (const auto& lawId : bindingIt->second) {
                            _agenda.push_back(ReteActivation{lawId, token, std::time(nullptr)});
                        }
                    }
                }
            }
        }
        
        if (betaActivated) activatedBetas.push_back(beta.id);
    }

    return f->id;
}

void ReteNetwork::retractFirst(std::size_t count) {
    if (count == 0) return;
    if (count >= _facts.size()) {
        count = _facts.size();
    }

    std::unordered_set<std::string> removedIds;
    std::vector<FactPtr> new_facts;
    for (std::size_t i = 0; i < count; ++i) {
        if (!_facts[i]->isState) {
            removedIds.insert(_facts[i]->id);
        } else {
            new_facts.push_back(_facts[i]);
        }
    }

    for (auto& alpha : _alphaNodes) {
        alpha.memory.erase(std::remove_if(alpha.memory.begin(), alpha.memory.end(), 
            [&](const FactPtr& f) { return removedIds.count(f->id); }), alpha.memory.end());
    }
    for (auto& beta : _betaNodes) {
        beta.memory.erase(std::remove_if(beta.memory.begin(), beta.memory.end(), 
            [&](const ReteToken& t) { 
                for (const auto& f : t.facts) if (removedIds.count(f->id)) return true;
                return false;
            }), beta.memory.end());
    }
    _agenda.erase(std::remove_if(_agenda.begin(), _agenda.end(), 
        [&](const ReteActivation& a) { 
            for (const auto& f : a.token.facts) if (removedIds.count(f->id)) return true;
            return false;
        }), _agenda.end());

    // A retracted fact must also leave the dirty queue. _dirtyFacts holds the
    // fact alive, but `fact->subject` is a RAW pointer into a being that may
    // be gone — evaluateDirty() dereferences it next tick. Nothing purged this
    // queue on any retraction path.
    _dirtyFacts.erase(std::remove_if(_dirtyFacts.begin(), _dirtyFacts.end(),
                                     [&](const FactPtr& f) { return removedIds.count(f->id) != 0; }),
                      _dirtyFacts.end());

    for (std::size_t i = count; i < _facts.size(); ++i) {
        new_facts.push_back(_facts[i]);
    }
    _facts = std::move(new_facts);
}

bool ReteNetwork::retractFact(const std::string& factId) {
    auto oldSize = _facts.size();
    _facts.erase(std::remove_if(_facts.begin(), _facts.end(), [&](const FactPtr& fact) {
        return fact->id == factId;
    }), _facts.end());
    if (_facts.size() == oldSize) return false;

    for (auto& alpha : _alphaNodes) {
        alpha.memory.erase(std::remove_if(alpha.memory.begin(), alpha.memory.end(),
                                          [&](const FactPtr& f) { return f->id == factId; }),
                           alpha.memory.end());
    }
    for (auto& beta : _betaNodes) {
        beta.memory.erase(std::remove_if(beta.memory.begin(), beta.memory.end(),
                                         [&](const ReteToken& token) {
                                             for (const auto& f : token.facts) if (f->id == factId) return true;
                                             return false;
                                         }),
                          beta.memory.end());
    }
    _agenda.erase(std::remove_if(_agenda.begin(), _agenda.end(),
                                 [&](const ReteActivation& act) {
                                     for (const auto& fact : act.token.facts) if (fact->id == factId) return true;
                                     return false;
                                 }),
                  _agenda.end());
    return true;
}

void ReteNetwork::retractStateFactsBySubject(const std::string& subjectId) {
    std::unordered_set<std::string> removedIds;
    _facts.erase(std::remove_if(_facts.begin(), _facts.end(), [&](const FactPtr& fact) {
        if (fact->isState && fact->subjectId == subjectId) {
            removedIds.insert(fact->id);
            return true;
        }
        return false;
    }), _facts.end());

    if (removedIds.empty()) return;

    for (auto& alpha : _alphaNodes) {
        alpha.memory.erase(std::remove_if(alpha.memory.begin(), alpha.memory.end(), 
            [&](const FactPtr& f) { return removedIds.count(f->id); }), alpha.memory.end());
    }
    for (auto& beta : _betaNodes) {
        beta.memory.erase(std::remove_if(beta.memory.begin(), beta.memory.end(), 
            [&](const ReteToken& t) { 
                for (const auto& f : t.facts) if (removedIds.count(f->id)) return true;
                return false;
            }), beta.memory.end());
    }
    _agenda.erase(std::remove_if(_agenda.begin(), _agenda.end(), 
        [&](const ReteActivation& a) { 
            for (const auto& f : a.token.facts) if (removedIds.count(f->id)) return true;
            return false;
        }), _agenda.end());
    // A retracted fact must also leave the dirty queue. _dirtyFacts holds the
    // fact alive, but `fact->subject` is a RAW pointer into a being that may
    // be gone — evaluateDirty() dereferences it next tick. Nothing purged this
    // queue on any retraction path.
    _dirtyFacts.erase(std::remove_if(_dirtyFacts.begin(), _dirtyFacts.end(),
                                     [&](const FactPtr& f) { return removedIds.count(f->id) != 0; }),
                      _dirtyFacts.end());
}

void ReteNetwork::markFactDirty(const std::string& subjectId, const std::string& attribute) {
    for (const auto& fact : _facts) {
        if (fact->isState && fact->subjectId == subjectId && fact->attribute == attribute) {
            if (!fact->dirty) {
                fact->dirty = true;
                _dirtyFacts.push_back(fact);
            }
        }
    }
}

void ReteNetwork::evaluateDirty() {
    std::vector<FactPtr> dirty = std::move(_dirtyFacts);
    _dirtyFacts.clear();
    
    for (auto& fact : dirty) {
        if (!fact->dirty) continue;
        fact->dirty = false;
        
        if (!fact->subject) continue;
        
        auto* prop = fact->subject->findProperty(fact->attribute);
        if (prop) {
            nlohmann::json newValue = propertyValueToJson(prop->value());
            if (fact->value == newValue) continue; // no change
            
            // Retract the old fact
            retractFact(fact->id);
            
            // Update value and re-assert
            fact->value = newValue;
            assertFact(fact);
        }
    }
}

std::vector<std::string> ReteNetwork::retractFactsAbout(const Singular* being) {
    std::vector<std::string> orphanedSubjects;
    if (!being) return orphanedSubjects;
    std::unordered_set<std::string> removedIds;
    std::unordered_set<std::string> subjects;
    _facts.erase(std::remove_if(_facts.begin(), _facts.end(),
                                [&](const FactPtr& fact) {
                                    if (fact->subject == being || fact->object == being) {
                                        removedIds.insert(fact->id);
                                        // Only when it was the SUBJECT: a fact
                                        // where the dead being was the other
                                        // participant says nothing about
                                        // whether its subject is still seeded.
                                        if (fact->subject == being && !fact->subjectId.empty()) {
                                            subjects.insert(fact->subjectId);
                                        }
                                        return true;
                                    }
                                    return false;
                                }),
                 _facts.end());
    orphanedSubjects.assign(subjects.begin(), subjects.end());
    if (removedIds.empty()) return orphanedSubjects;

    for (auto& alpha : _alphaNodes) {
        alpha.memory.erase(std::remove_if(alpha.memory.begin(), alpha.memory.end(),
                                          [&](const FactPtr& fact) { return removedIds.count(fact->id); }),
                           alpha.memory.end());
    }
    for (auto& beta : _betaNodes) {
        beta.memory.erase(std::remove_if(beta.memory.begin(), beta.memory.end(),
                                         [&](const ReteToken& token) {
                                             for (const auto& f : token.facts) if (removedIds.count(f->id)) return true;
                                             return false;
                                         }),
                          beta.memory.end());
    }
    _agenda.erase(std::remove_if(_agenda.begin(), _agenda.end(),
                                 [&](const ReteActivation& activation) {
                                     for (const auto& fact : activation.token.facts) {
                                         if (removedIds.count(fact->id)) return true;
                                     }
                                     return false;
                                 }),
                  _agenda.end());
    // Same reason as the other retraction paths: a dirty entry still naming
    // this being is a dangling read waiting for the next evaluateDirty().
    _dirtyFacts.erase(std::remove_if(_dirtyFacts.begin(), _dirtyFacts.end(),
                                     [&](const FactPtr& f) { return removedIds.count(f->id) != 0; }),
                      _dirtyFacts.end());
    return orphanedSubjects;
}

void ReteNetwork::clearFacts() {
    _facts.clear();
    _dirtyFacts.clear();
    _agenda.clear();
    for (auto& alpha : _alphaNodes) alpha.memory.clear();
    for (auto& beta : _betaNodes) beta.memory.clear();
}

std::size_t ReteNetwork::addAlphaNode(const std::string& description, AlphaPredicate predicate,
                                     AlphaSource source) {
    AlphaNode node;
    node.id = _nextNodeId++;
    node.description = description;
    node.predicate = std::move(predicate);
    node.source = source;
    _alphaNodes.push_back(std::move(node));
    return _alphaNodes.back().id;
}

std::size_t ReteNetwork::addBetaNode(const std::string& description,
                                     bool leftIsBeta,
                                     std::size_t leftId,
                                     std::size_t rightAlphaId,
                                     BetaJoin join) {
    BetaNode node;
    node.id = _nextNodeId++;
    node.description = description;
    node.leftIsBeta = leftIsBeta;
    node.leftId = leftId;
    node.rightAlphaId = rightAlphaId;
    node.join = std::move(join);
    _betaNodes.push_back(std::move(node));
    BetaNode& added = _betaNodes.back();

    // Creating the beta is what makes its two alphas read. Until now they may
    // have been skipped by every assert, so refill them and then build the
    // join over what is already there — otherwise this node is permanently
    // blind to every fact that predates it.
    if (!added.leftIsBeta) {
        if (AlphaNode* left = findAlpha(added.leftId)) refillAlphaMemory(*left);
    }
    // Technically rightAlphaId can equal leftId if both point to the same Alpha node, so we only refill once
    if (added.leftIsBeta || added.rightAlphaId != added.leftId) {
        if (AlphaNode* right = findAlpha(added.rightAlphaId)) refillAlphaMemory(*right);
    }
    refillBetaMemory(added);
    return added.id;
}

void ReteNetwork::bindLawToAlpha(const std::string& lawId, std::size_t alphaNodeId) {
    if (lawId.empty()) return;

    AlphaNode* alpha = findAlpha(alphaNodeId);
    if (!alpha) {
        // No such node (yet). Record the binding so a later node with this id
        // is bound, but there is nothing to backfill from.
        _alphaLawBindings[alphaNodeId].push_back(lawId);
        return;
    }

    // Refill BEFORE recording the binding: while nothing read this node,
    // assertFact skipped it, so its memory can be stale by every fact now
    // live. Once a law is on it the node is read and stays current.
    if (!alphaIsRead(alphaNodeId)) refillAlphaMemory(*alpha);

    _alphaLawBindings[alphaNodeId].push_back(lawId);

    // Live facts are this law's backlog. Queue them for THIS law only — laws
    // already bound to the node were queued when their own facts arrived and
    // must not fire twice.
    const std::time_t now = std::time(nullptr);
    for (const auto& fact : alpha->memory) {
        _agenda.push_back(ReteActivation{lawId, alphaToken(fact), now});
    }
}

void ReteNetwork::bindLawToBeta(const std::string& lawId, std::size_t betaNodeId) {
    if (lawId.empty()) return;
    _betaLawBindings[betaNodeId].push_back(lawId);

    // Beta memory is kept current from creation onward (addBetaNode refills
    // it), so the backlog is simply whatever the join already holds.
    const std::time_t now = std::time(nullptr);
    for (const auto& beta : _betaNodes) {
        if (beta.id != betaNodeId) continue;
        for (const auto& token : beta.memory) {
            _agenda.push_back(ReteActivation{lawId, token, now});
        }
        break;
    }
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
    // Dropping the binding is not enough: activations queued by earlier
    // propagation still sit on the agenda, and the next tick drains them --
    // so an "unbound" law keeps firing from its backlog. Every other
    // retraction path (truncateFacts, retractFact) purges the agenda; these
    // two were the omission that made unbinding cosmetic.
    purgeAgendaOf(lawId);
    dropUnboundAlphaNodes();
}

void ReteNetwork::unbindLawFromAlpha(const std::string& lawId, std::size_t alphaNodeId) {
    auto it = _alphaLawBindings.find(alphaNodeId);
    if (it == _alphaLawBindings.end()) return;
    auto& laws = it->second;
    laws.erase(std::remove(laws.begin(), laws.end(), lawId), laws.end());
    // Activations do not record which alpha queued them, so only purge once
    // this law has no alpha binding left to fire it -- otherwise unbinding
    // one trigger would silently cancel pending work from another.
    bool stillBound = false;
    for (const auto& binding : _alphaLawBindings) {
        if (std::find(binding.second.begin(), binding.second.end(), lawId) !=
            binding.second.end()) {
            stillBound = true;
            break;
        }
    }
    if (!stillBound) purgeAgendaOf(lawId);
    dropUnboundAlphaNodes();
}

void ReteNetwork::purgeAgendaOf(const std::string& lawId) {
    _agenda.erase(std::remove_if(_agenda.begin(), _agenda.end(),
                                 [&](const ReteActivation& a) { return a.lawId == lawId; }),
                  _agenda.end());
}

std::size_t ReteNetwork::internTypeAlpha(const std::string& eventType) {
    auto existing = _typeAlphaIndex.find(eventType);
    if (existing != _typeAlphaIndex.end() && findAlpha(existing->second)) {
        return existing->second;
    }
    const std::size_t id = addAlphaNode(
        "type == " + eventType,
        [eventType](const FactPtr& f) { return f->type == eventType; },
        AlphaSource::Interned);
    _typeAlphaIndex[eventType] = id;
    return id;
}

bool ReteNetwork::hearsType(const std::string& eventType) const {
    auto interned = _typeAlphaIndex.find(eventType);
    if (interned == _typeAlphaIndex.end()) return false;
    auto binding = _alphaLawBindings.find(interned->second);
    if (binding != _alphaLawBindings.end() && !binding->second.empty()) return true;
    for (const auto& beta : _betaNodes) {
        if ((!beta.leftIsBeta && beta.leftId == interned->second) || beta.rightAlphaId == interned->second) {
            auto betaBinding = _betaLawBindings.find(beta.id);
            if (betaBinding != _betaLawBindings.end() && !betaBinding->second.empty()) return true;
        }
    }
    return false;
}

bool ReteNetwork::hasForeignBoundAlpha() const {
    for (const auto& binding : _alphaLawBindings) {
        if (binding.second.empty()) continue;
        const AlphaNode* alpha = findAlpha(binding.first);
        if (alpha && alpha->source == AlphaSource::Foreign) return true;
    }
    return false;
}

bool ReteNetwork::hasOpaqueBoundAlpha() const {
    for (const auto& binding : _alphaLawBindings) {
        if (binding.second.empty()) continue;
        bool interned = false;
        for (const auto& entry : _typeAlphaIndex) {
            if (entry.second == binding.first) { interned = true; break; }
        }
        if (!interned) return true;   // an unreadable predicate: assume it listens
    }
    return false;
}

// A node nobody is bound to can never produce an activation. Propagation
// already skips it, but it still costs the beta scan on every assert and a
// full refill on every bind, so reclaim it rather than letting a session's
// worth of rebinds and world loads pile up.
void ReteNetwork::dropUnboundAlphaNodes() {
    std::vector<std::size_t> doomed;
    for (const auto& alpha : _alphaNodes) {
        auto binding = _alphaLawBindings.find(alpha.id);
        const bool boundToLaw = binding != _alphaLawBindings.end() && !binding->second.empty();
        if (boundToLaw) continue;
        // A beta node reading this alpha still needs it alive.
        bool feedsBeta = false;
        for (const auto& beta : _betaNodes) {
            if ((!beta.leftIsBeta && beta.leftId == alpha.id) || beta.rightAlphaId == alpha.id) {
                feedsBeta = true;
                break;
            }
        }
        if (!feedsBeta) doomed.push_back(alpha.id);
    }
    if (doomed.empty()) return;

    for (std::size_t id : doomed) {
        _alphaLawBindings.erase(id);
        for (auto it = _typeAlphaIndex.begin(); it != _typeAlphaIndex.end();) {
            it = it->second == id ? _typeAlphaIndex.erase(it) : std::next(it);
        }
    }
    _alphaNodes.erase(
        std::remove_if(_alphaNodes.begin(), _alphaNodes.end(),
                       [&doomed](const AlphaNode& alpha) {
                           return std::find(doomed.begin(), doomed.end(), alpha.id) !=
                                  doomed.end();
                       }),
        _alphaNodes.end());
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
            {"id", fact->id},
            {"type", fact->type},
            {"subjectId", fact->subjectId},
            {"attribute", fact->attribute},
            {"value", fact->value}
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
            {"leftIsBeta", beta.leftIsBeta},
            {"leftId", beta.leftId},
            {"rightAlphaId", beta.rightAlphaId},
            {"memorySize", beta.memory.size()}
        });
    }

    nlohmann::json agendaJson = nlohmann::json::array();
    for (const auto& activation : _agenda) {
        nlohmann::json factIds = nlohmann::json::array();
        for (const auto& fact : activation.token.facts) factIds.push_back(fact->id);
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

std::vector<Singular*> ReteNetwork::collectTerminalSubjects(
    const std::vector<std::size_t>& terminalIds) const {
    // Collect unique subjects from terminal node memories.
    // Terminal IDs may refer to alpha or beta nodes — we check both.
    std::unordered_set<Singular*> seen;
    std::vector<Singular*> result;

    for (std::size_t termId : terminalIds) {
        // Check alpha nodes first.
        const AlphaNode* alpha = findAlpha(termId);
        if (alpha) {
            for (const auto& fact : alpha->memory) {
                if (fact->subject && seen.insert(fact->subject).second) {
                    result.push_back(fact->subject);
                }
            }
            continue;
        }
        // Check beta nodes.
        for (const auto& beta : _betaNodes) {
            if (beta.id != termId) continue;
            for (const auto& token : beta.memory) {
                Singular* subj = nullptr;
                for (const auto& fact : token.facts) {
                    if (fact->subject) { subj = fact->subject; break; }
                }
                if (subj && seen.insert(subj).second) {
                    result.push_back(subj);
                }
            }
            break;
        }
    }
    return result;
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
    // The register changed, so every conclusion the Prophetic index drew
    // about it is now about a different set of laws.
    Law::bumpTextRevision();

    // Compile continuous laws' conditions into Rete terminals at registration
    // time, so the O(Matching) path is ready from the first tick.
    if (law->activation() != Law::Activation::OnEvent &&
        law->conditionModel()) {
        compileConditionsToRete(*law);
    }

    LawRegisteredEvent event{law, std::time(nullptr)};
    Core::EventBus::instance().publish(event);

    ECA::Event echo;
    echo.type = "law-registered";
    echo.subject = law.get();
    echo.timestamp = std::time(nullptr);
    Core::EventBus::instance().publish(echo);
}

// The one manager that owns the static Singular hooks. They are statics with
// no owner of their own, and a LawManager is usually block-scoped in tests, so
// somebody has to put them back.
static LawManager* s_singularHookOwner = nullptr;

LawManager::~LawManager() {
    if (s_singularHookOwner == this) {
        Singular::setPropertyChangeCallback(nullptr);
        Singular::setBeingReleasedCallback(nullptr);
        Universe::instance().setEventInterest(nullptr);
        s_singularHookOwner = nullptr;
    }
}

void LawManager::connectToEventBus() {
    if (_connected) return;
    _connected = true;
    s_singularHookOwner = this;

    // "Is anyone listening for this?" — which lets a law skip publishing an
    // echo nobody hears (see Law::publishAppliedEvent). Asked of the NETWORK
    // rather than the trigger table, because laws can be bound to alpha
    // nodes directly (the graph editor does, and so do tests); a trigger-only
    // answer would call those laws deaf and silently stop feeding them.
    // Captured by `this`: the LawManager is an engine-lifetime object, the
    // same contract as the bus subscriptions below.
    Universe::instance().setEventInterest([this](const std::string& type) {
        return _rete.hearsType(type) || _rete.hasOpaqueBoundAlpha();
    });

    // A being that stops existing takes its facts with it. Facts hold RAW
    // participant pointers, so one left behind is a dangling read on the next
    // tick — and until now only the law-driven unmaking path retracted them,
    // which left every being freed by ordinary scope exit (a stack-local
    // Object, a Law and the provenance Relations it owns) behind as a corpse
    // in the fact list. It went unnoticed because almost nothing ever
    // RE-READ those facts: property changes on a ComputedProperty never
    // marked them dirty. Now that they do, this is load-bearing.
    //
    // The pointer is all this may touch — see Singular::notifyBeingReleased.
    Singular::setBeingReleasedCallback([this](const Singular* being) {
        for (const std::string& subjectId : _rete.retractFactsAbout(being)) {
            _seededSubjects.erase(subjectId);
        }
    });

    Singular::setPropertyChangeCallback([this](Singular* owner, const std::string& name) {
        if (!owner) return;
        // Prophetic Rete, Pass 1/2: a property no authored condition can read
        // cannot matter, whoever just wrote it. markFactDirty scans the whole
        // fact list — one state fact per property per being — so this is the
        // difference between O(world) and O(1) on the single hottest callback
        // in the engine. It answers "no" only where the abstract
        // interpretation PROVED no; see LawManager::propheticHears.
        if (!propheticHears(name)) return;
        _rete.markFactDirty(owner->getIdentifier(), name);
        _dirty = true;
    });

    Core::EventBus::instance().subscribe<ECA::Event>([this](const ECA::Event& e) {
        std::string subjectId = e.subject ? e.subject->getIdentifier() : "null";
        std::string objectId = e.object ? e.object->getIdentifier() : "null";

        ECA::LawAuditLogger::instance().log("EVENT", "Event \"" + e.type + "\" triggered (Subject: " + subjectId + ", Object: " + objectId + ")", {
            {"eventType", e.type},
            {"subjectId", subjectId},
            {"objectId", objectId},
            {"timestamp", e.timestamp.toJson()}
        });

        // A being that has just left the world is released from every law
        // that named it. Formations hold RAW pointers and nothing else
        // releases them, so without this a law targeting a deleted object
        // dereferences freed memory every tick for the rest of the session.
        // This covers beings unmade by the delete tool as well as by law.
        if ((e.type == "object-destroyed" || e.type == "relation-destroyed") && e.subject) {
            releaseFromLaws(e.subject);
            _rete.retractStateFactsBySubject(e.subject->getIdentifier());
        }

        auto fact = std::make_shared<ReteFact>();
        fact->type = e.type;
        fact->subject = e.subject;
        fact->subjectId = subjectId;
        fact->object = e.object;
        _rete.assertFact(fact);
        _dirty = true;

        if ((e.type == "object-created" || e.type == "relation-formed") && e.subject) {
            seedStateFacts(e.subject);
        }
    });

    Core::EventBus::instance().subscribe<Core::Event::Custom>([this](const Core::Event::Custom& e) {
        if (!e.relation) return;
        
        std::string evType = e.relation->type;
        std::string aId = e.relation->aId();
        std::string bId = e.relation->bId();
        
        ECA::LawAuditLogger::instance().log("EVENT", "Custom Event \"" + evType + "\" triggered (Id: " + e.relation->getIdentifier() + ")", {
            {"eventType", evType},
            {"eventId", e.relation->getIdentifier()},
            {"sourceId", aId},
            {"targetId", bId}
        });
        
        auto fact = std::make_shared<ReteFact>();
        fact->type = evType;
        fact->subject = e.relation->a();
        fact->subjectId = aId;
        fact->object = e.relation->b();
        
        _rete.assertFact(fact);
        _dirty = true;
    });


    // Per-node action outcomes reach the audit log through the application
    // record (Law::applyTo), which knows the law, the subject, AND whether
    // the write landed. A second, thinner line per successful node here was
    // duplicate noise on the hot path.
}

// ---------------------------------------------------------------------------
// Prophetic Rete — the ahead-of-time half.
//
// Zach's realization (`docs/architecture/law/B-time Rete.md`): in Earthcall
// nothing changes by itself. Every change comes from a Law doing authored
// mathematics on an exposed property, or from a First Mover. The Laws are
// data, and they exist before they fire — so the engine can work out where a
// change could possibly matter BEFORE the change happens, and only has to
// resolve the concrete value when it does.
//
// This is where that ahead-of-time work is kept current. The analysis lives in
// PropheticRete.cpp; all that happens here is deciding when to redo it.
// ---------------------------------------------------------------------------

void LawManager::syncProphetic() {
    const std::uint64_t revision = Law::textRevision();
    if (_propheticRevision == revision) return;   // the law text has not moved

    _prophetic.rebuild(_laws);
    _propheticRevision = revision;

    // Say what was concluded. A condition no authored law can drive into its
    // satisfying range is exactly the thing an author stares at wondering why
    // their law never fires, and the engine already knows.
    for (const auto& finding : _prophetic.unreachable()) {
        ECA::LawAuditLogger::instance().log(
            "LAW",
            std::string(finding.selfImpossible ? "Law \"" : "Law \"") + finding.lawId +
                "\" has a condition on " + finding.path + " that cannot be satisfied: " +
                finding.why,
            {{"lawId", finding.lawId},
             {"path", finding.path},
             {"why", finding.why},
             {"selfImpossible", finding.selfImpossible}});
    }
}

bool LawManager::propheticHears(const std::string& propertyName) const {
    ++_propheticCounters.asked;

    // Three ways to fail open, in the order they are cheapest to check.
    //
    // (1) STALE. A law was added, removed, or edited since the last rebuild,
    //     so the index describes a different law set than the one that is
    //     live. One integer compare, and it is the reason the gate is safe to
    //     consult from a callback that runs between ticks.
    if (_propheticRevision != Law::textRevision()) return true;
    // (2) INCOMPLETE. Some law reads through a closure, a collision test, or a
    //     condition kind this build cannot read. Nothing may be pruned around
    //     a law whose reads are not enumerable.
    if (!_prophetic.complete()) return true;
    // (3) FOREIGN ALPHA. A node bound with a hand-written predicate (the graph
    //     editor, a test, a channel) matches on whatever it likes, and no law
    //     text accounts for it. Deliberately NOT hasOpaqueBoundAlpha(), which
    //     counts every compiled condition too — an authored condition's reads
    //     are exactly the law's own text, which this index has read.
    if (_rete.hasForeignBoundAlpha()) return true;

    if (_prophetic.anyConditionReads(propertyName)) return true;
    ++_propheticCounters.filtered;
    return false;
}

std::vector<Law::ApplicationRecord> LawManager::tick() {
    auto T0 = glfwGetTime();

    // Bring the possibility-space index up to date with the law text before
    // anything consults it. Cheap when nothing moved: one integer compare.
    syncProphetic();

    // Bring compiled terminals up to date with the conditions they were
    // compiled from, before anything reads either. Conditions are edited from
    // the graph window, from tools, and from loaded worlds; asking here means
    // no editing path has to remember to recompile, and the reactive and
    // sweep evaluations cannot be looking at different conditions.
    for (const auto& law : _laws) {
        if (law) syncReteCompilation(*law);
    }
    auto T1 = glfwGetTime();

    // Introduce any being the network has not met. Only while connected: the
    // property-change callback installed by connectToEventBus() is what keeps
    // a seeded fact current, and a snapshot nothing refreshes is worse than no
    // snapshot — the reactive path would answer confidently from stale values.
    // Disconnected, the sweep below reads the beings themselves and is right.
    if (_connected) {
        for (Singular* being : Universe::instance().beings()) {
            seedStateFacts(being);
        }
    }

    auto T2 = glfwGetTime();

    if (_rete.hasDirtyFacts()) {
        _rete.evaluateDirty();
        _dirty = true;
    }

    std::vector<Law::ApplicationRecord> records;
    for (int round = 0; round < _maxChainRounds && _dirty; ++round) {
        _dirty = false;
        // Facts asserted before this round are consumed by it; facts asserted
        // DURING it (laws firing events from applyTo) survive into the next
        // round — that's how law chains resolve, bounded by _maxChainRounds.
        const std::size_t consumed = _rete.facts().size();
        // Straight to the drain: the agenda is already complete. (An
        // _rete.evaluate() call sat here whose result was discarded — the
        // last trace of the rebuild-every-frame design.)
        std::vector<ReteActivation> agenda = _rete.drainAgenda();
        for (const auto& activation : agenda) {
            Law* law = find(activation.lawId);
            if (!law) continue;
            // An activation is a SIGNAL that a match set changed — it is not
            // permission to fire. Continuous laws belong to the continuous
            // pass below, which is the only place that knows about edges.
            // Firing them here as well made OnBecomeTrue level-triggered: it
            // fired from the drain, again in the continuous pass, and again
            // on every re-assert while its condition merely stayed true.
            // "Edges, not levels."
            if (law->activation() != Law::Activation::OnEvent) continue;
            // Disabled and unauthored laws are NOT filtered here. applyTo
            // refuses them and says so in the record, and the refusal is the
            // point: "the attempt is what gets noticed." Skipping them here
            // would make an unauthored law's attempt to enter the world
            // silent, which is the one outcome this gate exists to prevent.
            Singular* subject = activation.token.facts.empty()
                                    ? nullptr
                                    : activation.token.facts.front()->subject;
            Singular* eventObject = activation.token.facts.empty()
                                        ? nullptr
                                        : activation.token.facts.front()->object;

            // The event's PARTICIPANTS stay addressable while the law
            // responds — "@event.subject" / "@event.object" paths let the
            // condition and action phases name them BY CHOICE, whoever the
            // application's subject is. Restores on every exit path.
            Universe::EventScope eventScope(subject, eventObject);

            if (law->scope() == Law::Scope::Everyone) {
                // The event is the OCCASION; the application sweeps every
                // being (targets, or the Universe) that CARRIES THE LAW'S
                // VOCABULARY and satisfies the conditions — "every instance
                // of the category".
                std::vector<Singular*> subjects = sweepSubjects(*law);
                for (Singular* being : subjects) {
                    if (!being || Universe::instance().isUnmade(being)) continue;
                    if (!law->conditionsSatisfied(*being)) continue;
                    // A live drive session OWNS the process: one process,
                    // one clock, per law-and-subject. What a re-firing
                    // event means is the AUTHOR'S choice — Absorb (a block
                    // resting in constant collision cannot stack or reset
                    // the process) or Restart (the new trigger is a new t=0).
                    if (law->drives() &&
                        hasDriveSession(law->getIdentifier(), being->getIdentifier())) {
                        if (law->retrigger() == Law::Retrigger::Absorb) continue;
                        restartDriveSession(*law, being->getIdentifier());
                    }
                    applyAndMaybeDrive(*law, *being, records);
                }
                continue;
            }

            if (!subject || Universe::instance().isUnmade(subject)) continue;
            if (law->drives() &&
                hasDriveSession(law->getIdentifier(), subject->getIdentifier())) {
                if (law->retrigger() == Law::Retrigger::Absorb) {
                    continue;   // the session owns the process (see above)
                }
                restartDriveSession(*law, subject->getIdentifier());
            }
            applyAndMaybeDrive(*law, *subject, records);
        }
        _rete.retractFirst(consumed);
    }

    // ------------------------------------------------------------------
    // Continuous pass: level-triggered laws don't wait for events — their
    // condition phase monitors the program every tick. Subjects come from
    // the law's targets Formation when present, otherwise from the beings
    // that carry its vocabulary. (Events a continuous application fires —
    // the law-applied echo — become facts for the NEXT tick's event rounds.)
    //
    // Iterating a COPY of the register: a law may create or destroy laws,
    // and mutating _laws under the loop would invalidate the iterator.
    // ------------------------------------------------------------------
    const std::vector<std::shared_ptr<Law>> continuousLaws = _laws;
    for (const auto& law : continuousLaws) {
        if (!law || law->activation() == Law::Activation::OnEvent) continue;
        if (!law->isEnabled() || !law->isAuthored()) continue;

        const std::string lawId = law->getIdentifier();
        auto termIt = _reteTerminals.find(lawId);
        // Terminals alone do not license the reactive path: it answers from
        // state facts, and state facts are only as current as the change feed
        // that maintains them. Unconnected, there is no feed — so there is no
        // reactive answer to give, only a stale one. Fall through to the sweep.
        const bool hasTerminals =
            _connected && termIt != _reteTerminals.end() && !termIt->second.empty();

        // WhileTrue laws with compiled Rete terminals: O(Matching) path.
        // The terminal node memories already contain exactly the beings that
        // satisfy all conditions, so we skip both sweepSubjects AND
        // conditionsSatisfied — the Rete network has done both reactively.
        //
        // The activation test is LOAD-BEARING and reads as redundant, so it has
        // been deleted once already (04c52ed4, inside a commit about FPS
        // measuring). This path applies to every matching subject with no edge
        // check at all — that is what makes it right for a level-triggered law
        // and wrong for an edge-triggered one, which then re-fires every tick
        // for as long as its condition keeps holding. CLAUDE.md's
        // non-negotiable: "Event-transitions must be edges, not levels... A
        // per-frame 'still happening' event is a bug — that is what WhileTrue
        // is for." The comment below this block already says so. Guarded by
        // tests/law/rete_compile_test.cpp, which went red the day it was
        // dropped ("an edge fires once, not once per tick").
        if (hasTerminals && law->activation() == Law::Activation::WhileTrue) {
            std::vector<std::size_t> termIds;
            termIds.reserve(termIt->second.size());
            for (const auto& info : termIt->second) termIds.push_back(info.nodeId);

            std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);

            // WHOM the law is about is the author's answer, not the network's.
            // The terminal memories hold every being that satisfies the
            // conditions; a law scoped to one being still applies to that one
            // being. Without this, an authored targets Formation was silently
            // ignored on the reactive path and honored on the sweep — the same
            // law meaning two different things depending on which path ran.
            const auto& targets = law->targets().getMembers();
            if (!targets.empty()) {
                std::unordered_set<const Singular*> allowed(targets.begin(), targets.end());
                subjects.erase(std::remove_if(subjects.begin(), subjects.end(),
                                              [&allowed](Singular* s) {
                                                  return allowed.count(s) == 0;
                                              }),
                               subjects.end());
            }

            // Onset bookkeeping, which the sweep below does and this path did
            // not: t=0 for `time.sinceApplied` is the moment the condition went
            // false->true for this subject. With no onset remembered, applyTo
            // falls back to "now" every tick, so every Flow and Drive authored
            // against that clock read t=0 forever.
            std::unordered_set<std::string> matching;
            matching.reserve(subjects.size());
            for (Singular* subject : subjects) {
                if (!subject || Universe::instance().isUnmade(subject)) continue;
                const std::string subjectId = subject->getIdentifier();
                matching.insert(subjectId);
                const bool wasHolding = law->lastConditionState(subjectId);
                law->rememberConditionState(subjectId, true);
                if (!wasHolding && Universe::instance().hasClock()) {
                    law->rememberOnset(subjectId, Universe::instance().now());
                }
            }

            // Release: whoever the law held for last tick and does not now.
            // Collected first, because forgetting mutates what we are reading.
            std::vector<std::string> released;
            for (const auto& [subjectId, held] : law->conditionMemory()) {
                if (held && matching.count(subjectId) == 0) released.push_back(subjectId);
            }
            for (const auto& subjectId : released) {
                law->rememberConditionState(subjectId, false);
                law->forgetOnset(subjectId);
            }

            for (Singular* subject : subjects) {
                if (!subject || Universe::instance().isUnmade(subject)) continue;
                const std::string subjectId = subject->getIdentifier();
                if (law->drives() &&
                    hasDriveSession(lawId, subjectId)) {
                    if (law->retrigger() == Law::Retrigger::Absorb) continue;
                    restartDriveSession(*law, subjectId);
                }
                applyAndMaybeDrive(*law, *subject, records);
            }
            continue;
        }

        // OnBecomeTrue and laws without Rete terminals: full sweep path.
        // Edge detection requires knowing when a being LEAVES the match set,
        // so the full sweep is still necessary here.
        std::vector<Singular*> subjects = sweepSubjects(*law);

        for (Singular* subject : subjects) {
            if (!subject || Universe::instance().isUnmade(subject)) continue;
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
                if (law->retrigger() == Law::Retrigger::Absorb) {
                    continue;   // a re-edge while the launched process still
                                // runs is absorbed — the session owns it
                }
                restartDriveSession(*law, subjectId);   // a re-edge = new t=0
            }
            // An OnBecomeTrue law that drives launches its process at the
            // edge and runs it to the end of its authored bounds.
            applyAndMaybeDrive(*law, *subject, records);
        }
    }

    auto T3 = glfwGetTime();
    runDriveSessions(records);
    auto T4 = glfwGetTime();
    reapUnmade();
    auto T5 = glfwGetTime();

    _tickTiming.syncMs  = static_cast<float>((T1 - T0) * 1000.0);
    _tickTiming.seedMs  = static_cast<float>((T2 - T1) * 1000.0);
    _tickTiming.evalMs  = static_cast<float>((T3 - T2) * 1000.0);
    _tickTiming.driveMs = static_cast<float>((T4 - T3) * 1000.0);
    _tickTiming.reapMs  = static_cast<float>((T5 - T4) * 1000.0);
    _tickTiming.totalMs = static_cast<float>((T5 - T0) * 1000.0);

    return records;
}

// ---------------------------------------------------------------------------
// One place decides whether an application earns a drive session, because
// there were three and they all asked the wrong question.
//
// The old test was `applyTo(...) == Applied` — which only says the action
// branch was reached. A law whose every write failed passed that test, got
// handed a process, and re-applied itself forever, failing every tick. The
// question a drive session answers is "did this law DO something", and the
// node trace is what knows.
// ---------------------------------------------------------------------------
void LawManager::applyAndMaybeDrive(Law& law, Singular& subject,
                                    std::vector<Law::ApplicationRecord>& records) {
    const Law::ApplicationResult result = law.applyTo(subject);
    if (law.applicationLog().empty()) return;
    const Law::ApplicationRecord& record = law.applicationLog().back();
    records.push_back(record);

    if (result != Law::ApplicationResult::Applied) return;
    maybeStartDriveSession(law, subject);
}

// Who a law sweeps when it has no targets Formation: not everyone, but
// everyone who CARRIES ITS VOCABULARY (see Law::requiredProperties).
std::vector<Singular*> LawManager::sweepSubjects(const Law& law) const {
    const auto& targets = law.targets().getMembers();
    if (!targets.empty()) {
        // An explicit targets Formation is the author's own answer to "whom",
        // and it overrides the derived filter — but a target that has since
        // been unmade is still no one.
        std::vector<Singular*> chosen;
        chosen.reserve(targets.size());
        for (Singular* target : targets) {
            if (target && !Universe::instance().isUnmade(target)) chosen.push_back(target);
        }
        return chosen;
    }

    std::vector<Singular*> beings = Universe::instance().beings();
    if (law.requiredProperties().empty()) return beings;   // truly about everyone
    beings.erase(std::remove_if(beings.begin(), beings.end(),
                                [&law](Singular* being) {
                                    return !being || !law.couldApplyTo(*being);
                                }),
                 beings.end());
    return beings;
}

// ---------------------------------------------------------------------------
// The reaper. Every pass of the tick is done; no snapshot vector, no agenda
// activation, and no applyTo stack frame still holds a pointer to a victim.
// Only now is it safe to free — and before freeing, every Formation that
// names the victim must let go, because Formations hold RAW pointers and
// Zone::removeObject only releases the ones held by other Objects. A law
// targeting a destroyed object used to dereference freed memory every tick
// for the rest of the session.
// ---------------------------------------------------------------------------
void LawManager::reapUnmade() {
    if (!Universe::instance().hasUnmakings()) return;
    // Snapshot the victims for the fact purge below; reapUnmadeBeings consumes
    // the queue and does the freeing.
    const std::vector<Singular*> victims = Universe::instance().unmakings();

    // Release from OUR laws directly rather than waiting for the
    // "object-destroyed" announcement to come back around. The subscription
    // still exists — it is what catches beings the delete tool unmakes — but
    // a LawManager's own bookkeeping must not depend on having been connected
    // to a global bus that cannot be unsubscribed from.
    for (Singular* victim : victims) {
        releaseFromLaws(victim);
    }
    reapUnmadeBeings();
    // The unmaking announcement became a fact carrying a pointer that is now
    // dangling, and facts outlive the round that asserted them. Purge them:
    // a law that responds to unmaking hears it live, through the bus, while
    // the being still exists — it must not be handed the corpse next tick.
    for (Singular* victim : victims) {
        _rete.retractFactsAbout(victim);
    }
}

// The freeing itself, callable without a LawManager: a compiled Destroy node
// fired by a tool or a test defers exactly the same way, and something has to
// finish what it started.
void reapUnmadeBeings() {
    if (!Universe::instance().hasUnmakings()) return;
    std::vector<Singular*> victims = Universe::instance().takeUnmakings();

    // Collect the Zones BEFORE any removal: beings() rebuilds from the
    // provider each call, and a Zone is not what we are freeing anyway.
    std::vector<Zone*> zones;
    for (Singular* being : Universe::instance().beings()) {
        if (auto* zone = dynamic_cast<Zone*>(being)) zones.push_back(zone);
    }

    for (Singular* victim : victims) {
        auto* asObject = dynamic_cast<Object*>(victim);
        if (!asObject) continue;
        for (Zone* zone : zones) {
            if (zone->removeObject(asObject)) break;   // publishes object-destroyed
        }
    }
}

// A being that leaves the world leaves every law that named it. Called by the
// reaper and by the "object-destroyed" subscriber, so beings unmade by the
// delete tool are released too — not only the ones a law unmade.
void LawManager::releaseFromLaws(Singular* being) {
    if (!being) return;
    for (const auto& law : _laws) {
        if (!law) continue;
        law->targets().removeMember(being);
        law->conditions().removeMember(being);
        law->authors().removeMember(being);
    }
    const std::string id = being->getIdentifier();
    // Forget that we introduced it to the network, so an id reused by a later
    // being is seeded afresh instead of being taken for one we already know.
    _seededSubjects.erase(id);
    _driveSessions.erase(
        std::remove_if(_driveSessions.begin(), _driveSessions.end(),
                       [&id](const DriveSession& s) { return s.subjectId == id; }),
        _driveSessions.end());
}

void LawManager::restartDriveSession(Law& law, const std::string& subjectId) {
    // The retrigger IS a new t=0 (the author chose Restart): the session
    // keeps its identity but its clock and event participants begin again.
    if (!Universe::instance().hasClock()) return;
    const double now = Universe::instance().now();
    for (auto& session : _driveSessions) {
        if (session.lawId != law.getIdentifier() || session.subjectId != subjectId) {
            continue;
        }
        session.onset = now;
        session.eventSubjectId.clear();
        session.eventObjectId.clear();
        if (Universe::instance().hasApplicationEvent()) {
            if (Singular* s = Universe::instance().applicationEventSubject()) {
                session.eventSubjectId = s->getIdentifier();
            }
            if (Singular* o = Universe::instance().applicationEventObject()) {
                session.eventObjectId = o->getIdentifier();
            }
        }
        law.rememberOnset(subjectId, now);
        return;
    }
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
    DriveSession session;
    session.lawId = law.getIdentifier();
    session.subjectId = subjectId;
    session.onset = onset;
    // Remember the launching event's participants (when there is one) so
    // "@event.*" paths keep resolving for the drive's whole life.
    if (Universe::instance().hasApplicationEvent()) {
        if (Singular* s = Universe::instance().applicationEventSubject()) {
            session.eventSubjectId = s->getIdentifier();
        }
        if (Singular* o = Universe::instance().applicationEventObject()) {
            session.eventObjectId = o->getIdentifier();
        }
    }
    _driveSessions.push_back(std::move(session));
}

void LawManager::runDriveSessions(std::vector<Law::ApplicationRecord>& records) {
    if (_driveSessions.empty() || !Universe::instance().hasClock()) return;
    const double now = Universe::instance().now();

    // ONE snapshot for the whole pass. Universe::beings() rebuilds the vector
    // from the provider on every call — every object, law, relation, and zone
    // in the world — and this used to happen three times per session per tick
    // (subject, event subject, event object).
    const std::vector<Singular*> beings = Universe::instance().beings();

    for (auto it = _driveSessions.begin(); it != _driveSessions.end();) {
        Law* law = find(it->lawId);
        const auto findBeing = [&](const std::string& id) -> Singular* {
            if (id.empty()) return nullptr;
            for (Singular* being : beings) {
                if (being && being->getIdentifier() == id) return being;
            }
            if (law) {
                for (Singular* target : law->targets().getMembers()) {
                    if (!target || Universe::instance().isUnmade(target)) continue;
                    if (target->getIdentifier() == id) return target;
                }
            }
            return nullptr;
        };
        Singular* subject = law ? findBeing(it->subjectId) : nullptr;

        // A law or being that left the world ends its sessions silently.
        if (!law || !subject || !law->isEnabled()) {
            if (law) law->forgetOnset(it->subjectId);
            it = _driveSessions.erase(it);
            continue;
        }

        // The launching event's participants stay addressable ("@event.*")
        // for the drive's whole life; a participant that left the world
        // resolves to nothing. Restores on every exit path.
        Universe::EventScope eventScope(findBeing(it->eventSubjectId),
                                        findBeing(it->eventObjectId));

        // The authored bounds ARE the duration — and ANY bound variable may
        // cut them (time, another being's position, the subject's own
        // state). The drive lives while the function is still defined for
        // the subject; checked in the law's application context so
        // time.sinceApplied resolves like any other input variable. A law
        // whose action has no bounded function drives until disabled.
        bool alive = true;
        if (law->hasActionModel()) {
            Universe::OnsetScope onsetScope(it->onset);
            alive = law->actionModel()->definedFor(*subject);
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

    _rete.unbindLaw(lawId);
    _triggers.erase(lawId);
    _reteTerminals.erase(lawId);
    _compiledConditionRevision.erase(lawId);
    _lawFormation.removeMember(it->get());
    _laws.erase(it);
    Law::bumpTextRevision();
    return true;
}

void LawManager::bindTrigger(const std::string& lawId, const std::string& eventType) {
    if (eventType.empty()) return;
    auto& bound = _triggers[lawId];
    if (std::find(bound.begin(), bound.end(), eventType) != bound.end()) return;
    bound.push_back(eventType);
    _rete.bindLawToAlpha(lawId, _rete.internTypeAlpha(eventType));
    // Binding backfills any still-live fact of this type onto the agenda. Mark
    // the manager dirty so a round actually drains that backlog — a law bound
    // mid-tick (laws may create laws) would otherwise wait for the next
    // unrelated event before hearing what was already asserted. Only when
    // something is genuinely pending, so an ordinary bind on a quiet network
    // does not buy a wasted round.
    if (!_rete.agenda().empty()) _dirty = true;
}

void LawManager::unbindTrigger(const std::string& lawId, const std::string& eventType) {
    auto it = _triggers.find(lawId);
    if (it == _triggers.end()) return;
    auto& bound = it->second;
    const bool had = std::find(bound.begin(), bound.end(), eventType) != bound.end();
    bound.erase(std::remove(bound.begin(), bound.end(), eventType), bound.end());
    // Surgical: unbind this law from THAT type's shared node. The old code
    // tore down every binding and rebuilt the survivors on brand-new nodes,
    // which leaked a node per call and left the other laws bound to nodes
    // that were about to be orphaned.
    if (had) {
        _rete.unbindLawFromAlpha(lawId, _rete.internTypeAlpha(eventType));
    }
    if (bound.empty()) _triggers.erase(it);
}

const std::vector<std::string>& LawManager::triggersOf(const std::string& lawId) const {
    static const std::vector<std::string> kNone;
    auto it = _triggers.find(lawId);
    return it == _triggers.end() ? kNone : it->second;
}

void LawManager::seedStateFacts(Singular* being) {
    if (!being) return;
    const std::string subjectId = being->getIdentifier();
    if (subjectId.empty()) return;
    if (!_seededSubjects.insert(subjectId).second) return;   // already known

    for (auto* prop : being->listProperties()) {
        if (!prop) continue;
        auto stateFact = std::make_shared<ReteFact>();
        stateFact->type = "property-state";
        stateFact->subject = being;
        stateFact->subjectId = subjectId;
        stateFact->attribute = prop->name();
        stateFact->value = propertyValueToJson(prop->value());
        stateFact->isState = true;
        stateFact->dirty = false;
        _rete.assertFact(stateFact);
    }
}

void LawManager::syncReteCompilation(Law& law) {
    const std::string lawId = law.getIdentifier();
    const bool wantsRete =
        law.activation() != Law::Activation::OnEvent && law.conditionModel() != nullptr;

    auto revIt = _compiledConditionRevision.find(lawId);
    const bool alreadyCompiled = revIt != _compiledConditionRevision.end();

    if (!wantsRete) {
        // Cleared its condition, or became an OnEvent law. Terminals compiled
        // from a condition it no longer holds would keep queueing it.
        if (alreadyCompiled) {
            _rete.unbindLaw(lawId);
            _reteTerminals.erase(lawId);
            _compiledConditionRevision.erase(revIt);
        }
        return;
    }

    if (alreadyCompiled && revIt->second == law.conditionRevision()) return;
    compileConditionsToRete(law);
}

void LawManager::compileConditionsToRete(Law& law) {
    const std::string lawId = law.getIdentifier();
    // Stamped first, and unconditionally: the paths below that give up early
    // (no model, nothing compilable) are still a complete answer for THIS
    // revision, and re-deciding it every tick would be a standing tax.
    _compiledConditionRevision[lawId] = law.conditionRevision();

    // Unbind old terminals for this law (if recompiling).
    auto oldIt = _reteTerminals.find(lawId);
    if (oldIt != _reteTerminals.end()) {
        // The old condition nodes are still in the rete — we need to unbind
        // the law from them. The nodes themselves will be dropped by
        // dropUnboundAlphaNodes if nothing else reads them.
        _rete.unbindLaw(lawId);
        _reteTerminals.erase(oldIt);
    }

    if (!law.conditionModel()) return;
    const ConditionModel& model = *law.conditionModel();

    // Compile the tree into the rete network, getting terminal node IDs.
    std::vector<std::size_t> terminals = model.compileToRete(_rete, lawId);
    if (terminals.empty()) return;

    // Determine whether each terminal is an alpha or beta node.
    std::vector<TerminalInfo> infos;
    infos.reserve(terminals.size());
    for (std::size_t termId : terminals) {
        bool isBeta = !_rete.isAlphaNode(termId);
        infos.push_back({termId, isBeta});

        // Bind the law to each terminal so that the Rete network knows to
        // queue activations for this law when facts match.
        if (isBeta) {
            _rete.bindLawToBeta(lawId, termId);
        } else {
            _rete.bindLawToAlpha(lawId, termId);
        }
    }
    _reteTerminals[lawId] = std::move(infos);
}

void LawManager::loadFromJson(const nlohmann::json& j) {
    // Replace-all, like the world loader — EXCEPT first movers, whose truth
    // lives in the engine and survives every load.
    std::vector<std::shared_ptr<Law>> firstMovers;
    for (const auto& law : _laws) {
        if (!law) continue;
        if (law->isFirstMover()) {
            firstMovers.push_back(law);
            continue;
        }
        _rete.unbindLaw(law->getIdentifier());
        _lawFormation.removeMember(law.get());
        _triggers.erase(law->getIdentifier());
        _reteTerminals.erase(law->getIdentifier());
        _compiledConditionRevision.erase(law->getIdentifier());
    }
    _laws = std::move(firstMovers);
    _driveSessions.clear();
    Law::bumpTextRevision();
    _reteTerminals.clear();
    _compiledConditionRevision.clear();

    if (j.contains("maxChainRounds")) {
        _maxChainRounds = j["maxChainRounds"].get<int>();
    } else {
        _maxChainRounds = 8;
    }

    const auto findBeing = [](const std::string& id) -> Singular* {
        for (Singular* being : Universe::instance().beings()) {
            if (being && being->getIdentifier() == id) return being;
        }
        return nullptr;
    };

    if (j.contains("laws")) {
        for (const auto& lj : j["laws"]) {
            auto law = Law::fromJson(lj);
            // World references reattach BY IDENTIFIER. An author who is not
            // in the world stays detached: the law remains Unauthored and
            // cannot fire — visible in the Law Author, never silent.
            if (lj.contains("authors")) {
                for (const auto& idJson : lj["authors"]) {
                    if (Singular* being = findBeing(idJson.get<std::string>())) {
                        law->addAuthor(*being);
                    }
                }
            }
            if (lj.contains("targets")) {
                for (const auto& idJson : lj["targets"]) {
                    if (Singular* being = findBeing(idJson.get<std::string>())) {
                        law->addTarget(*being);
                    }
                }
            }
            add(law);
        }
    }
    if (j.contains("triggers")) {
        for (auto it = j["triggers"].begin(); it != j["triggers"].end(); ++it) {
            // A trigger for a law that is not in the register binds nothing
            // real: it creates a live binding on an alpha node whose
            // activations resolve to null every tick, forever. Saved worlds
            // accumulate these as laws come and go.
            if (!find(it.key())) continue;
            for (const auto& type : it.value()) {
                bindTrigger(it.key(), type.get<std::string>());
            }
        }
    }

    // Re-apply the Person's first-mover gates onto the engine-owned survivors.
    // Missing keys leave the first mover at its boot default (on, except
    // channels that opt in disabled). A first mover that is not in this
    // world yet cannot be addressed — that is the same covenant as authors.
    if (j.contains("firstMoverEnabled") && j["firstMoverEnabled"].is_object()) {
        for (auto it = j["firstMoverEnabled"].begin(); it != j["firstMoverEnabled"].end(); ++it) {
            Law* law = find(it.key());
            if (!law || !law->isFirstMover()) continue;
            if (it.value().is_boolean()) law->setEnabled(it.value().get<bool>());
        }
    }

    // Compile continuous laws' conditions into Rete terminals.
    // This must happen after add() and after trigger binding, because the
    // condition model needs to be set and the Rete network populated.
    for (const auto& law : _laws) {
        if (!law) continue;
        if (law->activation() == Law::Activation::OnEvent) continue;
        if (!law->conditionModel()) continue;
        compileConditionsToRete(*law);
    }
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
    nlohmann::json firstMoverEnabled = nlohmann::json::object();
    for (const auto& law : _laws) {
        if (!law) continue;
        // First movers' truth lives in the engine (physics laws persist in
        // their own save section); serializing the bridge would forge it.
        // Their ENABLED bit is not engine truth — it is the Person's choice
        // to set a first mover down so authored law can take over. Persist
        // only that bit, keyed by the stable slug.
        if (law->isFirstMover()) {
            firstMoverEnabled[law->getIdentifier()] = law->isEnabled();
            continue;
        }
        arr.push_back(law->toJson());
    }
    nlohmann::json triggersJson = nlohmann::json::object();
    for (const auto& entry : _triggers) {
        triggersJson[entry.first] = entry.second;
    }
    return nlohmann::json{
        {"laws", arr},
        {"triggers", triggersJson},
        {"formationMembers", formationMemberIds(_lawFormation)},
        {"rete", _rete.toJson()},
        {"firstMoverEnabled", firstMoverEnabled},
        {"maxChainRounds", _maxChainRounds}
    };
}
