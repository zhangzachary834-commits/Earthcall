#pragma once
#include <unordered_set>
#include <string>

class Object;

namespace Rendering {

class HighlightSystem {
public:
    static void setSelected(Object* obj);
    static Object* getSelected();

    static void setLawCandidateIds(const std::unordered_set<std::string>& ids);
    static bool isLawCandidate(const Object* obj);
    static bool isSelected(const Object* obj);

private:
    static Object* s_selected;
    static std::unordered_set<std::string> s_lawIds;
};

} // namespace Rendering


