#include "Singularity/TransferPolicy.hpp"

#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <algorithm>

namespace {
bool s_transferPolicyAlive = false;
const Law* s_activeLaw = nullptr;
Singular* s_activeActor = nullptr;
uint64_t s_nextScopeId = 0;
uint64_t s_activeScopeId = 0;

const Law* resolveGoverningLaw(const PolicyRule& rule) {
    if (rule.governingLaw) return rule.governingLaw;
    if (!rule.governingLawId.empty()) {
        for (Singular* b : Universe::instance().beings()) {
            if (const auto* l = dynamic_cast<const Law*>(b)) {
                if (l->getIdentifier() == rule.governingLawId) {
                    rule.governingLaw = l;
                    return l;
                }
            }
        }
    }
    return nullptr;
}

Zone* resolveZoneForBeing(const Singular* being, const Law* fallbackLaw = nullptr) {
    if (being) {
        if (const auto* asZone = dynamic_cast<const Zone*>(being)) {
            return const_cast<Zone*>(asZone);
        }
        if (ZoneManager* zm = ZoneManager::live()) {
            if (Zone* res = zm->residenceOf(*being)) {
                return res;
            }
        }
        for (Singular* candidate : Universe::instance().beings()) {
            if (auto* z = dynamic_cast<Zone*>(candidate)) {
                if (z->getFormation().hasMember(being)) return z;
                for (const auto& obj : z->objects()) {
                    if (obj.get() == being) return z;
                }
                for (const auto& stored : z->storedSingulars()) {
                    if (stored.get() == being) return z;
                }
            }
        }
    }
    if (fallbackLaw) {
        if (fallbackLaw->jurisdiction()) {
            return fallbackLaw->jurisdiction().get();
        }
        for (Singular* candidate : Universe::instance().beings()) {
            if (auto* z = dynamic_cast<Zone*>(candidate)) {
                if (z->getFormation().hasMember(fallbackLaw)) return z;
            }
        }
    }
    return nullptr;
}
} // namespace

TransferPolicy::ExecutionScope::ExecutionScope(const Law* law, Singular* actor)
    : _prevLaw(s_activeLaw), _prevActor(s_activeActor), _prevScopeId(s_activeScopeId) {
    s_activeLaw = law;
    s_activeActor = actor;
    s_activeScopeId = ++s_nextScopeId;
}

TransferPolicy::ExecutionScope::~ExecutionScope() {
    s_activeLaw = _prevLaw;
    s_activeActor = _prevActor;
    s_activeScopeId = _prevScopeId;
}

AccessContext TransferPolicy::currentContext(AccessOp op) {
    AccessContext ctx;
    ctx.actor = s_activeActor;
    ctx.authorizingLaw = s_activeLaw;
    ctx.op = op;
    return ctx;
}

uint64_t TransferPolicy::activeScopeId() {
    return s_activeScopeId;
}

void TransferPolicy::notifyBearerDestroyed(const Singular* bearer) {
    if (!s_transferPolicyAlive || !bearer) return;
    TransferPolicy::instance().removeRulesForBearer(bearer);
}

TransferPolicy& TransferPolicy::instance() {
    static TransferPolicy policy;
    return policy;
}

TransferPolicy::~TransferPolicy() {
    s_transferPolicyAlive = false;
}

TransferPolicy::TransferPolicy() {
    s_transferPolicyAlive = true;
    setObjectID(std::string("transfer-policy"));
    setPhysicalObject(0);   // extra-spatial: an interface between Person and Singularity

    // The seed covenant. Kernel: spatial placement is creation's substance —
    // universally transferable, immune to lower-order constraints.
    _tiers["position"] = Tier::Kernel;
    _tiers["rotation"] = Tier::Kernel;
    _tiers["center"] = Tier::Kernel;
    _tiers["authors"] = Tier::Kernel;
    _tiers["targets"] = Tier::Kernel;
    _tiers["authorityOver"] = Tier::Kernel;
    _tiers["isFirstMover"] = Tier::Kernel;
    _tiers["authority"] = Tier::Kernel;
    // Governable: open by default, but laws may close them.
    _tiers["shape"] = Tier::Governable;
    // Gated: governance state — taking it requires authorization by law.
    _tiers["enabled"] = Tier::Gated;
    _tiers["conditionMode"] = Tier::Gated;
    _tiers["drives"] = Tier::Gated;
    _tiers["name"] = Tier::Gated;
    _tiers["type"] = Tier::Gated;
    _tiers["weight"] = Tier::Gated;
    _tiers["directed"] = Tier::Gated;

    for (const auto& entry : _tiers) {
        _open[entry.first] = entry.second != Tier::Gated;
    }
}

