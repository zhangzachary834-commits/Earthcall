#include "Identity/IdentityLedger.hpp"
#include "Identity/KeyStore.hpp"
#include <filesystem>
#include <iostream>
#include <cassert>

using namespace Identity;

int main() {
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "earthcall_identity_ledger_test";
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);

    auto ledgerPath = tempDir / "ledger.json";
    IdentityLedger ledger(ledgerPath);

    KeyStore ks(tempDir);

    auto maybeId = ledger.resolveOrMint("Zach", ks, "passphrase");
    assert(maybeId.has_value());

    auto id = *maybeId;
    assert(ledger.find("Zach") == id);

    auto maybeId2 = ledger.resolveOrMint("Zach", ks, "passphrase");
    assert(maybeId2.has_value());
    assert(*maybeId2 == id); // same identity

    assert(ledger.save()); // Added missing save call before load

    IdentityLedger ledger2(ledgerPath);
    assert(ledger2.load());
    assert(ledger2.find("Zach") == id);

    std::filesystem::remove_all(tempDir);
    std::cout << "IdentityLedger tests passed!" << std::endl;
    return 0;
}
