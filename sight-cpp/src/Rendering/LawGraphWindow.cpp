#include "Rendering/LawGraphWindow.hpp"

#include "Rendering/CardTreeLayout.hpp"
#include "Form/Object/Creation/ObjectConcept.hpp"
#include "Form/Object/Object.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace Rendering {

namespace {

// ---------------------------------------------------------------------------
// Session state — UI scratch only; the models remain the single truth.
// ---------------------------------------------------------------------------
struct SessionState {
    std::string selectedLawId;
    int selectedCard = -1;

    // lawId -> bound trigger event types (session view of the Rete bindings).
    std::unordered_map<std::string, std::vector<std::string>> triggers;
    int eventCombo = 0;
    char eventBuf[64] = "";

    char pathBuf[128] = "";
    std::string customPathTarget;    // which picker is in custom-entry mode
    char varBuf[32] = "";

    std::string lastEditLaw;
    int lastEditCard = -1;
};
SessionState g;

const ImVec4 kHeaderColor(0.95f, 0.85f, 0.55f, 1.0f);

ImU32 cardColor(LawCard::Kind kind) {
    switch (kind) {
        case LawCard::Kind::Law:       return IM_COL32(122, 92, 200, 255);
        case LawCard::Kind::Event:     return IM_COL32(196, 148, 44, 255);
        case LawCard::Kind::Condition: return IM_COL32(46, 128, 190, 255);
        case LawCard::Kind::Action:    return IM_COL32(52, 158, 92, 255);
    }
    return IM_COL32(96, 96, 96, 255);
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

// ---------------------------------------------------------------------------
// The substrate's own vocabulary, offered instead of quizzed.
// ---------------------------------------------------------------------------

// The engine's string-typed events (the echoes wired in commit 4), with what
// each one means. Custom names are equally legal — a Person may mint a new
// event kind and fire it from their own laws.
struct EventOption {
    const char* type;
    const char* meaning;
};
constexpr EventOption kEngineEvents[] = {
    {"collision",                "two objects collide (subject: the first object)"},
    {"object-hover-enter",       "the cursor enters an object (subject: the object)"},
    {"object-hover-exit",        "the cursor leaves an object (subject: the object)"},
    {"automation-clip-finished", "a non-looping automation clip ends (subject: the object)"},
    {"law-applied",              "any law successfully applies (subject: its target)"},
    {"law-registered",           "a new law is registered (subject: the law)"},
    {"concept-registered",       "a new concept is captured (subject: the concept)"},
};
constexpr int kEngineEventCount = sizeof(kEngineEvents) / sizeof(kEngineEvents[0]);

// Every property path the substrate actually registers, probed live from the
// registries and annotated with WHO owns it and WHAT it holds — the picker
// tells you which Singular a path belongs to instead of leaving you to guess.
struct PathOption {
    std::string path;
    const char* group;         // owning Singular + facet
    const char* type;          // what the property holds
    bool wholeVector = false;  // vec3 as a whole: numbers won't apply to it
};

const std::vector<PathOption>& knownPathOptions() {
    static std::vector<PathOption> options;
    if (options.empty()) {
        static Object objectPrototype;   // registry probes (window renders in-app)
        for (Property* property : objectPrototype.listProperties()) {
            const bool isVec = std::holds_alternative<glm::vec3>(property->value());
            const char* group = property->name().rfind("shape.", 0) == 0
                                    ? "Object — shape"
                                    : "Object — spatial";
            options.push_back({property->name(), group,
                               isVec ? "vector" : "number", isVec});
            if (isVec) {
                options.push_back({property->name() + ".x", group, "number", false});
                options.push_back({property->name() + ".y", group, "number", false});
                options.push_back({property->name() + ".z", group, "number", false});
            }
        }
        static Law lawPrototype("prototype");
        for (Property* property : lawPrototype.listProperties()) {
            const char* type = "number";
            if (std::holds_alternative<bool>(property->value())) type = "toggle";
            else if (std::holds_alternative<std::string>(property->value())) type = "text";
            options.push_back({property->name(), "Law — governance (metalaws)", type, false});
        }
    }
    return options;
}

const PathOption* findPathOption(const std::string& path) {
    for (const auto& option : knownPathOptions()) {
        if (option.path == path) return &option;
    }
    return nullptr;
}

// A property picker: grouped by owning Singular, typed, with a "..." custom
// escape hatch for paths beyond the known registries. Paths resolve on the
// law's SUBJECT — the being the law applies to when its trigger fires.
bool pathPicker(const char* label, PropertyPath& path) {
    bool changed = false;
    const std::string current = path.empty() ? "(choose property)" : path.toString();
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::BeginCombo(label, current.c_str())) {
        ImGui::TextDisabled("Resolved on the law's subject —");
        ImGui::TextDisabled("the being the law applies to:");
        const char* lastGroup = nullptr;
        for (const auto& option : knownPathOptions()) {
            if (!lastGroup || std::strcmp(lastGroup, option.group) != 0) {
                lastGroup = option.group;
                ImGui::Separator();
                ImGui::TextDisabled("%s", option.group);
            }
            if (ImGui::Selectable(("  " + option.path).c_str(), option.path == current)) {
                path = PropertyPath::parse(option.path);
                g.customPathTarget.clear();
                changed = true;
            }
            ImGui::SameLine(230.0f);
            ImGui::TextDisabled("%s", option.type);
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::PushID(label);
    if (ImGui::SmallButton("...")) {
        g.customPathTarget = label;
        copyToBuf(g.pathBuf, sizeof(g.pathBuf), path.toString());
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("type a custom path for properties of other Singulars");
    }
    ImGui::PopID();
    if (g.customPathTarget == label) {
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::InputText("custom path (enter)", g.pathBuf, sizeof(g.pathBuf),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            path = PropertyPath::parse(g.pathBuf);
            g.customPathTarget.clear();
            changed = true;
        }
    }
    return changed;
}

// Orange nudge when a whole-vector property is about to receive a number.
void warnIfWholeVector(const PropertyPath& path) {
    const PathOption* option = findPathOption(path.toString());
    if (option && option->wholeVector) {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                           "! \"%s\" is a whole vector — numbers won't apply. Pick a "
                           "component (.x .y .z).",
                           option->path.c_str());
    }
}

// ---------------------------------------------------------------------------
// Triggers: what wakes the law. Shown on the law card and the event card.
// ---------------------------------------------------------------------------
void editTriggers(LawManager& laws, Law& law) {
    auto& bound = g.triggers[law.getIdentifier()];

    ImGui::TextColored(kHeaderColor, "Triggers — what wakes this law");
    if (bound.empty()) {
        ImGui::TextDisabled("No trigger bound: the law only runs when applied directly.");
        ImGui::TextDisabled("Bind an event below so the law LISTENS for it.");
    }
    int removeIdx = -1;
    for (std::size_t i = 0; i < bound.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::BulletText("on \"%s\"", bound[i].c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("unbind")) removeIdx = static_cast<int>(i);
        ImGui::PopID();
    }
    if (removeIdx >= 0) {
        bound.erase(bound.begin() + removeIdx);
        // Rebind from scratch: drop every binding, re-create the remaining
        // ones on fresh alpha nodes (unbound nodes stay inert).
        laws.rete().unbindLaw(law.getIdentifier());
        for (const auto& type : bound) {
            const std::size_t alpha = laws.rete().addAlphaNode(
                "type == " + type,
                [type](const ReteFact& f) { return f.type == type; });
            laws.rete().bindLawToAlpha(law.getIdentifier(), alpha);
        }
    }

    // Add a trigger: pick an engine event (with its meaning shown), or the
    // last entry to name a custom event.
    const bool custom = g.eventCombo >= kEngineEventCount;
    const char* preview = custom ? "(custom event...)" : kEngineEvents[g.eventCombo].type;
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::BeginCombo("##eventpick", preview)) {
        for (int i = 0; i < kEngineEventCount; ++i) {
            if (ImGui::Selectable(kEngineEvents[i].type, g.eventCombo == i)) g.eventCombo = i;
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", kEngineEvents[i].meaning);
        }
        if (ImGui::Selectable("(custom event...)", custom)) g.eventCombo = kEngineEventCount;
        ImGui::EndCombo();
    }
    if (!custom) {
        ImGui::SameLine();
        ImGui::TextDisabled("?");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", kEngineEvents[g.eventCombo].meaning);
    }
    if (custom) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputText("##customevent", g.eventBuf, sizeof(g.eventBuf));
    }
    ImGui::SameLine();
    if (ImGui::Button("Bind trigger")) {
        const std::string type = custom ? std::string(g.eventBuf)
                                        : std::string(kEngineEvents[g.eventCombo].type);
        if (!type.empty() &&
            std::find(bound.begin(), bound.end(), type) == bound.end()) {
            const std::size_t alpha = laws.rete().addAlphaNode(
                "type == " + type,
                [type](const ReteFact& f) { return f.type == type; });
            laws.rete().bindLawToAlpha(law.getIdentifier(), alpha);
            bound.push_back(type);
            g.eventBuf[0] = '\0';
        }
    }
    ImGui::TextDisabled("When the event fires, the law applies to the event's SUBJECT");
    ImGui::TextDisabled("if the conditions hold.");
}

