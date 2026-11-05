#pragma once

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ConstructedBeing/Singular/Property/PropertyCell.hpp"
#include "json.hpp"

#include <map>
#include <string>
#include <vector>
#include <functional>

enum class AccessOp : uint8_t {
    Read = 0,
    Write = 1,
    Share = 2,
    BorrowWeak = 3,
    TransferUnique = 4,
    Derive = 5
};

class Zone;
class Law;

struct AccessContext {
    Singular* actor = nullptr;
    const Law* authorizingLaw = nullptr;
    Zone* originZone = nullptr;
    Zone* targetZone = nullptr;
    AccessOp op = AccessOp::Write;
};

struct PolicyRule {
    std::string pathPattern; // matches path prefix or empty for all
    CellId cellId;           // matches cellId across all aliases
    AccessOp op = AccessOp::Write;
    bool allow = true;
    const Law* governingLaw = nullptr;
    std::function<bool(const AccessContext&)> filter;
};

// ============================================================================
// TransferPolicy — governed access to set-to-set creation, rooted at
// Singularity (all things related to permissions are rooted here).
//
// An object's properties may be taken from a source set only through these
// gates. Three tiers:
//   Kernel     — universally transferable; set at the Singularity level and
//                immune to lower-order law constraints (laws cannot close it)
//   Governable — accessible by default, but a law may close (and reopen) it
//   Gated      — closed by default; a law must open it
// Unlisted properties default to Governable-open.
//
// The policy is itself a LEGIBLE SINGULAR: every gate registers as a bool
// property ("gate.shape", "gate.enabled", ...), so ordinary laws govern
// transfer access — "@transfer-policy.gate.shape := false" — with no new
// permission machinery. Kernel gates register read-only: the anti-tyranny
// ceiling, same as the reserved time paths.
// ============================================================================
class TransferPolicy : public Object {
public:
    // Serialized as ints — APPEND-ONLY.
    enum class Tier { Kernel = 0, Governable = 1, Gated = 2 };

    static TransferPolicy& instance();

    std::string getIdentifier() const override { return "transfer-policy"; }

    // Is this source path currently transferable? Matched on the path's
    // FIRST segment ("shape.r" consults gate "shape"); qualified paths
    // ("@being.shape.r") consult the segment after the qualifier.
    bool canTransfer(const PropertyPath& source) const;

    Tier tierOf(const std::string& gateName) const;
    bool isOpen(const std::string& gateName) const;
    // Returns false when the gate is Kernel (laws cannot close the floor).
    bool setOpen(const std::string& gateName, bool open);

    // The declared gates (for UI badges); unlisted = Governable-open.
    const std::map<std::string, Tier>& gates() const { return _tiers; }

    // Policy-driven shared-cell & property access governance (AD-37)
    struct ExecutionScope {
        ExecutionScope(const Law* law, Singular* actor);
        ~ExecutionScope();
    private:
        const Law* _prevLaw = nullptr;
        Singular* _prevActor = nullptr;
    };
    static AccessContext currentContext(AccessOp op = AccessOp::Write);
    static void notifyBearerDestroyed(const Singular* bearer);

    uint64_t accessPolicyRevision() const { return _accessPolicyRevision; }
    void addPolicyRule(const PolicyRule& rule);
    void removeRulesForBearer(const Singular* bearer);
    void clearPolicyRules();
    bool checkAccess(Singular& bearer,
                     const PropertyPath::ResolvedSlot& slot,
                     const ValueCell* effectiveCell,
                     const AccessContext& ctx) const;

    nlohmann::json toJson() const;
    void loadFromJson(const nlohmann::json& j);

    ~TransferPolicy() override;

private:
    TransferPolicy();
    TransferPolicy(const TransferPolicy&) = delete;
    TransferPolicy& operator=(const TransferPolicy&) = delete;

    void buildProperties() override;

    std::map<std::string, Tier> _tiers;   // declared gates
    std::map<std::string, bool> _open;    // current state (Governable/Gated)

    uint64_t _accessPolicyRevision = 0;
    std::vector<PolicyRule> _rules;
};
