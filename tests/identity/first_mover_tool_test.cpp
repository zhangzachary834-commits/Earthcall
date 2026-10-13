#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstdlib>

#define main first_mover_tool_main
#include "../../src/Identity/FirstMoverTool.cpp"
#undef main

static void testUsageReturns2() {
    char* argv[] = { (char*)"earthcall_first_mover" };
    int result = first_mover_tool_main(1, argv);
    assert(result == 2);
    std::cout << "  usage returns 2 OK\n";
}

static void testParseCommand() {
    char* argv[] = { (char*)"earthcall_first_mover", (char*)"mint", (char*)"--name", (char*)"Claude" };
    auto args = parse(4, argv);
    assert(args.has_value());
    assert(args->command == "mint");
    assert(get(*args, "name") == "Claude");

    char* argv_scope[] = { (char*)"earthcall_first_mover", (char*)"grant", (char*)"--scope", (char*)"laws/**", (char*)"--scope", (char*)"homes/**" };
    auto args_scope = parse(6, argv_scope);
    assert(args_scope.has_value());
    assert(args_scope->command == "grant");
    assert(args_scope->scopes.size() == 2);
    assert(args_scope->scopes[0] == "laws/**");
    assert(args_scope->scopes[1] == "homes/**");
    std::cout << "  parse command OK\n";
}

static void testCmdMintMissingPassphrase() {
    // Clear any passphrase that might be set
    unsetenv("EARTHCALL_MOVER_PASSPHRASE");
    char* argv[] = { (char*)"earthcall_first_mover", (char*)"mint", (char*)"--name", (char*)"TestMover" };
    int result = first_mover_tool_main(4, argv);
    assert(result == 1); // Should fail because passphrase is not set
    std::cout << "  mint fails without passphrase OK\n";
}

static void testCheckMoverMissingPassphrase() {
    unsetenv("EARTHCALL_MOVER_PASSPHRASE");
    char* argv[] = { (char*)"earthcall_first_mover", (char*)"check-mover", (char*)"--mover", (char*)"id123" };
    int result = first_mover_tool_main(4, argv);
    assert(result == 1);
    std::cout << "  check-mover fails without passphrase OK\n";
}

static void testCmdSignMissingPassphrase() {
    unsetenv("EARTHCALL_MOVER_PASSPHRASE");
    char* argv[] = { (char*)"earthcall_first_mover", (char*)"sign-challenge", (char*)"--mover", (char*)"id123", (char*)"--challenge-id", (char*)"c", (char*)"--nonce", (char*)"n", (char*)"--connection", (char*)"k" };
    int result = first_mover_tool_main(10, argv);
    assert(result == 1);
    std::cout << "  sign-challenge fails without passphrase OK\n";
}

static void testCmdMintWithPassphrase() {
    setenv("EARTHCALL_MOVER_PASSPHRASE", "testpass", 1);

    // Set a custom saves directory to avoid writing to the real one
    std::filesystem::path testDir = std::filesystem::temp_directory_path() / "test_mover_mint";
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);

    // KeyStore uses ~/.earthcall/identity or EARTHCALL_DATA_HOME/identity
    // We can't easily mock KeyStore's defaultDirectory() without modifying FirstMoverTool.cpp
    // but FirstMoverTool uses KeyStore's default constructor which checks EARTHCALL_DATA_HOME
    setenv("EARTHCALL_DATA_HOME", testDir.string().c_str(), 1);

    std::string savesPath = testDir.string();

    char* argv[] = { (char*)"earthcall_first_mover", (char*)"mint", (char*)"--name", (char*)"TestMover", (char*)"--saves", (char*)savesPath.c_str() };
    int result = first_mover_tool_main(6, argv);
    assert(result == 0); // Minting should succeed

    unsetenv("EARTHCALL_MOVER_PASSPHRASE");
    unsetenv("EARTHCALL_DATA_HOME");
    std::filesystem::remove_all(testDir);
    std::cout << "  mint succeeds with passphrase OK\n";
}

static void testCmdListEmpty() {
    std::filesystem::path testDir = std::filesystem::temp_directory_path() / "test_mover_list";
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);

    std::string savesPath = testDir.string();
    char* argv[] = { (char*)"earthcall_first_mover", (char*)"list", (char*)"--saves", (char*)savesPath.c_str() };
    int result = first_mover_tool_main(4, argv);
    assert(result == 0);

    std::filesystem::remove_all(testDir);
    std::cout << "  list empty OK\n";
}

int main() {
    std::cout << "first_mover_tool_test:\n";
    testUsageReturns2();
    testParseCommand();
    testCmdMintMissingPassphrase();
    testCheckMoverMissingPassphrase();
    testCmdSignMissingPassphrase();
    testCmdMintWithPassphrase();
    testCmdListEmpty();
    std::cout << "first_mover_tool_test: ALL OK\n";
    return 0;
}
