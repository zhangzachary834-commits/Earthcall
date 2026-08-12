#include "Relation/RelationManager.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Singularity/Language/SyntacticParser.hpp"
#include <iostream>

extern ZoneManager mgr;

namespace Singularity {
namespace Language {

LanguageSystem& LanguageSystem::instance() {
    static LanguageSystem inst;
    return inst;
}

// A Formation holds its members as raw Singular*, so a Lexeme must be detached
// from every Formation before its last shared_ptr goes away. Detaching only
// from mgr.active() is not enough: a Lexeme instantiated while one Zone was
// active outlives that Zone becoming inactive, and would leave a dangling
// member behind in it.
void LanguageSystem::detachFromAllZones(Lexeme* lexeme) {
    if (!lexeme) return;
    for (Zone& z : mgr.zones()) {
        z.removeFromFormation(lexeme);
    }
}

LanguageSystem::LanguageSystem() {
    // Subscribe to Utterance events globally.
    Core::EventBus::instance().subscribe<Core::Event::Utterance>([this](const Core::Event::Utterance& evt) {
        this->queueUtterance(evt.payload, evt.sourceClient, evt.targetSingularId);
    });
}

std::shared_ptr<Lexeme> LanguageSystem::resolve(const std::string& symbol) {
    auto it = _symbolIndex.find(symbol);
    if (it != _symbolIndex.end()) {
        return it->second;
    }

    if (_lexemes.size() >= 1000) {
        auto oldest = _lexemes.front();
        detachFromAllZones(oldest.get());
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
    detachFromAllZones(lexeme.get());

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
        
        Zone& activeZone = mgr.active();
        
        auto parsedRelations = SyntacticParser::parse(u.payload, activeZone);
        
        if (!parsedRelations.empty()) {
            for (const auto& pr : parsedRelations) {
                auto lexA = resolve(pr.entityA);
                auto lexB = resolve(pr.entityB);
                
                activeZone.addToFormation(lexA.get());
                activeZone.addToFormation(lexB.get());
                
                auto rels = activeZone.formation().relations().getRelationsBetween(lexA->getIdentifier(), lexB->getIdentifier());
                bool found = false;
                for (auto r : rels) {
                    if (r->type == pr.relationType) {
                        float w = r->getWeight();
                        r->setWeight(std::min(1.0f, w + 0.2f)); // Reinforce existing pathway
                        found = true;
                        std::cout << "[LanguageSystem] Reinforced Graph Edge: [" << pr.entityA << "] -> " << pr.relationType << " -> [" << pr.entityB << "] (Weight: " << r->getWeight() << ")" << std::endl;
                        break;
                    }
                }
                
                if (!found) {
                    auto rel = std::make_shared<Relation>(pr.relationType, *lexA, *lexB, true);
                    rel->setWeight(0.5f); // Starting confidence
                    activeZone.formation().addRelation(rel);
                    std::cout << "[LanguageSystem] New Graph Parse: [" << pr.entityA << "] -> " << pr.relationType << " -> [" << pr.entityB << "]" << std::endl;
                }
            }
        } else {
            auto lexeme = resolve(u.payload);
            activeZone.addToFormation(lexeme.get());
            
            if (!u.targetSingularId.empty()) {
                Singular* target = activeZone.formation().findMemberByIdentifier(u.targetSingularId);
                if (target) {
                    auto rel = std::make_shared<Relation>("speaks", *target, *lexeme, true);
                    activeZone.formation().addRelation(rel);
                    std::cout << "[LanguageSystem] Routed utterance to target Object: " << u.targetSingularId << std::endl;
                }
            }
        }

        std::cout << "[LanguageSystem] Finished processing utterance '" << u.payload 
                  << "' in Zone: " << activeZone.name() << std::endl;
        
        localQueue.pop();
    }

    // 2. Synaptic Plasticity (Decay semantic weights)
    Zone& activeZone = mgr.active();
    std::vector<std::shared_ptr<Relation>> toRemove;
    
    for (const auto& rel : activeZone.formation().relations().getAll()) {
        // Skip structural/authored ontology relations
        if (rel->type == "is_pos" || rel->type == "resolves_to" || rel->type == "member" || rel->type == "attachment" || rel->type == "speaks") {
            continue;
        }
        
        float w = rel->getWeight();
        if (w > 0.0f) {
            w -= 0.02f * deltaTime; // Decay rate
            if (w <= 0.0f) {
                toRemove.push_back(rel);
                std::cout << "[LanguageSystem] Semantic pathway decayed and forgotten: " << rel->getIdentifier() << std::endl;
            } else {
                rel->setWeight(w);
            }
        }
    }
    
    for (const auto& rel : toRemove) {
        activeZone.formation().removeRelation(rel);
    }
}

void LanguageSystem::queueUtterance(const std::string& payload, const std::string& sourceClient, const std::string& targetSingularId) {
    std::lock_guard<std::mutex> lock(_queueMutex);
    if (_utteranceQueue.size() >= 1000) return;
    if (payload.length() > 1024) return;
    _utteranceQueue.push({payload, sourceClient, targetSingularId});
}

void LanguageSystem::clear() {
    // Detach before dropping the owning references, or every Zone Formation is
    // left pointing at freed Lexemes.
    for (const auto& lexeme : _lexemes) {
        detachFromAllZones(lexeme.get());
    }
    _lexemes.clear();
    _symbolIndex.clear();
    _idIndex.clear();
}

} // namespace Language
} // namespace Singularity
