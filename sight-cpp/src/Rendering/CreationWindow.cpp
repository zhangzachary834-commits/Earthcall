#include "Rendering/CreationWindow.hpp"

#include "Rendering/MathEditors.hpp"
#include "Form/Object/Creation/ObjectConcept.hpp"
#include "Form/Object/Object.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/World/World.hpp"

#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace Rendering {

namespace {

const ImVec4 kHeaderColor(0.95f, 0.85f, 0.55f, 1.0f);
const ImVec4 kWarnColor(1.0f, 0.6f, 0.2f, 1.0f);
const ImVec4 kInfoColor(0.45f, 0.75f, 1.0f, 1.0f);

// How one property redistributes across the new set — the authored row
// behind a PropertyMapping.
struct TransferRow {
    bool on = false;
    int agg = 0;                       // PropertyMapping::Aggregate
    int mode = 0;                      // 0 identity, 1 linear, 2 exact f(x)
    double c0 = 0.0, c1 = 1.0;         // linear: c0 + c1·x
    OntoMath::Piecewise exact;         // full OntoMath when mode == 2
    bool exactSeeded = false;
};

struct SessionState {
    std::vector<std::string> sourceIds;
    std::map<std::string, TransferRow> rows;   // property path -> choice
    char nameBuf[64] = "";
    float offset[3] = {2.0f, 0.0f, 0.0f};
    std::vector<std::string> lastCreatedIds;   // layer-3 slice: reuse output
};
SessionState g;

Object* findObject(const std::string& id) {
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == id) {
            return dynamic_cast<Object*>(being);
        }
    }
    return nullptr;
}

std::vector<Object*> resolveSources() {
    std::vector<Object*> sources;
    for (const auto& id : g.sourceIds) {
        if (Object* obj = findObject(id)) sources.push_back(obj);
    }
    return sources;
}

// The union of the LIVE source set's numeric property vocabulary — the
// actual beings offer their properties, not prototypes.
std::vector<std::string> unionOfProperties(const std::vector<Object*>& sources) {
    std::vector<std::string> paths;
    auto addUnique = [&](const std::string& p) {
        if (std::find(paths.begin(), paths.end(), p) == paths.end()) paths.push_back(p);
    };
    for (Object* source : sources) {
        if (!source) continue;
        for (Property* property : source->listProperties()) {
            if (std::holds_alternative<glm::vec3>(property->value())) {
                addUnique(property->name() + ".x");
                addUnique(property->name() + ".y");
                addUnique(property->name() + ".z");
            } else {
                addUnique(property->name());
            }
        }
    }
    return paths;
}

PropertyMapping rowToMapping(const std::string& path, const TransferRow& row) {
    PropertyMapping m;
    m.source = PropertyPath::parse(path);
    m.target = PropertyPath::parse(path);
    m.agg = static_cast<PropertyMapping::Aggregate>(row.agg);
    if (row.mode == 2) {
        m.hasExact = true;
        m.exact = row.exact;
    } else if (row.mode == 1) {
        m.transform = CurveModel::polynomial({row.c0, row.c1});
    } else {
        m.transform = CurveModel::polynomial({0.0, 1.0});   // identity
    }
    return m;
}

std::shared_ptr<ObjectConcept> captureWithMappings(
    const std::vector<Object*>& sources, Singular& author) {
    const std::string name = g.nameBuf[0] ? g.nameBuf : "Concept";
    auto concept = ObjectConcept::captureFrom(sources, name, &author);
    for (const auto& entry : g.rows) {
        if (entry.second.on) concept->addMapping(rowToMapping(entry.first, entry.second));
    }
    ConceptRegistry::instance().add(concept);
    return concept;
}

glm::mat4 placementFor(const std::vector<Object*>& sources, Object* selected) {
    glm::vec3 at(0.0f);
    if (selected) {
        at = selected->getPosition();
    } else if (!sources.empty()) {
        for (Object* s : sources) {
            if (s) at += s->getPosition();
        }
        at /= static_cast<float>(sources.size());
    }
    at += glm::vec3(g.offset[0], g.offset[1], g.offset[2]);
    return glm::translate(glm::mat4(1.0f), at);
}

} // namespace

