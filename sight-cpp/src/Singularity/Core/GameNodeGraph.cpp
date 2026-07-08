// GameNodeGraph.cpp — the in-scene SDF node graph (Mode3D::Graph).
//
// Every Field object is backed by a geom::SdfNode expression tree (the same tree
// that Blend/Boolean/Implicit/Combine/Clay all write). This tool makes that tree
// the interface: it walks the selected field's tree, lays it out, and draws it as
// floating cards beside the object in the scene (ImGui foreground draw list at
// projected screen positions). Click a card to select a node; the node panel edits
// or restructures it and the shape re-meshes live via setFieldShape.

#include "Game.hpp"
#include "Form/Object/Object.hpp"

#include <imgui.h>
#include <OpenGL/glu.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace Core {

namespace {
using geom::SdfNode;
using geom::SdfOp;
using geom::SdfPrim;

const char* opName(SdfOp op) {
    switch (op) {
        case SdfOp::Union:       return "Union";
        case SdfOp::Intersect:   return "Intersect";
        case SdfOp::Subtract:    return "Subtract";
        case SdfOp::SmoothUnion: return "Smooth Union";
        case SdfOp::Morph:       return "Morph";
        default:                 return "Leaf";
    }
}
const char* primName(SdfPrim p) {
    switch (p) {
        case SdfPrim::Sphere:    return "Sphere";
        case SdfPrim::Box:       return "Box";
        case SdfPrim::RoundBox:  return "Round Box";
        case SdfPrim::Ellipsoid: return "Ellipsoid";
        case SdfPrim::Cylinder:  return "Cylinder";
        case SdfPrim::Cone:      return "Cone";
        case SdfPrim::Torus:     return "Torus";
        case SdfPrim::Expr:      return "f(x,y,z)";
        case SdfPrim::Convex:    return "Polyhedron";
        default:                 return "?";
    }
}
std::string nodeLabel(const SdfNode& n) {
    if (n.children.size() == 2) {
        if (n.op == SdfOp::Morph || n.op == SdfOp::SmoothUnion) {
            char buf[48]; std::snprintf(buf, sizeof(buf), "%s  %.2f", opName(n.op), n.t);
            return buf;
        }
        return opName(n.op);
    }
    return primName(n.prim);
}

// True if path `a` is an ancestor-or-equal of `b` (a is a prefix of b). Used to
// forbid swapping a node with its own ancestor/descendant (which would form a cycle).
bool isPrefix(const std::vector<int>& a, const std::vector<int>& b) {
    if (a.size() > b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) if (a[i] != b[i]) return false;
    return true;
}

// Navigate to a node by its child-index path (empty path = root). Null if invalid.
SdfNode* nodeAtPath(SdfNode& root, const std::vector<int>& path) {
    SdfNode* n = &root;
    for (int i : path) {
        if (i < 0 || i >= static_cast<int>(n->children.size()) || !n->children[i]) return nullptr;
        n = n->children[i].get();
    }
    return n;
}
} // namespace