// ---------------------------------------------------------------------------
// Flatten helpers (public API used by tests — unchanged).
// ---------------------------------------------------------------------------
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
// Authored-mathematics editors: every foundational primitive — variables,
// bindings, coefficients, exponents, piece bounds — Person-modifiable in
// place, during creation and after.
// ---------------------------------------------------------------------------

bool editMathBindings(MathBindings& bindings) {
    bool changed = false;
    ImGui::TextColored(kHeaderColor, "Variables");
    ImGui::TextDisabled("Each variable names WHERE its value lives on the subject.");
    std::string removeKey;
    for (const auto& entry : bindings) {
        ImGui::PushID(entry.first.c_str());
        ImGui::BulletText("%s  <-  %s", entry.first.c_str(), entry.second.toString().c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("remove")) removeKey = entry.first;
        ImGui::PopID();
    }
    if (!removeKey.empty()) {
        bindings.erase(removeKey);
        changed = true;
    }
    ImGui::SetNextItemWidth(56.0f);
    ImGui::InputText("##bindvar", g.varBuf, sizeof(g.varBuf));
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("variable name, e.g. x");
    ImGui::SameLine();
    static PropertyPath pendingBindPath;
    pathPicker("##bindpath", pendingBindPath);
    ImGui::SameLine();
    if (ImGui::Button("Add variable") && g.varBuf[0] != '\0' && !pendingBindPath.empty()) {
        bindings[g.varBuf] = pendingBindPath;
        g.varBuf[0] = '\0';
        pendingBindPath = PropertyPath{};
        changed = true;
    }
    return changed;
}