void renderCreationWindow(bool* open, Singular& author, Object* selected, World& world) {
    ImGui::SetNextWindowSize(ImVec2(560, 620), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Creation Console", open)) {
        ImGui::End();
        return;
    }

    // ----------------------------------------------------------------
    // The source set: which beings the new set derives FROM.
    // ----------------------------------------------------------------
    ImGui::TextColored(kHeaderColor, "Source set — what the new set derives from");
    std::vector<Object*> sources = resolveSources();
    if (g.sourceIds.empty()) {
        ImGui::TextDisabled("Empty. Select an object in the world and add it (a set "
                            "of one is fine).");
    }
    std::string removeId;
    for (const auto& id : g.sourceIds) {
        ImGui::PushID(id.c_str());
        const bool alive = findObject(id) != nullptr;
        ImGui::BulletText("%s%s", id.c_str(), alive ? "" : "  (left the world)");
        ImGui::SameLine();
        if (ImGui::SmallButton("remove")) removeId = id;
        ImGui::PopID();
    }
    if (!removeId.empty()) {
        g.sourceIds.erase(std::remove(g.sourceIds.begin(), g.sourceIds.end(), removeId),
                          g.sourceIds.end());
    }
    if (selected) {
        const std::string selId = selected->getIdentifier();
        if (std::find(g.sourceIds.begin(), g.sourceIds.end(), selId) == g.sourceIds.end()) {
            if (ImGui::SmallButton(("+ add selected (" + selId + ")").c_str())) {
                g.sourceIds.push_back(selId);
            }
        }
    } else {
        ImGui::TextDisabled("(select an object in the 3D world to add it)");
    }
    if (!g.sourceIds.empty()) {
        ImGui::SameLine();
        if (ImGui::SmallButton("clear set")) g.sourceIds.clear();
    }
    ImGui::Separator();

    // ----------------------------------------------------------------
    // Property redistribution: pick and choose WHICH properties carry
    // over and HOW they spread across the new set.
    // ----------------------------------------------------------------
    if (!sources.empty()) {
        ImGui::TextColored(kHeaderColor, "Properties — pick what carries over, and how");
        ImGui::TextDisabled("Geometry and poses always carry (the concept's members).");
        static const char* aggNames[] = {"per member", "mean of set", "sum of set",
                                         "max of set"};
        static const char* modeNames[] = {"as is", "linear c0 + c1*x", "exact f(x)"};

        for (const auto& path : unionOfProperties(sources)) {
            TransferRow& row = g.rows[path];
            ImGui::PushID(path.c_str());
            if (ImGui::Checkbox(path.c_str(), &row.on)) {}

            // The Singularity gate, visible while authoring.
            const PropertyPath parsed = PropertyPath::parse(path);
            const bool openGate = TransferPolicy::instance().canTransfer(parsed);
            const auto tier = TransferPolicy::instance().tierOf(parsed.segments[0]);
            ImGui::SameLine(260.0f);
            if (tier == TransferPolicy::Tier::Kernel) {
                ImGui::TextDisabled("kernel");
            } else if (openGate) {
                ImGui::TextDisabled("open");
            } else {
                ImGui::TextColored(kWarnColor, "GATED");
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(tier == TransferPolicy::Tier::Kernel
                                      ? "universally transferable — set at the Singularity "
                                        "level;\nlaws cannot close it"
                                      : openGate
                                            ? "transferable; a law may close this gate\n"
                                              "(@transfer-policy.gate.%s := false)"
                                            : "closed: this transfer will be SKIPPED until "
                                              "a law\nopens @transfer-policy.gate.%s",
                                  parsed.segments[0].c_str());
            }

            if (row.on) {
                if (!openGate) {
                    ImGui::TextColored(kWarnColor,
                                       "  ! gated — will not transfer until a law opens it");
                }
                ImGui::SameLine(330.0f);
                ImGui::SetNextItemWidth(110.0f);
                ImGui::Combo("##agg", &row.agg, aggNames, 4);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(140.0f);
                ImGui::Combo("##mode", &row.mode, modeNames, 3);
                if (row.mode == 1) {
                    ImGui::SetNextItemWidth(80.0f);
                    ImGui::InputDouble("c0", &row.c0);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(80.0f);
                    ImGui::InputDouble("c1", &row.c1);
                } else if (row.mode == 2) {
                    if (!row.exactSeeded) {
                        row.exact = OntoMath::Piecewise::continuous(
                            OntoMath::Expression::variable("x"));
                        row.exactSeeded = true;
                    }
                    // x = the source value; the full exact editor (pieces,
                    // bounds, transcendentals). Undefined transfers nothing.
                    const MathBindings xOnly{{"x", parsed}};
                    MathEd::editPiecewise(row.exact, xOnly);
                }
            }
            ImGui::PopID();
        }
        ImGui::Separator();

        // ----------------------------------------------------------------
        // Create: the word for the thing, and/or the things themselves.
        // ----------------------------------------------------------------
        ImGui::TextColored(kHeaderColor, "Create");
        ImGui::SetNextItemWidth(200.0f);
        ImGui::InputText("Concept name", g.nameBuf, sizeof(g.nameBuf));
        ImGui::SetNextItemWidth(220.0f);
        ImGui::InputFloat3("offset", g.offset);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("where the new set appears, relative to the selected\n"
                              "object (or the source centroid)");
        }

        if (ImGui::Button("Capture concept")) {
            captureWithMappings(sources, author);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("stores the abstraction for later: instantiate it below,\n"
                              "or use its id in a law's SPAWN action (birth-by-law)");
        }
        ImGui::SameLine();
        if (ImGui::Button("Create objects now")) {
            auto concept = captureWithMappings(sources, author);
            auto newborns = concept->instantiate(placementFor(sources, selected), &sources);
            g.lastCreatedIds.clear();
            for (auto& newborn : newborns) {
                g.lastCreatedIds.push_back(newborn->getIdentifier());
                world.addObject(std::move(newborn));
            }
        }
        if (!g.lastCreatedIds.empty()) {
            ImGui::TextColored(kInfoColor, "Created %zu object(s).",
                               g.lastCreatedIds.size());
            ImGui::SameLine();
            if (ImGui::SmallButton("use output as new source set")) {
                g.sourceIds = g.lastCreatedIds;   // set-to-set-to-set...
                g.lastCreatedIds.clear();
            }
        }
        ImGui::Separator();
    }

    // ----------------------------------------------------------------
    // The register of concepts — words already coined.
    // ----------------------------------------------------------------
    ImGui::TextColored(kHeaderColor, "Concepts");
    const auto& concepts = ConceptRegistry::instance().getAll();
    if (concepts.empty()) {
        ImGui::TextDisabled("None captured yet.");
    }
    std::string removeConcept;
    for (const auto& concept : concepts) {
        if (!concept) continue;
        ImGui::PushID(concept->getIdentifier().c_str());
        ImGui::BulletText("%s  [%s]  %zu member(s), %zu mapping(s)",
                          concept->name().c_str(), concept->getIdentifier().c_str(),
                          concept->members().size(), concept->mappings().size());
        ImGui::SameLine();
        if (ImGui::SmallButton("instantiate")) {
            auto newborns = concept->instantiate(
                placementFor(sources, selected), sources.empty() ? nullptr : &sources);
            g.lastCreatedIds.clear();
            for (auto& newborn : newborns) {
                g.lastCreatedIds.push_back(newborn->getIdentifier());
                world.addObject(std::move(newborn));
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("birth at the selection/offset; the current source set\n"
                              "(when present) feeds the mappings");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("delete")) removeConcept = concept->getIdentifier();
        ImGui::PopID();
    }
    if (!removeConcept.empty()) ConceptRegistry::instance().remove(removeConcept);

    ImGui::Separator();
    ImGui::TextDisabled("Concepts save/load with the world. A concept's id works in a "
                        "law's SPAWN action.");
    ImGui::End();
}

} // namespace Rendering
