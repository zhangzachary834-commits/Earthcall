#include "Rendering/LawGraphWindow.hpp"

#include "Rendering/CardTreeLayout.hpp"
#include "Rendering/MathEditors.hpp"
#include "Form/Object/Creation/ObjectConcept.hpp"
#include "Form/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
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

    int eventCombo = 0;
    char eventBuf[64] = "";
    char filterBuf[48] = "";

    char pathBuf[128] = "";
    std::string customPathTarget;    // which picker is in custom-entry mode
    char varBuf[32] = "";
    char textBuf[128] = "";          // free-text scratch (exception ids, etc.)

    std::string selectedSubjectId;   // the 3D selection, for path qualifying
    Singular* testSubject = nullptr; // same selection, for live math readouts
                                     // (valid for this frame only)

    // Live event feed: what the bus is actually saying — answers "did my
    // trigger ever fire?" without guessing.
    struct FeedEntry {
        std::string type;
        std::string subjectId;
        int count = 1;               // consecutive repeats collapse
    };
    std::vector<FeedEntry> eventFeed;
    bool feedSubscribed = false;

    std::string lastEditLaw;
    int lastEditCard = -1;
};
SessionState g;

void subscribeEventFeed() {
    if (g.feedSubscribed) return;
    g.feedSubscribed = true;
    Core::EventBus::instance().subscribe<ECA::Event>([](const ECA::Event& e) {
        const std::string subjectId = e.subject ? e.subject->getIdentifier() : "";
        if (!g.eventFeed.empty() && g.eventFeed.front().type == e.type &&
            g.eventFeed.front().subjectId == subjectId) {
            ++g.eventFeed.front().count;
            return;
        }
        g.eventFeed.insert(g.eventFeed.begin(), {e.type, subjectId, 1});
        if (g.eventFeed.size() > 8) g.eventFeed.pop_back();
    });
}

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
    {"law-drive-finished",       "a law's change-over-time reaches the end of its authored "
                                 "bounds (subject: the driven being)"},
    {"relation-formed",          "a new relation joins the world's graph (subject: the "
                                 "relation being)"},
    {"concept-instantiated",     "a concept manifests new beings (subject: the concept)"},
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
                                : property->name().rfind("face.", 0) == 0
                                    ? "Object — surface (faces)"
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
        // The world clock — Singularity owns time. Read-only; bind these as
        // math variables to author change OVER TIME (position := f(t)).
        options.push_back({"time", "Time — Universe (read-only)", "seconds", false});
        options.push_back({"time.delta", "Time — Universe (read-only)", "seconds", false});
        options.push_back({"time.sinceApplied", "Time — Universe (read-only)", "seconds", false});
    }
    return options;
}

const PathOption* findPathOption(const std::string& path) {
    for (const auto& option : knownPathOptions()) {
        if (option.path == path) return &option;
    }
    return nullptr;
}

// The author's choice of REFERENT: whose property does this path name?
// Reads/writes the path's qualifier — plain (the law's subject),
// "@event.subject" / "@event.object" (the triggering event's participants),
// or "@being-id" (one specific being in the world, listed live).
bool whosePicker(PropertyPath& path) {
    bool changed = false;
    std::string qualifier;
    int strip = 0;   // qualifier segments to replace
    if (!path.segments.empty() && !path.segments[0].empty() &&
        path.segments[0][0] == '@') {
        if (path.segments[0] == "@event" && path.segments.size() >= 2) {
            qualifier = "@event." + path.segments[1];
            strip = 2;
        } else {
            qualifier = path.segments[0];
            strip = 1;
        }
    }
    const auto retarget = [&](const std::vector<std::string>& prefix) {
        path.segments.erase(path.segments.begin(), path.segments.begin() + strip);
        path.segments.insert(path.segments.begin(), prefix.begin(), prefix.end());
        changed = true;
    };

    ImGui::SameLine();
    ImGui::SetNextItemWidth(170.0f);
    const std::string preview = qualifier.empty() ? "of the subject" : "of " + qualifier;
    if (ImGui::BeginCombo("##whose", preview.c_str())) {
        if (ImGui::Selectable("the law's subject", qualifier.empty())) retarget({});
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("whoever this law applies to (the event's subject,\n"
                              "a watched being, or each being of an Everyone sweep)");
        }
        if (ImGui::Selectable("the event's subject", qualifier == "@event.subject")) {
            retarget({"@event", "subject"});
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("the being the triggering event was about —\n"
                              "even when the law applies to someone else");
        }
        if (ImGui::Selectable("the event's other object", qualifier == "@event.object")) {
            retarget({"@event", "object"});
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("the event's second participant (a collision has two);\n"
                              "undefined for events without one");
        }
        ImGui::Separator();
        ImGui::TextDisabled("a specific being in the world:");
        for (Singular* being : Universe::instance().beings()) {
            if (!being) continue;
            const std::string id = "@" + being->getIdentifier();
            if (ImGui::Selectable(id.c_str(), qualifier == id)) retarget({id});
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("WHOSE property this is — the author's choice of referent");
    }
    return changed;
}

