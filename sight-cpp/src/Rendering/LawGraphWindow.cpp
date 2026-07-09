#include "Rendering/LawGraphWindow.hpp"

#include "Rendering/CardTreeLayout.hpp"
#include "Form/Object/Creation/ObjectConcept.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace Rendering {

namespace {

// UI session state only — the models remain the single truth.
struct SessionState {
    std::string selectedLawId;
    int selectedCard = -1;
    std::unordered_map<std::string, std::string> eventBindings;   // lawId -> type
    char eventBuf[64] = "";
    char pathBuf[128] = "";
    char textBuf[128] = "";
    char varBuf[32] = "";        // new math-binding variable name
    char bindPathBuf[128] = "";  // new math-binding property path
    std::string lastEditLaw;
    int lastEditCard = -1;
};
SessionState g;

ImU32 cardColor(LawCard::Kind kind) {
    switch (kind) {
        case LawCard::Kind::Law:       return IM_COL32(122, 92, 200, 255);
        case LawCard::Kind::Event:     return IM_COL32(196, 148, 44, 255);
        case LawCard::Kind::Condition: return IM_COL32(46, 128, 190, 255);
        case LawCard::Kind::Action:    return IM_COL32(52, 158, 92, 255);
    }
    return IM_COL32(96, 96, 96, 255);
}

void flattenCondition(const ConditionNode& node, std::vector<LawCard>& cards,
                      int parent, std::vector<int> modelPath) {
    LawCard card;
    card.kind = LawCard::Kind::Condition;
    card.label = node.describe();
    card.isCondition = true;
    card.modelPath = modelPath;
    const int idx = static_cast<int>(cards.size());
    cards.push_back(std::move(card));
    if (parent >= 0) cards[parent].children.push_back(idx);
    for (std::size_t i = 0; i < node.children.size(); ++i) {
        auto path = modelPath;
        path.push_back(static_cast<int>(i));
        flattenCondition(node.children[i], cards, idx, std::move(path));
    }
}

void flattenAction(const ActionNode& node, std::vector<LawCard>& cards,
                   int parent, std::vector<int> modelPath) {
    LawCard card;
    card.kind = LawCard::Kind::Action;
    card.label = node.describe();
    card.isCondition = false;
    card.modelPath = modelPath;
    const int idx = static_cast<int>(cards.size());
    cards.push_back(std::move(card));
    if (parent >= 0) cards[parent].children.push_back(idx);
    for (std::size_t i = 0; i < node.children.size(); ++i) {
        auto path = modelPath;
        path.push_back(static_cast<int>(i));
        flattenAction(node.children[i], cards, idx, std::move(path));
    }
}

void copyToBuf(char* buf, std::size_t size, const std::string& value) {
    std::strncpy(buf, value.c_str(), size - 1);
    buf[size - 1] = '\0';
}

double numericOr(const PropertyValue& v, double fallback) {
    double out = fallback;
    propertyValueToNumber(v, out);
    return out;
}

} // namespace

std::vector<LawCard> flattenLaw(const Law& law, const std::string& eventBinding) {
    std::vector<LawCard> cards;
    LawCard root;
    root.kind = LawCard::Kind::Law;
    root.label = law.name() + (law.isEnabled() ? "" : " (disabled)");
    cards.push_back(std::move(root));

    if (!eventBinding.empty()) {
        LawCard event;
        event.kind = LawCard::Kind::Event;
        event.label = "on: " + eventBinding;
        const int idx = static_cast<int>(cards.size());
        cards.push_back(std::move(event));
        cards[0].children.push_back(idx);
    }
    if (law.hasConditionModel()) {
        flattenCondition(*law.conditionModel(), cards, 0, {});
    }
    if (law.hasActionModel()) {
        flattenAction(*law.actionModel(), cards, 0, {});
    }
    return cards;
}

