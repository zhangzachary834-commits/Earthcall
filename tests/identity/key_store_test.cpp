#include "Identity/KeyStore.hpp"
#include "Identity/KeyPair.hpp"
#include <filesystem>
#include <iostream>
#include <cassert>
#include <chrono>
#include <random>

using namespace Identity;

int main() {
    if (!cryptoAvailable()) {
        std::cout << "Crypto unavailable; KeyStore test skipped.\n";
        return 0;
    }
    const auto nonce = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                       "-" + std::to_string(std::random_device{}());
    const auto tempDir = std::filesystem::temp_directory_path() / ("earthcall-key-store-" + nonce);
    std::filesystem::create_directories(tempDir);
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code ec; std::filesystem::remove_all(path, ec); }
    } cleanup{tempDir};

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

    std::cout << "KeyStore tests passed!" << std::endl;
    return 0;
}
