#include "ToyLanguageModel.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return {};
    }

    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

std::string defaultCorpus() {
    return
        "Machine learning helps computers find patterns in data. "
        "Language models learn which words often appear together. "
        "A tiny model can still demonstrate prediction and generation. "
        "Good training data shapes the answers a model can produce. "
        "Educational projects should stay small enough to inspect line by line. "
        "Real LLMs use neural networks, attention, tokens, and huge datasets. "
        "This project uses counting instead of deep learning so the logic stays visible.";
}
}

int main() {
    ToyLanguageModel model;

    std::string corpus = readFile("corpus.txt");
    if (corpus.empty()) {
        corpus = defaultCorpus();
    }

    model.trainFromText(corpus);

    std::cout << "Toy Language Model\n";
    std::cout << "------------------\n";
    std::cout << "This is not a real LLM. It is a word-to-word predictor built to teach the core idea.\n";
    std::cout << "Type a short prompt and it will continue by sampling likely next words.\n";
    std::cout << "Press Enter on an empty line to quit.\n\n";

    for (;;) {
        std::cout << "Prompt> ";
        std::string prompt;
        std::getline(std::cin, prompt);

        if (prompt.empty()) {
            break;
        }

        try {
            const std::string output = model.generate(prompt, 20);
            std::cout << "Model > " << output << "\n\n";
        } catch (const std::exception& error) {
            std::cerr << "Error: " << error.what() << '\n';
            return 1;
        }
    }

    return 0;
}
