#include "Singularity/Screen/MathEditors.hpp"

#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

namespace Rendering {
namespace MathEd {

namespace {
const ImVec4 kHeaderColor(0.95f, 0.85f, 0.55f, 1.0f);
const ImVec4 kWarnColor(1.0f, 0.6f, 0.2f, 1.0f);

void copyToBuf(char* buf, std::size_t size, const std::string& value) {
    std::strncpy(buf, value.c_str(), size - 1);
    buf[size - 1] = '\0';
}

bool containsCaseInsensitive(const std::string& haystack, const char* needleText) {
    if (!needleText || !needleText[0]) return true;
    std::string a = haystack;
    std::string b = needleText;
    std::transform(a.begin(), a.end(), a.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::transform(b.begin(), b.end(), b.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return a.find(b) != std::string::npos;
}

void fieldCaption(const char* label, const char* help = nullptr) {
    ImGui::TextDisabled("%s", label);
    if (help && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", help);
}

bool textField(const char* label, char* value, std::size_t size, const char* hint) {
    ImGui::PushID(label);
    fieldCaption(label);
    ImGui::SetNextItemWidth(-1.0f);
    const bool changed = ImGui::InputTextWithHint("##value", hint, value, size);
    ImGui::PopID();
    return changed;
}
} // namespace

bool editMathBindings(MathBindings& bindings, const PathPickerFn& pathPicker) {
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
    static char varBuf[32] = "";
    textField("New variable name", varBuf, sizeof(varBuf), "for example: x or elapsed");
    static PropertyPath pendingBindPath;
    if (pathPicker) pathPicker("Property supplying its value", pendingBindPath);
    const bool ready = varBuf[0] != '\0' && !pendingBindPath.empty();
    if (!ready) ImGui::BeginDisabled();
    if (ImGui::Button("Bind this variable", ImVec2(-1.0f, 0.0f)) && ready) {
        bindings[varBuf] = pendingBindPath;
        varBuf[0] = '\0';
        pendingBindPath = PropertyPath{};
        changed = true;
    }
    if (!ready) ImGui::EndDisabled();
    return changed;
}

bool editExpression(OntoMath::ScalarForm& e, const MathBindings& bindings) {
    bool changed = false;
    int removeTerm = -1;
    for (std::size_t t = 0; t < e.terms.size(); ++t) {
        auto& term = e.terms[t];
        ImGui::PushID(static_cast<int>(t) + 100);
        ImGui::TextColored(kHeaderColor, "Term %zu", t + 1);
        double c = term.coefficient;
        fieldCaption("Coefficient");
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputDouble("##coefficient", &c, 0.0, 0.0, "%.4f")) {
            term.coefficient = c;
            changed = true;
        }
        for (auto& factor : term.factors) {
            ImGui::PushID(factor.first.c_str());
            const std::string label = "Exponent of " + factor.first;
            fieldCaption(label.c_str());
            ImGui::SetNextItemWidth(-1.0f);
            double exp = factor.second;
            if (ImGui::InputDouble("##exponent", &exp, 0.0, 0.0, "%.4f")) {
                factor.second = exp;
                changed = true;
            }
            ImGui::PopID();
        }
        ImGui::TextDisabled("Multiply by another bound variable");
        for (const auto& binding : bindings) {
            if (term.factors.count(binding.first)) continue;
            if (ImGui::SmallButton(("*" + binding.first).c_str())) {
                term.factors[binding.first] = 1.0;
                changed = true;
            }
            ImGui::SameLine();
        }
        ImGui::NewLine();
        // Transcendental factors: exact sin/cos/exp/ln of a bound variable.
        if (ImGui::SmallButton("+ transcendental factor")) ImGui::OpenPopup("addtrans");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("multiply in an exact transcendental factor:\n"
                              "sin / cos / exp / ln of a bound variable");
        }
        if (ImGui::BeginPopup("addtrans")) {
            static const char* transNames[] = {"sin", "cos", "exp", "ln"};
            for (int k = 0; k < 4; ++k) {
                for (const auto& binding : bindings) {
                    const std::string label =
                        std::string(transNames[k]) + "(" + binding.first + ")";
                    if (ImGui::MenuItem(label.c_str())) {
                        term.addTrans(OntoMath::TransFactor(
                            static_cast<OntoMath::TransFactor::Kind>(k),
                            binding.first));
                        changed = true;
                    }
                }
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("remove term")) removeTerm = static_cast<int>(t);

        int removeTrans = -1;
        for (std::size_t tfIndex = 0; tfIndex < term.trans.size(); ++tfIndex) {
            auto& tf = term.trans[tfIndex];
            ImGui::PushID(static_cast<int>(tfIndex) + 300);
            static const char* transNames[] = {"sin", "cos", "exp", "ln"};
            ImGui::Text("   × %s(", transNames[static_cast<int>(tf.kind)]);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(56.0f);
            double scale = tf.scale;
            if (ImGui::InputDouble("##tscale", &scale)) {
                tf.scale = scale;
                changed = true;
            }
            ImGui::SameLine();
            ImGui::Text("·%s", tf.variable.c_str());
            if (tf.kind != OntoMath::TransFactor::Kind::Ln) {
                ImGui::SameLine();
                ImGui::Text("+");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(56.0f);
                double shift = tf.shift;
                if (ImGui::InputDouble("##tshift", &shift)) {
                    tf.shift = shift;
                    changed = true;
                }
            }
            ImGui::SameLine();
            ImGui::Text(")");
            ImGui::SameLine();
            if (ImGui::SmallButton("x##rmtrans")) removeTrans = static_cast<int>(tfIndex);
            ImGui::PopID();
        }
        if (removeTrans >= 0) {
            term.trans.erase(term.trans.begin() + removeTrans);
            changed = true;
        }
        ImGui::PopID();
    }
    if (removeTerm >= 0) {
        e.terms.erase(e.terms.begin() + removeTerm);
        changed = true;
    }
    if (ImGui::SmallButton("+ term")) {
        e.terms.emplace_back(1.0);
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
        for (const auto& term : piece.mathNode->scalarForm.terms) {
            for (const auto& factor : term.factors) {
                if (!bindings.count(factor.first)) {
                    ImGui::TextColored(kWarnColor,
                                       "! variable \"%s\" has no binding — the function "
                                       "cannot evaluate",
                                       factor.first.c_str());
                }
            }
            for (const auto& tf : term.trans) {
                if (!bindings.count(tf.variable)) {
                    ImGui::TextColored(kWarnColor,
                                       "! variable \"%s\" has no binding — the function "
                                       "cannot evaluate",
                                       tf.variable.c_str());
                }
            }
        }
    }

    // Which variable the piece bounds cut (only matters with bounded pieces).
    bool anyBounded = f.pieces.size() > 1;
    for (const auto& piece : f.pieces) anyBounded = anyBounded || piece.hasLo || piece.hasHi;
    if (anyBounded && !bindings.empty()) {
        fieldCaption("Variable cut by the piece boundaries");
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##bounds-variable", f.inputVariable.c_str())) {
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

        // ScalarForm-guarded piece: a CONDITION decides where this formula
        // applies ("wherever g <= 0") — min/max/abs and the SDF boolean
        // algebra live here. The guard supersedes interval bounds.
        if (piece.guard) {
            ImGui::TextColored(kHeaderColor, "Piece %zu — guarded:", p + 1);
            ImGui::SameLine();
            ImGui::TextDisabled("%s", piece.guard->describe().c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("remove guard")) {
                piece.guard.reset();
                piece.guardCompiled = nullptr;
                changed = true;
            }
            if (piece.guard &&
                piece.guard->kind == ConditionNode::Kind::Zone) {
                ImGui::TextDisabled("  applies where g(variables) <= 0; g is:");
                ImGui::PushID("guard-g");
                ImGui::Indent();
                if (editPiecewise(piece.guard->zoneFunction, bindings)) {
                    piece.guardCompiled = nullptr;
                    changed = true;
                }
                ImGui::Unindent();
                ImGui::PopID();
            }
        } else if (ImGui::SmallButton("+ guard (world condition)")) {
            const std::string firstVar =
                bindings.empty() ? std::string("x") : bindings.begin()->first;
            piece.guard = std::make_shared<ConditionNode>(ConditionNode::zone(
                OntoMath::Piecewise::continuous(OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::variable(firstVar))),
                bindings, PropertyValue{}, PropertyValue(0.0)));
            piece.hasLo = piece.hasHi = false;   // the guard decides now
            piece.guardCompiled = nullptr;
            changed = true;
        } else if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("gate this piece by a CONDITION about the subject —\n"
                              "IsKind / Related / Overlaps: mathematics that\n"
                              "branches on ontology");
        }

        // The PURE guard: local mathematics gating local mathematics —
        // "applies where g(variables) <= 0", no subject needed. This is
        // what recursion base cases over parameters use.
        if (piece.whereLEZero) {
            ImGui::TextColored(kHeaderColor, "  where g <= 0; g is:");
            ImGui::SameLine();
            if (ImGui::SmallButton("remove##rmwhere")) {
                piece.whereLEZero.reset();
                changed = true;
            }
            if (piece.whereLEZero) {
                ImGui::PushID("where-g");
                ImGui::Indent();
                if (editExpression(piece.whereLEZero->scalarForm, bindings)) changed = true;
                ImGui::Unindent();
                ImGui::PopID();
            }
        } else {
            if (!piece.guard) ImGui::SameLine();
            if (ImGui::SmallButton("+ where g <= 0 (of the variables)")) {
                piece.whereLEZero = OntoMath::MathNode::fromLegacyExpression(
                    OntoMath::ScalarForm::variable(
                        bindings.empty() ? "x" : bindings.begin()->first));
                piece.hasLo = piece.hasHi = false;
                changed = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("gate this piece by the VARIABLES alone — min/max/abs\n"
                                  "and recursion base cases (no subject needed)");
            }
        }

        if (!piece.guard && (f.pieces.size() > 1 || piece.hasLo || piece.hasHi)) {
            ImGui::Text("Piece %zu over %s:", p + 1, f.inputVariable.c_str());
            if (ImGui::Checkbox("Use lower boundary", &piece.hasLo)) changed = true;
            if (piece.hasLo) {
                double lo = piece.lo;
                fieldCaption("Lower boundary");
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::InputDouble("##lo", &lo, 0.0, 0.0, "%.4f")) {
                    piece.lo = lo;
                    changed = true;
                }
                if (ImGui::Checkbox("Include the lower boundary", &piece.includeLo)) changed = true;
            }
            if (ImGui::Checkbox("Use upper boundary", &piece.hasHi)) changed = true;
            if (piece.hasHi) {
                double hi = piece.hi;
                fieldCaption("Upper boundary");
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::InputDouble("##hi", &hi, 0.0, 0.0, "%.4f")) {
                    piece.hi = hi;
                    changed = true;
                }
                if (ImGui::Checkbox("Include the upper boundary", &piece.includeHi)) changed = true;
            }
        }

