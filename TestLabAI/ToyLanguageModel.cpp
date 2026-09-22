#include "ToyLanguageModel.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace {
constexpr const char* kStart = "<START>";
constexpr const char* kEnd = "<END>";
}

ToyLanguageModel::ToyLanguageModel() : rng_(std::random_device{}()) {}

void ToyLanguageModel::trainFromText(const std::string& text) {
    transitions_.clear();
    startTokens_.clear();

    std::vector<std::string> tokens = tokenize(text);
    if (tokens.empty()) {
        return;
    }

    std::string previous = kStart;
    for (const std::string& rawToken : tokens) {
        std::string token = normalize(rawToken);
        if (token.empty()) {
            continue;
        }

        if (previous == kStart) {
            startTokens_.push_back(token);
        }

        transitions_[previous][token] += 1;

        bool endsSentence = !rawToken.empty() &&
            (rawToken.back() == '.' || rawToken.back() == '!' || rawToken.back() == '?');

        previous = token;

        if (endsSentence) {
            transitions_[previous][kEnd] += 1;
            previous = kStart;
        }
    }

    if (previous != kStart) {
        transitions_[previous][kEnd] += 1;
    }
}

std::vector<std::string> ToyLanguageModel::tokenize(const std::string& text) const {
    std::istringstream input(text);
    std::vector<std::string> tokens;
    std::string token;

    while (input >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

std::string ToyLanguageModel::generate(const std::string& prompt, std::size_t maxTokens) {
    if (empty()) {
        throw std::runtime_error("Model has no training data.");
    }

    std::vector<std::string> promptTokens = tokenize(prompt);
    std::vector<std::string> output;

    std::string current = promptTokens.empty()
        ? randomStartToken()
        : normalize(promptTokens.back());

    if (current.empty() || transitions_.find(current) == transitions_.end()) {
        current = randomStartToken();
    } else {
        output = promptTokens;
    }

    for (std::size_t i = 0; i < maxTokens; ++i) {
        const std::string next = pickNextToken(current);
        if (next == kEnd || next.empty()) {
            break;
        }

        output.push_back(next);
        current = next;
    }

    std::ostringstream sentence;
    for (std::size_t i = 0; i < output.size(); ++i) {
        if (i > 0) {
            sentence << ' ';
        }
        sentence << output[i];
    }

    return sentence.str();
}

bool ToyLanguageModel::empty() const {
    return transitions_.empty() || startTokens_.empty();
}

std::string ToyLanguageModel::pickNextToken(const std::string& current) {
    auto it = transitions_.find(current);
    if (it == transitions_.end() || it->second.empty()) {
        return kEnd;
    }

    int totalWeight = 0;
    for (const auto& [token, count] : it->second) {
        totalWeight += count;
    }

    std::uniform_int_distribution<int> dist(1, totalWeight);
    int choice = dist(rng_);

    for (const auto& [token, count] : it->second) {
        choice -= count;
        if (choice <= 0) {
            return token;
        }
    }

    return kEnd;
}

std::string ToyLanguageModel::normalize(const std::string& token) const {
    std::string clean;

    for (char ch : token) {
        const unsigned char value = static_cast<unsigned char>(ch);
        if (std::isalnum(value) || ch == '\'') {
            clean.push_back(static_cast<char>(std::tolower(value)));
        }
    }

    return clean;
}

std::string ToyLanguageModel::randomStartToken() {
    if (startTokens_.empty()) {
        return {};
    }

    std::uniform_int_distribution<std::size_t> dist(0, startTokens_.size() - 1);
    return startTokens_[dist(rng_)];
}
