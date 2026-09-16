#!/usr/bin/env python3
from pathlib import Path
import subprocess


def replace_once(text: str, old: str, new: str, label: str) -> tuple[str, bool]:
    count = text.count(old)
    if count == 1:
        return text.replace(old, new, 1), True
    if new in text:
        return text, False
    raise SystemExit(f"{label}: expected one old block or already-patched new block; old count={count}")

changed_paths = []

# 1. Tighten SmoothSurface legacy recovery so the payload itself must describe
# the SAME named analytic kind as the semantic ShapeKind. Otherwise a Sphere
# shell plus stale Torus matter would recreate the split-brain state this PR is
# meant to eliminate.
zone_path = Path("src/ZonesOfEarth/ZoneManager.cpp")
zone = zone_path.read_text()
old_smooth_valid = '''            const bool enumOk =
                rawModel >= static_cast<int>(geom::SmoothSurfaceData::Model::Quadric) &&
                rawModel <= static_cast<int>(geom::SmoothSurfaceData::Model::Parametric) &&
                rawForm >= static_cast<int>(geom::SmoothSurfaceData::QuadricForm::Sphere) &&
                rawForm <= static_cast<int>(geom::SmoothSurfaceData::QuadricForm::Paraboloid) &&
                rawPkind >= static_cast<int>(geom::SmoothSurfaceData::ParametricKind::Torus) &&
                rawPkind <= static_cast<int>(geom::SmoothSurfaceData::ParametricKind::ProjectivePlane);
            bool valid = enumOk && sm->quadric_matrix()->size() == 16;
'''
new_smooth_valid = '''            const bool enumOk =
                rawModel >= static_cast<int>(geom::SmoothSurfaceData::Model::Quadric) &&
                rawModel <= static_cast<int>(geom::SmoothSurfaceData::Model::Parametric) &&
                rawForm >= static_cast<int>(geom::SmoothSurfaceData::QuadricForm::Sphere) &&
                rawForm <= static_cast<int>(geom::SmoothSurfaceData::QuadricForm::Paraboloid) &&
                rawPkind >= static_cast<int>(geom::SmoothSurfaceData::ParametricKind::Torus) &&
                rawPkind <= static_cast<int>(geom::SmoothSurfaceData::ParametricKind::ProjectivePlane);
            const Object::ShapeKind semanticKind = o->getShapeKind();
            const bool kindMatches =
                (semanticKind == Object::ShapeKind::Sphere &&
                 rawModel == static_cast<int>(geom::SmoothSurfaceData::Model::Quadric) &&
                 rawForm == static_cast<int>(geom::SmoothSurfaceData::QuadricForm::Sphere)) ||
                (semanticKind == Object::ShapeKind::Ellipsoid &&
                 rawModel == static_cast<int>(geom::SmoothSurfaceData::Model::Quadric) &&
                 rawForm == static_cast<int>(geom::SmoothSurfaceData::QuadricForm::Ellipsoid)) ||
                (semanticKind == Object::ShapeKind::Paraboloid &&
                 rawModel == static_cast<int>(geom::SmoothSurfaceData::Model::Quadric) &&
                 rawForm == static_cast<int>(geom::SmoothSurfaceData::QuadricForm::Paraboloid)) ||
                (semanticKind == Object::ShapeKind::Torus &&
                 rawModel == static_cast<int>(geom::SmoothSurfaceData::Model::Parametric) &&
                 rawPkind == static_cast<int>(geom::SmoothSurfaceData::ParametricKind::Torus)) ||
                (semanticKind == Object::ShapeKind::Ovoid &&
                 rawModel == static_cast<int>(geom::SmoothSurfaceData::Model::Parametric) &&
                 rawPkind == static_cast<int>(geom::SmoothSurfaceData::ParametricKind::Ovoid));
            bool valid = enumOk && kindMatches && sm->quadric_matrix()->size() == 16;
'''
zone, did = replace_once(zone, old_smooth_valid, new_smooth_valid, "smooth semantic-kind match")
if did:
    changed_paths.append(str(zone_path))

old_expr_check = '''            if (root->expr()) node.expr = root->expr()->str();
            if (valid && prim == geom::SdfPrim::Expr && node.expr.empty()) valid = false;
'''
new_expr_check = '''            if (root->expr()) node.expr = root->expr()->str();
            if (valid && prim == geom::SdfPrim::Expr) {
                if (node.expr.empty() || geom::compileExpr(node.expr).empty()) valid = false;
            }
'''
zone, did_expr = replace_once(zone, old_expr_check, new_expr_check, "legacy Expr validation")
if did_expr and str(zone_path) not in changed_paths:
    changed_paths.append(str(zone_path))
if did or did_expr:
    zone_path.write_text(zone)

# 2. Add two adversarial witnesses to the existing raw-FlatBuffer section:
# valid-but-mismatched smooth topology and syntactically invalid Expr matter.
test_path = Path("tests/constructed-being/shape_hydration_integrity_test.cpp")
test = test_path.read_text()

old_field_entity = '''        auto badField = Earthcall::Schema::CreateFieldData(builder, &extent, badSdf);
        entities.push_back(bareMatterEntity(
            builder, "bad-field-enum", zoneId, 0, 0, badField));

        std::vector<float> identityQ{
'''
new_field_entity = '''        auto badField = Earthcall::Schema::CreateFieldData(builder, &extent, badSdf);
        entities.push_back(bareMatterEntity(
            builder, "bad-field-enum", zoneId, 0, 0, badField));

        auto invalidExprText = builder.CreateString("this is not a valid implicit expression");
        auto badExprNode = Earthcall::Schema::CreateSdfNode(
            builder,
            static_cast<int>(geom::SdfPrim::Expr),
            static_cast<int>(geom::SdfOp::Leaf),
            0, 0, 0,
            &dims,
            &offset,
            0.0f, 0.0f, 0.5f,
            invalidExprText);
        auto badExprField = Earthcall::Schema::CreateFieldData(builder, &extent, badExprNode);
        entities.push_back(bareMatterEntity(
            builder, "bad-field-expr", zoneId, 0, 0, badExprField));

        std::vector<float> identityQ{
'''
test, did_test1 = replace_once(test, old_field_entity, new_field_entity, "invalid Expr matter witness")