        // The piece's VALUE: a call to a named function, or an expression.
        if (piece.call) {
            const OntoMath::FunctionDef* def =
                OntoMath::FunctionRegistry::instance().find(piece.call->function);
            ImGui::TextColored(kHeaderColor, "  calls %s(%s)",
                               piece.call->function.c_str(),
                               def ? std::to_string(def->params.size()).c_str() : "?");
            ImGui::SameLine();
            if (ImGui::SmallButton("remove call")) {
                piece.call.reset();
                changed = true;
            }
            if (!def) {
                ImGui::TextColored(kWarnColor,
                                   "  ! no function named \"%s\" is defined",
                                   piece.call->function.c_str());
            } else {
                if (piece.call->args.size() != def->params.size()) {
                    piece.call->args.resize(def->params.size(),
                                            OntoMath::ScalarForm::constant(0.0));
                    changed = true;
                }
                for (std::size_t a = 0; a < piece.call->args.size(); ++a) {
                    ImGui::PushID(static_cast<int>(a) + 700);
                    ImGui::TextDisabled("  %s =", def->params[a].c_str());
                    ImGui::Indent();
                    if (editExpression(piece.call->args[a], bindings)) changed = true;
                    ImGui::Unindent();
                    ImGui::PopID();
                }
            }
        } else if (piece.fold) {
            static const char* foldOps[] = {"sum", "mean", "min", "max", "count"};
            static const char* foldKinds[] = {"any being", "Object",    "Person",
                                              "Relation",  "Formation", "Law",
                                              "Zone"};
            static const int foldKindEnums[] = {0, 1, 2, 3, 4, 5, 7};
            ImGui::TextColored(kHeaderColor, "  folds over the world:");
            ImGui::SameLine();
            if (ImGui::SmallButton("remove fold")) {
                piece.fold.reset();
                changed = true;
            }
            if (piece.fold) {
                int op = static_cast<int>(piece.fold->op);
                fieldCaption("Aggregation");
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::Combo("##foldop", &op, foldOps, 5)) {
                    piece.fold->op = static_cast<OntoMath::Fold::Op>(op);
                    changed = true;
                }
                char pathBuf[96];
                copyToBuf(pathBuf, sizeof(pathBuf), piece.fold->path);
                if (textField("Property read from each being", pathBuf, sizeof(pathBuf),
                              "for example: position.y")) {
                    piece.fold->path = pathBuf;
                    changed = true;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("property read on each being (e.g. position.y);\n"
                                      "leave empty with 'count' to count beings");
                }
                int foldDisplay = 0;
                for (int i = 0; i < 7; ++i) {
                    if (foldKindEnums[i] == piece.fold->beingKind) {
                        foldDisplay = i;
                        break;
                    }
                }
                fieldCaption("Across every");
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::Combo("##foldkind", &foldDisplay, foldKinds, 7)) {
                    piece.fold->beingKind = foldKindEnums[foldDisplay];
                    changed = true;
                }
            }
        } else {
            if (piece.mathNode) {
                if (editMathNode(*piece.mathNode, bindings)) changed = true;
            } else {
                piece.mathNode = OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::constant(0.0));
                changed = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("fold over the world...")) {
                piece.fold = std::make_shared<OntoMath::Fold>();
                piece.fold->op = OntoMath::Fold::Op::Mean;
                piece.fold->path = "position.y";
                changed = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("the piece's value becomes an AGGREGATE across\n"
                                  "the world's beings: sum / mean / min / max / count");
            }
            if (!OntoMath::FunctionRegistry::instance().getAll().empty()) {
                ImGui::SameLine();
                if (ImGui::SmallButton("call a function...")) {
                    const auto& def =
                        OntoMath::FunctionRegistry::instance().getAll().front();
                    auto call = std::make_shared<OntoMath::FunctionCall>();
                    call->function = def.name;
                    call->args.assign(def.params.size(),
                                      OntoMath::ScalarForm::variable(
                                          bindings.empty() ? "x"
                                                           : bindings.begin()->first));
                    piece.call = std::move(call);
                    changed = true;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("the piece's value becomes a NAMED function call\n"
                                      "(composition and recursion; see Named functions)");
                }
            }
        }
        ImGui::PopID();
        ImGui::Separator();
    }
    if (ImGui::SmallButton("+ piece")) {
        OntoMath::Piecewise::Piece piece;
        piece.mathNode = OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::constant(0.0));
        f.pieces.push_back(std::move(piece));
        changed = true;
    }
    return changed;
}