ConditionNode* conditionAt(ConditionNode& root, const std::vector<int>& path) {
    ConditionNode* node = &root;
    for (int i : path) {
        if (i < 0 || i >= static_cast<int>(node->children.size())) return nullptr;
        node = &node->children[i];
    }
    return node;
}

ActionNode* actionAt(ActionNode& root, const std::vector<int>& path) {
    ActionNode* node = &root;
    for (int i : path) {
        if (i < 0 || i >= static_cast<int>(node->children.size())) return nullptr;
        node = &node->children[i];
    }
    return node;
}

namespace {

// ---------------------------------------------------------------------------
// Per-node editors. Each mutates a COPY of the model; the caller commits it
// through setConditionModel/setActionModel so the law recompiles.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Authored-mathematics editors. Every foundational primitive — variables,
// bindings, coefficients, exponents, piece bounds — is Person-modifiable,
// during creation and after. Numbers edit in place; names/paths add and
// remove as rows (delete + re-add to change).
// ---------------------------------------------------------------------------

bool editMathBindings(MathBindings& bindings) {
    bool changed = false;
    ImGui::TextDisabled("Bindings (variable <- property path)");
    std::string removeKey;
    for (const auto& entry : bindings) {
        ImGui::PushID(entry.first.c_str());
        ImGui::BulletText("%s <- %s", entry.first.c_str(), entry.second.toString().c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("x")) removeKey = entry.first;
        ImGui::PopID();
    }
    if (!removeKey.empty()) {
        bindings.erase(removeKey);
        changed = true;
    }
    ImGui::SetNextItemWidth(60.0f);
    ImGui::InputText("##bindvar", g.varBuf, sizeof(g.varBuf));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180.0f);
    ImGui::InputText("##bindpath", g.bindPathBuf, sizeof(g.bindPathBuf));
    ImGui::SameLine();
    if (ImGui::Button("Add binding") && g.varBuf[0] != '\0' && g.bindPathBuf[0] != '\0') {
        bindings[g.varBuf] = PropertyPath::parse(g.bindPathBuf);
        g.varBuf[0] = '\0';
        g.bindPathBuf[0] = '\0';
        changed = true;
    }
    return changed;
}

bool editPiecewise(OntoMath::Piecewise& f, const MathBindings& bindings) {
    bool changed = false;
    ImGui::TextDisabled("f = %s", f.print().c_str());

    for (std::size_t p = 0; p < f.pieces.size(); ++p) {
        auto& piece = f.pieces[p];
        ImGui::PushID(static_cast<int>(p));
        if (f.pieces.size() > 1 || piece.hasLo || piece.hasHi) {
            ImGui::Text("Piece %zu over %s:", p + 1, f.inputVariable.c_str());
            if (ImGui::Checkbox("lo", &piece.hasLo)) changed = true;
            if (piece.hasLo) {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80.0f);
                double lo = piece.lo;
                if (ImGui::InputDouble("##lo", &lo)) { piece.lo = lo; changed = true; }
                ImGui::SameLine();
                if (ImGui::Checkbox("incl##lo", &piece.includeLo)) changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("hi", &piece.hasHi)) changed = true;
            if (piece.hasHi) {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80.0f);
                double hi = piece.hi;
                if (ImGui::InputDouble("##hi", &hi)) { piece.hi = hi; changed = true; }
                ImGui::SameLine();
                if (ImGui::Checkbox("incl##hi", &piece.includeHi)) changed = true;
            }
        }

        // Terms: coefficient and every exponent edit in place.
        int removeTerm = -1;
        for (std::size_t t = 0; t < piece.expression.terms.size(); ++t) {
            auto& term = piece.expression.terms[t];
            ImGui::PushID(static_cast<int>(t) + 100);
            ImGui::SetNextItemWidth(80.0f);
            double c = term.coefficient;
            if (ImGui::InputDouble("coeff", &c)) { term.coefficient = c; changed = true; }
            for (auto& factor : term.factors) {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(56.0f);
                double e = factor.second;
                if (ImGui::InputDouble(factor.first.c_str(), &e)) {
                    factor.second = e;
                    changed = true;
                }
            }
            // Multiply this term by a bound variable (exponent +1).
            for (const auto& binding : bindings) {
                if (term.factors.count(binding.first)) continue;
                ImGui::SameLine();
                if (ImGui::SmallButton(("*" + binding.first).c_str())) {
                    term.factors[binding.first] = 1.0;
                    changed = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("x##rmterm")) removeTerm = static_cast<int>(t);
            ImGui::PopID();
        }
        if (removeTerm >= 0) {
            piece.expression.terms.erase(piece.expression.terms.begin() + removeTerm);
            changed = true;
        }
        if (ImGui::SmallButton("+ term")) {
            piece.expression.terms.emplace_back(1.0);
            changed = true;
        }
        ImGui::PopID();
        ImGui::Separator();
    }
    if (ImGui::SmallButton("+ piece")) {
        OntoMath::Piecewise::Piece piece;
        piece.expression = OntoMath::Expression::constant(0.0);
        f.pieces.push_back(std::move(piece));
        changed = true;
    }
    return changed;
}

