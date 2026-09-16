#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SdfJson.hpp"
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <glm/gtc/type_ptr.hpp>

extern MaterialManager materials;

static glm::mat4 vectorToMat4(const std::vector<float>& v){
    glm::mat4 m(1.0f);
    if(v.size()==16){ std::memcpy(glm::value_ptr(m), v.data(), sizeof(float)*16); }
    return m;
}

static std::vector<float> mat4ToVector(const glm::mat4& m) {
    std::vector<float> v(16);
    const float* ptr = glm::value_ptr(m);
    for (int i = 0; i < 16; ++i) v[i] = ptr[i];
    return v;
}

namespace {

constexpr int kFirstShapeKind = static_cast<int>(Object::ShapeKind::Cube);
constexpr int kLastShapeKind  = static_cast<int>(Object::ShapeKind::Text2D);

Object::ShapeKind checkedShapeKind(int raw) {
    if (raw >= kFirstShapeKind && raw <= kLastShapeKind) {
        return static_cast<Object::ShapeKind>(raw);
    }
    std::cerr << "[ObjectSerialization] invalid ShapeKind ordinal " << raw
              << " — refusing it and hydrating as Cube instead.\n";
    return Object::ShapeKind::Cube;
}

// The append-only integer array remains for old readers, but new writers also
// emit named parameters under `shape.params`.  A First Mover (human or AI)
// should never have to memorize that index 6 means paraboloidA while index 8
// means fillet.  Named values override the legacy array when both are present.
Object::ShapeParams parseShapeParams(const nlohmann::json& j) {
    Object::ShapeParams sp;
    if (j.contains("shapeParams") && j["shapeParams"].is_array() && j["shapeParams"].size() >= 9) {
        const auto& a = j["shapeParams"];
        sp.r = a[0]; sp.ry = a[1]; sp.rz = a[2]; sp.halfH = a[3]; sp.majorR = a[4];
        sp.minorR = a[5]; sp.paraboloidA = a[6]; sp.ovoidAsym = a[7]; sp.fillet = a[8];
        if (a.size() >= 11) {
            sp.width2D = a[9].get<float>();
            sp.height2D = a[10].get<float>();
        }
    }

    if (j.contains("shape") && j["shape"].is_object()) {
        const auto& shape = j["shape"];
        if (shape.contains("params") && shape["params"].is_object()) {
            const auto& p = shape["params"];
            const auto read = [&](const char* key, float& dst) {
                auto it = p.find(key);
                if (it != p.end() && it->is_number()) dst = it->get<float>();
            };
            read("r", sp.r);
            read("ry", sp.ry);
            read("rz", sp.rz);
            read("halfH", sp.halfH);
            read("majorR", sp.majorR);
            read("minorR", sp.minorR);
            read("paraboloidA", sp.paraboloidA);
            read("ovoidAsym", sp.ovoidAsym);
            read("fillet", sp.fillet);
            read("width2D", sp.width2D);
            read("height2D", sp.height2D);
        }
    }
    return sp;
}

bool declaredShapeKind(const nlohmann::json& j, Object::ShapeKind& out) {
    if (j.contains("shape") && j["shape"].is_object() &&
        j["shape"].contains("kind") && j["shape"]["kind"].is_number_integer()) {
        out = checkedShapeKind(j["shape"]["kind"].get<int>());
        return true;
    }
    if (j.contains("shapeKind") && j["shapeKind"].is_number_integer()) {
        out = checkedShapeKind(j["shapeKind"].get<int>());
        return true;
    }
    if (j.contains("geometryType") && j["geometryType"].is_number_integer()) {
        out = checkedShapeKind(j["geometryType"].get<int>());
        return true;
    }
    return false;
}

void hydratePatchPayload(const nlohmann::json& pj, Object& obj) {
    geom::BezierPatch p;
    p.du = pj.value("du", 3);
    p.dv = pj.value("dv", 3);
    if (pj.contains("ctrl") && pj["ctrl"].is_array()) {
        for (const auto& c : pj["ctrl"]) {
            if (c.is_array() && c.size() >= 3) {
                p.ctrl.push_back(glm::vec3(c[0].get<float>(), c[1].get<float>(), c[2].get<float>()));
            }
        }
    }
    obj.setBezierPatch(p);
}

void hydrateFieldPayload(const nlohmann::json& j, Object& obj) {
    glm::vec3 ext{1.0f};
    if (j.contains("fieldExtent")) {
        if (j["fieldExtent"].is_number()) {
            ext = glm::vec3(j["fieldExtent"].get<float>());
        } else if (j["fieldExtent"].is_array() && j["fieldExtent"].size() >= 3) {
            ext = glm::vec3(j["fieldExtent"][0].get<float>(), j["fieldExtent"][1].get<float>(), j["fieldExtent"][2].get<float>());
        }
    }
    obj.setFieldShape(geom::sdfFromJson(j["field"]), ext);
    if (j.contains("fieldCellSize") && j["fieldCellSize"].is_number()) {
        obj.setFieldCellSize(j["fieldCellSize"].get<float>());
    }
}

} // namespace

