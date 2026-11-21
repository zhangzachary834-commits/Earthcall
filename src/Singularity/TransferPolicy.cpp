#include "Singularity/TransferPolicy.hpp"

#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ConstructedBeing/Singular/SingularPersistence.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <algorithm>
#include <utility>

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

CellId resolveRuleCellId(const PolicyRule& rule) {
    if (rule.cellId.valid() && PropertyCellTable::instance().getCell(rule.cellId) != nullptr) {
        return rule.cellId;
    }
    if (!rule.targetCellPath.empty()) {
        PropertyPath cellPath = PropertyPath::parse(rule.targetCellPath);
        std::size_t startIdx = 0;
        Singular* fallbackSubj = s_activeActor ? s_activeActor : &TransferPolicy::instance();
        Singular* root = resolveLawRoot(*fallbackSubj, cellPath, startIdx);
        if (root && startIdx < cellPath.segments.size()) {
            auto slot = cellPath.resolve(*root, startIdx);
            if (slot.cellId.valid() && PropertyCellTable::instance().getCell(slot.cellId) != nullptr) {
                rule.cellId = slot.cellId;
                return rule.cellId;
            }
        }
    }
    return rule.cellId;
}

Zone* resolveZoneForBeing(const Singular* being, const Law* fallbackLaw = nullptr) {
    if (being) {
        if (const auto* asZone = dynamic_cast<const Zone*>(being)) {
            return const_cast<Zone*>(asZone);
        }
        for (Zone* z : ActiveZones::instance().zones()) {
            if (z && z->containsBeing(*being)) return z;
        }
        if (ZoneManager* zm = ZoneManager::live()) {
            if (Zone* res = zm->residenceOf(*being)) {
                return res;
            }
            for (const auto& zPtr : zm->zones()) {
                if (zPtr && zPtr->containsBeing(*being)) return zPtr.get();
            }
        }
        for (Singular* candidate : Universe::instance().beings()) {
            if (auto* z = dynamic_cast<Zone*>(candidate)) {
                if (z->containsBeing(*being)) return z;
            }
        }
    }
    if (fallbackLaw) {
        if (fallbackLaw->jurisdiction()) {
            return fallbackLaw->jurisdiction().get();
        }
        for (Zone* z : ActiveZones::instance().zones()) {
            if (z && z->containsBeing(*fallbackLaw)) return z;
        }
        if (ZoneManager* zm = ZoneManager::live()) {
            for (const auto& zPtr : zm->zones()) {
                if (zPtr && zPtr->containsBeing(*fallbackLaw)) return zPtr.get();
            }
        }
        for (Singular* candidate : Universe::instance().beings()) {
            if (auto* z = dynamic_cast<Zone*>(candidate)) {
                if (z->containsBeing(*fallbackLaw)) return z;
            }
        }
    }
    return nullptr;
}