TransferPolicy::Tier TransferPolicy::tierOf(const std::string& gateName) const {
    auto it = _tiers.find(gateName);
    return it == _tiers.end() ? Tier::Governable : it->second;
}

bool TransferPolicy::isOpen(const std::string& gateName) const {
    if (tierOf(gateName) == Tier::Kernel) return true;
    auto it = _open.find(gateName);
    return it == _open.end() ? true : it->second;   // unlisted: Governable-open
}

bool TransferPolicy::setOpen(const std::string& gateName, bool open) {
    if (tierOf(gateName) == Tier::Kernel) return false;   // the floor holds
    _open[gateName] = open;
    return true;
}

bool TransferPolicy::canTransfer(const PropertyPath& source) const {
    if (source.segments.empty()) return false;
    std::size_t first = 0;
    if (!source.segments[0].empty() && source.segments[0][0] == '@') {
        first = source.segments[0] == "@event" ? 2 : 1;
    }
    if (first >= source.segments.size()) return false;
    return isOpen(source.segments[first]);
}

// Every gate is a legible bool property "gate.<name>" — laws govern transfer
// access through the ordinary property bridge. Kernel gates register
// read-only: setValue refuses, exactly like writing to "time".
void TransferPolicy::buildProperties() {
    struct GateBridge {
        // ComputedProperty wants member-function getters/setters; a tiny
        // heap-stable closure object per gate does the adapting.
        TransferPolicy* policy;
        std::string gate;
        bool get() const { return policy->isOpen(gate); }
        void set(const bool& v) { policy->setOpen(gate, v); }
    };
    static std::vector<std::unique_ptr<GateBridge>> bridges;
    bridges.clear();
    for (const auto& entry : _tiers) {
        auto bridge = std::make_unique<GateBridge>();
        bridge->policy = this;
        bridge->gate = entry.first;
        GateBridge* raw = bridge.get();
        bridges.push_back(std::move(bridge));
        if (entry.second == Tier::Kernel) {
            registerProperty(std::make_unique<ComputedProperty<GateBridge, bool>>(
                "gate." + entry.first, raw, &GateBridge::get, nullptr));
        } else {
            registerProperty(std::make_unique<ComputedProperty<GateBridge, bool>>(
                "gate." + entry.first, raw, &GateBridge::get, &GateBridge::set));
        }
    }

    registerProperty(std::make_unique<ComputedProperty<TransferPolicy, std::string>>(
        "rule.pathPattern", this, &TransferPolicy::propRulePathPattern, &TransferPolicy::propSetRulePathPattern));
    registerProperty(std::make_unique<ComputedProperty<TransferPolicy, std::string>>(
        "rule.targetCellPath", this, &TransferPolicy::propRuleTargetCellPath, &TransferPolicy::propSetRuleTargetCellPath));
    registerProperty(std::make_unique<ComputedProperty<TransferPolicy, std::string>>(
        "rule.op", this, &TransferPolicy::propRuleOp, &TransferPolicy::propSetRuleOp));
    registerProperty(std::make_unique<ComputedProperty<TransferPolicy, bool>>(
        "rule.allow", this, &TransferPolicy::propRuleAllow, &TransferPolicy::propSetRuleAllow));
    registerProperty(std::make_unique<ComputedProperty<TransferPolicy, std::string>>(
        "rule.governingZoneId", this, &TransferPolicy::propRuleGoverningZoneId, &TransferPolicy::propSetRuleGoverningZoneId));
    registerProperty(std::make_unique<ComputedProperty<TransferPolicy, std::string>>(
        "rule.originZoneId", this, &TransferPolicy::propRuleOriginZoneId, &TransferPolicy::propSetRuleOriginZoneId));
    registerProperty(std::make_unique<ComputedProperty<TransferPolicy, std::string>>(
        "rule.targetZoneId", this, &TransferPolicy::propRuleTargetZoneId, &TransferPolicy::propSetRuleTargetZoneId));
    registerProperty(std::make_unique<ComputedProperty<TransferPolicy, bool>>(
        "rule.commit", this, &TransferPolicy::propRuleCommit, &TransferPolicy::propSetRuleCommit));
    registerProperty(std::make_unique<ComputedProperty<TransferPolicy, int>>(
        "ruleCount", this, &TransferPolicy::propRuleCount, nullptr));
}