old_bad_smooth_entity = '''        entities.push_back(bareMatterEntity(
            builder, "bad-smooth-enum", zoneId, 0, badSmooth));

        const std::vector<uint8_t> badMatter = finishMatter(builder, entities);
'''
new_bad_smooth_entity = '''        entities.push_back(bareMatterEntity(
            builder, "bad-smooth-enum", zoneId, 0, badSmooth));

        auto validButWrongTorus = Earthcall::Schema::CreateSmoothSurfaceData(
            builder,
            true, true, false, false,
            static_cast<int>(geom::SmoothSurfaceData::Model::Parametric),
            q,
            static_cast<int>(geom::SmoothSurfaceData::QuadricForm::Sphere),
            static_cast<int>(geom::SmoothSurfaceData::ParametricKind::Torus),
            &axes,
            -0.5f, 0.5f,
            smoothParams);
        entities.push_back(bareMatterEntity(
            builder, "mismatched-smooth-kind", zoneId, 0, validButWrongTorus));

        const std::vector<uint8_t> badMatter = finishMatter(builder, entities);
'''
test, did_test2 = replace_once(test, old_bad_smooth_entity, new_bad_smooth_entity, "smooth kind mismatch witness")

old_targets = '''        auto badFieldTarget = std::make_shared<Object>("bad-field-enum");
        badFieldTarget->setShape(Object::ShapeKind::Field);
        zone->addObject(badFieldTarget);

        auto badSmoothTarget = std::make_shared<Object>("bad-smooth-enum");
        badSmoothTarget->setShape(Object::ShapeKind::Sphere);
        badSmoothTarget->clearTopologyModel();
        zone->addObject(badSmoothTarget);

        ZoneManager reader;
'''
new_targets = '''        auto badFieldTarget = std::make_shared<Object>("bad-field-enum");
        badFieldTarget->setShape(Object::ShapeKind::Field);
        zone->addObject(badFieldTarget);

        auto badExprTarget = std::make_shared<Object>("bad-field-expr");
        badExprTarget->setShape(Object::ShapeKind::Field);
        zone->addObject(badExprTarget);

        auto badSmoothTarget = std::make_shared<Object>("bad-smooth-enum");
        badSmoothTarget->setShape(Object::ShapeKind::Sphere);
        badSmoothTarget->clearTopologyModel();
        zone->addObject(badSmoothTarget);

        auto mismatchedSmoothTarget = std::make_shared<Object>("mismatched-smooth-kind");
        mismatchedSmoothTarget->setShape(Object::ShapeKind::Sphere);
        mismatchedSmoothTarget->clearTopologyModel();
        zone->addObject(mismatchedSmoothTarget);

        ZoneManager reader;
'''
test, did_test3 = replace_once(test, old_targets, new_targets, "malformed matter targets")

old_checks = '''        check(badFieldTarget->getShapeKind() == Object::ShapeKind::Field &&
                  !badFieldTarget->hasField(),
              "invalid matter SdfPrim/SdfOp ordinals never enter runtime geometry");
        check(badSmoothTarget->getShapeKind() == Object::ShapeKind::Sphere &&
                  !badSmoothTarget->hasSmoothSurface(),
              "invalid matter SmoothSurface enum ordinals never enter runtime geometry");
'''
new_checks = '''        check(badFieldTarget->getShapeKind() == Object::ShapeKind::Field &&
                  !badFieldTarget->hasField(),
              "invalid matter SdfPrim/SdfOp ordinals never enter runtime geometry");
        check(badExprTarget->getShapeKind() == Object::ShapeKind::Field &&
                  !badExprTarget->hasField(),
              "unparseable legacy Expr matter is refused instead of hydrating an inert Field");
        check(badSmoothTarget->getShapeKind() == Object::ShapeKind::Sphere &&
                  !badSmoothTarget->hasSmoothSurface(),
              "invalid matter SmoothSurface enum ordinals never enter runtime geometry");
        check(mismatchedSmoothTarget->getShapeKind() == Object::ShapeKind::Sphere &&
                  !mismatchedSmoothTarget->hasSmoothSurface(),
              "valid Torus matter cannot recover into a semantic Sphere shell");
'''
test, did_test4 = replace_once(test, old_checks, new_checks, "new malformed matter assertions")

if any((did_test1, did_test2, did_test3, did_test4)):
    test_path.write_text(test)
    changed_paths.append(str(test_path))

subprocess.run(["git", "diff", "--check"], check=True)
if changed_paths:
    subprocess.run(["git", "add", *changed_paths], check=True)
    subprocess.run(["git", "config", "user.name", "GPT-5.6 Sol"], check=True)
    subprocess.run([
        "git", "config", "user.email",
        "41898282+github-actions[bot]@users.noreply.github.com"
    ], check=True)
    subprocess.run([
        "git", "commit", "-m", "Require legacy smooth matter to match semantic kind"
    ], check=True)
    subprocess.run([
        "git", "push", "origin", "HEAD:sol/shape-hydration-integrity-20260916"
    ], check=True)
else:
    print("final shape guard patch already landed")
