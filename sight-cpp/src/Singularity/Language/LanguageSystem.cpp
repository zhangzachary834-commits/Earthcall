#include "Singularity/Language/LanguageSystem.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <iostream>

extern ZoneManager mgr;

namespace Singularity {
namespace Language {

LanguageSystem& LanguageSystem::instance() {
    static LanguageSystem inst;
    return inst;
}

LanguageSystem::LanguageSystem() {
    // Subscribe to Utterance events globally.
    Core::EventBus::instance().subscribe<Core::Event::Utterance>([this](const Core::Event::Utterance& evt) {
        this->queueUtterance(evt.payload, evt.sourceClient);
    });
}

std::shared_ptr<Lexeme> LanguageSystem::resolve(const std::string& symbol) {
    auto it = _symbolIndex.find(symbol);
    if (it != _symbolIndex.end()) {
        return it->second;
    }

    if (_lexemes.size() >= 1000) {
        auto oldest = _lexemes.front();
        mgr.active().removeFromFormation(oldest.get());
        _idIndex.erase(oldest->getIdentifier());
        for (auto sit = _symbolIndex.begin(); sit != _symbolIndex.end(); ++sit) {
            if (sit->second == oldest) {
                _symbolIndex.erase(sit);
                break;
            }
        }
        _lexemes.erase(_lexemes.begin());
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
    mgr.active().removeFromFormation(lexeme.get());
    
    _symbolIndex.erase(it);
    _idIndex.erase(lexeme->getIdentifier());

    auto vecIt = std::find(_lexemes.begin(), _lexemes.end(), lexeme);
    if (vecIt != _lexemes.end()) {
        _lexemes.erase(vecIt);
    }
}

void LanguageSystem::tick(float deltaTime) {
    // 1. Process queued utterances from WebSocket/WebBindings
    std::queue<PendingUtterance> localQueue;
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        std::swap(localQueue, _utteranceQueue);
    }
    
    while (!localQueue.empty()) {
        const auto& u = localQueue.front();
        std::cout << "[LanguageSystem] Spawning Lexeme for utterance: " << u.payload << std::endl;
        
        // Resolve or spawn the lexeme
        auto lexeme = resolve(u.payload);
        
        // Phase 5: Phenomenological Instantiation
        // Assign the newly spawned Lexeme to the active Zone's Formation.
        // It inherits the medium of the Zone (e.g. 3D space, UI, Text) rather
        // than being hard-locked to a 3D coordinate struct.
        Zone& activeZone = mgr.active();
        activeZone.addToFormation(lexeme.get());
        
        std::cout << "[LanguageSystem] Lexeme '" << u.payload 
                  << "' joined Zone Formation: " << activeZone.name() << std::endl;
        
        localQueue.pop();
    }

    // 2. Decay conceptual weights (Future)
}

void LanguageSystem::queueUtterance(const std::string& payload, const std::string& sourceClient) {
    std::lock_guard<std::mutex> lock(_queueMutex);
    if (_utteranceQueue.size() >= 1000) return;
    if (payload.length() > 1024) return;
    _utteranceQueue.push({payload, sourceClient});
}

void LanguageSystem::clear() {
    _lexemes.clear();
    _symbolIndex.clear();
    _idIndex.clear();
}

} // namespace Language
} // namespace Singularity