// ------------------------------------------------------------------
// Object (.ecform / Semantic Text Substrate)
// ------------------------------------------------------------------

void to_json(nlohmann::json& j, const Object& obj){
    j = nlohmann::json{};
    j["geometryType"] = static_cast<int>(obj.getShapeKind()); // legacy axis
    j["shapeKind"]    = static_cast<int>(obj.getShapeKind()); // legacy topology axis
    {
        const auto& sp = obj.getShapeParams();
        j["shapeParams"] = { sp.r, sp.ry, sp.rz, sp.halfH, sp.majorR,
                             sp.minorR, sp.paraboloidA, sp.ovoidAsym, sp.fillet,
                             sp.width2D, sp.height2D };
        // `shape` is the canonical, self-describing authoring surface. Keep the
        // two integer/array fields above for append-only compatibility with
        // existing saves and old readers; new readers prefer this map.
        j["shape"] = {
            {"kind", static_cast<int>(obj.getShapeKind())},
            {"params", {
                {"r", sp.r}, {"ry", sp.ry}, {"rz", sp.rz}, {"halfH", sp.halfH},
                {"majorR", sp.majorR}, {"minorR", sp.minorR},
                {"paraboloidA", sp.paraboloidA}, {"ovoidAsym", sp.ovoidAsym},
                {"fillet", sp.fillet}, {"width2D", sp.width2D},
                {"height2D", sp.height2D}
            }}
        };
    }

    // Sculpted / authored topology is SEMANTIC truth, not a disposable matter
    // cache.  The old writer only persisted Field payloads here; Patch and
    // custom Polyhedron payloads survived solely because .ecmatter happened to
    // be present.  A Zone identity must be independently complete.
    if (obj.hasPatch()) {
        const auto& patch = obj.getPatchData();
        nlohmann::json pj{
            {"du", patch.du},
            {"dv", patch.dv},
            {"ctrl", nlohmann::json::array()}
        };
        for (const auto& c : patch.ctrl) pj["ctrl"].push_back({c.x, c.y, c.z});
        j["patch"] = std::move(pj);
    }

    if (obj.hasField()) {
        // A Field's ShapeKind is only its low-level representation label; the
        // SDF/OntoMath tree is the authored form. Persist the complete tree and
        // its evaluation extent so a reload cannot produce a bare Field shell.
        j["field"] = geom::sdfToJson(obj.getFieldData());
        const auto& ext = obj.getFieldExtent();
        j["fieldExtent"] = {ext.x, ext.y, ext.z};
        if (const auto cell = obj.getFieldCellSize()) {
            j["fieldCellSize"] = *cell;
        }
    }

    if (obj.getShapeKind() == Object::ShapeKind::Polyhedron) {
        const auto& poly = obj.getPolyhedronData();
        if (!poly.vertices.empty() && !poly.faces.empty()) {
            nlohmann::json pj{
                {"vertices", nlohmann::json::array()},
                {"faces", nlohmann::json::array()}
            };
            for (const auto& v : poly.vertices) pj["vertices"].push_back({v.x, v.y, v.z});
            for (const auto& face : poly.faces) pj["faces"].push_back(face);
            j["polyhedron"] = std::move(pj);
        }
    }

    j["objectID"] = obj.getIdentifier();
    j["materialId"] = obj.materialId(); // reference to a Material being, by identifier

    // Placement is Person-meaningful, Law-addressable state — not "purely
    // physical" density that only the conglomerate .ecmatter sidecar may
    // hold. Commit 946a6240 (2026-09-01, "Substrate Split Serialization")
    // removed these five fields from here on that theory; from_json below
    // never stopped reading them, so every Object loaded through a Zone
    // identity that had no matching matter sidecar silently lost its
    // transform to the identity default. Diagnosed by Sol (Codex,
    // 2026-09-09) from Zach's live report of Sanctum/Synthesis
    // Studio/Chess collapsing to the origin — see
    // docs/Agenda/Tasks/Specific Tasks/Per_Zone_serialization_pathway/
    // Per_Zone_serialization_pathway.md, "Live failure" section. Physical
    // topology (vertex/face density) is the part that may still live in
    // matter alone; placement may not disappear from the semantic record.
    j["transform"] = mat4ToVector(obj.getTransform());
    j["center"] = { obj.getCenter().x, obj.getCenter().y, obj.getCenter().z };
    j["authoritativeAxis"] = { obj.getAuthoritativeAxis().x, obj.getAuthoritativeAxis().y, obj.getAuthoritativeAxis().z };
    j["targetRotation"] = { obj.getTargetRotationEulerDegrees().x, obj.getTargetRotationEulerDegrees().y, obj.getTargetRotationEulerDegrees().z };
    j["rotationResponsiveness"] = obj.getRotationResponsiveness();

    j["renderMode"] = static_cast<int>(obj.getRenderModeProp());
    // Screen-space position for Shape2D / Text2D.
    j["x2D"] = obj.getX2D();
    j["y2D"] = obj.getY2D();
    j["zOrder2D"] = obj.getZOrder2D();
    if (!obj.getTextString().empty()) {
        j["textString"] = obj.getTextString();
    }

    // Persist all attributes & tags (eliminates Temporal Black Box)
    if (!obj.getAttributes().empty()) {
        nlohmann::json attrObj = nlohmann::json::object();
        for (const auto& kv : obj.getAttributes()) {
            attrObj[kv.first] = kv.second;
        }
        j["attributes"] = std::move(attrObj);
    }
    if (obj.hasAttribute("baseline")) {
        j["baseline"] = obj.getAttribute("baseline");
    }
    if (obj.hasAttribute("mass")) {
        j["mass"] = obj.getAttribute("mass");
    }
    if (!obj.getTags().empty()) {
        j["tags"] = obj.getTags();
    }

    // Face colours (legacy / baseline tint) are being migrated to Material,
    // but that migration is not finished — Shape2D/Text2D's flat, untextured
    // fallback (Object::draw2DObject) still reads faceColors[0] directly, not
    // any Material. Omitting this field from serialization (as a prior pass
    // here intended, to "complete the substrate split") silently regressed
    // every such object's authored colour to the raw C++ default the moment
    // it round-tripped through a Zone identity store: faceColors[6][3]'s
    // in-class initializer is {1,0,0},{1,0,0},{0,1,0},{0,1,0},{0,0,1},{0,0,1}
    // (a legacy cube-face default), so an authored white 2D plate came back
    // red. Found 2026-09-07 via the Basic Pixel Changer canvas and ~14 other
    // Zone identity files with the same gap. Serialize it until the render
    // side actually stops reading it — remove this again only alongside that
    // migration, not before.
    j["faceColors"] = nlohmann::json::array({
        {obj.faceColors[0][0], obj.faceColors[0][1], obj.faceColors[0][2]},
        {obj.faceColors[1][0], obj.faceColors[1][1], obj.faceColors[1][2]},
        {obj.faceColors[2][0], obj.faceColors[2][1], obj.faceColors[2][2]},
        {obj.faceColors[3][0], obj.faceColors[3][1], obj.faceColors[3][2]},
        {obj.faceColors[4][0], obj.faceColors[4][1], obj.faceColors[4][2]},
        {obj.faceColors[5][0], obj.faceColors[5][1], obj.faceColors[5][2]}
    });

    // Properties a LAW granted this being (ActionNode::AddProperty).
    if (!obj.dynamicProperties().empty()) {
        nlohmann::json dyn = nlohmann::json::object();
        for (const auto& entry : obj.dynamicProperties()) {
            PropertyValue live = entry.second;
            obj.getDynamicProperty(entry.first, live);
            dyn[Earthcall::StringInterner::resolve(entry.first)] = propertyValueToJson(live);
        }
        j["authoredProperties"] = std::move(dyn);
    }

    if (!obj.stakeholders().empty()) {
        nlohmann::json shJson = nlohmann::json::array();
        for (const auto& sh : obj.stakeholders()) {
            shJson.push_back({
                {"propertyPath", sh.propertyPath},
                {"authorId", sh.authorId},
                {"lawId", sh.lawId},
                {"timestamp", sh.timestamp}
            });
        }
        j["stakeholders"] = std::move(shJson);
    }

    // Elements: what this object is composed of, remembered BY IDENTIFIER.
    if (obj.elementCount() > 0) {
        nlohmann::json els = nlohmann::json::array();
        for (const Singular* member : obj.elementFormation().getMembers()) {
            if (member) els.push_back(member->getIdentifier());
        }
        j["elements"] = std::move(els);
    }
}

