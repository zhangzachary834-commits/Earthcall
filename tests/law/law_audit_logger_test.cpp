#include "ZonesOfEarth/AuthorsOfLaw/LawAuditLogger.hpp"
#include <iostream>

using namespace ECA;

int main() {
    int failed = 0;

    // Test SingletonInstance
    LawAuditLogger& instance1 = LawAuditLogger::instance();
    LawAuditLogger& instance2 = LawAuditLogger::instance();
    if (&instance1 != &instance2) {
        std::cerr << "FAIL: Singleton instance returned different pointers." << std::endl;
        failed++;
    }

    // Test LevelManagement
    LawAuditLogger& logger = LawAuditLogger::instance();
    auto originalLevel = logger.level();

    logger.setLevel(LogLevel::Verbose);
    if (logger.level() != LogLevel::Verbose) {
        std::cerr << "FAIL: Failed to set log level to Verbose." << std::endl;
        failed++;
    }

    logger.setLevel(LogLevel::Summary);
    if (logger.level() != LogLevel::Summary) {
        std::cerr << "FAIL: Failed to set log level to Summary." << std::endl;
        failed++;
    }

    logger.setLevel(originalLevel);

    // Test WouldLog
    logger.setLevel(LogLevel::Verbose);
    if (!logger.wouldLog("LAW")) {
        std::cerr << "FAIL: should log LAW when Verbose." << std::endl;
        failed++;
    }
    if (!logger.wouldLog("EVENT")) {
        std::cerr << "FAIL: should log EVENT when Verbose." << std::endl;
        failed++;
    }

    logger.setLevel(LogLevel::Off);
    if (logger.wouldLog("LAW")) {
        std::cerr << "FAIL: should not log LAW when Off." << std::endl;
        failed++;
    }

    logger.setLevel(originalLevel);

    if (failed == 0) {
        std::cout << "PASS" << std::endl;
        return 0;
    } else {
        std::cerr << failed << " test(s) failed." << std::endl;
        return 1;
    }
}