bool editPiecewise(OntoMath::Piecewise& f, const MathBindings& bindings) {
    bool changed = false;
    ImGui::TextColored(kHeaderColor, "Function");
    ImGui::TextDisabled("f = %s", f.print().c_str());

    // A variable the function uses but nothing binds can never evaluate —
    // say so before the author wonders why the law never fires.
    for (const auto& piece : f.pieces) {
        for (const auto& term : piece.expression.terms) {
            for (const auto& factor : term.factors) {
                if (!bindings.count(factor.first)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                                       "! variable \"%s\" has no binding — the function "
                                       "cannot evaluate",
                                       factor.first.c_str());
                }
            }
        }
    }

    // Which variable the piece bounds cut (only matters with bounded pieces).
    bool anyBounded = f.pieces.size() > 1;
    for (const auto& piece : f.pieces) anyBounded = anyBounded || piece.hasLo || piece.hasHi;
    if (anyBounded && !bindings.empty()) {
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::BeginCombo("bounds cut", f.inputVariable.c_str())) {
            for (const auto& binding : bindings) {
                if (ImGui::Selectable(binding.first.c_str(),
                                      binding.first == f.inputVariable)) {
                    f.inputVariable = binding.first;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
    }

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

// ---------------------------------------------------------------------------
// Node editors. Each mutates a COPY of the model; the caller commits through
// setConditionModel/setActionModel so the law recompiles.
// ---------------------------------------------------------------------------

void seedConditionKind(ConditionNode& node) {
    switch (node.kind) {
        case ConditionNode::Kind::Compare:
            if (node.path.empty()) {
                node.path = PropertyPath::parse("position.y");
                node.op = ConditionNode::Op::Lt;
                node.operand = PropertyValue(0.0);
            }
            break;
        case ConditionNode::Kind::Zone:
            if (node.zoneFunction.pieces.empty()) {
                node.zoneFunction = OntoMath::Piecewise::continuous(
                    OntoMath::Expression::variable("x"));
                node.bindings = MathBindings{{"x", PropertyPath::parse("position.x")}};
                node.hi = PropertyValue(0.0);
            }
            break;
        case ConditionNode::Kind::InRegion:
            if (node.probe.empty()) node.probe = PropertyPath::parse("position");
            break;
        default:
            break;
    }
}

bool editConditionNode(ConditionNode& node) {
    bool changed = false;

    static const char* kinds[] = {"compare", "in shape region", "related (soon)",
                                  "all of...", "any of...", "not", "math zone"};
    int kind = static_cast<int>(node.kind);
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::Combo("Condition type", &kind, kinds, 7)) {
        node.kind = static_cast<ConditionNode::Kind>(kind);
        seedConditionKind(node);
        changed = true;
    }

    switch (node.kind) {
        case ConditionNode::Kind::Compare: {
            ImGui::TextDisabled("True when the property compares against the value.");
            if (pathPicker("Property", node.path)) changed = true;
            warnIfWholeVector(node.path);
            static const char* ops[] = {"==", "!=", "<", "<=", ">", ">=", "near", "in-range"};
            int op = static_cast<int>(node.op);
            ImGui::SetNextItemWidth(90.0f);
            if (ImGui::Combo("Compares", &op, ops, 8)) {
                node.op = static_cast<ConditionNode::Op>(op);
                changed = true;
            }
            double value = numericOr(node.operand, 0.0);
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::InputDouble("Value", &value)) {
                node.operand = PropertyValue(value);
                changed = true;
            }
            const bool comparesToPath = !node.operandPath.empty();
            if (!comparesToPath) {
                ImGui::SameLine();
                if (ImGui::SmallButton("compare to a property instead")) {
                    node.operandPath = PropertyPath::parse("position.y");
                    changed = true;
                }
            } else {
                if (pathPicker("Against", node.operandPath)) changed = true;
                ImGui::SameLine();
                if (ImGui::SmallButton("use a number instead")) {
                    node.operandPath = PropertyPath{};
                    changed = true;
                }
            }
            if (node.op == ConditionNode::Op::Near) {
                double tol = node.tolerance;
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputDouble("Tolerance", &tol)) {
                    node.tolerance = tol;
                    changed = true;
                }
            }
            if (node.op == ConditionNode::Op::InRange) {
                double lo = numericOr(node.lo, 0.0), hi = numericOr(node.hi, 0.0);
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputDouble("Low", &lo)) { node.lo = PropertyValue(lo); changed = true; }
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputDouble("High", &hi)) { node.hi = PropertyValue(hi); changed = true; }
            }
            break;
        }
        case ConditionNode::Kind::InRegion: {
            ImGui::TextDisabled("True when the probed point lies inside the shape region");
            ImGui::TextDisabled("(regions are sketched with the shape tools in projection mode).");
            if (pathPicker("Probe point", node.probe)) changed = true;
            if (node.region.op == geom::SdfOp::Leaf &&
                node.region.prim == geom::SdfPrim::Sphere) {
                float radius = node.region.dims.x;
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::DragFloat("Sphere radius", &radius, 0.02f, 0.01f, 100.0f)) {
                    node.region.dims.x = radius;
                    changed = true;
                }
            }
            break;
        }
        case ConditionNode::Kind::Related: {
            ImGui::TextDisabled("Relation conditions resolve when the relation-graph work lands;");
            ImGui::TextDisabled("until then this condition never passes (laws must not fire on");
            ImGui::TextDisabled("unproven relations).");
            break;
        }
        case ConditionNode::Kind::All:
        case ConditionNode::Kind::Any:
        case ConditionNode::Kind::Not: {
            const char* what = node.kind == ConditionNode::Kind::All
                                   ? "ALL members must hold"
                                   : node.kind == ConditionNode::Kind::Any
                                         ? "ANY one member suffices"
                                         : "inverts its single member";
            ImGui::TextDisabled("%s (%zu member(s)).", what, node.children.size());
            if (node.kind != ConditionNode::Kind::Not || node.children.empty()) {
                if (ImGui::Button("+ compare")) {
                    ConditionNode child;
                    child.kind = ConditionNode::Kind::Compare;
                    seedConditionKind(child);
                    node.children.push_back(std::move(child));
                    changed = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("+ math zone")) {
                    ConditionNode child;
                    child.kind = ConditionNode::Kind::Zone;
                    seedConditionKind(child);
                    node.children.push_back(std::move(child));
                    changed = true;
                }
            }
            break;
        }
        case ConditionNode::Kind::Zone: {
            ImGui::TextDisabled("True when f(variables) lies inside the zone bounds.");
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

void seedActionKind(ActionNode& node) {
    switch (node.kind) {
        case ActionNode::Kind::Set:
        case ActionNode::Kind::Add:
        case ActionNode::Kind::Scale:
        case ActionNode::Kind::Lerp:
        case ActionNode::Kind::Drive:
            if (node.path.empty()) node.path = PropertyPath::parse("position.y");
            break;
        case ActionNode::Kind::Map:
            if (node.mapFunction.pieces.empty()) {
                node.path = node.path.empty() ? PropertyPath::parse("position.y") : node.path;
                node.mapFunction = OntoMath::Piecewise::continuous(
                    OntoMath::Expression::variable("x"));
                node.bindings = MathBindings{{"x", PropertyPath::parse("position.x")}};
            }
            break;
        default:
            break;
    }
}

bool editActionNode(ActionNode& node) {
    bool changed = false;

    static const char* kinds[] = {"set", "add", "scale", "lerp", "drive (curve)",
                                  "sequence", "parallel", "spawn concept", "map (math)"};
    int kind = static_cast<int>(node.kind);
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::Combo("Action type", &kind, kinds, 9)) {
        node.kind = static_cast<ActionNode::Kind>(kind);
        seedActionKind(node);
        changed = true;
    }

    switch (node.kind) {
        case ActionNode::Kind::Set:
        case ActionNode::Kind::Add:
        case ActionNode::Kind::Scale:
        case ActionNode::Kind::Lerp: {
            const char* what = node.kind == ActionNode::Kind::Set   ? "property := value"
                               : node.kind == ActionNode::Kind::Add ? "property += value"
                               : node.kind == ActionNode::Kind::Scale
                                   ? "property *= value"
                                   : "property blends toward value";
            ImGui::TextDisabled("%s. Component paths (position.y) take numbers.", what);
            if (pathPicker("Property", node.path)) changed = true;
            warnIfWholeVector(node.path);
            double value = numericOr(node.operand, 0.0);
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::InputDouble("Value", &value)) {
                node.operand = PropertyValue(value);
                changed = true;
            }
            if (node.kind == ActionNode::Kind::Lerp) {
                double factor = node.factor;
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputDouble("Blend", &factor)) {
                    node.factor = factor;
                    changed = true;
                }
            }
            break;
        }
        case ActionNode::Kind::Drive: {
            ImGui::TextDisabled("property := curve(input) — a single-input curve.");
            ImGui::TextDisabled("For multivariate or piecewise math, use map.");
            if (pathPicker("Property", node.path)) changed = true;
            if (pathPicker("Input", node.input)) changed = true;
            static const char* forms[] = {"constant", "polynomial", "sinusoid"};
            int form = static_cast<int>(node.curve.form);
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::Combo("Curve", &form, forms, 3)) {
                node.curve.form = static_cast<CurveModel::Form>(form);
                changed = true;
            }
            if (node.curve.form == CurveModel::Form::Constant ||
                node.curve.form == CurveModel::Form::Polynomial) {
                if (node.curve.coeffs.size() < 2) node.curve.coeffs.resize(2, 0.0);
                double c0 = node.curve.coeffs[0], c1 = node.curve.coeffs[1];
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputDouble("c0", &c0)) { node.curve.coeffs[0] = c0; changed = true; }
                if (node.curve.form == CurveModel::Form::Polynomial) {
                    ImGui::SetNextItemWidth(120.0f);
                    if (ImGui::InputDouble("c1 (slope)", &c1)) {
                        node.curve.coeffs[1] = c1;
                        changed = true;
                    }
                }
            } else {
                double amp = node.curve.amplitude, freq = node.curve.frequency;
                double phase = node.curve.phase, bias = node.curve.bias;
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputDouble("Amplitude", &amp)) { node.curve.amplitude = amp; changed = true; }
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputDouble("Frequency", &freq)) { node.curve.frequency = freq; changed = true; }
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputDouble("Phase", &phase)) { node.curve.phase = phase; changed = true; }
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::InputDouble("Bias", &bias)) { node.curve.bias = bias; changed = true; }
            }
            break;
        }
        case ActionNode::Kind::Map: {
            ImGui::TextDisabled("property := f(variables) — authored mathematics governs");
            ImGui::TextDisabled("the output. Undefined math (outside the domain) writes nothing.");
            if (pathPicker("Property", node.path)) changed = true;
            warnIfWholeVector(node.path);
            if (editMathBindings(node.bindings)) changed = true;
            if (editPiecewise(node.mapFunction, node.bindings)) changed = true;
            break;
        }
        case ActionNode::Kind::Sequence:
        case ActionNode::Kind::Parallel: {
            ImGui::TextDisabled("Runs its %zu step(s) %s.", node.children.size(),
                                node.kind == ActionNode::Kind::Sequence ? "in order"
                                                                        : "together");
            if (ImGui::Button("+ set step")) {
                ActionNode child;
                child.kind = ActionNode::Kind::Set;
                seedActionKind(child);
                child.operand = PropertyValue(0.0);
                node.children.push_back(std::move(child));
                changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("+ map step")) {
                ActionNode child;
                child.kind = ActionNode::Kind::Map;
                seedActionKind(child);
                node.children.push_back(std::move(child));
                changed = true;
            }
            break;
        }
        case ActionNode::Kind::Spawn: {
            ImGui::TextDisabled("Births the concept's objects into the law's target World.");
            ImGui::TextDisabled("Bind the law to an event whose SUBJECT is the World.");
            const auto& concepts = ConceptRegistry::instance().getAll();
            const char* preview = node.conceptId.empty() ? "(choose concept)"
                                                         : node.conceptId.c_str();
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::BeginCombo("Concept", preview)) {
                if (concepts.empty()) {
                    ImGui::TextDisabled("No concepts captured yet.");
                }
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
            break;
        }
    }
    return changed;
}

