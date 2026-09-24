// Zones as mathematical bounds, Rung 1 — the locator and its persistence.
//
// docs/plans/ZONES_AS_MATHEMATICAL_BOUNDS_PLAN_2026-09-23.md. Zach, 2026-09-23:
// Zones are bounds in a continuum, and the continuum is "a Dimensional Zone
// which basically represents an entire continuum of a dimension upon which
// values could be had"; and "ownership is already a Relation, not merely an
// arbitrary position in a bound." So the cases that matter:
//
//   * nesting: a being inside Choir is inside Cathedral is inside Earth;
//   * residence ≠ location: a being held in Cathedral's store but standing
//     outside Cathedral's extent is located in Earth only;
//   * a Zone is moved by writing its placement, like any authored property;
//   * a world that authors none of this answers exactly as before (the
//     residence chain), and saves exactly as before (no new keys);
//   * authored mistakes (a `within` cycle) do not hang the locator.
//
// Claude Opus 5.5, session b0dcb70f-a02a-4081-8589-0aae3ab30551, 2026-09-23.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <cstdio>
#include <memory>
#include <string>

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++g_failures;
        std::printf("  FAILED: %s\n", what.c_str());
        return;
    }
    std::printf("  ok: %s\n", what.c_str());
}

// An extent f(axes) authored as OntoMath, the way a Person would.
std::shared_ptr<OntoMath::ScalarField> extentOf(const OntoMath::ScalarForm& form) {
    auto field = std::make_shared<OntoMath::ScalarField>();
    field->mode = OntoMath::ScalarField::EvaluationMode::AST;
    field->astDefinition =
        OntoMath::Piecewise::continuous(OntoMath::MathNode::fromLegacyExpression(form));
    return field;
}

std::string pathOf(const ZoneManager& mgr, const Singular& being) {
    std::string path;
    for (const Zone* z : mgr.locate(being)) {
        if (!path.empty()) path += "/";
        path += z->getIdentifier();
    }
    return path;
}

std::shared_ptr<Object> standing(const std::string& id, float x, float y, float z) {
    auto obj = std::make_shared<Object>();
    obj->setObjectID(id);
    obj->setPosition(glm::vec3(x, y, z));
    return obj;
}

std::string reading(Singular& being, const char* path) {
    PropertyValue v;
    if (!lawGetValue(being, PropertyPath::parse(path), v)) return "<undefined>";
    const auto* s = std::get_if<std::string>(&v);
    return s ? *s : "<not text>";
}

} // namespace

