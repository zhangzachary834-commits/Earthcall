#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SdfJson.hpp"
#include <cstring>
#include <ctime>
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
// ------------------------------------------------------------------
// Object (.ecform / Semantic Text Substrate)
// ------------------------------------------------------------------

void to_json(nlohmann::json& j, const Object& obj){
    j = nlohmann::json{};
    j["geometryType"] = static_cast<int>(obj.getShapeKind()); // legacy axis
    j["shapeKind"]    = static_cast<int>(obj.getShapeKind());    // topology framework
    {
        const auto& sp = obj.getShapeParams();
        j["shapeParams"] = { sp.r, sp.ry, sp.rz, sp.halfH, sp.majorR,
                             sp.minorR, sp.paraboloidA, sp.ovoidAsym, sp.fillet,
                             sp.width2D, sp.height2D };
    }
    j["objectID"] = obj.getIdentifier();
    j["materialId"] = obj.materialId(); // reference to a Material being, by identifier

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

    // Face colours (legacy / baseline tint) are migrated to Material; 
    // we omit them from new ecform serializations to complete the substrate split.

    // Properties a LAW granted this being (ActionNode::AddProperty).
    if (!obj.dynamicProperties().empty()) {
        nlohmann::json dyn = nlohmann::json::object();
        for (const auto& entry : obj.dynamicProperties()) {
            dyn[Earthcall::StringInterner::resolve(entry.first)] = propertyValueToJson(entry.second);
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

static Object::ShapeParams parseShapeParams(const nlohmann::json& j) {
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
    return sp;
}

void from_json(const nlohmann::json& j, Object& obj){
    // Prefer the topology framework's shapeKind; fall back to the legacy
    // geometryType int (which setShapeKind migrates into the new model).
    if (j.contains("patch")) {
        const auto& pj = j["patch"];
        geom::BezierPatch p;
        p.du = pj.value("du", 3); p.dv = pj.value("dv", 3);
        if (pj.contains("ctrl")) {
            for (const auto& c : pj["ctrl"])
                p.ctrl.push_back(glm::vec3(c[0].get<float>(), c[1].get<float>(), c[2].get<float>()));
        }
        obj.setBezierPatch(p);
    } else if (j.contains("field")) {
        glm::vec3 ext{1.0f};
        if (j.contains("fieldExtent")) {
            if (j["fieldExtent"].is_number()) {
                ext = glm::vec3(j["fieldExtent"].get<float>());
            } else if (j["fieldExtent"].is_array() && j["fieldExtent"].size() >= 3) {
                ext = glm::vec3(j["fieldExtent"][0].get<float>(), j["fieldExtent"][1].get<float>(), j["fieldExtent"][2].get<float>());
            }
        }
        obj.setFieldShape(geom::sdfFromJson(j["field"]), ext);
    } else if (j.contains("shapeKind")) {
        obj.setShape(static_cast<Object::ShapeKind>(j["shapeKind"].get<int>()), parseShapeParams(j));
    } else {
        int gt = j.value("geometryType", 0);
        obj.setShapeKind(static_cast<Object::ShapeKind>(gt));
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
    // For polyhedron in legacy saves, restore geometry
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

