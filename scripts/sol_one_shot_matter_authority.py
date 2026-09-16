#!/usr/bin/env python3
from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> tuple[str, bool]:
    count = text.count(old)
    if count == 1:
        return text.replace(old, new, 1), True
    if new in text:
        return text, False
    raise SystemExit(f"{label}: expected one old block or an already-patched new block; old count={count}")


zone_path = Path("src/ZonesOfEarth/ZoneManager.cpp")
text = zone_path.read_text()
changed = False

anchor = '''    // Pass 2: apply fields, but only for entities whose composite key was
    // unique in this buffer. Sol's Invariant 3: "do not use iteration
    // order, unordered_map replacement, or last-record-wins anywhere."
    std::unordered_set<std::string> loggedDuplicates;
'''
replacement = '''    // Pass 2: apply fields, but only for entities whose composite key was
    // unique in this buffer. Sol's Invariant 3: "do not use iteration
    // order, unordered_map replacement, or last-record-wins anywhere."
    //
    // IMPORTANT: authored semantic topology is already hydrated before this
    // sidecar is applied. Matter may recover topology that an old semantic
    // record genuinely lacks, but it may never redefine an already-authored
    // representation. The checks below therefore ask both WHAT the semantic
    // ShapeKind says the being is and WHETHER that representation is already
    // complete before admitting legacy matter as a compatibility fill.
    const auto checkedSdfPrim = [](int raw, geom::SdfPrim& out) {
        const int first = static_cast<int>(geom::SdfPrim::Sphere);
        const int last = static_cast<int>(geom::SdfPrim::Convex);
        if (raw < first || raw > last) return false;
        out = static_cast<geom::SdfPrim>(raw);
        return true;
    };
    const auto checkedSdfOp = [](int raw, geom::SdfOp& out) {
        const int first = static_cast<int>(geom::SdfOp::Leaf);
        const int last = static_cast<int>(geom::SdfOp::SmoothUnion);
        if (raw < first || raw > last) return false;
        out = static_cast<geom::SdfOp>(raw);
        return true;
    };
    const auto smoothKind = [](Object::ShapeKind kind) {
        switch (kind) {
            case Object::ShapeKind::Sphere:
            case Object::ShapeKind::Ellipsoid:
            case Object::ShapeKind::Ovoid:
            case Object::ShapeKind::Paraboloid:
            case Object::ShapeKind::Torus:
                return true;
            default:
                return false;
        }
    };
    const auto finiteVec3 = [](const glm::vec3& v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    };

    std::unordered_set<std::string> loggedDuplicates;
'''
text, did = replace_once(text, anchor, replacement, "pass-2 helpers")
changed |= did

