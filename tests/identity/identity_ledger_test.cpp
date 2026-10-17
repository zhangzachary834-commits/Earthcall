#include "Identity/IdentityLedger.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "json.hpp"

using namespace Identity;

static void testEmptyLoadSave() {
    std::filesystem::path testPath = std::filesystem::temp_directory_path() / "test_ledger.json";
    std::filesystem::remove(testPath);

    IdentityLedger ledger(testPath);
    assert(!ledger.load()); // Should return false when not present
    assert(ledger.save());
    assert(std::filesystem::exists(testPath));
    assert(ledger.load()); // Should return true now
    assert(ledger.size() == 0);
    std::filesystem::remove(testPath);
    std::cout << "  empty load/save OK\n";
}

static void testRecordAndFind() {
    std::filesystem::path testPath = std::filesystem::temp_directory_path() / "test_ledger2.json";
    std::filesystem::remove(testPath);

    IdentityLedger ledger(testPath);

    SingularId id = PrivateKey::generate().id();

    ledger.record("legacy_name", id);
    assert(ledger.size() == 1);

    auto found = ledger.find("legacy_name");
    assert(found.has_value());
    assert(found->toString() == id.toString());

    auto notFound = ledger.find("unknown");
    assert(!notFound.has_value());
    std::filesystem::remove(testPath);
    std::cout << "  record and find OK\n";
}

static void testPersistence() {
    std::filesystem::path testPath = std::filesystem::temp_directory_path() / "test_ledger3.json";
    std::filesystem::remove(testPath);

    SingularId id = PrivateKey::generate().id();

    {
        IdentityLedger ledger(testPath);
        ledger.record("test_user", id);
        assert(ledger.save());
    }

    {
        IdentityLedger ledger(testPath);
        assert(ledger.load());
        auto found = ledger.find("test_user");
        assert(found.has_value());
        assert(found->toString() == id.toString());
    }
    std::filesystem::remove(testPath);
    std::cout << "  persistence OK\n";
}

static void testMalformedLedger() {
    std::filesystem::path testPath = std::filesystem::temp_directory_path() / "test_ledger4.json";
    // Write garbage to file
    std::ofstream out(testPath);
    out << "not a json file";
    out.close();

    IdentityLedger ledger(testPath);
    assert(!ledger.load());
    std::filesystem::remove(testPath);
    std::cout << "  malformed ledger OK\n";
}

static void testResolveOrMint() {
    std::filesystem::path testPath = std::filesystem::temp_directory_path() / "test_ledger5.json";
    std::filesystem::remove(testPath);
    // We need a keystore for this
    std::filesystem::path ksPath = std::filesystem::temp_directory_path() / "test_keystore";
    std::filesystem::remove_all(ksPath);

    KeyStore ks(ksPath);
    IdentityLedger ledger(testPath);

    auto id1 = ledger.resolveOrMint("test_user", ks, "password");
    assert(id1.has_value());

    auto id2 = ledger.resolveOrMint("test_user", ks, "password");
    assert(id2.has_value());
    assert(id1->toString() == id2->toString());

    // Test missing key in keystore
    std::filesystem::remove_all(ksPath); // key is gone!
    auto id3 = ledger.resolveOrMint("test_user", ks, "password");
    assert(!id3.has_value()); // Should fail because ledger has it, but keystore doesn't
    std::filesystem::remove_all(ksPath);
    std::filesystem::remove(testPath);
    std::cout << "  resolve or mint OK\n";
}

int main() {
    std::cout << "identity_ledger_test:\n";
    testEmptyLoadSave();
    testRecordAndFind();
    testPersistence();
    testMalformedLedger();
    testResolveOrMint();
    std::cout << "identity_ledger_test: ALL OK\n";
    return 0;
}
