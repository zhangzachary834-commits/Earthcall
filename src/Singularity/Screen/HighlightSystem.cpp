#include "HighlightSystem.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

namespace Rendering {

Object* HighlightSystem::s_selected = nullptr;
std::unordered_set<std::string> HighlightSystem::s_selectedIds = {};
std::unordered_set<std::string> HighlightSystem::s_lawIds = {};

void HighlightSystem::setSelected(Object* obj) { s_selected = obj; }
Object* HighlightSystem::getSelected() { return s_selected; }

void HighlightSystem::setSelectedIds(const std::unordered_set<std::string>& ids) {
    s_selectedIds = ids;
}

void HighlightSystem::setLawCandidateIds(const std::unordered_set<std::string>& ids) {
    s_lawIds = ids;
}

bool HighlightSystem::isLawCandidate(const Object* obj) {
    if (!obj) return false;
    const std::string& id = obj->getIdentifier();
    if (id.empty()) return false;
    return s_lawIds.find(id) != s_lawIds.end();
}

bool HighlightSystem::isSelected(const Object* obj) {
    if (!obj) return false;
    if (obj == s_selected) return true;
    const std::string& id = obj->getIdentifier();
    return !id.empty() && s_selectedIds.find(id) != s_selectedIds.end();
}

} // namespace Rendering

// C-linkage like bridging helpers so UI code can update highlight state without including headers everywhere
extern "C" {
    void HighlightSystem_setSelected(Object* obj) {
        Rendering::HighlightSystem::setSelected(obj);
    }
    void HighlightSystem_setSelectedIds(const std::unordered_set<std::string>& ids) {
        Rendering::HighlightSystem::setSelectedIds(ids);
    }
    void HighlightSystem_setLawIds(const std::unordered_set<std::string>& ids) {
        Rendering::HighlightSystem::setLawCandidateIds(ids);
    }
}

