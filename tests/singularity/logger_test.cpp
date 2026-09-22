#include "Singularity/Core/Logger.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Running Logger unit tests..." << std::endl;

    auto& logger = ECA::Logger::instance();
    logger.setActiveWorld("test_world");

    // Test log entries across multiple categories
    logger.log(ECA::LogCategory::Laws, "LAW_TEST", "Test law entry", nlohmann::json{{"lawId", "law_1"}});
    logger.log(ECA::LogCategory::System, "SYS_TEST", "Test system entry", nlohmann::json{{"sys", "ok"}});
    logger.log(ECA::LogCategory::Person, "PERSON_TEST", "Test person entry", nlohmann::json{{"person", "Alex"}});
    logger.log(ECA::LogCategory::State, "STATE_TEST", "Test state entry", nlohmann::json{{"key", "val"}});
    logger.log(ECA::LogCategory::Language, "LANG_TEST", "Test lang entry", nlohmann::json{{"utt", "hello"}});
    logger.log(ECA::LogCategory::Audio, "AUDIO_TEST", "Test audio entry", nlohmann::json{{"freq", 440}});

    // Flush logs by shutting down
    logger.shutdown();

    // Verify subdirectories and file outputs
    std::vector<std::string> categories = {"laws", "system", "person", "state", "language", "audio"};
    for (const auto& cat : categories) {
        std::string dir = "logs/" + cat;
        std::string logFile = dir + "/" + cat + ".log";
        std::string jsonlFile = dir + "/" + cat + ".jsonl";

        assert(std::filesystem::exists(dir));
        assert(std::filesystem::exists(logFile));
        assert(std::filesystem::exists(jsonlFile));

        std::ifstream inLog(logFile);
        assert(inLog.is_open());
        std::string line;
        bool found = false;
        while (std::getline(inLog, line)) {
            if (line.find("TEST") != std::string::npos) {
                found = true;
                break;
            }
        }
        assert(found);
    }

    // Verify backward compatibility file logs/law_audit.log
    assert(std::filesystem::exists("logs/law_audit.log"));
    assert(std::filesystem::exists("logs/law_audit.jsonl"));

    std::cout << "Logger unit tests passed successfully!" << std::endl;
    return 0;
}
