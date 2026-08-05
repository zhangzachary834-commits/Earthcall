import os
import re

src_dir = '/Users/zacharyzhang/documents/github/earthcall/src'

def update_formation_hpp():
    path = os.path.join(src_dir, 'Form/Object/Formation/Formation.hpp')
    with open(path, 'r') as f:
        content = f.read()
    
    if "nlohmann::json toJson" not in content:
        insert = """
    // (De)Serialization Helpers (Logos Phase 2)
    nlohmann::json toJson() const;
    static std::shared_ptr<Formation> fromJson(const nlohmann::json& json);
"""
        content = content.replace("void draw() const;", "void draw() const;\n" + insert)
        with open(path, 'w') as f:
            f.write(content)
        print("Updated Formation.hpp")

def update_formation_cpp():
    path = os.path.join(src_dir, 'Form/Object/Formation/Formation.cpp')
    with open(path, 'r') as f:
        content = f.read()
        
    if "nlohmann::json Formation::toJson() const" not in content:
        impl = """
nlohmann::json Formation::toJson() const {
    nlohmann::json j = nlohmann::json::object();
    // Serialize members (Lexemes, etc.)
    nlohmann::json jMembers = nlohmann::json::array();
    for (auto* m : members) {
        if (auto* lexeme = dynamic_cast<Singularity::Language::Lexeme*>(m)) {
            jMembers.push_back({
                {"type", "Lexeme"},
                {"symbol", lexeme->getSymbol()},
                {"id", lexeme->getIdentifier()}
            });
        } else {
            // Stub for other singulars
            jMembers.push_back({
                {"type", "Singular"},
                {"id", m->getIdentifier()}
            });
        }
    }
    j["members"] = jMembers;
    
    // Subformations
    nlohmann::json jSub = nlohmann::json::array();
    for (const auto& sub : subformations) {
        jSub.push_back(sub->toJson());
    }
    j["subformations"] = jSub;
    
    // Relations
    j["relations"] = relationMgr.toJson();
    return j;
}

std::shared_ptr<Formation> Formation::fromJson(const nlohmann::json& json) {
    auto f = std::make_shared<Formation>(std::vector<Singular*>{});
    // This is a stub for the full recursive traversal. 
    // In a real system we'd look up the Singulars from the engine by ID or create new Lexemes
    // using LanguageSystem::instance().resolve(sym);
    return f;
}
"""
        # Note: missing include <Singularity/Language/Lexeme.hpp> in Formation.cpp might cause error.
        content = "#include \"Singularity/Language/Lexeme.hpp\"\n" + content
        content += impl
        with open(path, 'w') as f:
            f.write(content)
        print("Updated Formation.cpp")

def update_languagesystem_cpp():
    path = os.path.join(src_dir, 'Singularity/Language/LanguageSystem.cpp')
    with open(path, 'r') as f:
        content = f.read()
        
    if "RelationManager" not in content:
        content = '#include "Relation/RelationManager.hpp"\n#include "Relation/Relation.hpp"\n' + content
        
    # Inject basic syntactic parsing into queueUtterance or tick
    if "auto lexeme = resolve(u.payload);" in content and "Syntactic" not in content:
        # replace the tick logic to handle simple S-V-O
        new_logic = """
        // Simple NLP / Syntactic Parse Stub (Phase 3)
        // If payload is like "Sword belongs_to Arthur", we split it.
        std::string payload = u.payload;
        size_t firstSpace = payload.find(' ');
        size_t lastSpace = payload.rfind(' ');
        
        if (firstSpace != std::string::npos && lastSpace != std::string::npos && firstSpace != lastSpace) {
            std::string entityA = payload.substr(0, firstSpace);
            std::string relType = payload.substr(firstSpace + 1, lastSpace - firstSpace - 1);
            std::string entityB = payload.substr(lastSpace + 1);
            
            auto lexA = resolve(entityA);
            auto lexB = resolve(entityB);
            
            Zone& activeZone = mgr.active();
            activeZone.addToFormation(lexA.get());
            activeZone.addToFormation(lexB.get());
            
            auto rel = std::make_shared<Relation>(relType, *lexA, *lexB, true);
            // In a real system, the Formation would manage the relations. 
            // We just instantiate the lexemes for now.
            std::cout << "[LanguageSystem] Syntactic parse: " << entityA << " -> " << relType << " -> " << entityB << std::endl;
        } else {
            auto lexeme = resolve(u.payload);
            Zone& activeZone = mgr.active();
            activeZone.addToFormation(lexeme.get());
        }
"""
        content = re.sub(r'auto lexeme = resolve\(u\.payload\);.*?activeZone\.addToFormation\(lexeme\.get\(\)\);', new_logic, content, flags=re.DOTALL)
        with open(path, 'w') as f:
            f.write(content)
        print("Updated LanguageSystem.cpp")


def update_relationmanager():
    path = os.path.join(src_dir, 'Relation/RelationManager.hpp')
    with open(path, 'r') as f:
        content = f.read()
    if "findAdjacentLexemes" not in content:
        insert = """
    // Semantic Traversal Utilities (Logos Phase 4)
    std::vector<std::string> findAdjacentEntities(const std::string& entityId, const std::string& relationType = "") const;
"""
        content = content.replace("std::vector<std::shared_ptr<Relation>> getRelationsOfType(const std::string& type) const;", "std::vector<std::shared_ptr<Relation>> getRelationsOfType(const std::string& type) const;\n" + insert)
        with open(path, 'w') as f:
            f.write(content)
        print("Updated RelationManager.hpp")
        
    path2 = os.path.join(src_dir, 'Relation/RelationManager.cpp')
    with open(path2, 'r') as f:
        content2 = f.read()
    if "findAdjacentEntities" not in content2:
        impl = """
std::vector<std::string> RelationManager::findAdjacentEntities(const std::string& entityId, const std::string& relationType) const {
    std::vector<std::string> result;
    for (const auto& r : relations) {
        if (!relationType.empty() && r->type != relationType) continue;
        if (r->entityA == entityId) result.push_back(r->entityB);
        else if (!r->directed && r->entityB == entityId) result.push_back(r->entityA);
    }
    return result;
}
"""
        content2 += "\n" + impl
        with open(path2, 'w') as f:
            f.write(content2)
        print("Updated RelationManager.cpp")

update_formation_hpp()
update_formation_cpp()
update_languagesystem_cpp()
update_relationmanager()
