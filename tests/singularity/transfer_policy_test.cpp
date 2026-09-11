#include "Singularity/TransferPolicy.hpp"
#include <iostream>
#include <cassert>

static void testDefaultTiers() {
    auto& policy = TransferPolicy::instance();

    // Check known Kernel tier
    assert(policy.tierOf("position") == TransferPolicy::Tier::Kernel);
    assert(policy.tierOf("rotation") == TransferPolicy::Tier::Kernel);

    // Check known Governable tier
    assert(policy.tierOf("shape") == TransferPolicy::Tier::Governable);

    // Check known Gated tier
    assert(policy.tierOf("enabled") == TransferPolicy::Tier::Gated);

    // Unlisted is Governable
    assert(policy.tierOf("unknown") == TransferPolicy::Tier::Governable);

    std::cout << "  default tiers are as expected OK\n";
}

static void testGateOpenClose() {
    auto& policy = TransferPolicy::instance();

    // Kernel cannot be closed
    assert(policy.isOpen("position"));
    assert(!policy.setOpen("position", false));
    assert(policy.isOpen("position"));

    // Governable can be closed
    assert(policy.isOpen("shape"));
    assert(policy.setOpen("shape", false));
    assert(!policy.isOpen("shape"));
    assert(policy.setOpen("shape", true));
    assert(policy.isOpen("shape"));

    // Gated can be opened
    assert(!policy.isOpen("enabled"));
    assert(policy.setOpen("enabled", true));
    assert(policy.isOpen("enabled"));
    assert(policy.setOpen("enabled", false));
    assert(!policy.isOpen("enabled"));

    // Unlisted (Governable by default) can be closed
    assert(policy.isOpen("unknown"));
    assert(policy.setOpen("unknown", false));
    assert(!policy.isOpen("unknown"));
    assert(policy.setOpen("unknown", true));

    std::cout << "  gate open/close logic OK\n";
}

static void testCanTransfer() {
    auto& policy = TransferPolicy::instance();

    // Reset state for deterministic testing
    policy.setOpen("shape", true);
    policy.setOpen("enabled", false);

    // Simple path
    assert(policy.canTransfer(PropertyPath::parse("position")));
    assert(policy.canTransfer(PropertyPath::parse("shape")));
    assert(!policy.canTransfer(PropertyPath::parse("enabled")));

    // Nested path
    assert(policy.canTransfer(PropertyPath::parse("shape.r")));

    // "@event.subject.shape" skips @event and subject
    assert(policy.canTransfer(PropertyPath::parse("@event.subject.shape")));
    assert(!policy.canTransfer(PropertyPath::parse("@event.subject.enabled")));

    // Qualified path (skips other prefix) e.g. "@being.shape"
    assert(policy.canTransfer(PropertyPath::parse("@being.shape")));
    assert(!policy.canTransfer(PropertyPath::parse("@being.enabled")));

    // Empty path
    assert(!policy.canTransfer(PropertyPath::parse("")));

    std::cout << "  canTransfer resolution OK\n";
}

static void testSerialization() {
    auto& policy = TransferPolicy::instance();

    policy.setOpen("shape", false);
    policy.setOpen("enabled", true);

    nlohmann::json j = policy.toJson();

    // Reset to defaults
    policy.setOpen("shape", true);
    policy.setOpen("enabled", false);

    policy.loadFromJson(j);

    assert(!policy.isOpen("shape"));
    assert(policy.isOpen("enabled"));

    // Cleanup
    policy.setOpen("shape", true);
    policy.setOpen("enabled", false);

    std::cout << "  serialization round trip OK\n";
}

int main() {
    std::cout << "transfer_policy_test:\n";
    testDefaultTiers();
    testGateOpenClose();
    testCanTransfer();
    testSerialization();
    std::cout << "transfer_policy_test: ALL OK\n";
    return 0;
}