void Game::renderNodeGraph() {
    if (_current3DMode != Mode3D::Graph) return;
    Object* o = _selectedObject3D;
    if (!o || !o->hasField()) return;
    const SdfNode& root = o->getFieldData();

    // 1. Flatten the tree with a layout: depth -> column, in-order leaf -> row,
    //    internal node row = average of its children's rows (a clean binary tree).
    struct NV { std::vector<int> path; int depth; float row; int parent; std::string label;
                bool scrub; float sval; };
    std::vector<NV> nodes;
    int nextLeaf = 0;
    std::function<int(const SdfNode&, int, int, std::vector<int>)> collect =
        [&](const SdfNode& n, int depth, int parent, std::vector<int> path) -> int {
            int idx = static_cast<int>(nodes.size());
            // Morph / SmoothUnion nodes expose a blend strip (scrubbable t).
            bool scrub = (n.children.size() == 2 && (n.op == SdfOp::Morph || n.op == SdfOp::SmoothUnion));
            nodes.push_back({ path, depth, 0.0f, parent, nodeLabel(n), scrub, n.t });
            if (n.children.size() == 2 && n.children[0] && n.children[1]) {
                auto p0 = path; p0.push_back(0);
                auto p1 = path; p1.push_back(1);
                int c0 = collect(*n.children[0], depth + 1, idx, p0);
                int c1 = collect(*n.children[1], depth + 1, idx, p1);
                nodes[idx].row = (nodes[c0].row + nodes[c1].row) * 0.5f;
            } else {
                nodes[idx].row = static_cast<float>(nextLeaf++);
            }
            return idx;
        };
    collect(root, 0, -1, {});

    // 2. Lay the graph out in WORLD space on a camera-facing plane anchored at the
    //    object, then project each node individually -> true 3D billboards: they sit
    //    at the object's depth and parallax/scale with the camera. (Drawn via ImGui,
    //    so they overlay rather than being occluded by geometry — that last step
    //    would need a GL text system.)
    const glm::mat4 T = o->getTransform();
    const glm::vec3 c    = glm::vec3(T[3]);
    const glm::vec3 camP = _camera.pos;
    const glm::vec3 fwd  = glm::normalize(_camera.front);
    const glm::vec3 wup  = glm::normalize(_camera.up);
    const glm::vec3 right= glm::normalize(glm::cross(fwd, wup));
    const glm::vec3 up   = glm::normalize(glm::cross(right, fwd));
    const float objScale = (glm::length(glm::vec3(T[0])) + glm::length(glm::vec3(T[1])) +
                            glm::length(glm::vec3(T[2]))) / 3.0f;
    const float sideGap = objScale * 1.2f + 0.6f;   // clear the object
    const float COLWw = 0.95f, ROWHw = 0.42f;       // world-space node spacing

    float minRow = 1e9f, maxRow = -1e9f;
    for (auto& nv : nodes) { minRow = std::min(minRow, nv.row); maxRow = std::max(maxRow, nv.row); }
    const float midRow = (minRow + maxRow) * 0.5f;
    auto worldOf = [&](const NV& nv) {
        return c + right * (sideGap + nv.depth * COLWw) + up * (-(nv.row - midRow) * ROWHw);
    };

    // Card/font screen size follows the object's distance (world spacing already
    // foreshortens with perspective, so cards & gaps scale together).
    const float dist = glm::length(c - camP);
    const float gs = glm::clamp(4.0f / std::max(dist, 0.1f), 0.6f, 1.5f);
    const float CARDW = 124.0f * gs, CARDH = 30.0f * gs;
    const float fsz = ImGui::GetFontSize() * gs;
    ImFont* const font = ImGui::GetFont();

    const ImVec2 fbs = ImGui::GetIO().DisplayFramebufferScale;
    const float scaleX = fbs.x > 0.0f ? fbs.x : 1.0f;
    const float scaleY = fbs.y > 0.0f ? fbs.y : 1.0f;
    const float vpH = static_cast<float>(_camera.viewport[3]);
    auto project = [&](const glm::vec3& w, ImVec2& out) -> bool {
        GLdouble px, py, pz;
        if (!gluProject(w.x, w.y, w.z, _camera.modelview, _camera.projection, _camera.viewport, &px, &py, &pz))
            return false;
        if (pz < 0.0 || pz > 1.0) return false; // behind the camera
        out = ImVec2(static_cast<float>(px) / scaleX, (vpH - static_cast<float>(py)) / scaleY);
        return true;
    };
    { ImVec2 tmp; if (!project(c, tmp)) return; } // object behind camera -> skip

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImVec2 mouse = ImGui::GetMousePos();
    const bool overWindow = ImGui::GetIO().WantCaptureMouse;
    const float STRIPH = CARDH * 0.34f;

    // Per-node projection (auto layout) + hand-dragged manual offset.
    auto manOff = [&](const std::vector<int>& path) -> ImVec2 {
        auto it = _graphManualOffset.find(path);
        return it != _graphManualOffset.end() ? ImVec2(it->second.x, it->second.y) : ImVec2(0.0f, 0.0f);
    };
    std::vector<ImVec2> autoPos(nodes.size()), cpos(nodes.size());
    std::vector<unsigned char> vis(nodes.size(), 1);
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (!project(worldOf(nodes[i]), autoPos[i])) { vis[i] = 0; continue; }
        ImVec2 mo = manOff(nodes[i].path);
        cpos[i] = ImVec2(autoPos[i].x + mo.x, autoPos[i].y + mo.y);
    }
    auto findIdx = [&](const std::vector<int>& path) {
        for (size_t i = 0; i < nodes.size(); ++i) if (nodes[i].path == path) return static_cast<int>(i);
        return -1;
    };
    auto stripRect = [&](int i, ImVec2& a, ImVec2& b) {
        a = ImVec2(cpos[i].x - CARDW * 0.5f + 5.0f, cpos[i].y + CARDH * 0.5f - STRIPH - 3.0f);
        b = ImVec2(cpos[i].x + CARDW * 0.5f - 5.0f, cpos[i].y + CARDH * 0.5f - 3.0f);
    };

    // Which card (and whether a scrub strip) the mouse is over.
    int hovered = -1, hoveredStrip = -1;
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (!vis[i] || overWindow) continue;
        if (mouse.x >= cpos[i].x - CARDW * 0.5f && mouse.x <= cpos[i].x + CARDW * 0.5f &&
            mouse.y >= cpos[i].y - CARDH * 0.5f && mouse.y <= cpos[i].y + CARDH * 0.5f)
            hovered = static_cast<int>(i);
        if (nodes[i].scrub) { ImVec2 sa, sb; stripRect(static_cast<int>(i), sa, sb);
            if (mouse.x >= sa.x && mouse.x <= sb.x && mouse.y >= sa.y && mouse.y <= sb.y) hoveredStrip = static_cast<int>(i); }
    }

    // Left press: strip -> scrub blend; body -> select (+ maybe drag). Right press -> menu.
    if (!overWindow && ImGui::IsMouseClicked(0)) {
        if (hoveredStrip >= 0) { _graphSelPath = nodes[hoveredStrip].path; _graphHasSel = true; _graphScrubbing = true; }
        else if (hovered >= 0) { _graphSelPath = nodes[hovered].path; _graphHasSel = true; }
    }
    if (!overWindow && ImGui::IsMouseClicked(1) && hovered >= 0) {
        _graphSelPath = nodes[hovered].path; _graphHasSel = true;
        _graphCtxPath = nodes[hovered].path; ImGui::OpenPopup("nodeCtx");
    }
    const int srcIdx = _graphHasSel ? findIdx(_graphSelPath) : -1;

    // Scrub the blend t by dragging along the strip of the selected node.
    if (!ImGui::IsMouseDown(0)) _graphScrubbing = false;
    if (_graphScrubbing && srcIdx >= 0) {
        float L = cpos[srcIdx].x - CARDW * 0.5f + 5.0f, R = cpos[srcIdx].x + CARDW * 0.5f - 5.0f;
        float frac = (R > L) ? (mouse.x - L) / (R - L) : 0.0f;
        frac = frac < 0.0f ? 0.0f : (frac > 1.0f ? 1.0f : frac);
        SdfNode tree = o->getFieldData();
        if (SdfNode* sn = nodeAtPath(tree, _graphSelPath)) { sn->t = frac; o->setFieldShape(tree, o->getFieldExtent()); }
    }

    // Drag-to-move/swap (not while scrubbing): the dragged card follows the cursor;
    // drop on another card = swap subtrees, drop on empty = leave it there (reposition).
    if (!overWindow && !_graphScrubbing && srcIdx >= 0 && vis[srcIdx] && ImGui::IsMouseDragging(0))
        _graphDragging = true;
    bool validTarget = false;
    if (_graphDragging && srcIdx >= 0 && vis[srcIdx]) {
        cpos[srcIdx] = mouse;                       // the card follows the cursor
        if (hovered >= 0 && hovered != srcIdx)
            validTarget = !isPrefix(_graphSelPath, nodes[hovered].path) &&
                          !isPrefix(nodes[hovered].path, _graphSelPath);
    }

    // 3. Wires + A/B input labels.
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].parent < 0 || !vis[i] || !vis[nodes[i].parent]) continue;
        ImVec2 ap(cpos[nodes[i].parent].x + CARDW * 0.5f, cpos[nodes[i].parent].y);
        ImVec2 bp(cpos[i].x - CARDW * 0.5f, cpos[i].y);
        dl->AddLine(ap, bp, IM_COL32(150, 180, 210, 200), 2.0f);
        dl->AddText(font, fsz, ImVec2(bp.x - 13.0f * gs, bp.y - 7.0f * gs),
                    IM_COL32(150, 180, 210, 255), nodes[i].path.back() == 0 ? "A" : "B");
    }

    // 5. Cards + labels + blend strips (with a soft drop shadow -> floating billboards).
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (!vis[i]) continue;
        ImVec2 p = cpos[i];
        ImVec2 tl(p.x - CARDW * 0.5f, p.y - CARDH * 0.5f);
        ImVec2 br(p.x + CARDW * 0.5f, p.y + CARDH * 0.5f);
        bool sel = _graphHasSel && _graphSelPath == nodes[i].path;
        bool tgt = _graphDragging && static_cast<int>(i) == hovered && static_cast<int>(i) != srcIdx;
        bool leaf = nodes[i].path.empty() ? (root.children.size() != 2) : (nodes[i].depth > 0);
        ImU32 fill = sel ? IM_COL32(60, 90, 130, 240)
                         : (leaf ? IM_COL32(40, 52, 44, 225) : IM_COL32(35, 40, 52, 225));
        ImU32 bord = sel ? IM_COL32(255, 215, 60, 255) : IM_COL32(120, 140, 170, 255);
        if (tgt) bord = validTarget ? IM_COL32(80, 230, 120, 255) : IM_COL32(230, 90, 90, 255);
        dl->AddRectFilled(ImVec2(tl.x + 3, tl.y + 4), ImVec2(br.x + 3, br.y + 4), IM_COL32(0, 0, 0, 70), 5.0f);
        dl->AddRectFilled(tl, br, fill, 5.0f);
        dl->AddRect(tl, br, bord, 5.0f, 0, (sel || tgt) ? 2.5f : 1.5f);
        ImVec2 ts = ImGui::CalcTextSize(nodes[i].label.c_str());
        dl->AddText(font, fsz, ImVec2(p.x - ts.x * gs * 0.5f,
                    p.y - ts.y * gs * 0.5f - (nodes[i].scrub ? STRIPH * 0.45f : 0.0f)),
                    IM_COL32(235, 240, 250, 255), nodes[i].label.c_str());
        if (nodes[i].scrub) {                       // blend strip: a mini slider for t
            ImVec2 sa, sb; stripRect(static_cast<int>(i), sa, sb);
            dl->AddRectFilled(sa, sb, IM_COL32(20, 24, 32, 220), 3.0f);
            float frac = nodes[i].sval; frac = frac < 0.0f ? 0.0f : (frac > 1.0f ? 1.0f : frac);
            ImU32 fillc = (_graphScrubbing && sel) ? IM_COL32(255, 215, 60, 255) : IM_COL32(90, 150, 220, 255);
            dl->AddRectFilled(sa, ImVec2(sa.x + (sb.x - sa.x) * frac, sb.y), fillc, 3.0f);
        }
        if (sel) _graphSelScreen = glm::vec2(br.x, tl.y);
    }

    // 6. Release: drop-on-card = swap subtrees; drop-on-empty = reposition (manual offset).
    if (ImGui::IsMouseReleased(0)) {
        if (_graphDragging && srcIdx >= 0) {
            if (hovered >= 0 && hovered != srcIdx && validTarget) {
                SdfNode tree = o->getFieldData();
                SdfNode* ns = nodeAtPath(tree, _graphSelPath);
                SdfNode* nt = nodeAtPath(tree, nodes[hovered].path);
                if (ns && nt) { std::swap(*ns, *nt); o->setFieldShape(tree, o->getFieldExtent()); }
                _graphManualOffset.clear();          // structure changed -> auto-relayout
            } else if (vis[srcIdx]) {
                _graphManualOffset[_graphSelPath] = glm::vec2(mouse.x - autoPos[srcIdx].x,
                                                              mouse.y - autoPos[srcIdx].y);
            }
        }
        _graphDragging = false;
    }

    // Right-click context menu: quick structural actions in the scene.
    if (ImGui::BeginPopup("nodeCtx")) {
        SdfNode tree = o->getFieldData();
        SdfNode* cn = nodeAtPath(tree, _graphCtxPath);
        bool changed = false, structural = false;
        if (cn) {
            ImGui::TextDisabled("%s", nodeLabel(*cn).c_str());
            ImGui::Separator();
            if (cn->children.size() == 2 && cn->children[0] && cn->children[1]) {
                if (ImGui::MenuItem("Swap A / B")) { std::swap(cn->children[0], cn->children[1]); changed = true; }
            }
            if (ImGui::MenuItem("Wrap in Union (+sphere)")) {
                *cn = SdfNode::binary(SdfOp::Union, *cn, SdfNode::leaf(SdfPrim::Sphere, glm::vec3(0.4f)), 0.5f);
                changed = true; structural = true;
            }
            if (!_graphCtxPath.empty() && ImGui::MenuItem("Delete (keep sibling)")) {
                std::vector<int> pp = _graphCtxPath; int idx = pp.back(); pp.pop_back();
                SdfNode* parent = nodeAtPath(tree, pp);
                if (parent && parent->children.size() == 2 && parent->children[1 - idx]) {
                    *parent = *parent->children[1 - idx];
                    _graphSelPath = pp; changed = true; structural = true;
                }
            }
        }
        ImGui::EndPopup();
        if (changed) { o->setFieldShape(tree, o->getFieldExtent()); if (structural) _graphManualOffset.clear(); }
    }

    renderNodePanel();
}

