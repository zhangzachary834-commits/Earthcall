#include "Singularity/TransferPolicy.hpp"

#include "Form/Singular/Property/ComputedProperty.hpp"

TransferPolicy& TransferPolicy::instance() {
    static TransferPolicy policy;
    return policy;
}

TransferPolicy::TransferPolicy() {
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
            _propertyRegistry.push_back(std::make_unique<ComputedProperty<GateBridge, bool>>(
                "gate." + entry.first, raw, &GateBridge::get, nullptr));
        } else {
            _propertyRegistry.push_back(std::make_unique<ComputedProperty<GateBridge, bool>>(
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
