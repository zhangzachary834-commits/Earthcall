#pragma once

#include "Singularity/Language/Lexeme.hpp"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Singularity {
namespace Language {

// The LanguageSystem manages the lifecycle and indexing of all linguistic-symbolic 
// Singulars (Lexemes) in the active simulation. It allows Laws to efficiently
// lookup words by string value, or iterate over the language graph.
class LanguageSystem {
public:
    static LanguageSystem& instance();

    // Instantiate or retrieve a Lexeme for a given string
    std::shared_ptr<Lexeme> resolve(const std::string& symbol);

    // Get an exact Lexeme by its Singular ID
    std::shared_ptr<Lexeme> findById(const std::string& id) const;

    // Remove a Lexeme (rare, usually Lexemes persist as language nodes)
    void remove(const std::string& symbol);

    const std::vector<std::shared_ptr<Lexeme>>& getAll() const { return _lexemes; }

    // Tick the Language system (e.g., decay conceptual weights, cull unused transient symbols)
    void tick(float deltaTime);

    void clear();

private:
    LanguageSystem() = default;
    LanguageSystem(const LanguageSystem&) = delete;
    LanguageSystem& operator=(const LanguageSystem&) = delete;

    std::vector<std::shared_ptr<Lexeme>> _lexemes;
    std::unordered_map<std::string, std::shared_ptr<Lexeme>> _symbolIndex;
    std::unordered_map<std::string, std::shared_ptr<Lexeme>> _idIndex;
};

} // namespace Language
} // namespace Singularity
