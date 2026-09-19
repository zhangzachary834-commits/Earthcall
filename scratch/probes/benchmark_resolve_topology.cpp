#include "Relation/Formation/Formation.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <vector>
#include <memory>

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "benchmark_resolve_topology: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "benchmark_resolve_topology", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "benchmark_resolve_topology: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    std::cout << "Starting Formation::resolveTopology Benchmark Probe..." << std::endl;

    // Create a formation with 10,000 lexeme members and 5,000 component groups
    constexpr size_t numMembers = 10000;
    std::vector<std::unique_ptr<Singularity::Language::Lexeme>> lexemes;
    lexemes.reserve(numMembers);
    std::vector<Singular*> memberPtrs;
    memberPtrs.reserve(numMembers);

    for (size_t i = 0; i < numMembers; ++i) {
        auto lex = std::make_unique<Singularity::Language::Lexeme>("lex_" + std::to_string(i));
        memberPtrs.push_back(lex.get());
        lexemes.push_back(std::move(lex));
    }

    Formation formation;
    for (auto* m : memberPtrs) {
        formation.addMember(m);
    }

    // Add relations forming pairs (i, i+1) so components[i].size() < 3 and validCores is evaluated for 5000 components
    for (size_t i = 0; i < numMembers; i += 2) {
        auto rel = std::make_shared<Relation>("pair", *memberPtrs[i], *memberPtrs[i+1]);
        formation.addRelation(rel);
    }

    // Warmup
    for (int i = 0; i < 5; ++i) {
        auto res = formation.resolveTopology();
    }

    // Benchmark
    constexpr int iterations = 100;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto res = formation.resolveTopology();
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double avg_ms = (double)total_ns / (iterations * 1e6);

    std::cout << "Average resolveTopology time over " << iterations << " iterations: "
              << avg_ms << " ms" << std::endl;

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