void editFunctionRegistry() {
    auto& registry = OntoMath::FunctionRegistry::instance();
    ImGui::TextDisabled("Define a function once; call it from any piece —");
    ImGui::TextDisabled("composition and recursion (depth-capped at %d).",
                        OntoMath::FunctionRegistry::kMaxCallDepth);

    static int selected = -1;
    std::string removeName;
    const auto& defs = registry.getAll();
    for (std::size_t i = 0; i < defs.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        std::string signature = defs[i].name + "(";
        for (std::size_t p = 0; p < defs[i].params.size(); ++p) {
            if (p) signature += ", ";
            signature += defs[i].params[p];
        }
        signature += ")";
        if (ImGui::Selectable(signature.c_str(), static_cast<int>(i) == selected)) {
            selected = static_cast<int>(i);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("delete")) removeName = defs[i].name;
        ImGui::PopID();
    }
    if (!removeName.empty()) {
        registry.remove(removeName);
        selected = -1;
    }

    static char nameBuf[48] = "";
    static char paramsBuf[96] = "x";
    textField("Function name", nameBuf, sizeof(nameBuf), "name this reusable function…");
    textField("Parameters", paramsBuf, sizeof(paramsBuf), "comma-separated, for example: x, n");
    const bool canDefine = nameBuf[0] != '\0';
    if (!canDefine) ImGui::BeginDisabled();
    if (ImGui::Button("Define this function", ImVec2(-1.0f, 0.0f)) && canDefine) {
        OntoMath::FunctionDef def;
        def.name = nameBuf;
        std::string token;
        for (const char* c = paramsBuf;; ++c) {
            if (*c == ',' || *c == '\0' || *c == ' ') {
                if (!token.empty()) def.params.push_back(token);
                token.clear();
                if (*c == '\0') break;
            } else {
                token += *c;
            }
        }
        if (def.params.empty()) def.params.push_back("x");
        def.body = OntoMath::Piecewise::continuous(OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::variable(def.params.front())));
        def.body.inputVariable = def.params.front();
        registry.define(std::move(def));
        selected = static_cast<int>(registry.getAll().size()) - 1;
        nameBuf[0] = '\0';
    }
    if (!canDefine) ImGui::EndDisabled();

    if (selected >= 0 && selected < static_cast<int>(registry.getAll().size())) {
        // Edit the selected definition's body. Its "bindings" are its own
        // parameters — pure functions see nothing else.
        auto def = registry.getAll()[static_cast<std::size_t>(selected)];
        MathBindings paramBindings;
        for (const auto& p : def.params) paramBindings[p] = PropertyPath{};
        ImGui::Separator();
        ImGui::TextColored(kHeaderColor, "Body of %s:", def.name.c_str());
        ImGui::PushID("fnbody");
        const bool bodyChanged = editPiecewise(def.body, paramBindings);
        ImGui::PopID();
        if (bodyChanged) registry.define(std::move(def));
    }
}