int main() {
    std::printf("Running dimensional zone test...\n");
    Universe::instance().setClock(0.0, 1.0 / 60.0);

    ZoneManager mgr;
    mgr.bindLive();

    // Earth: the continuum. Three axes, each a property path on beings.
    auto earth = std::make_shared<Zone>("earth", "");
    earth->setDynamicProperty("dimension.x", PropertyValue(std::string("position.x")));
    earth->setDynamicProperty("dimension.y", PropertyValue(std::string("position.y")));
    earth->setDynamicProperty("dimension.z", PropertyValue(std::string("position.z")));

    // Cathedral: a ball of radius 10, its frame placed at x = 100 in Earth.
    //   f = x² + y² + z²,  inside ⇔ f ≤ 100
    auto cathedral = std::make_shared<Zone>("cathedral", "");
    cathedral->setParentZone("earth");
    cathedral->setDynamicProperty("placement.x", PropertyValue(100.0));
    cathedral->setExtent(extentOf(OntoMath::ScalarForm::variable("x", 2.0)
                                      .plus(OntoMath::ScalarForm::variable("y", 2.0))
                                      .plus(OntoMath::ScalarForm::variable("z", 2.0))));
    cathedral->setDynamicProperty(Zone::kExtentHi, PropertyValue(100.0));

    // Choir: a slab |x| ≤ 2 of Cathedral's frame, placed at x = 5 within it.
    auto choir = std::make_shared<Zone>("choir", "");
    choir->setParentZone("cathedral");
    choir->setDynamicProperty("placement.x", PropertyValue(5.0));
    choir->setExtent(extentOf(OntoMath::ScalarForm::variable("x")));
    choir->setDynamicProperty(Zone::kExtentLo, PropertyValue(-2.0));
    choir->setDynamicProperty(Zone::kExtentHi, PropertyValue(2.0));

    // Chess: today's kind of Zone. No parent, no axes — its own world.
    auto chess = std::make_shared<Zone>("chess", "");

    mgr.addZone(earth);
    mgr.addZone(cathedral);
    mgr.addZone(choir);
    mgr.addZone(chess);

    auto pilgrim = standing("pilgrim", 104.0f, 0.0f, 0.0f);    // in the choir
    auto stranger = standing("stranger", 50.0f, 0.0f, 0.0f);   // in Earth only
    auto resident = standing("resident", 20.0f, 0.0f, 0.0f);   // held by Cathedral, outside it
    auto pawn = standing("pawn", 3.0f, 0.0f, 3.0f);
    earth->addObject(pilgrim);
    earth->addObject(stranger);
    cathedral->addObject(resident);
    chess->addObject(pawn);

    // ------------------------------------------------------------------
    // 1. The surface is registered and authorable.
    // ------------------------------------------------------------------
    {
        check(earth->isDimensional() && !cathedral->isDimensional(),
              "a Zone with dimension.* axes is Dimensional; one without is not");
        PropertyValue v;
        check(PropertyPath::parse("within").getValue(*choir, v) == PropertyPath::PathResult::Ok &&
                  std::get<std::string>(v) == "cathedral",
              "`within` is a registered path");
        check(lawGetValue(*cathedral, PropertyPath::parse("placement.x"), v),
              "placement.x resolves as a path on the Zone");
        check(mgr.dimensionalRootOf(*choir) == earth.get(), "Choir's continuum is Earth");
    }

    // ------------------------------------------------------------------
    // 2. Location is derived, nests, and is not residence.
    // ------------------------------------------------------------------
    {
        check(pathOf(mgr, *pilgrim) == "earth/cathedral/choir",
              "a being inside Choir is inside Cathedral is inside Earth");
        check(pathOf(mgr, *stranger) == "earth", "a being outside Cathedral is in Earth only");
        check(mgr.residenceOf(*resident) == cathedral.get() &&
                  pathOf(mgr, *resident) == "earth",
              "held in Cathedral's store, standing outside its extent: located in Earth only");
        check(pathOf(mgr, *pawn) == "chess",
              "a Zone with no continuum axes answers as today: the residence");
        // Inside Choir's slab (unbounded in z) but far outside Cathedral's
        // ball: containment nests, so it is NOT in the Choir.
        auto beyond = standing("beyond", 105.0f, 0.0f, 50.0f);
        earth->addObject(beyond);
        check(pathOf(mgr, *beyond) == "earth",
              "inside Choir's own extent but outside Cathedral: not in the Choir");
    }

    // ------------------------------------------------------------------
    // 3. Laws read it.
    // ------------------------------------------------------------------
    {
        check(reading(*pilgrim, "@world.zoneId") == "choir", "@world.zoneId is the innermost Zone");
        check(reading(*pilgrim, "@world.zonePath") == "earth/cathedral/choir",
              "@world.zonePath is the whole chain");
        check(reading(*resident, "@world.dimensionalZoneId") == "earth",
              "@world.dimensionalZoneId is the continuum");
        check(reading(*pawn, "@world.zoneId") == "chess", "and legacy Zones still answer");
    }

    // ------------------------------------------------------------------
    // 4. A Zone moves by writing its placement; its shape comes with it.
    // ------------------------------------------------------------------
    {
        check(lawSetValue(*cathedral, PropertyPath::parse("placement.x"), PropertyValue(50.0)) ==
                  PropertyPath::PathResult::Ok,
              "placement.x is law-writable");
        check(pathOf(mgr, *stranger) == "earth/cathedral", "the stranger is now inside Cathedral");
        check(pathOf(mgr, *pilgrim) == "earth", "and the pilgrim is left outside it");
        lawSetValue(*cathedral, PropertyPath::parse("placement.x"), PropertyValue(100.0));
    }

    // ------------------------------------------------------------------
    // 5. A Person is located by the same bounds.
    // ------------------------------------------------------------------
    {
        Soul soul;
        Body body;
        Person person(soul, body, "walker");
        lawSetValue(person, PropertyPath::parse("position"),
                    PropertyValue(glm::vec3(100.0f, 0.0f, 3.0f)));
        // Present in Earth: the first Zone added is the current one.
        check(pathOf(mgr, person) == "earth/cathedral",
              "a Person standing in the nave is in Cathedral, not in the Choir");
    }

    // ------------------------------------------------------------------
    // 6. A being born this tick is found (the residence index's miss path).
    // ------------------------------------------------------------------
    {
        auto newborn = standing("newborn", 105.0f, 0.0f, 0.0f);
        earth->addObject(newborn);
        check(pathOf(mgr, *newborn) == "earth/cathedral/choir",
              "a being added mid-tick is located without waiting for the clock");
    }

    // ------------------------------------------------------------------
    // 7. Persistence: authored bounds round-trip; unauthored Zones gain no keys.
    // ------------------------------------------------------------------
    {
        const nlohmann::json saved = zoneToJson(*choir);
        auto restored = makeZoneFromJson(saved);
        PropertyValue lo, hi;
        check(restored && restored->getParentZone() == "cathedral" &&
                  restored->placementAlong("x") == 5.0 && restored->extent() &&
                  restored->getDynamicProperty(Zone::kExtentLo, lo) &&
                  restored->getDynamicProperty(Zone::kExtentHi, hi),
              "within, placement, extent and its bounds survive a save");
        std::map<std::string, PropertyValue> inside{{"x", PropertyValue(1.0)}};
        std::map<std::string, PropertyValue> outside{{"x", PropertyValue(3.0)}};
        check(restored && restored->extentHolds(inside) && !restored->extentHolds(outside),
              "and the restored extent means the same thing");
        const nlohmann::json earthSaved = zoneToJson(*earth);
        auto earthBack = makeZoneFromJson(earthSaved);
        check(earthBack && earthBack->dimensionAxes().size() == 3, "axes survive a save");
        const nlohmann::json plain = zoneToJson(*chess);
        check(!plain.contains("dimensions") && !plain.contains("placement") &&
                  !plain.contains("extent"),
              "a Zone that authored none of it saves no new keys");
        Zone copy(*choir);
        check(copy.getParentZone() == "cathedral" && copy.placementAlong("x") == 5.0 &&
                  copy.extent(),
              "copying a Zone keeps its bounds (and its parent, which it used to drop)");
    }

    // ------------------------------------------------------------------
    // 8. Authored mistakes do not hang the locator.
    // ------------------------------------------------------------------
    {
        auto loopA = std::make_shared<Zone>("loop-a", "");
        auto loopB = std::make_shared<Zone>("loop-b", "");
        loopA->setParentZone("loop-b");
        loopB->setParentZone("loop-a");
        mgr.addZone(loopA);
        mgr.addZone(loopB);
        auto lost = standing("lost", 0.0f, 0.0f, 0.0f);
        loopA->addObject(lost);
        const auto located = mgr.locate(*lost);
        check(located.size() <= 2, "a `within` cycle terminates");
    }

    if (g_failures) {
        std::printf("dimensional_zone_test: %d FAILURES\n", g_failures);
        return 1;
    }
    std::printf("dimensional_zone_test: all checks passed\n");
    return 0;
}
