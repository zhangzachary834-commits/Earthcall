#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#define main first_mover_tool_main
#include "../../src/Identity/FirstMoverTool.cpp"
#undef main

namespace {
namespace fs = std::filesystem;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class ScopedEnv {
public:
    explicit ScopedEnv(const char* name) : name_(name) {
        if (const char* value = std::getenv(name)) previous_ = value;
    }
    ~ScopedEnv() {
        if (previous_) setenv(name_.c_str(), previous_->c_str(), 1);
        else unsetenv(name_.c_str());
    }
    void set(const std::string& value) { require(setenv(name_.c_str(), value.c_str(), 1) == 0, "setenv failed"); }
    void clear() { require(unsetenv(name_.c_str()) == 0, "unsetenv failed"); }
private:
    std::string name_;
    std::optional<std::string> previous_;
};

class ScopedTempDir {
public:
    ScopedTempDir() {
        std::string pattern = (fs::temp_directory_path() / "earthcall-first-mover-test-XXXXXX").string();
        std::vector<char> writable(pattern.begin(), pattern.end());
        writable.push_back('\0');
        char* created = mkdtemp(writable.data());
        require(created != nullptr, "mkdtemp failed");
        path_ = created;
    }
    ~ScopedTempDir() {
        std::error_code ec;
        if (!path_.empty()) fs::remove_all(path_, ec);
    }
    const fs::path& path() const { return path_; }
private:
    fs::path path_;
};

class CaptureStdout {
public:
    CaptureStdout() : previous_(std::cout.rdbuf(captured_.rdbuf())) {}
    ~CaptureStdout() { std::cout.rdbuf(previous_); }
    std::string output() const { return captured_.str(); }
private:
    std::ostringstream captured_;
    std::streambuf* previous_;
};

int cli(std::initializer_list<std::string> arguments) {
    std::vector<std::string> values(arguments);
    std::vector<char*> argv;
    for (auto& value : values) argv.push_back(value.data());
    return first_mover_tool_main(static_cast<int>(argv.size()), argv.data());
}

void testParserAndUsage() {
    require(cli({"earthcall_first_mover"}) == 2, "usage exit code");
    char* mintArgs[] = {(char*)"earthcall_first_mover", (char*)"mint",
                        (char*)"--name", (char*)"TestMover"};
    auto mint = parse(4, mintArgs);
    require(mint.has_value() && mint->command == "mint" && get(*mint, "name") == "TestMover",
            "mint parsing");
    char* grantArgs[] = {(char*)"earthcall_first_mover", (char*)"grant",
                         (char*)"--scope", (char*)"laws/**",
                         (char*)"--scope", (char*)"homes/**"};
    auto grant = parse(6, grantArgs);
    require(grant.has_value() && grant->scopes.size() == 2 &&
            grant->scopes[0] == "laws/**" && grant->scopes[1] == "homes/**",
            "repeated scope parsing");
    char* invalidArgs[] = {(char*)"earthcall_first_mover", (char*)"mint",
                           (char*)"name", (char*)"TestMover"};
    require(!parse(4, invalidArgs), "reject malformed flag");
}

void testIsolatedMintUnlockAndSign() {
    ScopedTempDir temp;
    ScopedEnv home("EARTHCALL_HOME");
    ScopedEnv pass("EARTHCALL_MOVER_PASSPHRASE");
    ScopedEnv personPass("EARTHCALL_KEY_PASSPHRASE");
    home.set(temp.path().string());
    pass.clear();
    personPass.clear();
    const std::string saves = (temp.path() / "saves").string();

    require(cli({"earthcall_first_mover", "mint", "--name", "TestMover", "--saves", saves}) == 1,
            "mint must reject missing passphrase");
    require(!fs::exists(temp.path() / "identity"), "failed mint wrote an identity");
    pass.set("isolated-test-passphrase");
    {
        CaptureStdout captured;
        require(cli({"earthcall_first_mover", "mint", "--name", "TestMover", "--saves", saves}) == 0,
                "isolated mint failed");
    }
    Identity::KeyStore keys;
    require(keys.directory() == temp.path() / "identity", "key directory escaped sandbox");
    const auto identities = keys.list();
    require(identities.size() == 1, "mint did not create exactly one isolated key");
    const std::string id = identities.front().toString();
    require(keys.contains(identities.front()), "minted key missing");

    require(cli({"earthcall_first_mover", "check-mover", "--mover", "not-a-key", "--saves", saves}) == 1,
            "check-mover accepted invalid id");
    pass.set("wrong-test-passphrase");
    require(cli({"earthcall_first_mover", "check-mover", "--mover", id, "--saves", saves}) == 1,
            "check-mover accepted wrong passphrase");
    pass.set("isolated-test-passphrase");
    {
        CaptureStdout captured;
        require(cli({"earthcall_first_mover", "check-mover", "--mover", id, "--saves", saves}) == 0,
                "check-mover rejected valid key");
    }
    require(cli({"earthcall_first_mover", "sign-challenge", "--mover", id,
                 "--challenge-id", "test", "--nonce", "nonce", "--saves", saves}) == 1,
            "sign-challenge accepted missing connection");
    {
        CaptureStdout captured;
        require(cli({"earthcall_first_mover", "sign-challenge", "--mover", id,
                     "--challenge-id", "test", "--nonce", "nonce",
                     "--connection", "conn", "--saves", saves}) == 0,
                "sign-challenge failed for isolated key");
        std::string signature = captured.output();
        if (!signature.empty() && signature.back() == '\n') signature.pop_back();
        require(signature.size() == 128 &&
                std::all_of(signature.begin(), signature.end(), [](unsigned char c) {
                    return std::isxdigit(c) != 0;
                }), "sign-challenge did not return Ed25519 hex signature");
    }
    {
        CaptureStdout captured;
        require(cli({"earthcall_first_mover", "list", "--saves", saves}) == 0,
                "list failed on empty isolated register");
        require(captured.output().find("no First Movers registered") != std::string::npos,
                "list did not report empty register");
    }
}
} // namespace

int main() {
    try {
        testParserAndUsage();
        testIsolatedMintUnlockAndSign();
        std::cout << "first_mover_tool_test: parser, sandboxed mint/unlock/sign and list OK\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "first_mover_tool_test: FAIL: " << e.what() << "\n";
        return 1;
    }
}
