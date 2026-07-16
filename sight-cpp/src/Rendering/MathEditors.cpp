#include "Rendering/MathEditors.hpp"

#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"

#include <imgui.h>

#include <cstring>
#include <string>

namespace Rendering {
namespace MathEd {

namespace {
const ImVec4 kHeaderColor(0.95f, 0.85f, 0.55f, 1.0f);
const ImVec4 kWarnColor(1.0f, 0.6f, 0.2f, 1.0f);
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
    ImGui::SetNextItemWidth(56.0f);
    ImGui::InputText("##bindvar", varBuf, sizeof(varBuf));
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("variable name, e.g. x");
    ImGui::SameLine();
    static PropertyPath pendingBindPath;
    if (pathPicker) pathPicker("##bindpath", pendingBindPath);
    ImGui::SameLine();
    if (ImGui::Button("Add variable") && varBuf[0] != '\0' && !pendingBindPath.empty()) {
        bindings[varBuf] = pendingBindPath;
        varBuf[0] = '\0';
        pendingBindPath = PropertyPath{};
        changed = true;
    }
    return changed;
}

bool editExpression(OntoMath::Expression& e, const MathBindings& bindings) {
    bool changed = false;
    int removeTerm = -1;
    for (std::size_t t = 0; t < e.terms.size(); ++t) {
        auto& term = e.terms[t];
        ImGui::PushID(static_cast<int>(t) + 100);
        ImGui::SetNextItemWidth(80.0f);
        double c = term.coefficient;
        if (ImGui::InputDouble("coeff", &c)) { term.coefficient = c; changed = true; }
        for (auto& factor : term.factors) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(56.0f);
            double exp = factor.second;
            if (ImGui::InputDouble(factor.first.c_str(), &exp)) {
                factor.second = exp;
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
        // Transcendental factors: exact sin/cos/exp/ln of a bound variable.
        ImGui::SameLine();
        if (ImGui::SmallButton("+f()")) ImGui::OpenPopup("addtrans");
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
        if (ImGui::SmallButton("x##rmterm")) removeTerm = static_cast<int>(t);

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
        for (const auto& term : piece.expression.terms) {
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

        // Expression-guarded piece: a CONDITION decides where this formula
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
                OntoMath::Piecewise::continuous(OntoMath::Expression::variable(firstVar)),
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
                if (editExpression(*piece.whereLEZero, bindings)) changed = true;
                ImGui::Unindent();
                ImGui::PopID();
            }
        } else {
            if (!piece.guard) ImGui::SameLine();
            if (ImGui::SmallButton("+ where g <= 0 (of the variables)")) {
                piece.whereLEZero = std::make_shared<OntoMath::Expression>(
                    OntoMath::Expression::variable(
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
                                            OntoMath::Expression::constant(0.0));
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
        } else {
            if (editExpression(piece.expression, bindings)) changed = true;
            if (!OntoMath::FunctionRegistry::instance().getAll().empty()) {
                ImGui::SameLine();
                if (ImGui::SmallButton("call a function...")) {
                    const auto& def =
                        OntoMath::FunctionRegistry::instance().getAll().front();
                    auto call = std::make_shared<OntoMath::FunctionCall>();
                    call->function = def.name;
                    call->args.assign(def.params.size(),
                                      OntoMath::Expression::variable(
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
        piece.expression = OntoMath::Expression::constant(0.0);
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
    ImGui::SetNextItemWidth(120.0f);
    ImGui::InputText("##fnname", nameBuf, sizeof(nameBuf));
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("function name");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::InputText("##fnparams", paramsBuf, sizeof(paramsBuf));
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("parameters, comma-separated: x, n");
    ImGui::SameLine();
    if (ImGui::Button("Define") && nameBuf[0] != '\0') {
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
        def.body = OntoMath::Piecewise::continuous(
            OntoMath::Expression::variable(def.params.front()));
        def.body.inputVariable = def.params.front();
        registry.define(std::move(def));
        selected = static_cast<int>(registry.getAll().size()) - 1;
        nameBuf[0] = '\0';
    }

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

} // namespace MathEd
} // namespace Rendering