// A property picker: grouped by owning Singular, typed, with a "..." custom
// escape hatch for paths beyond the known registries. Beside it, the
// "whose" combo chooses the REFERENT the path resolves on.
bool pathPicker(const char* label, PropertyPath& path) {
    bool changed = false;
    const std::string current = path.empty() ? "(choose property)" : path.toString();
    // The referent qualifier survives re-picking WHAT the property is.
    std::vector<std::string> qualifierPrefix;
    if (!path.segments.empty() && !path.segments[0].empty() &&
        path.segments[0][0] == '@') {
        const std::size_t n =
            (path.segments[0] == "@event" && path.segments.size() >= 2) ? 2 : 1;
        qualifierPrefix.assign(path.segments.begin(), path.segments.begin() + n);
    }
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::BeginCombo(label, current.c_str())) {
        ImGui::TextDisabled("WHAT the property is; choose WHOSE");
        ImGui::TextDisabled("it is in the 'of ...' box beside this:");
        const char* lastGroup = nullptr;
        for (const auto& option : knownPathOptions()) {
            if (!lastGroup || std::strcmp(lastGroup, option.group) != 0) {
                lastGroup = option.group;
                ImGui::Separator();
                ImGui::TextDisabled("%s", option.group);
            }
            if (ImGui::Selectable(("  " + option.path).c_str(), option.path == current)) {
                path = PropertyPath::parse(option.path);
                path.segments.insert(path.segments.begin(), qualifierPrefix.begin(),
                                     qualifierPrefix.end());
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
    // WHOSE property: subject / event participants / a specific being.
    if (!path.empty()) {
        if (whosePicker(path)) changed = true;
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
    // The manager owns the trigger truth (serialized with the world);
    // the window merely edits it.
    const auto& bound = laws.triggersOf(law.getIdentifier());

    ImGui::TextColored(kHeaderColor, "Triggers — what wakes this law");
    if (bound.empty()) {
        ImGui::TextDisabled("No trigger bound: the law only runs when applied directly.");
        ImGui::TextDisabled("Bind an event below so the law LISTENS for it.");
    }
    std::string removeType;
    for (std::size_t i = 0; i < bound.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::BulletText("on \"%s\"", bound[i].c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("unbind")) removeType = bound[i];
        ImGui::PopID();
    }
    if (!removeType.empty()) {
        laws.unbindTrigger(law.getIdentifier(), removeType);
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
        if (!type.empty()) {
            laws.bindTrigger(law.getIdentifier(), type);
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

// The shared MathEd editors, offered THIS window's property vocabulary.
bool editMathBindings(MathBindings& bindings) {
    return MathEd::editMathBindings(
        bindings, [](const char* label, PropertyPath& path) {
            return pathPicker(label, path);
        });
}

bool editPiecewise(OntoMath::Piecewise& f, const MathBindings& bindings) {
    return MathEd::editPiecewise(f, bindings);
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
        case ConditionNode::Kind::Overlaps:
            if (node.otherId.empty()) node.otherId = "@event.object";
            break;
        case ConditionNode::Kind::IsKind:
        case ConditionNode::Kind::ForAny:
        case ConditionNode::Kind::ForAll:
            if (node.beingKind == ConditionNode::BeingKind::AnyBeing &&
                node.kind != ConditionNode::Kind::IsKind) {
                node.beingKind = ConditionNode::BeingKind::Object;
            }
            if ((node.kind == ConditionNode::Kind::ForAny ||
                 node.kind == ConditionNode::Kind::ForAll) &&
                node.children.empty()) {
                ConditionNode inner;
                inner.kind = ConditionNode::Kind::Compare;
                seedConditionKind(inner);
                node.children.push_back(std::move(inner));
            }
            break;
        default:
            break;
    }
}

const char* kBeingKindNames[] = {"any being", "Object", "Person", "Relation",
                                 "Formation", "Law", "World"};

bool beingKindCombo(ConditionNode& node) {
    int kind = static_cast<int>(node.beingKind);
    ImGui::SetNextItemWidth(130.0f);
    if (ImGui::Combo("Being kind", &kind, kBeingKindNames, 7)) {
        node.beingKind = static_cast<ConditionNode::BeingKind>(kind);
        return true;
    }
    return false;
}

bool editConditionNode(ConditionNode& node) {
    bool changed = false;

    static const char* kinds[] = {"compare", "in shape region", "related to...",
                                  "all of... (&&)", "any of... (||)", "not (!)",
                                  "math zone", "is a (type)", "this specific being",
                                  "for ANY being...", "for ALL beings...",
                                  "overlaps (touching)"};
    int kind = static_cast<int>(node.kind);
    ImGui::SetNextItemWidth(170.0f);
    if (ImGui::Combo("Condition type", &kind, kinds, 12)) {
        node.kind = static_cast<ConditionNode::Kind>(kind);
        seedConditionKind(node);
        changed = true;
    }

    switch (node.kind) {
        case ConditionNode::Kind::Compare: {
            ImGui::TextDisabled("True when the property compares against the value.");
            ImGui::TextDisabled("For full authored mathematics (multivariate, piecewise,");
            ImGui::TextDisabled("exact functions) switch Condition type to \"math zone\".");
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
        case ConditionNode::Kind::Overlaps: {
            ImGui::TextDisabled("True while the subject GEOMETRICALLY TOUCHES the other —");
            ImGui::TextDisabled("the engine's collision test, as an ordinary condition.");
            ImGui::TextDisabled("Perception is authorable: pair with a 'publish event'");
            ImGui::TextDisabled("action and you have written a perception law.");
            const char* preview = node.otherId.empty() ? "(choose the other)"
                                                       : node.otherId.c_str();
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::BeginCombo("Touching", preview)) {
                if (ImGui::Selectable("the event's subject",
                                      node.otherId == "@event.subject")) {
                    node.otherId = "@event.subject";
                    changed = true;
                }
                if (ImGui::Selectable("the event's other object",
                                      node.otherId == "@event.object")) {
                    node.otherId = "@event.object";
                    changed = true;
                }
                for (Singular* being : Universe::instance().beings()) {
                    if (!being) continue;
                    const std::string id = being->getIdentifier();
                    if (ImGui::Selectable(id.c_str(), node.otherId == id)) {
                        node.otherId = id;
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }
            break;
        }
        case ConditionNode::Kind::Related: {
            ImGui::TextDisabled("True when the subject participates in a relation from the");
            ImGui::TextDisabled("world's relation graph. Directed relations satisfy only");
            ImGui::TextDisabled("their source (\"a owns b\" holds OF a, not of b).");
            char typeBuf[64];
            copyToBuf(typeBuf, sizeof(typeBuf), node.relationType);
            ImGui::SetNextItemWidth(150.0f);
            if (ImGui::InputText("Relation type", typeBuf, sizeof(typeBuf))) {
                node.relationType = typeBuf;
                changed = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("free-form: \"attachment\", \"friend\", \"owns\"...\n"
                                  "leave empty to accept ANY relation kind");
            }
            const char* preview =
                node.otherId.empty() ? "(anyone)" : node.otherId.c_str();
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::BeginCombo("Related to", preview)) {
                if (ImGui::Selectable("(anyone)", node.otherId.empty())) {
                    node.otherId.clear();
                    changed = true;
                }
                if (ImGui::Selectable("the event's subject",
                                      node.otherId == "@event.subject")) {
                    node.otherId = "@event.subject";
                    changed = true;
                }
                if (ImGui::Selectable("the event's other object",
                                      node.otherId == "@event.object")) {
                    node.otherId = "@event.object";
                    changed = true;
                }
                for (Singular* being : Universe::instance().beings()) {
                    if (!being) continue;
                    const std::string id = being->getIdentifier();
                    if (ImGui::Selectable(id.c_str(), node.otherId == id)) {
                        node.otherId = id;
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }
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
        case ConditionNode::Kind::IsKind: {
            ImGui::TextDisabled("True when the subject IS this kind of being");
            ImGui::TextDisabled("(runtime type check; a Law is also an Object).");
            if (beingKindCombo(node)) changed = true;
            break;
        }
        case ConditionNode::Kind::Identity: {
            ImGui::TextDisabled("True only for one specific being, by identity.");
            char idBuf[96];
            copyToBuf(idBuf, sizeof(idBuf), node.otherId);
            ImGui::SetNextItemWidth(180.0f);
            if (ImGui::InputText("Being id", idBuf, sizeof(idBuf))) {
                node.otherId = idBuf;
                changed = true;
            }
            break;
        }
        case ConditionNode::Kind::ForAny:
        case ConditionNode::Kind::ForAll: {
            ImGui::TextDisabled(node.kind == ConditionNode::Kind::ForAny
                                    ? "True when ANY being of the kind passes the inner test"
                                    : "True when ALL beings of the kind pass the inner test");
            ImGui::TextDisabled("(quantifies over the Universe; the inner test's subject");
            ImGui::TextDisabled("is each INSTANCE, not the law's subject).");
            if (beingKindCombo(node)) changed = true;
            // Exceptions: "every instance ... with possible exceptions".
            int removeIdx = -1;
            for (std::size_t i = 0; i < node.exceptIds.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));
                ImGui::BulletText("except %s", node.exceptIds[i].c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("x")) removeIdx = static_cast<int>(i);
                ImGui::PopID();
            }
            if (removeIdx >= 0) {
                node.exceptIds.erase(node.exceptIds.begin() + removeIdx);
                changed = true;
            }
            ImGui::SetNextItemWidth(150.0f);
            ImGui::InputText("##exceptid", g.textBuf, sizeof(g.textBuf));
            ImGui::SameLine();
            if (ImGui::Button("Add exception") && g.textBuf[0] != '\0') {
                node.exceptIds.emplace_back(g.textBuf);
                g.textBuf[0] = '\0';
                changed = true;
            }
            ImGui::TextDisabled("The child card is the inner test each instance must pass.");
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
        case ActionNode::Kind::Publish:
            if (node.eventType.empty()) node.eventType = "custom-signal";
            break;
        case ActionNode::Kind::Flow:
            // Rate of change: seed t -> the change-over-time clock, so the
            // authored f(t) is dp/dt from the moment the law takes hold.
            if (node.mapFunction.pieces.empty()) {
                node.path = node.path.empty() ? PropertyPath::parse("position.y") : node.path;
                node.mapFunction = OntoMath::Piecewise::continuous(
                    OntoMath::Expression::constant(1.0));
                node.mapFunction.inputVariable = "t";
                node.bindings = MathBindings{{"t", PropertyPath::parse("time.sinceApplied")}};
            }
            break;
        default:
            break;
    }
}

bool editActionNode(ActionNode& node) {
    bool changed = false;

    static const char* kinds[] = {"set", "add", "scale", "lerp", "drive (curve)",
                                  "sequence", "parallel", "spawn concept", "map (math)",
                                  "flow (rate of change)", "publish event"};
    int kind = static_cast<int>(node.kind);
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::Combo("Action type", &kind, kinds, 11)) {
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
            ImGui::TextDisabled("For authored functions (OntoMath: multivariate, piecewise,");
            ImGui::TextDisabled("calculus-exact) switch Action type to \"map\" or \"flow\".");
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
        case ActionNode::Kind::Map:
        case ActionNode::Kind::Flow: {
            if (node.kind == ActionNode::Kind::Map) {
                ImGui::TextDisabled("property := f(variables) — authored mathematics governs");
                ImGui::TextDisabled("the output. Undefined math (outside the domain) writes nothing.");
            } else {
                ImGui::TextDisabled("property += f(variables) x dt each tick — f is the RATE");
                ImGui::TextDisabled("of change (dp/dt). Undefined math flows nothing.");
            }
            if (pathPicker("Property", node.path)) changed = true;
            warnIfWholeVector(node.path);
            if (editMathBindings(node.bindings)) changed = true;
            if (editPiecewise(node.mapFunction, node.bindings)) changed = true;
            // Context everywhere: what f evaluates to RIGHT NOW against the
            // selected object — undefined shown honestly.
            if (g.testSubject) {
                auto vars = readMathBindings(*g.testSubject, node.bindings);
                const auto value = vars ? node.mapFunction.evaluate(*vars)
                                        : std::optional<double>{};
                if (value) {
                    ImGui::TextDisabled("f = %.4f right now (on %s)", *value,
                                        g.selectedSubjectId.c_str());
                } else {
                    ImGui::TextDisabled("f = undefined right now (on %s)",
                                        g.selectedSubjectId.c_str());
                }
            }
            break;
        }
        case ActionNode::Kind::Publish: {
            ImGui::TextDisabled("MINT an event: the law authors vocabulary instead of");
            ImGui::TextDisabled("only consuming it. Other laws can bind this as their");
            ImGui::TextDisabled("trigger. Cascades stay under the anti-Babel ceiling.");
            char typeBuf[64];
            copyToBuf(typeBuf, sizeof(typeBuf), node.eventType);
            ImGui::SetNextItemWidth(180.0f);
            if (ImGui::InputText("Event type", typeBuf, sizeof(typeBuf))) {
                node.eventType = typeBuf;
                changed = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("any name — new event kinds are minted by naming them");
            }
            const auto participantCombo = [&](const char* label, std::string& token,
                                              bool allowLawSubject) {
                const char* preview =
                    token.empty() ? (allowLawSubject ? "the law's subject" : "(none)")
                                  : token.c_str();
                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::BeginCombo(label, preview)) {
                    if (ImGui::Selectable(allowLawSubject ? "the law's subject"
                                                          : "(none)",
                                          token.empty())) {
                        token.clear();
                        changed = true;
                    }
                    if (ImGui::Selectable("the event's subject",
                                          token == "@event.subject")) {
                        token = "@event.subject";
                        changed = true;
                    }
                    if (ImGui::Selectable("the event's other object",
                                          token == "@event.object")) {
                        token = "@event.object";
                        changed = true;
                    }
                    for (Singular* being : Universe::instance().beings()) {
                        if (!being) continue;
                        const std::string id = being->getIdentifier();
                        if (ImGui::Selectable(id.c_str(), token == id)) {
                            token = id;
                            changed = true;
                        }
                    }
                    ImGui::EndCombo();
                }
            };
            participantCombo("Its subject", node.publishSubject, true);
            participantCombo("Its object", node.publishObject, false);
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
    std::string sentence;
    if (law.activation() == Law::Activation::WhileTrue) {
        sentence = "EVERY FRAME, watching its subjects";
    } else if (law.activation() == Law::Activation::OnBecomeTrue) {
        sentence = "WHENEVER the condition STARTS holding on a watched subject";
    } else if (!triggers || triggers->empty()) {
        sentence = "WHEN applied directly";
    } else {
        sentence = "WHEN ";
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
    if (law.activation() == Law::Activation::OnEvent &&
        law.scope() == Law::Scope::Everyone) {
        sentence += "  — on EVERY being satisfying the IF.";
    } else if (law.activation() == Law::Activation::OnEvent) {
        sentence += "  — on the event's subject.";
    } else {
        sentence += "  — on each watched being.";
    }
    if (law.drives() && law.activation() != Law::Activation::WhileTrue) {
        sentence += "  KEEPS APPLYING until its function's bounds end.";
    }
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
    // A touch of finish: rounded frames throughout the window. RAII so the
    // pops survive every early return.
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    struct StyleScope {
        ~StyleScope() { ImGui::PopStyleVar(2); }
    } styleScope;

    subscribeEventFeed();
    g.selectedSubjectId = testSubject ? testSubject->getIdentifier() : std::string();
    g.testSubject = testSubject;

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
        ImGui::OpenPopup("new-law-templates");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("start from a template — every part stays editable");
    }
    if (ImGui::BeginPopup("new-law-templates")) {
        const auto select = [&](const std::shared_ptr<Law>& law) {
            g.selectedLawId = law->getIdentifier();
            g.selectedCard = 0;
        };
        if (ImGui::MenuItem("Blank — shape WHEN/IF/THEN yourself")) {
            auto law = laws.createLaw("New Law", {&player});
            law->setConditionModel(ConditionNode::compare(
                "position.y", ConditionNode::Op::Lt, PropertyValue(0.0)));
            law->setActionModel(ActionNode::set("position.y", PropertyValue(0.0)));
            select(law);
        }
        if (ImGui::MenuItem("On collision -> act")) {
            auto law = laws.createLaw("On collision", {&player});
            law->setActionModel(ActionNode::set("position.y", PropertyValue(2.0)));
            laws.bindTrigger(law->getIdentifier(), "collision");
            select(law);
        }
        if (ImGui::MenuItem("While a condition holds -> keep acting")) {
            auto law = laws.createLaw("While watching", {&player});
            law->setActivation(Law::Activation::WhileTrue);
            law->setConditionModel(ConditionNode::compare(
                "position.y", ConditionNode::Op::Lt, PropertyValue(0.0)));
            law->setActionModel(ActionNode::set("position.y", PropertyValue(0.0)));
            select(law);
        }
        if (ImGui::MenuItem("On event -> change over time (drive)")) {
            auto law = laws.createLaw("Drive over time", {&player});
            law->setDrives(true);
            OntoMath::Piecewise arc;              // y := 2t for t in [0, 2]
            arc.inputVariable = "t";
            OntoMath::Piecewise::Piece piece;
            piece.hasLo = true;
            piece.lo = 0.0;
            piece.hasHi = true;
            piece.hi = 2.0;
            piece.expression = OntoMath::Expression::variable("t", 1.0, 2.0);
            arc.pieces.push_back(std::move(piece));
            law->setActionModel(ActionNode::map(
                "position.y", std::move(arc),
                MathBindings{{"t", PropertyPath::parse("time.sinceApplied")}}));
            laws.bindTrigger(law->getIdentifier(), "collision");
            select(law);
        }
        if (ImGui::MenuItem("Perception law — announce contact (mints an event)")) {
            auto law = laws.createLaw("Perceive contact", {&player});
            law->setActivation(Law::Activation::WhileTrue);
            law->setConditionModel(ConditionNode::overlaps(""));   // pick the other
            law->setActionModel(ActionNode::publish("contact-perceived"));
            select(law);
        }
        if (ImGui::MenuItem("On collision -> bounce (velocity is law now)")) {
            auto law = laws.createLaw("Bounce", {&player});
            // Collision RESPONSE as authored text: reflect the vertical
            // velocity with restitution 0.8 — physics legislated, not coded.
            law->setActionModel(ActionNode::scale("velocity.y", -0.8));
            laws.bindTrigger(law->getIdentifier(), "collision");
            select(law);
        }
        if (ImGui::MenuItem("Metalaw — govern another law")) {
            auto law = laws.createLaw("Metalaw", {&player});
            law->setActionModel(ActionNode::set("enabled", PropertyValue(false)));
            laws.bindTrigger(law->getIdentifier(), "law-registered");
            select(law);
        }
        ImGui::EndPopup();
    }
    ImGui::TextDisabled("A law: WHEN an event fires,");
    ImGui::TextDisabled("IF conditions hold, THEN act.");
    ImGui::Separator();

    std::size_t authoredCount = 0, firstMoverCount = 0;
    for (const auto& law : laws.getAll()) {
        if (!law) continue;
        (law->isFirstMover() ? firstMoverCount : authoredCount)++;
    }

    ImGui::TextColored(kHeaderColor, "The register (%zu)", laws.getAll().size());
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##lawfilter", "filter by name...", g.filterBuf,
                             sizeof(g.filterBuf));
    const auto passesFilter = [&](const Law& law) {
        if (g.filterBuf[0] == '\0') return true;
        std::string haystack = law.name(), needle = g.filterBuf;
        std::transform(haystack.begin(), haystack.end(), haystack.begin(), ::tolower);
        std::transform(needle.begin(), needle.end(), needle.begin(), ::tolower);
        return haystack.find(needle) != std::string::npos;
    };
    const auto lawRow = [&](const std::shared_ptr<Law>& law) {
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
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", law->getIdentifier().c_str());
        }
        ImGui::SameLine();
        if (!law->isAuthored() && !law->isFirstMover()) {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "!");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("unauthored — cannot fire");
        } else {
            const char* tag = law->activation() == Law::Activation::WhileTrue ? "while"
                              : law->activation() == Law::Activation::OnBecomeTrue
                                  ? "edge"
                                  : "event";
            ImGui::TextDisabled("%s", tag);
        }
        ImGui::PopID();
    };
    if (authoredCount == 0) {
        ImGui::Spacing();
        ImGui::TextDisabled("No authored laws yet. To make one:");
        ImGui::TextDisabled(" 1. + New Law above");
        ImGui::TextDisabled(" 2. bind a trigger (the WHEN)");
        ImGui::TextDisabled(" 3. shape the IF and THEN cards");
        ImGui::TextDisabled(" 4. watch it fire in Recent events");
        ImGui::Spacing();
    }
    for (const auto& law : laws.getAll()) {
        if (law && !law->isFirstMover() && passesFilter(*law)) lawRow(law);
    }
    if (firstMoverCount > 0) {
        ImGui::Spacing();
        ImGui::TextDisabled("First movers — the engine, legible:");
        for (const auto& law : laws.getAll()) {
            if (law && law->isFirstMover() && passesFilter(*law)) lawRow(law);
        }
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
        ImGui::Spacing();
        ImGui::TextColored(kHeaderColor, "The Law Author");
        ImGui::TextWrapped(
            "Laws are the ordered principles of this world: WHEN an event "
            "fires, IF the conditions hold, THEN the action applies.");
        ImGui::Spacing();
        ImGui::TextDisabled("Select a law on the left, or press + New Law.");
        ImGui::TextDisabled("Everything a law touches is authored: triggers, exact");
        ImGui::TextDisabled("mathematics, scope, drives — and laws can govern laws.");
        ImGui::EndChild();
        ImGui::End();
        return;
    }

    std::string binding;
    {
        const auto& types = laws.triggersOf(g.selectedLawId);
        for (std::size_t i = 0; i < types.size(); ++i) {
            if (i) binding += ", ";
            binding += types[i];
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

        // The law's health at a glance — one row of chips.
        {
            const ImVec4 kOk(0.45f, 0.85f, 0.5f, 1.0f);
            const ImVec4 kInfo(0.55f, 0.75f, 1.0f, 1.0f);
            const ImVec4 kWarn(1.0f, 0.6f, 0.2f, 1.0f);
            const ImVec4 kMuted(0.62f, 0.62f, 0.62f, 1.0f);
            const auto chip = [](const ImVec4& color, const std::string& text) {
                ImGui::TextColored(color, "[%s]", text.c_str());
                ImGui::SameLine();
            };
            // The first chip is a SWITCH: click to enable/disable the law.
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  law->isEnabled() ? ImVec4(0.16f, 0.42f, 0.22f, 1.0f)
                                                   : ImVec4(0.5f, 0.32f, 0.12f, 1.0f));
            if (ImGui::SmallButton(law->isEnabled() ? "active" : "disabled")) {
                law->setEnabled(!law->isEnabled());
            }
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("click to toggle");
            ImGui::SameLine();
            chip(kInfo, law->activation() == Law::Activation::WhileTrue
                            ? "while true"
                        : law->activation() == Law::Activation::OnBecomeTrue
                            ? "on edge"
                            : "on event");
            if (law->drives()) {
                chip(kInfo, law->retrigger() == Law::Retrigger::Restart
                                ? "drives, restarts"
                                : "drives");
            }
            const std::size_t triggerCount = laws.triggersOf(g.selectedLawId).size();
            if (law->activation() == Law::Activation::OnEvent) {
                chip(triggerCount ? kMuted : kWarn,
                     triggerCount ? std::to_string(triggerCount) + " trigger(s)"
                                  : "no triggers");
            }
            if (!law->isFirstMover()) {
                chip(law->isAuthored() ? kMuted : kWarn,
                     law->isAuthored() ? "authored" : "UNAUTHORED");
            } else {
                chip(kMuted, "first mover");
            }
            std::size_t appliedCount = 0;
            for (const auto& record : law->applicationLog()) {
                if (record.result == Law::ApplicationResult::Applied) ++appliedCount;
            }
            chip(kMuted, "applied " + std::to_string(appliedCount) + "x");
            ImGui::NewLine();
        }

        // The law in one sentence.
        {
            const auto& boundTypes = laws.triggersOf(g.selectedLawId);
            ImGui::TextWrapped("%s", lawSentence(*law, &boundTypes).c_str());
        }

        // Edge vs level: does the law wait for events, or watch continuously?
        {
            static const char* modes[] = {
                "on event (fires when a bound trigger fires)",
                "while true (watches every frame; fires each frame it holds)",
                "on becoming true (watches every frame; fires once per onset)"};
            int mode = static_cast<int>(law->activation());
            ImGui::SetNextItemWidth(360.0f);
            if (ImGui::Combo("Activation", &mode, modes, 3)) {
                law->setActivation(static_cast<Law::Activation>(mode));
            }
            if (law->activation() != Law::Activation::OnEvent) {
                // The watched set is an explicit choice, same as OnEvent's
                // "Applies to" — every being, or only named targets.
                const bool hasTargets = !law->targets().getMembers().empty();
                int watch = hasTargets ? 1 : 0;
                static const char* watches[] = {
                    "every being in the Universe",
                    "only its targets (add them under Targets below)"};
                ImGui::SetNextItemWidth(360.0f);
                if (ImGui::Combo("Watches", &watch, watches, 2)) {
                    if (watch == 0) law->clearTargets();
                }
                if (watch == 1 && !hasTargets) {
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                                       "! No targets yet — until one is added, the law "
                                       "still watches everyone.");
                }
                if (!laws.triggersOf(law->getIdentifier()).empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                                       "! Continuous activation IGNORES triggers — the "
                                       "condition alone gates it.");
                }
            }
        }

        // The drive: an authored choice — after its trigger, the law keeps
        // applying every frame until its function's authored bounds end.
        // ANY bound variable may cut the bounds: time is one input among
        // the rest (positions, sizes, other beings' state).
        if (law->activation() != Law::Activation::WhileTrue) {
            bool drives = law->drives();
            if (ImGui::Checkbox("Drive: keep applying after the trigger", &drives)) {
                law->setDrives(drives);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "The law keeps applying every frame until its function becomes\n"
                    "undefined for the subject — the authored piece bounds are the\n"
                    "duration, and any bound variable can cut them (time,\n"
                    "another being's position, the subject's own state).\n"
                    "Ends with a \"law-drive-finished\" event. With no bounded\n"
                    "function it drives until disabled.");
            }
            if (drives) {
                ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f),
                                   "~ Drives: after the trigger it keeps applying, until "
                                   "the authored bounds end.");
                static const char* retriggerModes[] = {
                    "absorb it (the running process keeps its clock)",
                    "restart (the new trigger is a new t = 0)"};
                int retrigger = static_cast<int>(law->retrigger());
                ImGui::SetNextItemWidth(360.0f);
                if (ImGui::Combo("While driving, a re-trigger will", &retrigger,
                                 retriggerModes, 2)) {
                    law->setRetrigger(static_cast<Law::Retrigger>(retrigger));
                }
            } else if (law->hasActionModel() &&
                       law->actionModel()->referencesSinceApplied()) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                                   "! This action reads time.sinceApplied but the law does "
                                   "not drive —");
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                                   "  it will only see t = 0. Enable Drive to run it over "
                                   "time.");
            }
        }

        // WHOM an application reaches — the "whose position?" question.
        if (law->activation() == Law::Activation::OnEvent) {
            static const char* scopes[] = {
                "the event's subject (the being the event is about)",
                "EVERY being that satisfies the IF (the event is the occasion)"};
            int scope = static_cast<int>(law->scope());
            ImGui::SetNextItemWidth(360.0f);
            if (ImGui::Combo("Applies to", &scope, scopes, 2)) {
                law->setScope(static_cast<Law::Scope>(scope));
            }
        }

        // Targets: scope a law to specific beings; empty = the Universe for
        // Everyone/continuous laws.
        {
            ImGui::TextColored(kHeaderColor, "Targets");
            const auto members = law->targets().getMembers();
            if (members.empty()) {
                ImGui::TextDisabled("None — Everyone/continuous laws range over the whole Universe.");
            }
            Singular* removeTarget = nullptr;
            for (auto* member : members) {
                if (!member) continue;
                ImGui::PushID(member);
                ImGui::BulletText("%s", member->getIdentifier().c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("remove")) removeTarget = member;
                ImGui::PopID();
            }
            if (removeTarget) law->targets().removeMember(removeTarget);
            if (testSubject) {
                if (ImGui::SmallButton("+ add selected object as target")) {
                    law->addTarget(*testSubject);
                }
            }
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
            if (authors.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                                   "Authored by: nobody — the law CANNOT FIRE.");
                ImGui::SameLine();
                // Re-authoring is a Person's signature, not a load-time
                // forgery — laws saved before authors had stable identities
                // reload unauthored, and this is their lawful way back.
                if (ImGui::SmallButton("sign as author")) {
                    law->addAuthor(player);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("add yourself (%s) as this law's author",
                                      player.getIdentifier().c_str());
                }
            } else {
                ImGui::TextDisabled("Authored by: %s", authors.c_str());
            }
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
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("runs the full IF/THEN gauntlet directly, "
                                  "skipping the trigger");
            }
            // Fire the law's own trigger as a REAL event — the convenient
            // way to test the whole pipeline without staging a collision.
            const auto& testTriggers = laws.triggersOf(g.selectedLawId);
            if (!testTriggers.empty()) {
                ImGui::SameLine();
                const std::string fireLabel =
                    "Fire '" + testTriggers[0] + "' at selected";
                if (ImGui::Button(fireLabel.c_str())) {
                    Core::EventBus::instance().publish(ECA::Event{
                        testTriggers[0], testSubject, nullptr, std::time(nullptr)});
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(
                        "publishes a real \"%s\" event (subject: %s) through the "
                        "bus.\nEvery listening law hears it on the next tick — "
                        "watch Recent events.",
                        testTriggers[0].c_str(),
                        testSubject->getIdentifier().c_str());
                }
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
            // What wakes a law is part of its text: the twin listens too.
            for (const auto& type : laws.triggersOf(law->getIdentifier())) {
                laws.bindTrigger(twin->getIdentifier(), type);
            }
            g.selectedLawId = twin->getIdentifier();
            g.selectedCard = 0;
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.16f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.20f, 0.20f, 1.0f));
        if (ImGui::Button("Delete law")) {
            ImGui::OpenPopup("Delete law?");
        }
        ImGui::PopStyleColor(2);
        if (ImGui::BeginPopupModal("Delete law?", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Delete \"%s\" permanently?", law->name().c_str());
            ImGui::TextDisabled("Its triggers and bindings go with it.");
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.16f, 0.16f, 1.0f));
            if (ImGui::Button("Delete", ImVec2(120, 0))) {
                laws.remove(law->getIdentifier());   // drops triggers + Rete bindings
                g.selectedLawId.clear();
                g.selectedCard = -1;
                ImGui::PopStyleColor();
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                ImGui::EndChild();
                ImGui::End();
                return;
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
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
            // Multiple conditions, like binding multiple triggers: wrap the
            // root in All/Any and append — each member then edits and
            // deletes as its own card.
            if (law->hasConditionModel() &&
                law->conditionModel()->kind != ConditionNode::Kind::All &&
                law->conditionModel()->kind != ConditionNode::Kind::Any) {
                if (ImGui::Button("+ AND another condition")) {
                    ConditionNode wrapped;
                    wrapped.kind = ConditionNode::Kind::All;
                    wrapped.children.push_back(*law->conditionModel());
                    ConditionNode extra;
                    extra.kind = ConditionNode::Kind::Compare;
                    seedConditionKind(extra);
                    wrapped.children.push_back(std::move(extra));
                    law->setConditionModel(std::move(wrapped));
                }
                ImGui::SameLine();
                if (ImGui::Button("+ OR another condition")) {
                    ConditionNode wrapped;
                    wrapped.kind = ConditionNode::Kind::Any;
                    wrapped.children.push_back(*law->conditionModel());
                    ConditionNode extra;
                    extra.kind = ConditionNode::Kind::Compare;
                    seedConditionKind(extra);
                    wrapped.children.push_back(std::move(extra));
                    law->setConditionModel(std::move(wrapped));
                }
                ImGui::TextDisabled("(both conditions become cards — click each to edit; "
                                    "nest all/any for grouping)");
            }
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
            // Multiple actions, like binding multiple triggers: wrap the
            // root in a Sequence and append another step.
            if (law->hasActionModel() &&
                law->actionModel()->kind != ActionNode::Kind::Sequence &&
                law->actionModel()->kind != ActionNode::Kind::Parallel) {
                if (ImGui::Button("+ then another action")) {
                    ActionNode wrapped;
                    wrapped.kind = ActionNode::Kind::Sequence;
                    wrapped.children.push_back(*law->actionModel());
                    ActionNode extra;
                    extra.kind = ActionNode::Kind::Set;
                    seedActionKind(extra);
                    extra.operand = PropertyValue(0.0);
                    wrapped.children.push_back(std::move(extra));
                    law->setActionModel(std::move(wrapped));
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(each step becomes a card — click to edit)");
            }
            if (ImGui::Button("Remove action (law recognizes but does not act)")) {
                law->clearActionModel();
                g.selectedCard = 0;
            }
        }
    }

    // The bus, live: what events are ACTUALLY flowing. If a trigger never
    // appears here, the world never produced it — the law is not at fault.
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Recent events (live)")) {
        if (g.eventFeed.empty()) {
            ImGui::TextDisabled("No events yet this session.");
        }
        for (const auto& entry : g.eventFeed) {
            if (entry.count > 1) {
                ImGui::BulletText("%s  (subject: %s)  x%d", entry.type.c_str(),
                                  entry.subjectId.empty() ? "-" : entry.subjectId.c_str(),
                                  entry.count);
            } else {
                ImGui::BulletText("%s  (subject: %s)", entry.type.c_str(),
                                  entry.subjectId.empty() ? "-" : entry.subjectId.c_str());
            }
        }
    }
    // The world's pulse — visible progress, at a glance.
    {
        std::size_t authoredLaws = 0, engineLaws = 0;
        for (const auto& registered : laws.getAll()) {
            if (!registered) continue;
            (registered->isFirstMover() ? engineLaws : authoredLaws)++;
        }
        std::string pulse = std::to_string(authoredLaws) + " authored law(s) | " +
                            std::to_string(engineLaws) + " first mover(s) | " +
                            std::to_string(ConceptRegistry::instance().getAll().size()) +
                            " concept(s) | " +
                            std::to_string(laws.driveSessions().size()) +
                            " live drive(s)";
        if (Universe::instance().hasClock()) {
            char clock[32];
            std::snprintf(clock, sizeof(clock), " | t = %.1fs",
                          Universe::instance().now());
            pulse += clock;
        }
        ImGui::TextDisabled("%s", pulse.c_str());
    }
    ImGui::TextDisabled("Laws, triggers, and concepts save and load with the world.");
    ImGui::EndChild();
    ImGui::End();
}

} // namespace Rendering
