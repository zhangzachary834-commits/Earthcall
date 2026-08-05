// Probe: the load-bearing claims of docs/architecture/AUTHORED_CATEGORIES.md.
//
//   §5  Related is ONE HOP — inherited membership is NOT answered by it.
//   §5a a materialized "in-category" edge answers it in one hop.
//   §2  direction: instance-of points instance -> category, so the test is
//       true OF the instance and false of the category.
//   §7  acyclicity is NOT enforced today: a cycle is accepted without complaint,
//       which is why the doc specifies the check as an obligation.
//   §9a Materials are Law-addressable in principle (buildProperties registers
//       them) but absent from the Universe provider in GameInit.

#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Form/Object/Object.hpp"
#include "Form/Material/Material.hpp"
#include "Relation/RelationManager.hpp"

#include <cassert>
#include <cstdio>
#include <memory>

int main() {
    const ECA::Event probe{};

    // category.furniture  <-- subcategory-of --  category.chair  <-- instance-of -- chair0037
    Object furniture;  furniture.setObjectID("category.furniture");
    Object chairCat;   chairCat.setObjectID("category.chair");
    Object chair0037;  chair0037.setObjectID("chair-0037");

    RelationManager graph;
    graph.add(std::make_shared<Relation>("subcategory-of", chairCat, furniture, true));
    graph.add(std::make_shared<Relation>("instance-of",    chair0037, chairCat,  true));
    Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
        for (const auto& r : graph.getAll()) if (r) out.push_back(r.get());
    });

    // ---- §2: direction ----------------------------------------------------
    assert(ConditionNode::related("instance-of", "category.chair")
               .compile()(probe, chair0037) &&
           "instance-of should hold OF the instance");
    assert(!ConditionNode::related("instance-of", "chair-0037")
               .compile()(probe, chairCat) &&
           "instance-of must NOT hold of the category (direction is honored)");
    std::printf("  direction: instance-of holds of the instance, not the category\n");

    // ---- §5: ONE HOP ------------------------------------------------------
    // The chair IS furniture in the taxonomy. Related will not say so.
    const bool inheritedViaRelated =
        ConditionNode::related("instance-of", "category.furniture")
            .compile()(probe, chair0037);
    assert(!inheritedViaRelated &&
           "Related must NOT traverse subcategory-of; the doc's central claim");

    const bool anyEdgeToFurniture =
        ConditionNode::related("", "category.furniture").compile()(probe, chair0037);
    assert(!anyEdgeToFurniture && "no edge of ANY type reaches furniture from the chair");
    std::printf("  ONE HOP confirmed: chair is-a furniture in the DAG, Related says false\n");

    // ---- §5a: materialize the closure, then ask -------------------------
    graph.add(std::make_shared<Relation>("in-category", chair0037, furniture, true));
    assert(ConditionNode::related("in-category", "category.furniture")
               .compile()(probe, chair0037) &&
           "the materialized edge answers inherited membership in one hop");
    std::printf("  materialized in-category edge answers it in one hop\n");

    // ---- §7: acyclicity is NOT enforced ---------------------------------
    // Close a cycle: furniture subcategory-of chair. Nothing refuses it.
    const std::size_t before = graph.getAll().size();
    graph.add(std::make_shared<Relation>("subcategory-of", furniture, chairCat, true));
    assert(graph.getAll().size() == before + 1 &&
           "the graph accepted a cycle: the §7 check is an obligation, not a guarantee");
    std::printf("  acyclicity NOT enforced by RelationManager (cycle accepted) -> §7 check needed\n");

    // ---- §9a: Material's property surface exists ------------------------
    Material clay("clay");
    clay.baseColor = glm::vec3(0.7f, 0.4f, 0.2f);
    PropertyValue v;
    const auto r = PropertyPath::parse("baseColor").getValue(clay, v);
    assert(r == PropertyPath::PathResult::Ok &&
           "Material::buildProperties registers baseColor");
    assert(clay.getIdentifier() == "material.clay" && "namespaced identity");
    std::printf("  Material surface is built and namespaced (%s.baseColor readable)\n",
                clay.getIdentifier().c_str());

    Universe::instance().setRelationProvider(nullptr);
    std::printf("OK  AUTHORED_CATEGORIES.md §§2,5,5a,7,9a verified against this build\n");
    return 0;
}