old_poly = '''        // 2. Polyhedron
        if (entity->polyhedron() && entity->polyhedron()->vertices() && entity->polyhedron()->face_data() && entity->polyhedron()->face_offsets()) {
            const auto* poly = entity->polyhedron();
            std::vector<glm::vec3> verts;
            verts.reserve(poly->vertices()->size());
            for (const auto* v : *poly->vertices()) {
                verts.emplace_back(v->x(), v->y(), v->z());
            }

            const auto* fData = poly->face_data();
            const auto* fOffsets = poly->face_offsets();
            std::vector<std::vector<int>> faces;
            if (fOffsets->size() >= 2) {
                faces.reserve(fOffsets->size() - 1);
                for (size_t i = 0; i + 1 < fOffsets->size(); ++i) {
                    int start = fOffsets->Get(i);
                    int end = fOffsets->Get(i + 1);
                    std::vector<int> face;
                    face.reserve(end - start);
                    for (int fi = start; fi < end && fi < (int)fData->size(); ++fi) {
                        face.push_back(fData->Get(fi));
                    }
                    faces.push_back(std::move(face));
                }
            }
            if (!verts.empty() && !faces.empty()) {
                o->setPolyhedronData(PolyhedronData::createCustomPolyhedron(verts, faces));
            }
        }
'''
new_poly = '''        // 2. Polyhedron. The sidecar may fill a legacy Polyhedron whose
        // semantic identity names Polyhedron but has no topology. It may not
        // turn some other authored shape INTO a Polyhedron, nor replace a
        // semantic Polyhedron that already carries vertices/faces.
        if (entity->polyhedron() && entity->polyhedron()->vertices() &&
            entity->polyhedron()->face_data() && entity->polyhedron()->face_offsets()) {
            if (o->getShapeKind() == Object::ShapeKind::Polyhedron &&
                (o->getPolyhedronData().vertices.empty() || o->getPolyhedronData().faces.empty())) {
                const auto* poly = entity->polyhedron();
                const auto* fData = poly->face_data();
                const auto* fOffsets = poly->face_offsets();
                bool valid = poly->vertices()->size() >= 3 && fOffsets->size() >= 2;

                std::vector<glm::vec3> verts;
                verts.reserve(poly->vertices()->size());
                for (const auto* v : *poly->vertices()) {
                    if (!v) { valid = false; break; }
                    const glm::vec3 p(v->x(), v->y(), v->z());
                    if (!finiteVec3(p)) { valid = false; break; }
                    verts.push_back(p);
                }

                std::vector<std::vector<int>> faces;
                if (valid) {
                    // A flattened face stream is accepted only when offsets
                    // form an exact partition [0, face_data.size()]. This
                    // proves end-start before reserve() and prevents negative,
                    // non-monotone, truncated, or overrun ranges.
                    valid = fOffsets->Get(0) == 0 &&
                            fOffsets->Get(fOffsets->size() - 1) ==
                                static_cast<int>(fData->size());
                }
                if (valid) {
                    faces.reserve(fOffsets->size() - 1);
                    for (size_t faceIndex = 0; faceIndex + 1 < fOffsets->size(); ++faceIndex) {
                        const int start = fOffsets->Get(faceIndex);
                        const int end = fOffsets->Get(faceIndex + 1);
                        if (start < 0 || end < start ||
                            end > static_cast<int>(fData->size()) || end - start < 3) {
                            valid = false;
                            break;
                        }
                        std::vector<int> face;
                        face.reserve(static_cast<size_t>(end - start));
                        for (int fi = start; fi < end; ++fi) {
                            const int vertex = fData->Get(fi);
                            if (vertex < 0 || vertex >= static_cast<int>(verts.size())) {
                                valid = false;
                                break;
                            }
                            face.push_back(vertex);
                        }
                        if (!valid) break;
                        faces.push_back(std::move(face));
                    }
                }
                if (valid && !verts.empty() && !faces.empty()) {
                    o->setPolyhedronData(PolyhedronData::createCustomPolyhedron(verts, faces));
                } else {
                    std::cerr << "[ZoneManager] applyMatterFlatBuffer: rejected malformed "
                                 "legacy Polyhedron matter for '" << key << "'.\\n";
                }
            }
        }
'''
text, did = replace_once(text, old_poly, new_poly, "polyhedron matter block")
changed |= did

old_patch = '''        // 3. Bezier Patch
        if (entity->patch() && entity->patch()->ctrl()) {
            geom::BezierPatch patch;
            patch.du = entity->patch()->du();
            patch.dv = entity->patch()->dv();
            patch.ctrl.reserve(entity->patch()->ctrl()->size());
            for (const auto* c : *entity->patch()->ctrl()) {
                patch.ctrl.emplace_back(c->x(), c->y(), c->z());
            }
            if (patch.valid()) {
                o->setBezierPatch(patch);
            }
        }
'''
new_patch = '''        // 3. Bezier Patch. Same compatibility rule: only fill a semantic
        // Patch shell. A stale patch payload is never permission to reclassify
        // the current authored being.
        if (entity->patch() && entity->patch()->ctrl() &&
            o->getShapeKind() == Object::ShapeKind::Patch && !o->hasPatch()) {
            geom::BezierPatch patch;
            patch.du = entity->patch()->du();
            patch.dv = entity->patch()->dv();
            patch.ctrl.reserve(entity->patch()->ctrl()->size());
            bool finite = true;
            for (const auto* c : *entity->patch()->ctrl()) {
                if (!c) { finite = false; break; }
                const glm::vec3 p(c->x(), c->y(), c->z());
                if (!finiteVec3(p)) { finite = false; break; }
                patch.ctrl.push_back(p);
            }
            if (finite && patch.valid()) {
                o->setBezierPatch(patch);
            } else {
                std::cerr << "[ZoneManager] applyMatterFlatBuffer: rejected malformed "
                             "legacy Patch matter for '" << key << "'.\\n";
            }
        }
'''
text, did = replace_once(text, old_patch, new_patch, "patch matter block")
changed |= did