// The whole law as one plain sentence — glanceable meaning, no card-reading
// required.
std::string lawSentence(const Law& law, const std::vector<std::string>* triggers) {
    std::string sentence = "WHEN ";
    if (!triggers || triggers->empty()) {
        sentence += "applied directly";
    } else {
        for (std::size_t i = 0; i < triggers->size(); ++i) {
            if (i) sentence += "\" or \"";
            else sentence += "\"";
            sentence += (*triggers)[i];
        }
        sentence += "\" fires";
    }
    sentence += "  ->  IF ";
    sentence += law.hasConditionModel() ? law.conditionModel()->describe()
                                        : std::string("always");
    sentence += "  ->  THEN ";
    sentence += law.hasActionModel() ? law.actionModel()->describe()
                                     : std::string("(no action)");
    sentence += "  — on the event's subject.";
    return sentence;
}

void refreshEditBuffers() {
    if (g.lastEditLaw == g.selectedLawId && g.lastEditCard == g.selectedCard) return;
    g.lastEditLaw = g.selectedLawId;
    g.lastEditCard = g.selectedCard;
    g.customPathTarget.clear();
    g.pathBuf[0] = '\0';
}

} // namespace

void renderLawGraphWindow(bool* open, LawManager& laws, Singular& player,
                          Singular* testSubject) {
    ImGui::SetNextWindowSize(ImVec2(920, 560), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Law Author", open)) {
        ImGui::End();
        return;
    }

    // ------------------------------------------------------------------
    // Left: the register of laws, and the concept registry.
    // ------------------------------------------------------------------
    ImGui::BeginChild("law-list", ImVec2(240, 0), true);
    if (ImGui::Button("+ New Law", ImVec2(-1, 0))) {
        auto law = laws.createLaw("New Law", {&player});
        law->setConditionModel(ConditionNode::compare(
            "position.y", ConditionNode::Op::Lt, PropertyValue(0.0)));
        law->setActionModel(ActionNode::set("position.y", PropertyValue(0.0)));
        g.selectedLawId = law->getIdentifier();
        g.selectedCard = 0;
    }
    ImGui::TextDisabled("A law: WHEN an event fires,");
    ImGui::TextDisabled("IF conditions hold, THEN act.");
    ImGui::Separator();
    for (const auto& law : laws.getAll()) {
        if (!law) continue;
        ImGui::PushID(law->getIdentifier().c_str());
        bool enabled = law->isEnabled();
        if (ImGui::Checkbox("##enabled", &enabled)) law->setEnabled(enabled);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("enabled");
        ImGui::SameLine();
        if (ImGui::Selectable(law->name().c_str(),
                              law->getIdentifier() == g.selectedLawId)) {
            g.selectedLawId = law->getIdentifier();
            g.selectedCard = 0;
        }
        ImGui::PopID();
    }
    ImGui::Separator();
    ImGui::TextDisabled("Concepts (captured sets)");
    for (const auto& concept : ConceptRegistry::instance().getAll()) {
        if (!concept) continue;
        ImGui::BulletText("%s (%d members)", concept->name().c_str(),
                          static_cast<int>(concept->members().size()));
    }
    ImGui::EndChild();
    ImGui::SameLine();

    // ------------------------------------------------------------------
    // Right: card graph of the selected law + the editor for the
    // selected card.
    // ------------------------------------------------------------------
    ImGui::BeginChild("law-detail", ImVec2(0, 0), false);
    Law* law = g.selectedLawId.empty() ? nullptr : laws.find(g.selectedLawId);
    if (!law) {
        ImGui::TextDisabled("Select a law on the left, or create one.");
        ImGui::EndChild();
        ImGui::End();
        return;
    }

    std::string binding;
    auto triggersIt = g.triggers.find(g.selectedLawId);
    if (triggersIt != g.triggers.end()) {
        for (std::size_t i = 0; i < triggersIt->second.size(); ++i) {
            if (i) binding += ", ";
            binding += triggersIt->second[i];
        }
    }
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
        // WHEN/IF/THEN orientation badges on the roots of each branch.
        const char* badge = nullptr;
        if (cards[i].kind == LawCard::Kind::Event) badge = "WHEN";
        else if (cards[i].kind == LawCard::Kind::Condition && cards[i].modelPath.empty()) badge = "IF";
        else if (cards[i].kind == LawCard::Kind::Action && cards[i].modelPath.empty()) badge = "THEN";
        if (badge) {
            dl->AddText(ImVec2(a.x + 2.0f, a.y - ImGui::GetFontSize() - 1.0f),
                        IM_COL32(210, 210, 210, 200), badge);
        }
        if (hovered) {
            ImGui::SetTooltip("%s", cards[i].label.c_str());
            if (ImGui::IsMouseClicked(0)) g.selectedCard = static_cast<int>(i);
        }
    }
    ImGui::Dummy(ImVec2(28.0f + (maxDepth + 1.0f) * colW, canvasH - 20.0f));
    ImGui::EndChild();
    ImGui::TextDisabled("Click a card to edit it below.");

    // ------------------------------------------------------------------
    // Editor for the selected card.
    // ------------------------------------------------------------------
    refreshEditBuffers();
    ImGui::Separator();
    if (g.selectedCard < 0 || g.selectedCard >= static_cast<int>(cards.size())) {
        g.selectedCard = 0;
    }
    const LawCard& card = cards[g.selectedCard];
    ImGui::TextColored(kHeaderColor, "Editing: %s", card.label.c_str());

    if (g.selectedCard == 0) {
        char nameBuf[96];
        copyToBuf(nameBuf, sizeof(nameBuf), law->name());
        ImGui::SetNextItemWidth(240.0f);
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) law->setName(nameBuf);

        // The law in one sentence.
        {
            auto boundIt = g.triggers.find(g.selectedLawId);
            const std::vector<std::string>* bound =
                boundIt != g.triggers.end() ? &boundIt->second : nullptr;
            ImGui::TextWrapped("%s", lawSentence(*law, bound).c_str());
        }

        // Authored by whom.
        {
            std::string authors;
            int count = 0;
            for (auto* author : law->authors().getMembers()) {
                if (!author) continue;
                if (count++) authors += ", ";
                if (count > 3) { authors += "..."; break; }
                authors += author->getIdentifier();
            }
            ImGui::TextDisabled("Authored by: %s",
                                authors.empty() ? "(nobody — cannot fire)" : authors.c_str());
        }

        editTriggers(laws, *law);
        ImGui::Separator();

        // Feedback: what this law has actually done. Authoring without
        // feedback is guessing.
        ImGui::TextColored(kHeaderColor, "Recent applications");
        const auto& log = law->applicationLog();
        if (log.empty()) {
            ImGui::TextDisabled("Never applied yet.");
        } else {
            const std::size_t shown = std::min<std::size_t>(3, log.size());
            for (std::size_t i = 0; i < shown; ++i) {
                const auto& record = log[log.size() - 1 - i];
                ImGui::BulletText("%s -> %s", Law::resultName(record.result),
                                  record.targetId.empty() ? "(no target)"
                                                          : record.targetId.c_str());
            }
        }
        if (testSubject) {
            if (ImGui::Button("Apply now to selected object")) {
                law->applyTo(*testSubject);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("subject: %s", testSubject->getIdentifier().c_str());
        } else {
            ImGui::TextDisabled("Select an object in the 3D world to test-apply this law.");
        }
        ImGui::Separator();

        if (!law->hasConditionModel()) {
            if (ImGui::Button("+ Add condition")) {
                ConditionNode seed;
                seed.kind = ConditionNode::Kind::Compare;
                seedConditionKind(seed);
                law->setConditionModel(std::move(seed));
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(no condition: the law fires on every trigger)");
        }
        if (!law->hasActionModel()) {
            if (ImGui::Button("+ Add action")) {
                ActionNode seed;
                seed.kind = ActionNode::Kind::Set;
                seedActionKind(seed);
                seed.operand = PropertyValue(0.0);
                law->setActionModel(std::move(seed));
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(no action: the law recognizes but does not act)");
        }

        if (ImGui::Button("Duplicate law")) {
            nlohmann::json copy = law->toJson();
            copy.erase("id");   // a duplicate is a NEW being, not an alias
            auto twin = Law::fromJson(copy);
            twin->setName(law->name() + " (copy)");
            for (auto* author : law->authors().getMembers()) {
                if (author) twin->addAuthor(*author);
            }
            laws.add(twin);
            g.selectedLawId = twin->getIdentifier();
            g.selectedCard = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete law")) {
            laws.rete().unbindLaw(law->getIdentifier());
            g.triggers.erase(law->getIdentifier());
            laws.remove(law->getIdentifier());
            g.selectedLawId.clear();
            g.selectedCard = -1;
        }
    } else if (card.kind == LawCard::Kind::Event) {
        editTriggers(laws, *law);
    } else if (card.kind == LawCard::Kind::Condition && law->hasConditionModel()) {
        ConditionModel model = *law->conditionModel();
        if (ConditionNode* node = conditionAt(model, card.modelPath)) {
            if (editConditionNode(*node)) law->setConditionModel(std::move(model));
        }
        if (!card.modelPath.empty()) {
            ConditionModel again = *law->conditionModel();
            std::vector<int> parentPath(card.modelPath.begin(), card.modelPath.end() - 1);
            if (ConditionNode* parent = conditionAt(again, parentPath)) {
                ImGui::Separator();
                if (ImGui::Button("Delete this condition")) {
                    parent->children.erase(parent->children.begin() + card.modelPath.back());
                    law->setConditionModel(std::move(again));
                    g.selectedCard = 0;
                }
            }
        } else {
            ImGui::Separator();
            if (ImGui::Button("Remove condition (law fires on every trigger)")) {
                law->clearConditionModel();
                g.selectedCard = 0;
            }
        }
    } else if (card.kind == LawCard::Kind::Action && law->hasActionModel()) {
        ActionModel model = *law->actionModel();
        if (ActionNode* node = actionAt(model, card.modelPath)) {
            if (editActionNode(*node)) law->setActionModel(std::move(model));
        }
        if (!card.modelPath.empty()) {
            ActionModel again = *law->actionModel();
            std::vector<int> parentPath(card.modelPath.begin(), card.modelPath.end() - 1);
            if (ActionNode* parent = actionAt(again, parentPath)) {
                ImGui::Separator();
                if (ImGui::Button("Delete this step")) {
                    parent->children.erase(parent->children.begin() + card.modelPath.back());
                    law->setActionModel(std::move(again));
                    g.selectedCard = 0;
                }
            }
        } else {
            ImGui::Separator();
            if (ImGui::Button("Remove action (law recognizes but does not act)")) {
                law->clearActionModel();
                g.selectedCard = 0;
            }
        }
    }

    ImGui::Separator();
    ImGui::TextDisabled("Laws live in this session; saving them with the world is coming.");
    ImGui::EndChild();
    ImGui::End();
}

} // namespace Rendering
