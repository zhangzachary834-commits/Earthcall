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
        } else if (ImGui::SmallButton("+ guard (applies where g <= 0)")) {
            const std::string firstVar =
                bindings.empty() ? std::string("x") : bindings.begin()->first;
            piece.guard = std::make_shared<ConditionNode>(ConditionNode::zone(
                OntoMath::Piecewise::continuous(OntoMath::Expression::variable(firstVar)),
                bindings, PropertyValue{}, PropertyValue(0.0)));
            piece.hasLo = piece.hasHi = false;   // the guard decides now
            piece.guardCompiled = nullptr;
            changed = true;
        } else if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("gate this piece by a CONDITION instead of interval\n"
                              "bounds — the discrete-math fusion: min/max/abs and\n"
                              "ontology-branching mathematics live here");
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
            // Transcendental factors: exact sin/cos/exp/ln of a bound
            // variable — periodic and exponential change as law-text.
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

} // namespace MathEd
} // namespace Rendering