old_smooth = '''        // 4. Smooth Surface
        if (entity->smooth_data() && entity->smooth_data()->quadric_matrix()) {
            const auto* sm = entity->smooth_data();
            geom::SmoothSurfaceData sd;
            sd.closed = sm->closed();
            sd.orientable = sm->orientable();
            sd.hasBoundary = sm->has_boundary();
            sd.isVolume = sm->is_volume();
            sd.model = static_cast<geom::SmoothSurfaceData::Model>(sm->model());
            if (sm->quadric_matrix()->size() == 16) {
                std::vector<float> qv(sm->quadric_matrix()->begin(), sm->quadric_matrix()->end());
                sd.Q = vectorToMat4(qv);
            }
            sd.form = static_cast<geom::SmoothSurfaceData::QuadricForm>(sm->quadric_form());
            sd.pkind = static_cast<geom::SmoothSurfaceData::ParametricKind>(sm->parametric_kind());
            if (sm->axes()) {
                sd.axes = glm::vec3(sm->axes()->x(), sm->axes()->y(), sm->axes()->z());
            }
            sd.zTrim = glm::vec2(sm->z_trim_min(), sm->z_trim_max());
            if (sm->params()) {
                sd.params.assign(sm->params()->begin(), sm->params()->end());
            }
            o->setSmoothSurface(sd);
        }
'''
new_smooth = '''        // 4. Smooth Surface. Named analytic kinds rebuild their semantic
        // surface from ShapeParams during JSON hydration. Matter is therefore
        // only a legacy recovery source when the ShapeKind itself agrees AND
        // the semantic topology is actually missing.
        if (entity->smooth_data() && entity->smooth_data()->quadric_matrix() &&
            smoothKind(o->getShapeKind()) && !o->hasSmoothSurface()) {
            const auto* sm = entity->smooth_data();
            const int rawModel = sm->model();
            const int rawForm = sm->quadric_form();
            const int rawPkind = sm->parametric_kind();
            const bool enumOk =
                rawModel >= static_cast<int>(geom::SmoothSurfaceData::Model::Quadric) &&
                rawModel <= static_cast<int>(geom::SmoothSurfaceData::Model::Parametric) &&
                rawForm >= static_cast<int>(geom::SmoothSurfaceData::QuadricForm::Sphere) &&
                rawForm <= static_cast<int>(geom::SmoothSurfaceData::QuadricForm::Paraboloid) &&
                rawPkind >= static_cast<int>(geom::SmoothSurfaceData::ParametricKind::Torus) &&
                rawPkind <= static_cast<int>(geom::SmoothSurfaceData::ParametricKind::ProjectivePlane);
            bool valid = enumOk && sm->quadric_matrix()->size() == 16;

            geom::SmoothSurfaceData sd;
            sd.closed = sm->closed();
            sd.orientable = sm->orientable();
            sd.hasBoundary = sm->has_boundary();
            sd.isVolume = sm->is_volume();
            if (valid) {
                sd.model = static_cast<geom::SmoothSurfaceData::Model>(rawModel);
                std::vector<float> qv(sm->quadric_matrix()->begin(), sm->quadric_matrix()->end());
                for (float q : qv) valid = valid && std::isfinite(q);
                if (valid) sd.Q = vectorToMat4(qv);
                sd.form = static_cast<geom::SmoothSurfaceData::QuadricForm>(rawForm);
                sd.pkind = static_cast<geom::SmoothSurfaceData::ParametricKind>(rawPkind);
            }
            if (sm->axes()) {
                sd.axes = glm::vec3(sm->axes()->x(), sm->axes()->y(), sm->axes()->z());
                valid = valid && finiteVec3(sd.axes);
            }
            sd.zTrim = glm::vec2(sm->z_trim_min(), sm->z_trim_max());
            valid = valid && std::isfinite(sd.zTrim.x) && std::isfinite(sd.zTrim.y);
            if (sm->params()) {
                sd.params.assign(sm->params()->begin(), sm->params()->end());
                for (float p : sd.params) valid = valid && std::isfinite(p);
            }
            if (valid) {
                o->setSmoothSurface(sd);
            } else {
                std::cerr << "[ZoneManager] applyMatterFlatBuffer: rejected malformed "
                             "legacy SmoothSurface matter for '" << key << "'.\\n";
            }
        }
'''
text, did = replace_once(text, old_smooth, new_smooth, "smooth matter block")
changed |= did

