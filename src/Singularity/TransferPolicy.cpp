#include "Singularity/TransferPolicy.hpp"

#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <algorithm>

namespace {
bool s_transferPolicyAlive = false;
const Law* s_activeLaw = nullptr;
Singular* s_activeActor = nullptr;
} // namespace

TransferPolicy::ExecutionScope::ExecutionScope(const Law* law, Singular* actor)
    : _prevLaw(s_activeLaw), _prevActor(s_activeActor) {
    s_activeLaw = law;
    s_activeActor = actor;
}

TransferPolicy::ExecutionScope::~ExecutionScope() {
    s_activeLaw = _prevLaw;
    s_activeActor = _prevActor;
}

AccessContext TransferPolicy::currentContext(AccessOp op) {
    AccessContext ctx;
    ctx.actor = s_activeActor;
    ctx.authorizingLaw = s_activeLaw;
    ctx.op = op;
    return ctx;
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
}

nlohmann::json TransferPolicy::toJson() const {
    nlohmann::json open = nlohmann::json::object();
    for (const auto& entry : _open) open[entry.first] = entry.second;
    return nlohmann::json{{"open", open}};
}

void TransferPolicy::loadFromJson(const nlohmann::json& j) {
    if (!j.contains("open")) return;
    for (auto it = j["open"].begin(); it != j["open"].end(); ++it) {
        setOpen(it.key(), it.value().get<bool>());   // Kernel refuses, honestly
    }
}

void TransferPolicy::addPolicyRule(const PolicyRule& rule) {
    _rules.push_back(rule);
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
}

bool TransferPolicy::checkAccess(Singular& bearer,
                                 const PropertyPath::ResolvedSlot& slot,
                                 const ValueCell* effectiveCell,
                                 const AccessContext& ctx) const {
    (void)bearer;
    if (_accessPolicyRevision == 0) return true;

    AccessContext effectiveCtx = ctx;
    if (!effectiveCtx.authorizingLaw && s_activeLaw) {
        effectiveCtx.authorizingLaw = s_activeLaw;
    }
    if (!effectiveCtx.actor && s_activeActor) {
        effectiveCtx.actor = s_activeActor;
    }

    // Collect all matching rules
    std::vector<const PolicyRule*> matching;
    std::string dynamicKey = slot.dynamicKey;
    std::string propName = slot.prop ? slot.prop->name() : "";

    for (const auto& rule : _rules) {
        if (rule.op != effectiveCtx.op) continue;

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

    // Precedence resolution via authority-over DAG
    const PolicyRule* winner = matching[0];
    for (std::size_t i = 1; i < matching.size(); ++i) {
        const PolicyRule* candidate = matching[i];
        if (!winner->governingLaw && candidate->governingLaw) {
            winner = candidate;
        } else if (winner->governingLaw && candidate->governingLaw) {
            if (candidate->governingLaw->hasAuthorityOver(*winner->governingLaw)) {
                winner = candidate;
            } else if (!winner->governingLaw->hasAuthorityOver(*candidate->governingLaw)) {
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

    return winner->allow;
}
