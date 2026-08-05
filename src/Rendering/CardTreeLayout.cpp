#include "Rendering/CardTreeLayout.hpp"

namespace Rendering {

std::vector<CardSlot> layoutCardTree(int nodeCount, int root,
                                     const std::function<int(int)>& childCount,
                                     const std::function<int(int, int)>& childAt) {
    std::vector<CardSlot> slots(static_cast<std::size_t>(nodeCount > 0 ? nodeCount : 0));
    if (nodeCount <= 0 || root < 0 || root >= nodeCount) return slots;

    int nextLeaf = 0;
    std::function<void(int, int, int)> place = [&](int idx, int depth, int parent) {
        slots[idx].depth = depth;
        slots[idx].parent = parent;
        const int n = childCount(idx);
        if (n <= 0) {
            slots[idx].row = static_cast<float>(nextLeaf++);
            return;
        }
        float sum = 0.0f;
        for (int k = 0; k < n; ++k) {
            const int child = childAt(idx, k);
            if (child < 0 || child >= nodeCount) continue;
            place(child, depth + 1, idx);
            sum += slots[child].row;
        }
        slots[idx].row = sum / static_cast<float>(n);
    };
    place(root, 0, -1);
    return slots;
}

} // namespace Rendering