old_field = '''        // 5. Field Shape
        if (entity->field() && entity->field()->root_node()) {
            const auto* fbsField = entity->field();
            const auto* root = fbsField->root_node();
            geom::SdfNode node;
            node.prim = static_cast<geom::SdfPrim>(root->type());
            node.op = static_cast<geom::SdfOp>(root->operation());
            if (root->dims()) node.dims = glm::vec3(root->dims()->x(), root->dims()->y(), root->dims()->z());
            if (root->offset()) node.offset = glm::vec3(root->offset()->x(), root->offset()->y(), root->offset()->z());
            node.p0 = root->p0();
            node.p1 = root->p1();
            node.t = root->t();
            if (root->expr()) node.expr = root->expr()->str();
            glm::vec3 extent(1.0f);
            if (fbsField->extent()) extent = glm::vec3(fbsField->extent()->x(), fbsField->extent()->y(), fbsField->extent()->z());
            o->setFieldShape(node, extent);
        }
'''
new_field = '''        // 5. Field Shape. A semantic Field includes BOTH its mathematical
        // tree and its evaluation extent. If it already exists, matter is not
        // consulted at all: refusing a shallow tree but accepting a stale
        // extent would still let the cache clip/expand the authored form.
        if (entity->field() && entity->field()->root_node() &&
            o->getShapeKind() == Object::ShapeKind::Field && !o->hasField()) {
            const auto* fbsField = entity->field();
            const auto* root = fbsField->root_node();
            geom::SdfPrim prim = geom::SdfPrim::Sphere;
            geom::SdfOp op = geom::SdfOp::Leaf;
            bool valid = checkedSdfPrim(root->type(), prim) &&
                         checkedSdfOp(root->operation(), op);

            // Current .ecmatter stores only ONE root. It can honestly recover
            // a leaf, but not a boolean/morph tree (children are absent) and
            // not a Convex leaf (planes are absent). Refuse rather than invent.
            if (valid && (op != geom::SdfOp::Leaf || prim == geom::SdfPrim::Convex)) {
                valid = false;
            }

            geom::SdfNode node;
            node.prim = prim;
            node.op = op;
            if (root->dims()) node.dims = glm::vec3(root->dims()->x(), root->dims()->y(), root->dims()->z());
            if (root->offset()) node.offset = glm::vec3(root->offset()->x(), root->offset()->y(), root->offset()->z());
            valid = valid && finiteVec3(node.dims) && finiteVec3(node.offset);
            node.p0 = root->p0();
            node.p1 = root->p1();
            node.t = root->t();
            valid = valid && std::isfinite(node.p0) && std::isfinite(node.p1) && std::isfinite(node.t);
            if (root->expr()) node.expr = root->expr()->str();
            if (valid && prim == geom::SdfPrim::Expr && node.expr.empty()) valid = false;

            glm::vec3 extent(1.0f);
            if (fbsField->extent()) {
                extent = glm::vec3(fbsField->extent()->x(), fbsField->extent()->y(), fbsField->extent()->z());
            }
            valid = valid && finiteVec3(extent) &&
                    extent.x > 0.0f && extent.y > 0.0f && extent.z > 0.0f;

            if (valid) {
                o->setFieldShape(node, extent);
            } else {
                std::cerr << "[ZoneManager] applyMatterFlatBuffer: rejected lossy or malformed "
                             "legacy Field matter for '" << key << "'.\\n";
            }
        }
'''
text, did = replace_once(text, old_field, new_field, "field matter block")
changed |= did

if changed:
    zone_path.write_text(text)

obj_path = Path("src/ConstructedBeing/Singular/Object/Object.hpp")
obj = obj_path.read_text()
old_extent_guard = '''        if (existingComplete &&
            (incomingOperatorShell || incomingExprShell || incomingConvexShell)) {
            _fieldExtent = extent;
            rebuildGeometryCaches();
            std::cerr << "[Object] setFieldShape: refused a lossy field shell over an "
                         "already-complete authored SDF; semantic shape remains authoritative.\\n";
            return;
        }
'''
new_extent_guard = '''        if (existingComplete &&
            (incomingOperatorShell || incomingExprShell || incomingConvexShell)) {
            // The evaluation extent is part of the authored Field too. A
            // rejected lossy replacement does not get to clip or expand the
            // surviving mathematical form as a side effect.
            std::cerr << "[Object] setFieldShape: refused a lossy field shell over an "
                         "already-complete authored SDF; semantic shape and extent remain authoritative.\\n";
            return;
        }
'''
obj, obj_changed = replace_once(obj, old_extent_guard, new_extent_guard, "Object field extent guard")
if obj_changed:
    obj_path.write_text(obj)

print("patched" if (changed or obj_changed) else "already patched")