void from_json(const nlohmann::json& j, Object& obj){
    const Object::ShapeParams params = parseShapeParams(j);
    Object::ShapeKind kind = Object::ShapeKind::Cube;
    const bool hasDeclaredKind = declaredShapeKind(j, kind);

    // The shape discriminant is authoritative.  Previously payload PRESENCE
    // won over shapeKind, so JSON merge-patch could resurrect an obsolete
    // `field` key after a Field had been changed to a Sphere.  That turned a
    // newer authored shape back into its stale ancestor at hydration time.
    // Legacy records with no discriminant still infer from their payload.
    if (hasDeclaredKind) {
        if (kind == Object::ShapeKind::Patch && j.contains("patch") && j["patch"].is_object()) {
            hydratePatchPayload(j["patch"], obj);
        } else if (kind == Object::ShapeKind::Field && j.contains("field") && j["field"].is_object()) {
            hydrateFieldPayload(j, obj);
        } else {
            obj.setShape(kind, params);
        }
    } else if (j.contains("patch") && j["patch"].is_object()) {
        hydratePatchPayload(j["patch"], obj);
    } else if (j.contains("field") && j["field"].is_object()) {
        hydrateFieldPayload(j, obj);
    } else {
        obj.setShape(Object::ShapeKind::Cube, params);
    }

    if (j.contains("objectID") && j["objectID"].is_string()) {
        obj.setObjectID(j["objectID"].get<std::string>());
    }
    // Older saves predate materials; they resolve to material.default on load.
    obj.setMaterialId(j.value("materialId", std::string("material.default")));
    std::vector<float> tvals = j.value("transform", std::vector<float>{});
    if(tvals.size()==16){ obj.setTransform(vectorToMat4(tvals)); }
    if (j.contains("center") && j["center"].is_array() && j["center"].size() >= 3) {
        obj.setCenter(glm::vec3(j["center"][0].get<float>(),
                                j["center"][1].get<float>(),
                                j["center"][2].get<float>()));
    }
    if (j.contains("authoritativeAxis") && j["authoritativeAxis"].is_array() && j["authoritativeAxis"].size() >= 3) {
        obj.setAuthoritativeAxis(glm::vec3(j["authoritativeAxis"][0].get<float>(),
                                           j["authoritativeAxis"][1].get<float>(),
                                           j["authoritativeAxis"][2].get<float>()));
    }
    if (j.contains("rotationResponsiveness")) {
        obj.setRotationResponsiveness(j["rotationResponsiveness"].get<float>());
    }
    if (j.contains("renderMode")) {
        obj.setRenderModeProp(j["renderMode"].get<int>());
    }
    // Screen-space position for Shape2D / Text2D.
    obj.setX2D(j.value("x2D", 100.0f));
    obj.setY2D(j.value("y2D", 100.0f));
    obj.setZOrder2D(j.value("zOrder2D", 0));
    if (j.contains("textString") && j["textString"].is_string()) {
        obj.setTextString(j["textString"].get<std::string>());
    }
    if (j.contains("targetRotation") && j["targetRotation"].is_array() && j["targetRotation"].size() >= 3) {
        obj.setTargetRotationEulerDegrees(glm::vec3(j["targetRotation"][0].get<float>(),
                                                    j["targetRotation"][1].get<float>(),
                                                    j["targetRotation"][2].get<float>()));
    }
    if (j.contains("attributes") && j["attributes"].is_object()) {
        for (auto it = j["attributes"].begin(); it != j["attributes"].end(); ++it) {
            if (it.value().is_string()) {
                obj.setAttribute(it.key(), it.value().get<std::string>());
            }
        }
    }
    if (j.contains("tags") && j["tags"].is_array()) {
        for (const auto& tag : j["tags"]) {
            if (tag.is_string()) {
                obj.addTag(tag.get<std::string>());
            }
        }
    }
    if (j.contains("baseline") && j["baseline"].is_string()) {
        obj.setAttribute("baseline", j["baseline"].get<std::string>());
    }
    // Load mass attribute (store as attribute string)
    if (j.contains("mass")) {
        try {
            if (j["mass"].is_number()) {
                obj.setAttribute("mass", std::to_string(j["mass"].get<double>()));
            } else if (j["mass"].is_string()) {
                obj.setAttribute("mass", j["mass"].get<std::string>());
            }
        } catch (...) {}
    }

    // Properties a law granted this being; and the composition it was part of,
    // held by identifier until World::from_json can re-link it.
    if (j.contains("authoredProperties") && j["authoredProperties"].is_object()) {
        for (auto it = j["authoredProperties"].begin();
             it != j["authoredProperties"].end(); ++it) {
            const std::string& key = it.key();
            PropertyValue val = propertyValueFromJson(it.value());
            if (Property* prop = obj.findProperty(key)) {
                prop->setValue(val);
            }
            obj.setDynamicProperty(key, val);
        }
    }
    if (j.contains("elements") && j["elements"].is_array()) {
        for (const auto& id : j["elements"]) {
            if (id.is_string()) obj.getPendingElementIds().push_back(id.get<std::string>());
        }
    }
    
    if (j.contains("stakeholders") && j["stakeholders"].is_array()) {
        for (const auto& sh : j["stakeholders"]) {
            obj.addStakeholder(
                sh.value("propertyPath", ""),
                sh.value("authorId", ""),
                sh.value("lawId", ""),
                sh.value("timestamp", static_cast<std::time_t>(0))
            );
        }
    }

    // Custom polyhedron geometry belongs to the semantic identity too.  Only
    // apply it when the discriminant says this Object is still a Polyhedron;
    // an obsolete payload may coexist in a merged legacy record and must not
    // overturn the newer declared shape.
    if (obj.getShapeKind() == Object::ShapeKind::Polyhedron && j.contains("polyhedron")) {
        const auto& pj = j["polyhedron"];
        std::vector<glm::vec3> verts;
        std::vector<std::vector<int>> faces;
        if (pj.contains("vertices")) {
            const auto& vs = pj["vertices"];
            verts.reserve(vs.size());
            for (const auto& vj : vs) {
                if (vj.size() >= 3) verts.emplace_back(vj[0].get<float>(), vj[1].get<float>(), vj[2].get<float>());
            }
        }
        if (pj.contains("faces")) {
            const auto& fs = pj["faces"];
            faces.reserve(fs.size());
            for (const auto& fj : fs) {
                std::vector<int> face;
                face.reserve(fj.size());
                for (const auto& idx : fj) face.push_back(idx.get<int>());
                faces.push_back(std::move(face));
            }
        }
        if (!verts.empty() && !faces.empty()) {
            obj.setPolyhedronData(PolyhedronData::createCustomPolyhedron(verts, faces));
        }
    }

    // Face colours
    if (j.contains("faceColors")) {
        const auto& faceCols = j["faceColors"];
        const bool ownsItsSurface = obj.materialId() == "material." + obj.getIdentifier();
        auto alreadyPainted = ownsItsSurface ? materials.get(obj.materialId()) : nullptr;
        const bool texturesAlreadyHere =
            alreadyPainted && !alreadyPainted->faceTextures.empty();
        for (size_t f = 0; f < faceCols.size() && f < 6; ++f) {
            obj.faceColors[f][0] = faceCols[f][0].get<float>();
            obj.faceColors[f][1] = faceCols[f][1].get<float>();
            obj.faceColors[f][2] = faceCols[f][2].get<float>();
            if (ownsItsSurface && !texturesAlreadyHere) {
                obj.setFaceColor(static_cast<int>(f),
                                 obj.faceColors[f][0], obj.faceColors[f][1],
                                 obj.faceColors[f][2]);
            }
        }
    }

    // Load per-face textures if present (after geometry restoration for correct sizing)
    // if (j.contains("faceTextures")) {
    //     const auto& arr = j["faceTextures"];
    //     int limit = std::min<int>(static_cast<int>(arr.size()), static_cast<int>(obj.faceTextures.size()));
    //     for (int i = 0; i < limit; ++i) {
    //         const auto& ftj = arr[i];
    //         int size = ftj.value("size", (i < static_cast<int>(obj.faceTextures.size()) ? obj.faceTextures[i].size : 64));
    //         std::string b64 = ftj.value("pixelsB64", std::string());
    //         if (!b64.empty()) {
    //             std::vector<uint8_t> data = base64Decode(b64);
    //             if (size > 0 && static_cast<int>(data.size()) == size * size * 4 && i < static_cast<int>(obj.faceTextures.size())) {
    //                 auto& ft = obj.faceTextures[i];
    //                 ft.size = size;
    //                 ft.pixels = std::move(data);
    //                 ft.updateWholeGPU();
    //             }
    //         }
    //     }
    // }
}