bool editMathNode(OntoMath::MathNode& node, const MathBindings& bindings) {
    bool changed = false;

    struct OpDescriptor {
        OntoMath::MathNode::Op op;
        const char* name;
        std::size_t requiredChildren;
        bool isScalarLeaf;
        bool isValueLeaf;
        bool hasStringArg;
        const char* stringArgLabel;
    };

    static const OpDescriptor descriptors[] = {
        { OntoMath::MathNode::Op::ScalarLeaf, "Scalar Expression", 0, true, false, false, nullptr },
        { OntoMath::MathNode::Op::ValueLeaf, "Variable Leaf", 0, false, true, false, nullptr },
        { OntoMath::MathNode::Op::VectorConstruct, "Vector Construct", 3, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Component, "Component", 1, false, false, true, "index (0,1,2)" },
        { OntoMath::MathNode::Op::Add, "Add (+)", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Sub, "Subtract (-)", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Scale, "Scale (*)", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Div, "Divide (/)", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Dot, "Dot Product", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Cross, "Cross Product", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Hadamard, "Hadamard Product", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Normalize, "Normalize", 1, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Length, "Length", 1, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Map, "Map (Func)", 1, false, false, true, "func (round/floor/etc)" },
        { OntoMath::MathNode::Op::Stochastic, "Stochastic", 0, false, false, true, "distribution" },
        { OntoMath::MathNode::Op::Project, "Project", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Distance, "Distance", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Raycast, "Raycast", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::SDF, "SDF Sample", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Gradient, "Gradient", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::LineIntegral, "Line Integral", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Union, "CSG Union", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Intersection, "CSG Intersection", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Difference, "CSG Difference", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Pow, "Power (^)", 2, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Abs, "Abs", 1, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Clamp, "Clamp", 3, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Sqrt, "Sqrt", 1, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Tan, "Tan", 1, false, false, false, nullptr },
        { OntoMath::MathNode::Op::Noise, "Noise", 1, false, false, false, nullptr }
    };

    constexpr int numDescriptors = sizeof(descriptors) / sizeof(descriptors[0]);

    int currentOpIndex = 0;
    for (int i = 0; i < numDescriptors; ++i) {
        if (descriptors[i].op == node.op) {
            currentOpIndex = i;
            break;
        }
    }

    ImGui::PushID(&node);

    static char opSearch[96] = "";
    fieldCaption("Mathematical form");
    if (ImGui::Button(descriptors[currentOpIndex].name, ImVec2(-1.0f, 0.0f))) {
        opSearch[0] = '\0';
        ImGui::OpenPopup("choose-math-form");
    }
    if (ImGui::BeginPopup("choose-math-form")) {
        ImGui::TextColored(kHeaderColor, "CHOOSE A MATHEMATICAL FORM");
        ImGui::SetNextItemWidth(420.0f);
        ImGui::InputTextWithHint("##op-search", "type an operation…",
                                 opSearch, sizeof(opSearch));
        ImGui::Separator();
        ImGui::BeginChild("op-results", ImVec2(420.0f, 330.0f), false);
        const char* previousGroup = nullptr;
        for (int i = 0; i < numDescriptors; ++i) {
            if (!containsCaseInsensitive(descriptors[i].name, opSearch)) continue;
            const char* group = i <= 3 ? "VALUES"
                                : i <= 10 ? "ARITHMETIC & VECTORS"
                                : i <= 14 ? "TRANSFORMS"
                                : i <= 23 ? "SPACE & FIELDS"
                                          : "FUNCTIONS";
            if (!previousGroup || std::strcmp(group, previousGroup) != 0) {
                ImGui::TextDisabled("%s", group);
                previousGroup = group;
            }
            if (ImGui::Selectable(descriptors[i].name, i == currentOpIndex,
                                  0, ImVec2(0.0f, 31.0f))) {
                node.op = descriptors[i].op;
                currentOpIndex = i;
                changed = true;
                opSearch[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }

    const auto& desc = descriptors[currentOpIndex];

    if (node.children.size() != desc.requiredChildren) {
        node.children.resize(desc.requiredChildren);
        for (auto& child : node.children) {
            if (!child) {
                child = std::make_unique<OntoMath::MathNode>();
                child->op = OntoMath::MathNode::Op::ScalarLeaf;
                child->scalarForm = OntoMath::ScalarForm::constant(0.0);
            }
        }
        changed = true;
    }

    if (desc.isScalarLeaf) {
        ImGui::Indent();
        if (editExpression(node.scalarForm, bindings)) changed = true;
        ImGui::Unindent();
    }

    if (desc.isValueLeaf) {
        fieldCaption("Variable");
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##valleafvar", node.variableName.c_str())) {
            for (const auto& b : bindings) {
                if (ImGui::Selectable(b.first.c_str(), b.first == node.variableName)) {
                    node.variableName = b.first;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
    }

    if (desc.hasStringArg) {
        char argBuf[64];
        copyToBuf(argBuf, sizeof(argBuf), node.stringArg);
        if (textField(desc.stringArgLabel ? desc.stringArgLabel : "Argument",
                      argBuf, sizeof(argBuf), "enter an argument…")) {
            node.stringArg = argBuf;
            changed = true;
        }
    }

    for (std::size_t i = 0; i < node.children.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::TreeNode((void*)(intptr_t)i, "Child %zu", i + 1)) {
            if (node.children[i]) {
                if (editMathNode(*node.children[i], bindings)) changed = true;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    ImGui::PopID();
    return changed;
}

} // namespace MathEd
} // namespace Rendering