void TransferPolicy::propSetRuleCommit(const bool& v) {
    if (!v) {
        _stagedCommit = false;
        return;
    }
    PolicyRule r;
    r.pathPattern = _stagedPathPattern;
    r.op = accessOpFromString(_stagedOp);
    r.allow = _stagedAllow;
    r.governingZoneId = _stagedGoverningZoneId;
    r.originZoneId = _stagedOriginZoneId;
    r.targetZoneId = _stagedTargetZoneId;
    r.governingLaw = currentContext().authorizingLaw;
    if (r.governingLaw) {
        r.governingLawId = r.governingLaw->getIdentifier();
    }
    if (r.governingZoneId.empty() && r.governingLaw) {
        if (Zone* z = resolveZoneForBeing(nullptr, r.governingLaw)) {
            r.governingZoneId = z->getIdentifier();
        }
    }
    if (!_stagedTargetCellPath.empty()) {
        PropertyPath cellPath = PropertyPath::parse(_stagedTargetCellPath);
        std::size_t startIdx = 0;
        Singular* fallbackSubj = currentContext().actor ? currentContext().actor : this;
        Singular* root = resolveLawRoot(*fallbackSubj, cellPath, startIdx);
        if (root && startIdx < cellPath.segments.size()) {
            auto slot = cellPath.resolve(*root, startIdx);
            Singular* cellBearer = slot.owner ? slot.owner : root;
            if (slot.cellId.valid()) {
                r.cellId = slot.cellId;
            } else if (slot.prop || slot.dynamicSlot) {
                Earthcall::StringId propId = slot.prop
                    ? slot.prop->nameId()
                    : Earthcall::StringInterner::intern(slot.dynamicKey);
                PropertyValue initialVal = slot.prop ? slot.prop->value() : *slot.dynamicSlot;
                CellId newCellId = PropertyCellTable::instance().allocateCell(
                    initialVal, CellOwnershipMode::Shared, cellBearer, propId);
                cellBearer->bindPropertyCell(propId, newCellId, CellOwnershipMode::Shared);
                r.cellId = newCellId;
            }
        }
    }
    r.createdInScopeId = s_activeScopeId;
    addPolicyRule(r);

    _stagedPathPattern.clear();
    _stagedTargetCellPath.clear();
    _stagedOp = "write";
    _stagedAllow = true;
    _stagedGoverningZoneId.clear();
    _stagedOriginZoneId.clear();
    _stagedTargetZoneId.clear();
    _stagedCommit = false;

    Singular::notifyPropertyChanged(this, "ruleCount");
    Singular::notifyPropertyChanged(this, "rule.commit");
}

bool TransferPolicy::hasNonDefaultState() const {
    if (!_rules.empty()) return true;
    for (const auto& entry : _tiers) {
        if (entry.second == Tier::Kernel) continue;
        bool defaultOpen = (entry.second != Tier::Gated);
        if (isOpen(entry.first) != defaultOpen) return true;
    }
    return false;
}

nlohmann::json TransferPolicy::toJson() const {
    nlohmann::json open = nlohmann::json::object();
    for (const auto& entry : _open) open[entry.first] = entry.second;

    nlohmann::json rules = nlohmann::json::array();
    for (const auto& r : _rules) {
        nlohmann::json ruleObj;
        ruleObj["pathPattern"] = r.pathPattern;
        ruleObj["op"] = accessOpToString(r.op);
        ruleObj["allow"] = r.allow;
        ruleObj["governingZoneId"] = r.governingZoneId;
        ruleObj["originZoneId"] = r.originZoneId;
        ruleObj["targetZoneId"] = r.targetZoneId;
        if (r.cellId.valid()) {
            ruleObj["cellSlot"] = r.cellId.slot;
            ruleObj["cellGen"] = r.cellId.generation;
        }
        const Law* govLaw = resolveGoverningLaw(r);
        ruleObj["governingLawId"] = govLaw ? govLaw->getIdentifier() : r.governingLawId;
        rules.push_back(ruleObj);
    }
    return nlohmann::json{{"open", open}, {"rules", rules}};
}

