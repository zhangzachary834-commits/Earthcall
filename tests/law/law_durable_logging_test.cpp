#include "ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "support/test_harness.hpp"
#include <fstream>
#include <string>
#include <filesystem>
#include <iostream>

using namespace ECA;

// Mock test using standard assertions since gtest is not guaranteed here.
// In Earthcall, tests are usually defined as standalone mains.
int main(int argc, char** argv) {
    std::filesystem::remove_all(AppPaths::logDirectory() / "laws");
    std::cout << "Running LawDurableLoggingTest..." << std::endl;

    TestSupport::BootedEngineHarness h("Player", "humanoid", false);

    auto& logger = LawAuditLogger::instance();

    std::string lawA = "law_a_identity";
    std::string lawB = "law_b_identity";

    logger.logForLaw(lawA, "LAW", "Event A1");
    logger.logForLaw(lawB, "LAW", "Event B1");
    logger.logForLaw(lawA, "LAW", "Event A2");

    logger.shutdown(); // flushes and closes

    auto logDir = AppPaths::logDirectory() / "laws";
    auto pathA = logDir / (lawA + ".jsonl");
    auto pathB = logDir / (lawB + ".jsonl");

    if (!std::filesystem::exists(pathA)) {
        std::cerr << "Path A does not exist: " << pathA << std::endl;
        return 1;
    }
    if (!std::filesystem::exists(pathB)) {
        std::cerr << "Path B does not exist: " << pathB << std::endl;
        return 1;
    }

    std::ifstream inA(pathA);
    std::string line;
    int countA = 0;
    while (std::getline(inA, line)) {
        if (line.find("Event A") != std::string::npos) countA++;
        if (line.find("Event B") != std::string::npos) {
            std::cerr << "Law A log contains B's event!" << std::endl;
            return 1;
        }
    }
    if (countA != 2) {
        std::cerr << "Law A log has wrong event count: " << countA << std::endl;
        return 1;
    }

    std::ifstream inB(pathB);
    int countB = 0;
    while (std::getline(inB, line)) {
        if (line.find("Event B") != std::string::npos) countB++;
        if (line.find("Event A") != std::string::npos) {
            std::cerr << "Law B log contains A's event!" << std::endl;
            return 1;
        }
    }
    if (countB != 1) {
        std::cerr << "Law B log has wrong event count: " << countB << std::endl;
        return 1;
    }

    // Test reopen/append semantics
    // Restart harness/logger concept
    auto& logger2 = LawAuditLogger::instance();
    logger2.logForLaw(lawA, "LAW", "Event A3");
    logger2.shutdown();

    std::ifstream inA2(pathA);
    countA = 0;
    while (std::getline(inA2, line)) {
        if (line.find("Event A") != std::string::npos) countA++;
    }
    if (countA != 3) {
        std::cerr << "Law A log failed reopen test, expected 3 events, got: " << countA << std::endl;
        return 1;
    }

    std::cout << "LawDurableLoggingTest passed successfully!" << std::endl;
    return 0;
}
