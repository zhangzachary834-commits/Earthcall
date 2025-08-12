#include "HighlightSystem.hpp"
#include "Form/Object/Object.hpp"

namespace Rendering {

Object* HighlightSystem::s_selected = nullptr;
std::unordered_set<std::string> HighlightSystem::s_lawIds = {};

void HighlightSystem::setSelected(Object* obj) { s_selected = obj; }
Object* HighlightSystem::getSelected() { return s_selected; }

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
    return obj && obj == s_selected;
}

} // namespace Rendering

// C-linkage like bridging helpers so UI code can update highlight state without including headers everywhere
extern "C" {
    void HighlightSystem_setSelected(Object* obj) {
        Rendering::HighlightSystem::setSelected(obj);
    }
    void HighlightSystem_setLawIds(const std::unordered_set<std::string>& ids) {
        Rendering::HighlightSystem::setLawCandidateIds(ids);
    }
}


