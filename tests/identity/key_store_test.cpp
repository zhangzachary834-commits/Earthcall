#include "Identity/KeyStore.hpp"
#include "Identity/KeyPair.hpp"
#include <filesystem>
#include <iostream>
#include <cassert>

using namespace Identity;

int main() {
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "earthcall_key_store_test";
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);

    KeyStore ks(tempDir);

    // Store keypair
    PrivateKey priv = PrivateKey::generate();
    assert(priv.isValid());
    assert(ks.store(priv, "secret"));

    // Has
    assert(ks.contains(priv.id()));

    // Load
    auto loaded = ks.load(priv.id(), "secret");
    assert(loaded.has_value());
    assert(loaded->id() == priv.id());

    // Wrong password
    auto failed = ks.load(priv.id(), "wrong");
    assert(!failed.has_value());

    std::filesystem::remove_all(tempDir);
    std::cout << "KeyStore tests passed!" << std::endl;
    return 0;
}
