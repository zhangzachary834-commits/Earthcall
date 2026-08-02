#include "Singularity/Language/LanguageSystem.hpp"

namespace Singularity {
namespace Language {

LanguageSystem& LanguageSystem::instance() {
    static LanguageSystem inst;
    return inst;
}

std::shared_ptr<Lexeme> LanguageSystem::resolve(const std::string& symbol) {
    auto it = _symbolIndex.find(symbol);
    if (it != _symbolIndex.end()) {
        return it->second;
    }

    // Create a new Lexeme natively in the substrate
    auto lexeme = std::make_shared<Lexeme>(symbol);
    _lexemes.push_back(lexeme);
    _symbolIndex[symbol] = lexeme;
    _idIndex[lexeme->getIdentifier()] = lexeme;

    return lexeme;
}

std::shared_ptr<Lexeme> LanguageSystem::findById(const std::string& id) const {
    auto it = _idIndex.find(id);
    if (it != _idIndex.end()) {
        return it->second;
    }
    return nullptr;
}

void LanguageSystem::remove(const std::string& symbol) {
    auto it = _symbolIndex.find(symbol);
    if (it == _symbolIndex.end()) return;

    std::shared_ptr<Lexeme> lexeme = it->second;
    _symbolIndex.erase(it);
    _idIndex.erase(lexeme->getIdentifier());

    auto vecIt = std::find(_lexemes.begin(), _lexemes.end(), lexeme);
    if (vecIt != _lexemes.end()) {
        _lexemes.erase(vecIt);
    }
}

void LanguageSystem::tick(float deltaTime) {
    // Currently purely static, but in the future we could decay conceptual weights
    // if a Lexeme goes unused in the Zone for long periods.
}

void LanguageSystem::clear() {
    _lexemes.clear();
    _symbolIndex.clear();
    _idIndex.clear();
}

} // namespace Language
} // namespace Singularity