void TransferPolicy::loadFromJson(const nlohmann::json& j) {
    if (j.contains("open")) {
        for (auto it = j["open"].begin(); it != j["open"].end(); ++it) {
            setOpen(it.key(), it.value().get<bool>());   // Kernel refuses, honestly
        }
    }
    if (j.contains("rules") && j["rules"].is_array()) {
        _rules.clear();
        for (const auto& item : j["rules"]) {
            PolicyRule r;
            if (item.contains("pathPattern")) r.pathPattern = item["pathPattern"].get<std::string>();
            if (item.contains("op")) r.op = accessOpFromString(item["op"].get<std::string>());
            if (item.contains("allow")) r.allow = item["allow"].get<bool>();
            if (item.contains("governingZoneId")) r.governingZoneId = item["governingZoneId"].get<std::string>();
            if (item.contains("originZoneId")) r.originZoneId = item["originZoneId"].get<std::string>();
            if (item.contains("targetZoneId")) r.targetZoneId = item["targetZoneId"].get<std::string>();
            if (item.contains("cellSlot") && item.contains("cellGen") &&
                item["cellSlot"].is_number() && item["cellGen"].is_number()) {
                r.cellId = CellId{item["cellSlot"].get<uint32_t>(), item["cellGen"].get<uint32_t>()};
            }
            if (item.contains("governingLawId")) {
                r.governingLawId = item["governingLawId"].get<std::string>();
                (void)resolveGoverningLaw(r);
            }
            _rules.push_back(r);
        }
        _accessPolicyRevision = _rules.empty() ? 0 : (_accessPolicyRevision + 1);
    }
}

void TransferPolicy::addPolicyRule(const PolicyRule& rule) {
    PolicyRule copy = rule;
    if (copy.governingLawId.empty() && copy.governingLaw) {
        copy.governingLawId = copy.governingLaw->getIdentifier();
    }
    _rules.push_back(std::move(copy));
    ++_accessPolicyRevision;
}

void TransferPolicy::removeRulesForBearer(const Singular* bearer) {
    if (!bearer || _rules.empty()) return;
    auto oldSize = _rules.size();
    _rules.erase(
        std::remove_if(_rules.begin(), _rules.end(),
                       [bearer](const PolicyRule& r) {
                           return static_cast<const Singular*>(r.governingLaw) == bearer;
                       }),
        _rules.end());
    if (_rules.size() != oldSize) {
        if (_rules.empty()) {
            _accessPolicyRevision = 0;
        } else {
            ++_accessPolicyRevision;
        }
    }
}

void TransferPolicy::clearPolicyRules() {
    _rules.clear();
    _accessPolicyRevision = 0;
    _stagedPathPattern.clear();
    _stagedTargetCellPath.clear();
    _stagedOp = "write";
    _stagedAllow = true;
    _stagedGoverningZoneId.clear();
    _stagedOriginZoneId.clear();
    _stagedTargetZoneId.clear();
    _stagedCommit = false;
}