bool editConditionNode(ConditionNode& node) {
    bool changed = false;
    switch (node.kind) {
        case ConditionNode::Kind::Compare: {
            if (ImGui::InputText("Path", g.pathBuf, sizeof(g.pathBuf),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                node.path = PropertyPath::parse(g.pathBuf);
                changed = true;
            }
            static const char* ops[] = {"==", "!=", "<", "<=", ">", ">=", "near", "in-range"};
            int op = static_cast<int>(node.op);
            if (ImGui::Combo("Op", &op, ops, 8)) {
                node.op = static_cast<ConditionNode::Op>(op);
                changed = true;
            }
            double value = numericOr(node.operand, 0.0);
            if (ImGui::InputDouble("Value", &value)) {
                node.operand = PropertyValue(value);
                changed = true;
            }
            if (node.op == ConditionNode::Op::Near) {
                double tol = node.tolerance;
                if (ImGui::InputDouble("Tolerance", &tol)) {
                    node.tolerance = tol;
                    changed = true;
                }
            }
            if (node.op == ConditionNode::Op::InRange) {
                double lo = numericOr(node.lo, 0.0), hi = numericOr(node.hi, 0.0);
                if (ImGui::InputDouble("Low", &lo)) { node.lo = PropertyValue(lo); changed = true; }
                if (ImGui::InputDouble("High", &hi)) { node.hi = PropertyValue(hi); changed = true; }
            }
            break;
        }
        case ConditionNode::Kind::InRegion: {
            ImGui::TextDisabled("Region condition (sketched with the shape tools)");
            if (ImGui::InputText("Probe path", g.pathBuf, sizeof(g.pathBuf),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                node.probe = PropertyPath::parse(g.pathBuf);
                changed = true;
            }
            if (node.region.op == geom::SdfOp::Leaf &&
                node.region.prim == geom::SdfPrim::Sphere) {
                float radius = node.region.dims.x;
                if (ImGui::DragFloat("Sphere radius", &radius, 0.02f, 0.01f, 100.0f)) {
                    node.region.dims.x = radius;
                    changed = true;
                }
            }
            break;
        }
        case ConditionNode::Kind::Related: {
            ImGui::TextDisabled("Relation conditions resolve with the relation-graph work");
            if (ImGui::InputText("Relation type", g.textBuf, sizeof(g.textBuf),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                node.relationType = g.textBuf;
                changed = true;
            }
            break;
        }
        case ConditionNode::Kind::All:
        case ConditionNode::Kind::Any:
        case ConditionNode::Kind::Not: {
            ImGui::Text("%zu member condition(s)", node.children.size());
            if (node.kind != ConditionNode::Kind::Not || node.children.empty()) {
                if (ImGui::Button("+ Compare child")) {
                    node.children.push_back(ConditionNode::compare(
                        "position.y", ConditionNode::Op::Lt, PropertyValue(0.0)));
                    changed = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("+ Zone child")) {
                    node.children.push_back(ConditionNode::zone(
                        OntoMath::Piecewise::continuous(
                            OntoMath::Expression::variable("x")),
                        MathBindings{{"x", PropertyPath::parse("position.x")}},
                        PropertyValue{}, PropertyValue(0.0)));
                    changed = true;
                }
            }
            break;
        }
        case ConditionNode::Kind::Zone: {
            ImGui::TextDisabled("Satisfied when f(bindings) lies in the zone.");
            bool hasLo = !std::holds_alternative<std::monostate>(node.lo);
            if (ImGui::Checkbox("zone lo", &hasLo)) {
                node.lo = hasLo ? PropertyValue(0.0) : PropertyValue{};
                changed = true;
            }
            if (hasLo) {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(90.0f);
                double lo = numericOr(node.lo, 0.0);
                if (ImGui::InputDouble("##zlo", &lo)) { node.lo = PropertyValue(lo); changed = true; }
            }
            ImGui::SameLine();
            bool hasHi = !std::holds_alternative<std::monostate>(node.hi);
            if (ImGui::Checkbox("zone hi", &hasHi)) {
                node.hi = hasHi ? PropertyValue(0.0) : PropertyValue{};
                changed = true;
            }
            if (hasHi) {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(90.0f);
                double hi = numericOr(node.hi, 0.0);
                if (ImGui::InputDouble("##zhi", &hi)) { node.hi = PropertyValue(hi); changed = true; }
            }
            if (editMathBindings(node.bindings)) changed = true;
            if (editPiecewise(node.zoneFunction, node.bindings)) changed = true;
            break;
        }
    }
    return changed;
}

bool editActionNode(ActionNode& node) {
    bool changed = false;
    switch (node.kind) {
        case ActionNode::Kind::Set:
        case ActionNode::Kind::Add:
        case ActionNode::Kind::Scale:
        case ActionNode::Kind::Lerp: {
            if (ImGui::InputText("Path", g.pathBuf, sizeof(g.pathBuf),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                node.path = PropertyPath::parse(g.pathBuf);
                changed = true;
            }
            double value = numericOr(node.operand, 0.0);
            if (ImGui::InputDouble("Value", &value)) {
                node.operand = PropertyValue(value);
                changed = true;
            }
            if (node.kind == ActionNode::Kind::Lerp) {
                double factor = node.factor;
                if (ImGui::InputDouble("Blend", &factor)) {
                    node.factor = factor;
                    changed = true;
                }
            }
            break;
        }
        case ActionNode::Kind::Drive: {
            if (ImGui::InputText("Path", g.pathBuf, sizeof(g.pathBuf),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                node.path = PropertyPath::parse(g.pathBuf);
                changed = true;
            }
            if (ImGui::InputText("Input path", g.textBuf, sizeof(g.textBuf),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                node.input = PropertyPath::parse(g.textBuf);
                changed = true;
            }
            static const char* forms[] = {"constant", "polynomial", "sinusoid"};
            int form = static_cast<int>(node.curve.form);
            if (ImGui::Combo("Curve", &form, forms, 3)) {
                node.curve.form = static_cast<CurveModel::Form>(form);
                changed = true;
            }
            if (node.curve.form == CurveModel::Form::Constant ||
                node.curve.form == CurveModel::Form::Polynomial) {
                if (node.curve.coeffs.size() < 2) node.curve.coeffs.resize(2, 0.0);
                double c0 = node.curve.coeffs[0], c1 = node.curve.coeffs[1];
                if (ImGui::InputDouble("c0", &c0)) { node.curve.coeffs[0] = c0; changed = true; }
                if (node.curve.form == CurveModel::Form::Polynomial &&
                    ImGui::InputDouble("c1 (slope)", &c1)) {
                    node.curve.coeffs[1] = c1;
                    changed = true;
                }
            } else {
                double amp = node.curve.amplitude, freq = node.curve.frequency;
                double phase = node.curve.phase, bias = node.curve.bias;
                if (ImGui::InputDouble("Amplitude", &amp)) { node.curve.amplitude = amp; changed = true; }
                if (ImGui::InputDouble("Frequency", &freq)) { node.curve.frequency = freq; changed = true; }
                if (ImGui::InputDouble("Phase", &phase)) { node.curve.phase = phase; changed = true; }
                if (ImGui::InputDouble("Bias", &bias)) { node.curve.bias = bias; changed = true; }
            }
            break;
        }
        case ActionNode::Kind::Sequence:
        case ActionNode::Kind::Parallel: {
            ImGui::Text("%zu step(s)", node.children.size());
            if (ImGui::Button("+ Set step")) {
                node.children.push_back(ActionNode::set("position.y", PropertyValue(0.0)));
                changed = true;
            }
            break;
        }
        case ActionNode::Kind::Map: {
            ImGui::TextDisabled("path := f(bindings) — authored mathematics governs the output.");
            if (ImGui::InputText("Path", g.pathBuf, sizeof(g.pathBuf),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                node.path = PropertyPath::parse(g.pathBuf);
                changed = true;
            }
            if (editMathBindings(node.bindings)) changed = true;
            if (editPiecewise(node.mapFunction, node.bindings)) changed = true;
            break;
        }
        case ActionNode::Kind::Spawn: {
            const auto& concepts = ConceptRegistry::instance().getAll();
            const char* preview = node.conceptId.empty() ? "(choose concept)"
                                                         : node.conceptId.c_str();
            if (ImGui::BeginCombo("Concept", preview)) {
                for (const auto& concept : concepts) {
                    if (!concept) continue;
                    const std::string label =
                        concept->name() + "  [" + concept->getIdentifier() + "]";
                    if (ImGui::Selectable(label.c_str(),
                                          concept->getIdentifier() == node.conceptId)) {
                        node.conceptId = concept->getIdentifier();
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::TextDisabled("Target the law at a World: the container is the womb.");
            break;
        }
    }
    return changed;
}

// Refill the text buffers when the selection moves (buffers are per-selection
// scratch; the model is the truth).
void refreshEditBuffers(Law& law, const std::vector<LawCard>& cards, int selected) {
    if (g.lastEditLaw == g.selectedLawId && g.lastEditCard == selected) return;
    g.lastEditLaw = g.selectedLawId;
    g.lastEditCard = selected;
    g.pathBuf[0] = '\0';
    g.textBuf[0] = '\0';
    if (selected < 0 || selected >= static_cast<int>(cards.size())) return;
    const LawCard& card = cards[selected];
    if (card.kind == LawCard::Kind::Condition && law.hasConditionModel()) {
        ConditionModel model = *law.conditionModel();
        if (ConditionNode* node = conditionAt(model, card.modelPath)) {
            copyToBuf(g.pathBuf, sizeof(g.pathBuf),
                      node->kind == ConditionNode::Kind::InRegion ? node->probe.toString()
                                                                  : node->path.toString());
            copyToBuf(g.textBuf, sizeof(g.textBuf), node->relationType);
        }
    } else if (card.kind == LawCard::Kind::Action && law.hasActionModel()) {
        ActionModel model = *law.actionModel();
        if (ActionNode* node = actionAt(model, card.modelPath)) {
            copyToBuf(g.pathBuf, sizeof(g.pathBuf), node->path.toString());
            copyToBuf(g.textBuf, sizeof(g.textBuf), node->input.toString());
        }
    }
}

} // namespace

void renderLawGraphWindow(bool* open, LawManager& laws, Singular& player) {
    ImGui::SetNextWindowSize(ImVec2(860, 520), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Law Author", open)) {
        ImGui::End();
        return;
    }

    // ------------------------------------------------------------------
    // Left: the register of laws (and the concept registry below it).
    // ------------------------------------------------------------------
    ImGui::BeginChild("law-list", ImVec2(230, 0), true);
    if (ImGui::Button("+ New Law", ImVec2(-1, 0))) {
        auto law = laws.createLaw("New Law", {&player});
        // A working template to edit — the ground-rest rule.
        law->setConditionModel(ConditionNode::compare(
            "position.y", ConditionNode::Op::Lt, PropertyValue(0.0)));
        law->setActionModel(ActionNode::set("position.y", PropertyValue(0.0)));
        g.selectedLawId = law->getIdentifier();
        g.selectedCard = 0;
    }
    ImGui::Separator();
    for (const auto& law : laws.getAll()) {
        if (!law) continue;
        ImGui::PushID(law->getIdentifier().c_str());
        bool enabled = law->isEnabled();
        if (ImGui::Checkbox("##enabled", &enabled)) law->setEnabled(enabled);
        ImGui::SameLine();
        if (ImGui::Selectable(law->name().c_str(),
                              law->getIdentifier() == g.selectedLawId)) {
            g.selectedLawId = law->getIdentifier();
            g.selectedCard = 0;
        }
        ImGui::PopID();
    }
    ImGui::Separator();
    ImGui::TextDisabled("Concepts");
    for (const auto& concept : ConceptRegistry::instance().getAll()) {
        if (!concept) continue;
        ImGui::BulletText("%s (%d members, %d mappings)", concept->name().c_str(),
                          static_cast<int>(concept->members().size()),
                          static_cast<int>(concept->mappings().size()));
    }
    ImGui::EndChild();
    ImGui::SameLine();

    // ------------------------------------------------------------------
    // Right: card graph of the selected law + the node editor.
    // ------------------------------------------------------------------
    ImGui::BeginChild("law-detail", ImVec2(0, 0), false);
    Law* law = g.selectedLawId.empty() ? nullptr : laws.find(g.selectedLawId);
    if (!law) {
        ImGui::TextDisabled("Select or create a law.");
        ImGui::EndChild();
        ImGui::End();
        return;
    }

    const auto bindingIt = g.eventBindings.find(g.selectedLawId);
    const std::string binding =
        bindingIt != g.eventBindings.end() ? bindingIt->second : std::string();
    std::vector<LawCard> cards = flattenLaw(*law, binding);
    auto slots = layoutCardTree(
        static_cast<int>(cards.size()), 0,
        [&](int i) { return static_cast<int>(cards[i].children.size()); },
        [&](int i, int k) { return cards[i].children[k]; });

    const float colW = 200.0f, rowH = 46.0f, cardW = 184.0f, cardH = 36.0f;
    float maxRow = 0.0f, maxDepth = 0.0f;
    for (const auto& slot : slots) {
        maxRow = std::max(maxRow, slot.row);
        maxDepth = std::max(maxDepth, static_cast<float>(slot.depth));
    }
    const float canvasH = std::max(140.0f, (maxRow + 1.0f) * rowH + 20.0f);

    ImGui::BeginChild("canvas", ImVec2(0, canvasH), true,
                      ImGuiWindowFlags_HorizontalScrollbar);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    auto centerOf = [&](int i) {
        return ImVec2(origin.x + 14.0f + slots[i].depth * colW + cardW * 0.5f,
                      origin.y + 10.0f + slots[i].row * rowH + cardH * 0.5f);
    };
    for (std::size_t i = 0; i < cards.size(); ++i) {
        for (int child : cards[i].children) {
            const ImVec2 a = centerOf(static_cast<int>(i)), b = centerOf(child);
            dl->AddLine(ImVec2(a.x + cardW * 0.5f, a.y), ImVec2(b.x - cardW * 0.5f, b.y),
                        IM_COL32(170, 170, 170, 170), 1.5f);
        }
    }
    const ImVec2 mouse = ImGui::GetMousePos();
    for (std::size_t i = 0; i < cards.size(); ++i) {
        const ImVec2 c = centerOf(static_cast<int>(i));
        const ImVec2 a(c.x - cardW * 0.5f, c.y - cardH * 0.5f);
        const ImVec2 b(c.x + cardW * 0.5f, c.y + cardH * 0.5f);
        const bool hovered = ImGui::IsWindowHovered() &&
                             mouse.x >= a.x && mouse.x <= b.x &&
                             mouse.y >= a.y && mouse.y <= b.y;
        dl->AddRectFilled(a, b, cardColor(cards[i].kind), 6.0f);
        if (static_cast<int>(i) == g.selectedCard) {
            dl->AddRect(a, b, IM_COL32(255, 255, 255, 255), 6.0f, 0, 2.5f);
        } else if (hovered) {
            dl->AddRect(a, b, IM_COL32(255, 255, 255, 140), 6.0f);
        }
        dl->PushClipRect(a, b, true);
        dl->AddText(ImVec2(a.x + 8.0f, a.y + (cardH - ImGui::GetFontSize()) * 0.5f),
                    IM_COL32(255, 255, 255, 255), cards[i].label.c_str());
        dl->PopClipRect();
        if (hovered && ImGui::IsMouseClicked(0)) g.selectedCard = static_cast<int>(i);
    }
    ImGui::Dummy(ImVec2(28.0f + (maxDepth + 1.0f) * colW, canvasH - 20.0f));
    ImGui::EndChild();

    // ------------------------------------------------------------------
    // Editor for the selected card.
    // ------------------------------------------------------------------
    refreshEditBuffers(*law, cards, g.selectedCard);
    ImGui::Separator();
    if (g.selectedCard == 0) {
        char nameBuf[96];
        copyToBuf(nameBuf, sizeof(nameBuf), law->name());
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) law->setName(nameBuf);
        ImGui::Text("Authors: %d  |  Applications: %d",
                    static_cast<int>(law->authors().getMembers().size()),
                    static_cast<int>(law->applicationLog().size()));

        ImGui::InputText("Trigger event", g.eventBuf, sizeof(g.eventBuf));
        ImGui::SameLine();
        if (ImGui::Button("Bind") && g.eventBuf[0] != '\0') {
            const std::string type = g.eventBuf;
            const std::size_t alpha = laws.rete().addAlphaNode(
                "type == " + type,
                [type](const ReteFact& f) { return f.type == type; });
            laws.rete().bindLawToAlpha(law->getIdentifier(), alpha);
            g.eventBindings[g.selectedLawId] = type;
        }
        ImGui::TextDisabled("Bound laws fire when the event's subject satisfies the conditions.");
    } else if (g.selectedCard > 0 && g.selectedCard < static_cast<int>(cards.size())) {
        const LawCard& card = cards[g.selectedCard];
        if (card.kind == LawCard::Kind::Event) {
            ImGui::Text("Trigger: %s", card.label.c_str());
        } else if (card.kind == LawCard::Kind::Condition && law->hasConditionModel()) {
            ConditionModel model = *law->conditionModel();
            if (ConditionNode* node = conditionAt(model, card.modelPath)) {
                if (editConditionNode(*node)) law->setConditionModel(std::move(model));
            }
        } else if (card.kind == LawCard::Kind::Action && law->hasActionModel()) {
            ActionModel model = *law->actionModel();
            if (ActionNode* node = actionAt(model, card.modelPath)) {
                if (editActionNode(*node)) law->setActionModel(std::move(model));
            }
        }
    }

    ImGui::EndChild();
    ImGui::End();
}

} // namespace Rendering
