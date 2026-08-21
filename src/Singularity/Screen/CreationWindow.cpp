#include "Singularity/Screen/CreationWindow.hpp"

#include "Singularity/Screen/MathEditors.hpp"
#include "ConstructedBeing/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Object/Object.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

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

// WHAT a being is, at a glance — bare ids hid a 100x1x100 ground slab in a
// source set once. Never again. Beings without a body say their KIND instead
// of a size they do not have.
std::string describeBeing(Singular& being) {
    auto* o = dynamic_cast<Object*>(&being);
    if (!o) {
        using K = ConditionNode::BeingKind;
        static const std::pair<K, const char*> kNames[] = {
            {K::Person, "Person"},       {K::Relation, "Relation"},
            {K::Formation, "Formation"}, {K::Lexeme, "Lexeme"}};
        for (const auto& named : kNames) {
            if (ConditionNode::matchesKind(being, named.first)) return named.second;
        }
        return "being";
    }
    static const char* kindNames[] = {"Cube",      "Polyhedron", "Sphere",
                                      "Cylinder",  "Cone",       "Ellipsoid",
                                      "Ovoid",     "Paraboloid", "Torus",
                                      "RoundedBox", "Field",     "Patch"};
    const int k = static_cast<int>(o->getShapeKind());
    std::string out = (k >= 0 && k < 12) ? kindNames[k] : "Shape";
    const glm::mat4& t = o->getTransform();
    char buf[64];
    std::snprintf(buf, sizeof(buf), " ~%.1fx%.1fx%.1f", glm::length(glm::vec3(t[0])),
                  glm::length(glm::vec3(t[1])), glm::length(glm::vec3(t[2])));
    out += buf;
    return out;
}

// ANY being may be a source. The console used to resolve source ids through a
// dynamic_cast to Object and drop whatever was not one, so a Person or a Zone
// added to a set silently vanished from it — the manifesto's layers 4 and 5
// were unreachable from the one surface a Person authors through.
Singular* findBeing(const std::string& id) {
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == id) return being;
    }
    return nullptr;
}

std::vector<Singular*> resolveSources() {
    std::vector<Singular*> sources;
    for (const auto& id : g.sourceIds) {
        if (Singular* being = findBeing(id)) sources.push_back(being);
    }
    return sources;
}

// The union of the LIVE source set's property vocabulary — the actual beings
// offer their properties, not prototypes.
std::vector<std::string> unionOfProperties(const std::vector<Singular*>& sources) {
    std::vector<std::string> paths;
    auto addUnique = [&](const std::string& p) {
        if (std::find(paths.begin(), paths.end(), p) == paths.end()) paths.push_back(p);
    };
    for (Singular* source : sources) {
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
    const std::vector<Singular*>& sources, Singular& author) {
    const std::string name = g.nameBuf[0] ? g.nameBuf : "Concept";
    auto concept = ObjectConcept::captureFromBeings(sources, name, &author);
    for (const auto& entry : g.rows) {
        if (entry.second.on) concept->addMapping(rowToMapping(entry.first, entry.second));
    }
    ConceptRegistry::instance().add(concept);
    return concept;
}

glm::mat4 placementFor(const std::vector<Singular*>& sources, Object* selected) {
    glm::vec3 at(0.0f);
    if (selected) {
        at = selected->getPosition();
    } else {
        // Averaged over the members that HAVE a place: a Relation in the set
        // occupies no point, and folding in an origin it never held would drag
        // the placement off the beings the author can see.
        int placed = 0;
        for (Singular* s : sources) {
            if (auto* body = dynamic_cast<Object*>(s)) {
                at += body->getPosition();
                ++placed;
            }
        }
        if (placed > 0) at /= static_cast<float>(placed);
    }
    at += glm::vec3(g.offset[0], g.offset[1], g.offset[2]);
    return glm::translate(glm::mat4(1.0f), at);
}

} // namespace

void renderCreationWindow(bool* open, Singular& author, Object* selected, Zone& zone) {
    ImGui::SetNextWindowSize(ImVec2(560, 620), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Singular Set-to-Set Creation", open)) {
        ImGui::End();
        return;
    }

    // ----------------------------------------------------------------
    // The source set: which beings the new set derives FROM.
    // ----------------------------------------------------------------
    ImGui::TextColored(kHeaderColor, "Source set — what the new set derives from");
    std::vector<Singular*> sources = resolveSources();
    if (g.sourceIds.empty()) {
        ImGui::TextDisabled("Empty. Select an object in the world and add it (a set "
                            "of one is fine).");
    }
    std::string removeId;
    for (const auto& id : g.sourceIds) {
        ImGui::PushID(id.c_str());
        Singular* live = findBeing(id);
        if (live) {
            ImGui::BulletText("%s — %s", id.c_str(), describeBeing(*live).c_str());
        } else {
            ImGui::BulletText("%s  (left the world)", id.c_str());
        }
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
            if (ImGui::SmallButton(("+ add selected: " + selId + " — " +
                                    describeBeing(*selected))
                                       .c_str())) {
                g.sourceIds.push_back(selId);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("check this IS the object you mean — a click that "
                                  "misses a small\nobject often selects the ground "
                                  "behind it");
            }
        }
    } else {
        ImGui::TextDisabled("(select an object in the 3D world to add it)");
    }
    if (!g.sourceIds.empty()) {
        ImGui::SameLine();
        if (ImGui::SmallButton("clear set")) g.sourceIds.clear();
    }

    // Beings the 3D view cannot select, because they have no body to click:
    // Persons, Relations, Formations, Zones. They carry property surfaces like
    // anything else and may be sources like anything else — without this list
    // the only way into a source set was a mouse ray, so half the beings in
    // the world were unreachable from the surface a Person authors through.
    if (ImGui::TreeNode("Add a being without a body (Person, Zone, Relation…)")) {
        bool anyOffered = false;
        for (Singular* being : Universe::instance().beings()) {
            if (!being || dynamic_cast<Object*>(being)) continue;   // clickable already
            const std::string id = being->getIdentifier();
            if (std::find(g.sourceIds.begin(), g.sourceIds.end(), id) != g.sourceIds.end()) {
                continue;
            }
            anyOffered = true;
            ImGui::PushID(id.c_str());
            if (ImGui::SmallButton(("+ " + id + " — " + describeBeing(*being)).c_str())) {
                g.sourceIds.push_back(id);
            }
            ImGui::PopID();
        }
        if (!anyOffered) ImGui::TextDisabled("none in the world right now");
        ImGui::TextDisabled("a Person may be a SOURCE; a Person is never a birth");
        ImGui::TreePop();
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
                        row.exact = OntoMath::Piecewise::continuous(OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::variable("x")));
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
                zone.addObject(std::move(newborn));
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
        ImGui::BulletText("%s  [%s]  %zu member(s), %zu mapping(s), %zu relation(s)",
                          concept->name().c_str(), concept->getIdentifier().c_str(),
                          concept->members().size(), concept->mappings().size(),
                          concept->relationTemplates().size());
        ImGui::SameLine();
        if (ImGui::SmallButton("instantiate")) {
            auto newborns = concept->instantiate(
                placementFor(sources, selected), sources.empty() ? nullptr : &sources);
            g.lastCreatedIds.clear();
            for (auto& newborn : newborns) {
                g.lastCreatedIds.push_back(newborn->getIdentifier());
                zone.addObject(std::move(newborn));
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
