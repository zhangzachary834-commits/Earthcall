#pragma once

#include <functional>
#include <vector>

namespace Rendering {

// Pure tree-to-grid layout: depth -> column, in-order leaf -> row, internal
// node row = the average of its children's rows. This is the layout the
// in-scene SDF node graph uses (GameNodeGraph.cpp), generalized from binary
// to n-ary and extracted so the Law/Concept graph window shares one geometry.
// (GameNodeGraph converges onto this after its next visually-verified pass.)
struct CardSlot {
    int depth = 0;
    float row = 0.0f;
    int parent = -1;
};

// The tree is already flattened: node i's children are indices into the same
// array, described by childCount(i) / childAt(i, k). Returns one slot per
// node; nodes unreachable from `root` keep default slots.
std::vector<CardSlot> layoutCardTree(int nodeCount, int root,
                                     const std::function<int(int)>& childCount,
                                     const std::function<int(int, int)>& childAt);

} // namespace Rendering