bool TransferPolicy::checkAccess(Singular& bearer,
                                 const PropertyPath::ResolvedSlot& slot,
                                 const ValueCell* effectiveCell,
                                 const AccessContext& ctx) const {
    if (_accessPolicyRevision == 0) return true;

    AccessContext effectiveCtx = ctx;
    if (!effectiveCtx.authorizingLaw && s_activeLaw) {
        effectiveCtx.authorizingLaw = s_activeLaw;
    }
    if (!effectiveCtx.actor && s_activeActor) {
        effectiveCtx.actor = s_activeActor;
    }

    // Automatically infer originZone and targetZone when not explicitly supplied
    // so real Law executions across Zones enforce dual-jurisdiction governance.
    if (!effectiveCtx.originZone && !effectiveCtx.targetZone) {
        const Singular* targetBearer = slot.owner ? slot.owner : &bearer;
        effectiveCtx.originZone = resolveZoneForBeing(effectiveCtx.actor, effectiveCtx.authorizingLaw);
        effectiveCtx.targetZone = resolveZoneForBeing(targetBearer, nullptr);
    }

    const std::string originId = effectiveCtx.originZone ? effectiveCtx.originZone->getIdentifier() : "";
    const std::string targetId = effectiveCtx.targetZone ? effectiveCtx.targetZone->getIdentifier() : "";

    // Collect all matching rules
    std::vector<const PolicyRule*> matching;
    std::string dynamicKey = slot.dynamicKey;
    std::string propName = slot.prop ? slot.prop->name() : "";

    for (const auto& rule : _rules) {
        if (rule.op != effectiveCtx.op) continue;

        // Committed-snapshot guard (PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md §5b):
        // A newly authored allow-rule cannot authorize the same Law application
        // transaction that created it.
        if (rule.allow && rule.createdInScopeId != 0 && rule.createdInScopeId == s_activeScopeId) {
            continue;
        }

        if (!rule.originZoneId.empty() && rule.originZoneId != originId) continue;
        if (!rule.targetZoneId.empty() && rule.targetZoneId != targetId) continue;

        bool cellMatch = rule.cellId.valid() && effectiveCell && (rule.cellId == effectiveCell->id);
        bool pathMatch = false;
        if (!rule.pathPattern.empty()) {
            if (rule.pathPattern == dynamicKey || rule.pathPattern == propName) {
                pathMatch = true;
            } else if (!rule.cellId.valid() && slot.rootPropertyId.isValid()) {
                if (Earthcall::StringInterner::resolve(slot.rootPropertyId) == rule.pathPattern) {
                    pathMatch = true;
                }
            }
        } else if (!rule.cellId.valid()) {
            pathMatch = true; // Wildcard pattern
        }

        if (cellMatch || pathMatch) {
            if (!rule.filter || rule.filter(effectiveCtx)) {
                matching.push_back(&rule);
            }
        }
    }

    if (matching.empty()) return true; // Open-by-default migration baseline

    auto resolveWinner = [](const std::vector<const PolicyRule*>& rules) -> const PolicyRule* {
        if (rules.empty()) return nullptr;
        const PolicyRule* winner = rules[0];
        for (std::size_t i = 1; i < rules.size(); ++i) {
            const PolicyRule* candidate = rules[i];
            const Law* winLaw = resolveGoverningLaw(*winner);
            const Law* candLaw = resolveGoverningLaw(*candidate);
            if (!winLaw && candLaw) {
                winner = candidate;
            } else if (winLaw && candLaw) {
                if (candLaw->hasAuthorityOver(*winLaw)) {
                    winner = candidate;
                } else if (!winLaw->hasAuthorityOver(*candLaw)) {
                    // If equal or unranked standing: cell-specific rule wins over generic path,
                    // and if still tied, deny takes precedence.
                    if (candidate->cellId.valid() && !winner->cellId.valid()) {
                        winner = candidate;
                    } else if (!candidate->allow && winner->allow) {
                        winner = candidate;
                    }
                }
            } else {
                if (candidate->cellId.valid() && !winner->cellId.valid()) {
                    winner = candidate;
                } else if (!candidate->allow && winner->allow) {
                    winner = candidate;
                }
            }
        }
        return winner;
    };

    bool isInterZone = (effectiveCtx.originZone != effectiveCtx.targetZone) &&
                       (effectiveCtx.originZone != nullptr || effectiveCtx.targetZone != nullptr);

    if (isInterZone) {
        std::vector<const PolicyRule*> senderRules;
        std::vector<const PolicyRule*> receiverRules;
        std::vector<const PolicyRule*> generalRules;

        for (const auto* rule : matching) {
            bool isSender = false;
            bool isReceiver = false;

            if (!rule->governingZoneId.empty()) {
                if (!originId.empty() && rule->governingZoneId == originId) isSender = true;
                else if (!targetId.empty() && rule->governingZoneId == targetId) isReceiver = true;
            }

            const Law* ruleLaw = resolveGoverningLaw(*rule);
            if (!isSender && !isReceiver && ruleLaw) {
                if (Zone* lawZone = resolveZoneForBeing(nullptr, ruleLaw)) {
                    const std::string jurId = lawZone->getIdentifier();
                    if (!originId.empty() && jurId == originId) isSender = true;
                    else if (!targetId.empty() && jurId == targetId) isReceiver = true;
                }
            }

            if (!isSender && !isReceiver) {
                if (!rule->originZoneId.empty() && rule->targetZoneId.empty()) {
                    isSender = true;
                } else if (!rule->targetZoneId.empty() && rule->originZoneId.empty()) {
                    isReceiver = true;
                } else if (!rule->originZoneId.empty() && !rule->targetZoneId.empty()) {
                    isReceiver = true;
                }
            }

            if (isSender) {
                senderRules.push_back(rule);
            } else if (isReceiver) {
                receiverRules.push_back(rule);
            } else {
                generalRules.push_back(rule);
            }
        }

        const PolicyRule* senderWinner = resolveWinner(senderRules);
        if (senderWinner && !senderWinner->allow) return false;

        const PolicyRule* receiverWinner = resolveWinner(receiverRules);
        if (receiverWinner && !receiverWinner->allow) return false;

        const PolicyRule* generalWinner = resolveWinner(generalRules);
        if (generalWinner && !generalWinner->allow) return false;

        return true;
    }

    const PolicyRule* winner = resolveWinner(matching);
    return winner ? winner->allow : true;
}