void notifyRuleChanged(const PolicyRule& rule, const Singular* excludedBearer = nullptr) {
    if (rule.targetBearer) {
        if (rule.targetBearer != excludedBearer) {
            Singular::notifyPropertyChanged(rule.targetBearer, rule.pathPattern);
        }
        return;
    }
    CellId cid = resolveRuleCellId(rule);
    if (cid.valid()) {
        PropertyCellTable::instance().notifyMutation(cid);
    }
    if (!rule.targetCellPath.empty() && rule.targetCellPath.front() == '@') {
        PropertyPath parsed = PropertyPath::parse(rule.targetCellPath);
        std::size_t startIdx = 0;
        Singular* fallbackSubj = s_activeActor ? s_activeActor : &TransferPolicy::instance();
        Singular* root = resolveLawRoot(*fallbackSubj, parsed, startIdx);
        if (root && root != excludedBearer && startIdx < parsed.segments.size()) {
            auto slot = parsed.resolve(*root, startIdx);
            Singular* owner = (slot.owner && slot.owner != excludedBearer) ? slot.owner : root;
            std::string rootProp = slot.prop ? slot.prop->name()
                : (!slot.dynamicKey.empty() ? slot.dynamicKey : parsed.segments[startIdx]);
            Singular::notifyPropertyChanged(owner, rootProp);
            if (parsed.segments.back() != rootProp) {
                Singular::notifyPropertyChanged(owner, parsed.segments.back());
            }
        }
    }
    if (!rule.pathPattern.empty()) {
        std::string pattern = rule.pathPattern;
        if (pattern.front() == '@') {
            PropertyPath parsed = PropertyPath::parse(pattern);
            std::size_t startIdx = 0;
            Singular* fallbackSubj = s_activeActor ? s_activeActor : &TransferPolicy::instance();
            Singular* root = resolveLawRoot(*fallbackSubj, parsed, startIdx);
            if (root && root != excludedBearer && startIdx < parsed.segments.size()) {
                auto slot = parsed.resolve(*root, startIdx);
                Singular* owner = (slot.owner && slot.owner != excludedBearer) ? slot.owner : root;
                std::string rootProp = slot.prop ? slot.prop->name()
                    : (!slot.dynamicKey.empty() ? slot.dynamicKey : parsed.segments[startIdx]);
                Singular::notifyPropertyChanged(owner, rootProp);
                if (parsed.segments.back() != rootProp) {
                    Singular::notifyPropertyChanged(owner, parsed.segments.back());
                }
            }
        } else {
            std::vector<Singular*> targets = Universe::instance().beings();
            if (ZoneManager* zm = ZoneManager::live()) {
                for (const auto& z : zm->zones()) {
                    if (!z) continue;
                    targets.push_back(z.get());
                    for (const auto& obj : z->objects()) targets.push_back(obj.get());
                    for (const auto& s : z->storedSingulars()) targets.push_back(s.get());
                }
            }
            std::sort(targets.begin(), targets.end());
            targets.erase(std::unique(targets.begin(), targets.end()), targets.end());
            for (Singular* b : targets) {
                if (!b || b == excludedBearer) continue;
                if (b->findProperty(pattern) || b->hasDynamicProperty(pattern)) {
                    Singular::notifyPropertyChanged(b, pattern);
                }
            }
        }
    } else if (!cid.valid() && rule.targetCellPath.empty()) {
        // Wildcard rule: notify dynamic properties on beings in Universe so continuous Rete laws re-evaluate
        std::vector<Singular*> targets = Universe::instance().beings();
        std::sort(targets.begin(), targets.end());
        targets.erase(std::unique(targets.begin(), targets.end()), targets.end());
        for (Singular* b : targets) {
            if (!b || b == excludedBearer || b == &TransferPolicy::instance()) continue;
            for (const auto& [propId, _] : b->dynamicProperties()) {
                Singular::notifyPropertyChanged(b, Earthcall::StringInterner::resolve(propId));
            }
        }
    }
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

void TransferPolicy::syncRegisterMemoryLaws(LawManager& laws) {
    // Zach's temporary device-driver exception. The compiled recipe applies
    // named First Mover Laws through the same rule store as authored policy.
    // See PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md §5c.
    const auto registerSnapshotLaw = [this, &laws](const char* id, const char* name,
                                                  const char* property) {
        if (laws.find(id)) return;

        auto law = std::make_shared<FirstMoverLaw>(name);
        law->setLawIdentifier(id);
        law->addAuthor(*this);
        law->addTarget(*this);
        law->addAction("Permit detached reads and copies of " + std::string(property) +
                       " and refuse writes and live cell bindings",
            [this, property](const ECA::Event& event, Singular& subject) {
                const Law* governingLaw = currentContext().authorizingLaw;
                if (&subject != this || !governingLaw || governingLaw != event.object) return;
                for (AccessOp op : {AccessOp::Read, AccessOp::Write, AccessOp::Share,
                                    AccessOp::BorrowWeak, AccessOp::TransferUnique,
                                    AccessOp::Derive}) {
                    const bool installed = std::any_of(_rules.begin(), _rules.end(),
                        [&](const PolicyRule& rule) {
                            return rule.targetBearer == this && rule.governingLaw == governingLaw &&
                                rule.pathPattern == property && rule.op == op;
                        });
                    if (installed) continue;
                    PolicyRule rule;
                    rule.pathPattern = property;
                    rule.targetBearer = this;
                    rule.op = op;
                    rule.allow = op == AccessOp::Read;
                    rule.governingLaw = governingLaw;
                    rule.createdInScopeId = activeScopeId();
                    addPolicyRule(rule);
                }
            });
        laws.add(law);
        law->applyTo(*this);
    };
    registerSnapshotLaw("memory-policy-rule-list", "Memory Policy · Inspect Rule List", "rules");
    registerSnapshotLaw("memory-policy-rule-count", "Memory Policy · Inspect Rule Count", "ruleCount");
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
    registerProperty(std::make_unique<CheckedComputedProperty<TransferPolicy, bool>>(
        "rule.commit", this, &TransferPolicy::propRuleCommit, &TransferPolicy::propSetRuleCommit));
    registerProperty(std::make_unique<CheckedComputedProperty<TransferPolicy, std::string>>(
        "rule.revokeGoverningLawId", this, &TransferPolicy::propRuleRevokeGoverningLawId, &TransferPolicy::propSetRuleRevokeGoverningLawId));
    registerProperty(std::make_unique<ComputedProperty<TransferPolicy, std::shared_ptr<PropertyList>>>(
        "rules", this, &TransferPolicy::propRules, nullptr));
    registerProperty(std::make_unique<ComputedProperty<TransferPolicy, int>>(
        "ruleCount", this, &TransferPolicy::propRuleCount, nullptr));
}

std::shared_ptr<PropertyList> TransferPolicy::propRules() const {
    auto list = std::make_shared<PropertyList>();
    for (const auto& r : _rules) {
        auto dict = std::make_shared<PropertyDict>();
        dict->elements["pathPattern"] = PropertyValue(r.pathPattern);
        dict->elements["targetCellPath"] = PropertyValue(r.targetCellPath);
        dict->elements["op"] = PropertyValue(accessOpToString(r.op));
        dict->elements["allow"] = PropertyValue(r.allow);
        const Law* govLaw = resolveGoverningLaw(r);
        dict->elements["governingLawId"] = PropertyValue(govLaw ? govLaw->getIdentifier() : r.governingLawId);
        dict->elements["governingZoneId"] = PropertyValue(r.governingZoneId);
        dict->elements["originZoneId"] = PropertyValue(r.originZoneId);
        dict->elements["targetZoneId"] = PropertyValue(r.targetZoneId);
        if (r.targetBearer) {
            dict->elements["targetBearerId"] = PropertyValue(r.targetBearer->getIdentifier());
        }
        list->elements.push_back(PropertyValue(std::move(dict)));
    }
    return list;
}

bool TransferPolicy::propSetRuleRevokeGoverningLawId(const std::string& targetLawId) {
    if (targetLawId.empty()) {
        _stagedRevokeGoverningLawId = "";
        return true;
    }

    const Law* authLaw = currentContext().authorizingLaw;
    if (authLaw) {
        if (authLaw->getIdentifier() != targetLawId) {
            const Law* targetLaw = nullptr;
            for (const auto& r : _rules) {
                std::string gid = r.governingLawId;
                if (gid.empty() && r.governingLaw) gid = r.governingLaw->getIdentifier();
                if (gid == targetLawId) {
                    targetLaw = resolveGoverningLaw(r);
                    if (targetLaw) break;
                }
            }
            if (!targetLaw) {
                for (Singular* b : Universe::instance().beings()) {
                    if (const auto* l = dynamic_cast<const Law*>(b)) {
                        if (l->getIdentifier() == targetLawId) {
                            targetLaw = l;
                            break;
                        }
                    }
                }
            }
            if (!targetLaw || !authLaw->hasAuthorityOver(*targetLaw)) {
                return false;
            }
        }
    }

    std::vector<PolicyRule> remaining;
    std::vector<PolicyRule> removed;
    for (const auto& r : _rules) {
        std::string gid = r.governingLawId;
        if (gid.empty() && r.governingLaw) gid = r.governingLaw->getIdentifier();
        if (gid == targetLawId) {
            removed.push_back(r);
        } else {
            remaining.push_back(r);
        }
    }

    if (!removed.empty()) {
        _rules = std::move(remaining);
        _accessPolicyRevision = _rules.empty() ? 0 : (_accessPolicyRevision + 1);
        for (const auto& r : removed) {
            notifyRuleChanged(r);
        }
        Singular::notifyPropertyChanged(this, "ruleCount");
        Singular::notifyPropertyChanged(this, "rules");
    }

    _stagedRevokeGoverningLawId = "";
    return true;
}

bool TransferPolicy::propSetRuleCommit(const bool& v) {
    if (!v) {
        _stagedCommit = false;
        return true;
    }
    PolicyRule r;
    r.pathPattern = _stagedPathPattern;
    r.targetCellPath = _stagedTargetCellPath;
    r.op = accessOpFromString(_stagedOp);
    r.allow = _stagedAllow;
    r.governingZoneId = _stagedGoverningZoneId;
    r.originZoneId = _stagedOriginZoneId;
    r.targetZoneId = _stagedTargetZoneId;

    _stagedPathPattern.clear();
    _stagedTargetCellPath.clear();
    _stagedOp = "write";
    _stagedAllow = true;
    _stagedGoverningZoneId.clear();
    _stagedOriginZoneId.clear();
    _stagedTargetZoneId.clear();
    _stagedCommit = false;

    r.governingLaw = currentContext().authorizingLaw;
    if (r.governingLaw) {
        r.governingLawId = r.governingLaw->getIdentifier();
    }
    if (r.governingZoneId.empty() && r.governingLaw) {
        if (Zone* z = resolveZoneForBeing(nullptr, r.governingLaw)) {
            r.governingZoneId = z->getIdentifier();
        }
    }
    if (!r.targetCellPath.empty()) {
        PropertyPath cellPath = PropertyPath::parse(r.targetCellPath);
        std::size_t startIdx = 0;
        Singular* fallbackSubj = currentContext().actor ? currentContext().actor : this;
        Singular* root = resolveLawRoot(*fallbackSubj, cellPath, startIdx);
        if (root && startIdx < cellPath.segments.size()) {
            auto slot = cellPath.resolve(*root, startIdx);
            Singular* cellBearer = slot.owner ? slot.owner : root;
            if (slot.cellId.valid()) {
                r.cellId = slot.cellId;
            } else if (slot.prop || slot.dynamicSlot) {
                // Anchoring to inline memory promotes it to shared storage.
                // The policy author must be allowed to perform that binding.
                if (!checkAccess(*root, slot, nullptr, currentContext(AccessOp::Share)) ||
                    !checkAccess(*root, slot, nullptr, currentContext(AccessOp::Write)) ||
                    (slot.prop && !slot.prop->isStructurallyWritable()) ||
                    (slot.containerProperty && !slot.containerProperty->isStructurallyWritable())) {
                    return false;
                }
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

    Singular::notifyPropertyChanged(this, "ruleCount");
    Singular::notifyPropertyChanged(this, "rule.commit");
    return true;
}

bool TransferPolicy::hasNonDefaultState() const {
    if (std::any_of(_rules.begin(), _rules.end(),
                    [](const PolicyRule& rule) { return !rule.targetBearer; })) return true;
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
        if (r.targetBearer) continue; // Reconstructed by the device at boot.
        nlohmann::json ruleObj;
        ruleObj["pathPattern"] = r.pathPattern;
        if (!r.targetCellPath.empty()) {
            ruleObj["targetCellPath"] = r.targetCellPath;
        }
        ruleObj["op"] = accessOpToString(r.op);
        ruleObj["allow"] = r.allow;
        ruleObj["governingZoneId"] = r.governingZoneId;
        ruleObj["originZoneId"] = r.originZoneId;
        ruleObj["targetZoneId"] = r.targetZoneId;
        CellId effectiveCid = resolveRuleCellId(r);
        if (effectiveCid.valid()) {
            ruleObj["cellSlot"] = effectiveCid.slot;
            ruleObj["cellGen"] = effectiveCid.generation;
            ruleObj["cellKey"] = std::to_string(effectiveCid.slot) + ":" + std::to_string(effectiveCid.generation);
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
        _rules.erase(std::remove_if(_rules.begin(), _rules.end(),
            [](const PolicyRule& rule) { return !rule.targetBearer; }), _rules.end());
        auto* cohort = SingularPersistence::CohortContext::active();
        for (const auto& item : j["rules"]) {
            PolicyRule r;
            if (item.contains("pathPattern")) r.pathPattern = item["pathPattern"].get<std::string>();
            if (item.contains("targetCellPath")) r.targetCellPath = item["targetCellPath"].get<std::string>();
            if (item.contains("op")) r.op = accessOpFromString(item["op"].get<std::string>());
            if (item.contains("allow")) r.allow = item["allow"].get<bool>();
            if (item.contains("governingZoneId")) r.governingZoneId = item["governingZoneId"].get<std::string>();
            if (item.contains("originZoneId")) r.originZoneId = item["originZoneId"].get<std::string>();
            if (item.contains("targetZoneId")) r.targetZoneId = item["targetZoneId"].get<std::string>();

            std::string cellKey;
            if (item.contains("cellKey") && item["cellKey"].is_string()) {
                cellKey = item["cellKey"].get<std::string>();
            } else if (item.contains("cellSlot") && item.contains("cellGen") &&
                       item["cellSlot"].is_number() && item["cellGen"].is_number()) {
                cellKey = std::to_string(item["cellSlot"].get<uint32_t>()) + ":" +
                          std::to_string(item["cellGen"].get<uint32_t>());
            }

            if (cohort && !cellKey.empty()) {
                auto it = cohort->remappedCells.find(cellKey);
                if (it != cohort->remappedCells.end()) {
                    r.cellId = it->second;
                }
            }
            if (!r.cellId.valid() && item.contains("cellSlot") && item.contains("cellGen") &&
                item["cellSlot"].is_number() && item["cellGen"].is_number()) {
                r.cellId = CellId{item["cellSlot"].get<uint32_t>(), item["cellGen"].get<uint32_t>()};
            }
            (void)resolveRuleCellId(r);

            if (item.contains("governingLawId")) {
                r.governingLawId = item["governingLawId"].get<std::string>();
                (void)resolveGoverningLaw(r);
            }
            _rules.push_back(r);
        }
        _accessPolicyRevision = _rules.empty() ? 0 : (_accessPolicyRevision + 1);
        for (const auto& r : _rules) {
            notifyRuleChanged(r);
        }
        Singular::notifyPropertyChanged(this, "ruleCount");
        Singular::notifyPropertyChanged(this, "rules");
    }
}

void TransferPolicy::addPolicyRule(const PolicyRule& rule) {
    PolicyRule copy = rule;
    if (copy.governingLawId.empty() && copy.governingLaw) {
        copy.governingLawId = copy.governingLaw->getIdentifier();
    }
    _rules.push_back(std::move(copy));
    ++_accessPolicyRevision;
    notifyRuleChanged(_rules.back());
    Singular::notifyPropertyChanged(this, "ruleCount");
    Singular::notifyPropertyChanged(this, "rules");
}

void TransferPolicy::removeRulesForBearer(const Singular* bearer) {
    if (!bearer || _rules.empty()) return;
    std::vector<PolicyRule> remaining;
    std::vector<PolicyRule> removed;
    for (const auto& r : _rules) {
        if (static_cast<const Singular*>(r.governingLaw) == bearer || r.targetBearer == bearer) {
            removed.push_back(r);
        } else {
            remaining.push_back(r);
        }
    }
    if (!removed.empty()) {
        _rules = std::move(remaining);
        _accessPolicyRevision = _rules.empty() ? 0 : (_accessPolicyRevision + 1);
        for (const auto& r : removed) {
            notifyRuleChanged(r, bearer);
        }
        Singular::notifyPropertyChanged(this, "ruleCount");
        Singular::notifyPropertyChanged(this, "rules");
    }
}

void TransferPolicy::clearPolicyRules() {
    auto oldRules = std::move(_rules);
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
    _stagedRevokeGoverningLawId.clear();
    for (const auto& r : oldRules) {
        notifyRuleChanged(r);
    }
    Singular::notifyPropertyChanged(this, "ruleCount");
    Singular::notifyPropertyChanged(this, "rules");
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

    const Singular* targetBearer = slot.owner ? slot.owner : &bearer;
    // Device rules match very few slots. Resolve jurisdictions only after an
    // operation's bearer and slot match, so unrelated reads stay cheap at boot.
    bool resolvedJurisdiction = false;
    std::string originId;
    std::string targetId;
    std::vector<const PolicyRule*> matching;

    for (const auto& rule : _rules) {
        if (rule.op != effectiveCtx.op) continue;
        if (rule.targetBearer && rule.targetBearer != targetBearer) continue;

        // Committed-snapshot guard (PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md §5b):
        // A newly authored allow-rule cannot authorize the same Law application
        // transaction that created it.
        if (rule.allow && rule.createdInScopeId != 0 && rule.createdInScopeId == s_activeScopeId) {
            continue;
        }

        const CellId ruleCid = resolveRuleCellId(rule);
        bool cellMatch = ruleCid.valid() && effectiveCell && (ruleCid == effectiveCell->id);
        bool pathMatch = false;
        if (!rule.pathPattern.empty()) {
            if (rule.pathPattern == slot.dynamicKey ||
                (slot.prop && rule.pathPattern == slot.prop->name())) {
                pathMatch = true;
            } else if (!ruleCid.valid() && slot.rootPropertyId.isValid()) {
                if (Earthcall::StringInterner::resolve(slot.rootPropertyId) == rule.pathPattern) {
                    pathMatch = true;
                }
            }
        } else if (!ruleCid.valid()) {
            pathMatch = true; // Wildcard pattern
        }

        if (!cellMatch && !pathMatch) continue;

        if (!resolvedJurisdiction) {
            if (!effectiveCtx.originZone && !effectiveCtx.targetZone) {
                effectiveCtx.originZone = resolveZoneForBeing(effectiveCtx.actor, effectiveCtx.authorizingLaw);
                effectiveCtx.targetZone = resolveZoneForBeing(targetBearer, nullptr);
            }
            if (effectiveCtx.originZone) originId = effectiveCtx.originZone->getIdentifier();
            if (effectiveCtx.targetZone) targetId = effectiveCtx.targetZone->getIdentifier();
            resolvedJurisdiction = true;
        }
        if (!rule.originZoneId.empty() && rule.originZoneId != originId) continue;
        if (!rule.targetZoneId.empty() && rule.targetZoneId != targetId) continue;
        if (!rule.filter || rule.filter(effectiveCtx)) matching.push_back(&rule);
    }

    if (matching.empty()) return true; // Open-by-default migration baseline

    auto resolveWinner = [](const std::vector<const PolicyRule*>& rules) -> const PolicyRule* {
        if (rules.empty()) return nullptr;
        auto ruleSpecificity = [](const PolicyRule& r) {
            return std::pair{r.targetBearer != nullptr,
                (!r.pathPattern.empty() ? 2 : 0)
                    + (!r.originZoneId.empty() ? 1 : 0)
                    + (!r.targetZoneId.empty() ? 1 : 0)};
        };
        const PolicyRule* winner = rules[0];
        for (std::size_t i = 1; i < rules.size(); ++i) {
            const PolicyRule* candidate = rules[i];
            const Law* winLaw = resolveGoverningLaw(*winner);
            const Law* candLaw = resolveGoverningLaw(*candidate);
            const bool winHasCell = resolveRuleCellId(*winner).valid();
            const bool candHasCell = resolveRuleCellId(*candidate).valid();
            const auto winSpec = ruleSpecificity(*winner);
            const auto candSpec = ruleSpecificity(*candidate);
            if (!winLaw && candLaw) {
                winner = candidate;
            } else if (winLaw && candLaw) {
                if (candLaw->hasAuthorityOver(*winLaw)) {
                    winner = candidate;
                } else if (!winLaw->hasAuthorityOver(*candLaw)) {
                    // If equal or unranked standing: cell-specific rule wins over generic path,
                    // then higher path + Zone-pair specificity wins, and if still tied, deny wins.
                    if (candHasCell && !winHasCell) {
                        winner = candidate;
                    } else if (candHasCell == winHasCell) {
                        if (candSpec > winSpec) {
                            winner = candidate;
                        } else if (candSpec == winSpec && !candidate->allow && winner->allow) {
                            winner = candidate;
                        }
                    }
                }
            } else {
                if (candHasCell && !winHasCell) {
                    winner = candidate;
                } else if (candHasCell == winHasCell) {
                    if (candSpec > winSpec) {
                        winner = candidate;
                    } else if (candSpec == winSpec && !candidate->allow && winner->allow) {
                        winner = candidate;
                    }
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