void Game::renderNodePanel() {
    if (!_graphHasSel) return;
    Object* o = _selectedObject3D;
    if (!o || !o->hasField()) { _graphHasSel = false; return; }

    SdfNode tree = o->getFieldData();              // editable working copy
    SdfNode* n = nodeAtPath(tree, _graphSelPath);
    if (!n) { _graphHasSel = false; return; }

    // Float the editor next to the selected node card; reposition only when the
    // selection changes, so it stays draggable while editing one node.
    if (_graphSelPath != _graphPanelPath) {
        ImGui::SetNextWindowPos(ImVec2(_graphSelScreen.x + 16.0f, _graphSelScreen.y), ImGuiCond_Always);
        _graphPanelPath = _graphSelPath;
    }
    ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("SDF Node");
    bool changed = false;

    if (n->children.size() == 2) {
        const char* ops[] = { "Union", "Intersect", "Subtract", "Smooth Union", "Morph" };
        const SdfOp opMap[] = { SdfOp::Union, SdfOp::Intersect, SdfOp::Subtract,
                                SdfOp::SmoothUnion, SdfOp::Morph };
        int cur = 0; for (int i = 0; i < 5; ++i) if (opMap[i] == n->op) cur = i;
        if (ImGui::Combo("Operation", &cur, ops, 5)) { n->op = opMap[cur]; changed = true; }
        if (n->op == SdfOp::Morph || n->op == SdfOp::SmoothUnion) {
            const char* lbl = (n->op == SdfOp::Morph) ? "Blend t" : "Smoothness";
            if (ImGui::SliderFloat(lbl, &n->t, 0.0f, 1.0f, "%.2f")) changed = true;
        }
        // Operand order matters for Subtract (A-B) and Morph; let the user flip it.
        if (ImGui::Button("Swap A / B inputs")) { std::swap(n->children[0], n->children[1]); changed = true; }
        ImGui::TextDisabled("Inputs: child A and child B (click them in the graph).");
    } else if (n->prim == SdfPrim::Expr) {
        ImGui::TextUnformatted("Implicit leaf  f(x,y,z) = 0");
        // Editable expression: refresh the buffer when a different node is selected,
        // otherwise keep the user's in-progress text.
        static char ebuf[256] = "";
        static std::vector<int> ebufPath{ -999 };
        if (ebufPath != _graphSelPath) { std::snprintf(ebuf, sizeof(ebuf), "%s", n->expr.c_str()); ebufPath = _graphSelPath; }
        bool apply = ImGui::InputText("f(x,y,z)", ebuf, sizeof(ebuf), ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        apply |= ImGui::Button("Apply");
        if (apply) { *n = geom::makeImplicit(ebuf); changed = true; }
        ImGui::TextDisabled("ops + - * / ^   sin cos tan sqrt abs exp log");
    } else if (n->prim == SdfPrim::Convex) {
        ImGui::Text("Convex polyhedron leaf  (%d faces)", static_cast<int>(n->planes.size()));
        ImGui::TextDisabled("Shape baked from a polyhedron's faces (max of half-spaces).");
        if (ImGui::SliderFloat3("Offset", &n->offset.x, -2.0f, 2.0f, "%.2f")) changed = true;
    } else {
        const char* prims[] = { "Sphere", "Box", "Round Box", "Ellipsoid", "Cylinder", "Cone", "Torus" };
        const SdfPrim pMap[] = { SdfPrim::Sphere, SdfPrim::Box, SdfPrim::RoundBox, SdfPrim::Ellipsoid,
                                 SdfPrim::Cylinder, SdfPrim::Cone, SdfPrim::Torus };
        int cur = 0; for (int i = 0; i < 7; ++i) if (pMap[i] == n->prim) cur = i;
        if (ImGui::Combo("Primitive", &cur, prims, 7)) { n->prim = pMap[cur]; changed = true; }
        if (ImGui::SliderFloat3("Dimensions", &n->dims.x, 0.05f, 1.5f, "%.2f")) changed = true;
        if (ImGui::SliderFloat3("Offset", &n->offset.x, -2.0f, 2.0f, "%.2f")) changed = true;
    }

    ImGui::Separator();
    // Add / combine: wrap THIS node in (this <op> newPrimitive). Grows the tree.
    ImGui::TextUnformatted("Add / combine");
    static int addOp = 0, addPrim = 0;
    const char* addOps[] = { "Union", "Intersect", "Subtract", "Smooth Union", "Morph" };
    const SdfOp addOMap[] = { SdfOp::Union, SdfOp::Intersect, SdfOp::Subtract,
                              SdfOp::SmoothUnion, SdfOp::Morph };
    const char* addPrims[] = { "Sphere", "Box", "Round Box", "Ellipsoid", "Cylinder", "Cone", "Torus",
                               "Implicit f=0" };
    const SdfPrim addPMap[] = { SdfPrim::Sphere, SdfPrim::Box, SdfPrim::RoundBox, SdfPrim::Ellipsoid,
                                SdfPrim::Cylinder, SdfPrim::Cone, SdfPrim::Torus }; // [7]=Implicit, special-cased
    ImGui::SetNextItemWidth(130.0f); ImGui::Combo("op##add", &addOp, addOps, 5);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130.0f); ImGui::Combo("with##add", &addPrim, addPrims, 8);
    static char addExpr[256] = "x*x + y*y + z*z - 0.25";
    if (addPrim == 7) ImGui::InputText("expr##add", addExpr, sizeof(addExpr));
    if (ImGui::Button("Add (wrap this node)")) {
        SdfNode leaf = (addPrim == 7) ? geom::makeImplicit(addExpr)
                                      : SdfNode::leaf(addPMap[addPrim], glm::vec3(0.4f));
        *n = SdfNode::binary(addOMap[addOp], *n, leaf, 0.5f);
        _graphSelPath.push_back(1); // select the new leaf so it's ready to position
        _graphManualOffset.clear(); // structure changed -> auto-relayout
        changed = true;
    }

    ImGui::Separator();
    // Collapse: remove this node, keeping its sibling in the parent's place.
    if (!_graphSelPath.empty()) {
        if (ImGui::Button("Delete (keep sibling)")) {
            std::vector<int> pp = _graphSelPath; int idx = pp.back(); pp.pop_back();
            SdfNode* parent = nodeAtPath(tree, pp);
            if (parent && parent->children.size() == 2 && parent->children[1 - idx]) {
                *parent = *parent->children[1 - idx]; // collapse: parent becomes the other operand
                _graphSelPath = pp;
                _graphManualOffset.clear(); // structure changed -> auto-relayout
                changed = true;
            }
        }
    } else {
        ImGui::TextDisabled("Root node — delete disabled.");
    }
    ImGui::End();

    if (changed) o->setFieldShape(tree, o->getFieldExtent());
}

} // namespace Core
