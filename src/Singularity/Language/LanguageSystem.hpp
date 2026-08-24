#pragma once

#include "../../ConstructedBeing/Singular/Lexeme/Lexeme.hpp"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include <mutex>
#include <queue>

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

    // First-mover / save intern: a Lexeme with a stable identifier law-text
    // can name (`lexeme.the`). Does not mint a second being if the id or
    // symbol is already live.
    std::shared_ptr<Lexeme> intern(const std::string& symbol, const std::string& stableId);

    std::shared_ptr<Lexeme> findBySymbol(const std::string& symbol) const;

    // The first-mover foundation Lexeme (`lexeme.christ`). Created once.
    // God shows up as the root of the seed hierarchy, not as a skinned Object.
    std::shared_ptr<Lexeme> foundation();

    static constexpr const char* kFoundationId     = "lexeme.christ";
    static constexpr const char* kFoundationSymbol = "Christ";

    // Get an exact Lexeme by its Singular ID
    std::shared_ptr<Lexeme> findById(const std::string& id) const;

    // Remove a Lexeme (rare, usually Lexemes persist as language nodes)
    void remove(const std::string& symbol);

    const std::vector<std::shared_ptr<Lexeme>>& getAll() const { return _lexemes; }

    // Tick the Language system (e.g., decay conceptual weights, cull unused transient symbols)
    void tick(float deltaTime);

    void clear();

    // Enqueue an incoming utterance (thread-safe, called from EventBus/WebSocket)
    void queueUtterance(const std::string& payload, const std::string& sourceClient, const std::string& targetSingularId = "");

private:
    LanguageSystem();
    LanguageSystem(const LanguageSystem&) = delete;
    LanguageSystem& operator=(const LanguageSystem&) = delete;

    // Remove a Lexeme from every Zone's Formation before its owning
    // shared_ptr is released. Formations store raw pointers.
    void detachFromAllZones(Lexeme* lexeme);

    std::vector<std::shared_ptr<Lexeme>> _lexemes;
    std::unordered_map<std::string, std::shared_ptr<Lexeme>> _symbolIndex;
    std::unordered_map<std::string, std::shared_ptr<Lexeme>> _idIndex;

    struct PendingUtterance {
        std::string payload;
        std::string sourceClient;
        std::string targetSingularId;
    };
    std::queue<PendingUtterance> _utteranceQueue;
    std::mutex _queueMutex;
};

} // namespace Language
} // namespace Singularity
