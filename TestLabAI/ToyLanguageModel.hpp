#pragma once

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

class ToyLanguageModel {
public:
    ToyLanguageModel();

    void trainFromText(const std::string& text);
    std::vector<std::string> tokenize(const std::string& text) const;
    std::string generate(const std::string& prompt, std::size_t maxTokens);
    bool empty() const;

private:
    using NextWordCounts = std::unordered_map<std::string, int>;

    std::string pickNextToken(const std::string& current);
    std::string normalize(const std::string& token) const;
    std::string randomStartToken();

    std::unordered_map<std::string, NextWordCounts> transitions_;
    std::vector<std::string> startTokens_;
    std::mt19937 rng_;
};
