#pragma once

#include "../../ConstructedBeing/Singular/Lexeme/Lexeme.hpp"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

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

    // Instantiate or retrieve a Lexeme for a given string. If the input is the
    // exact identifier of an already-live Lexeme, return that being instead of
    // minting a new Lexeme whose visible symbol happens to equal the identifier.
    std::shared_ptr<Lexeme> resolve(const std::string& symbol);

    // First-mover / save intern: a Lexeme with a stable identifier law-text
    // can name (`lexeme.the`). Stable identity wins over spelling: a different
    // stableId may intentionally intern a second same-spelled Lexeme. The
    // single-symbol index remains the legacy/default binding; exact identity is
    // always available through findById, and findAllBySymbol exposes ambiguity.
    std::shared_ptr<Lexeme> intern(const std::string& symbol, const std::string& stableId);

    // Legacy/default spelling lookup. When multiple live Lexemes share the same
    // spelling, this preserves the existing last-bound behavior but reports the
    // ambiguity loudly so a human-facing channel cannot mistake spelling for
    // durable identity. Exact IDs (and @<exact-id>) select one exact being.
    std::shared_ptr<Lexeme> findBySymbol(const std::string& symbol) const;

    // Return every live Lexeme with this exact spelling. Human-facing channels
    // use this to present duplicate words without collapsing their identities.
    std::vector<std::shared_ptr<Lexeme>> findAllBySymbol(const std::string& symbol) const;

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

    // _symbolIndex is a convenience/default binding, not semantic identity.
    // Whenever its selected Lexeme leaves, bind the spelling to another live
    // duplicate if one remains instead of making the remaining beings unreachable.
    void rebindSymbolIndex(const std::string& symbol);

    // Multiplicity keeps ordinary symbol lookup O(1). We only walk the complete
    // Lexeme vector to enumerate candidates when a collision actually exists.
    void noteSymbolAdded(const std::string& symbol);
    void noteSymbolRemoved(const std::string& symbol);

    std::vector<std::shared_ptr<Lexeme>> _lexemes;
    std::unordered_map<std::string, std::shared_ptr<Lexeme>> _symbolIndex;
    std::unordered_map<std::string, std::shared_ptr<Lexeme>> _idIndex;
    std::unordered_map<std::string, size_t> _symbolCounts;
    mutable std::unordered_set<std::string> _reportedAmbiguities;

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
