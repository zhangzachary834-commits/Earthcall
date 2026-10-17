#include <cassert>
#include <iostream>
#include <string>

#define main first_mover_tool_main
#include "../../src/Identity/FirstMoverTool.cpp"
#undef main

int main() {
    char* usageArgs[] = {(char*)"earthcall_first_mover"};
    assert(first_mover_tool_main(1, usageArgs) == 2);

    char* mintArgs[] = {(char*)"earthcall_first_mover", (char*)"mint",
                        (char*)"--name", (char*)"TestMover"};
    const auto mint = parse(4, mintArgs);
    assert(mint.has_value());
    assert(mint->command == "mint");
    assert(get(*mint, "name") == "TestMover");

    char* grantArgs[] = {(char*)"earthcall_first_mover", (char*)"grant",
                         (char*)"--scope", (char*)"laws/**",
                         (char*)"--scope", (char*)"homes/**"};
    const auto grant = parse(6, grantArgs);
    assert(grant.has_value());
    assert(grant->scopes.size() == 2);
    assert(grant->scopes[0] == "laws/**");
    assert(grant->scopes[1] == "homes/**");

    char* invalidArgs[] = {(char*)"earthcall_first_mover", (char*)"mint",
                           (char*)"name", (char*)"TestMover"};
    assert(!parse(4, invalidArgs));
    std::cout << "first_mover_tool_test: parser and usage OK\n";
    return 0;
}
