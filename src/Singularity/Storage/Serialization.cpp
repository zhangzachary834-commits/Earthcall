#include "Singularity/Storage/Serialization.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "../../ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/CategoryManager.hpp"
#include "ZonesOfEarth/HomesOfEarth/Home.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SdfJson.hpp"
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <unordered_set>
#include <set>
#include <tuple>

extern MaterialManager materials;
extern CategoryManager categories;

// Helper: serialise glm::mat4 to vector<float>
static std::vector<float> mat4ToVector(const glm::mat4& m){
    std::vector<float> v(16);
    const float* p = glm::value_ptr(m);
    for(int i=0;i<16;++i) v[i]=p[i];
    return v;
}
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

// ------------------------------------------------------------------
// BodyPart (reuses Object faceTexture serialization)
// ------------------------------------------------------------------
nlohmann::json bodyPartToJson(const BodyPart& part) {
    nlohmann::json j;
    j["name"] = part.getName();
    j["type"] = static_cast<int>(part.getType());

    // Primary 3D shape
    j["geometryType"] = static_cast<int>(part.getPrimaryShape());

    // Geometry dimensions
    auto dims = part.getDimensions();
    j["dimensions"] = {dims.x, dims.y, dims.z};
    j["geometryShape"] = static_cast<int>(part.getPrimaryShape());

    // Base color
    const float* col = part.getColor();
    j["color"] = {col[0], col[1], col[2]};

    // Local transform (relative to body root)
    j["localTransform"] = mat4ToVector(part.localTransform());
    j["center"] = {part.getPrimaryObject()->getCenter().x, part.getPrimaryObject()->getCenter().y, part.getPrimaryObject()->getCenter().z};
    j["authoritativeAxis"] = {part.getPrimaryObject()->getAuthoritativeAxis().x,
                               part.getPrimaryObject()->getAuthoritativeAxis().y,
                               part.getPrimaryObject()->getAuthoritativeAxis().z};
    j["targetRotation"] = {part.getPrimaryObject()->getTargetRotationEulerDegrees().x,
                            part.getPrimaryObject()->getTargetRotationEulerDegrees().y,
                            part.getPrimaryObject()->getTargetRotationEulerDegrees().z};
    j["rotationResponsiveness"] = part.getPrimaryObject()->getRotationResponsiveness();

    // Face textures — same format as Object serialization
    // if (!part.faceTextures.empty()) {
    //     nlohmann::json texArr = nlohmann::json::array();
    //     for (const auto& ft : part.faceTextures) {
    //         if (ft.useLayers) {
    //             ft.compositeLayers();
    //         }
    //         nlohmann::json ftj;
    //         ftj["size"] = ft.size;
    //         ftj["pixelsB64"] = base64Encode(ft.pixels);
    //         texArr.push_back(std::move(ftj));
    //     }
    //     j["textureVersion"] = 1;
    //     j["faceTextures"] = std::move(texArr);
    // }

    // Legacy faceColors
    nlohmann::json faces = nlohmann::json::array();
    for (int f = 0; f < 6; ++f) {
        faces.push_back({part.getPrimaryObject()->faceColors[f][0], part.getPrimaryObject()->faceColors[f][1], part.getPrimaryObject()->faceColors[f][2]});
    }
    j["faceColors"] = faces;

    // Composite sub-objects — save local offsets, not world transforms
    if (part.getSubObjectCount() > 0) {
        nlohmann::json subArr = nlohmann::json::array();
        for (size_t si = 0; si < part.getSubObjectCount(); ++si) {
            const Object* sub = part.getSubObject(si);
            if (!sub) continue;
            nlohmann::json sj;
            to_json(sj, *sub);
            // Override transform with the local offset (to_json wrote world transform)
            sj["transform"] = mat4ToVector(part.getSubObjectLocalOffset(si));
            subArr.push_back(std::move(sj));
        }
        j["subObjects"] = std::move(subArr);
    }

    return j;
}

void bodyPartFromJson(const nlohmann::json& j, BodyPart& part) {
    // Primary 3D shape (must come before texture load so face count is correct)
    if (j.contains("geometryType")) {
        part.setPrimaryShape(static_cast<Object::ShapeKind>(j["geometryType"].get<int>()));
    }

    // Geometry dimensions
    if (j.contains("dimensions")) {
        auto dims = j["dimensions"];
        if (dims.size() >= 3) {
            part.setDimensions(glm::vec3(dims[0], dims[1], dims[2]));
        }
    }

    // Base color
    if (j.contains("color")) {
        auto col = j["color"];
        if (col.size() >= 3) {
            part.setColor(col[0], col[1], col[2]);
        }
    }

    // Local transform
    if (j.contains("localTransform")) {
        auto tvals = j["localTransform"].get<std::vector<float>>();
        if (tvals.size() == 16) {
            part.setLocalTransform(vectorToMat4(tvals));
        }
    }
    if (j.contains("center") && j["center"].is_array() && j["center"].size() >= 3) {
        part.getPrimaryObject()->setCenter(glm::vec3(j["center"][0].get<float>(),
                                 j["center"][1].get<float>(),
                                 j["center"][2].get<float>()));
    }
    if (j.contains("authoritativeAxis") && j["authoritativeAxis"].is_array() && j["authoritativeAxis"].size() >= 3) {
        part.getPrimaryObject()->setAuthoritativeAxis(glm::vec3(j["authoritativeAxis"][0].get<float>(),
                                            j["authoritativeAxis"][1].get<float>(),
                                            j["authoritativeAxis"][2].get<float>()));
    }
    if (j.contains("rotationResponsiveness")) {
        part.getPrimaryObject()->setRotationResponsiveness(j["rotationResponsiveness"].get<float>());
    }
    if (j.contains("targetRotation") && j["targetRotation"].is_array() && j["targetRotation"].size() >= 3) {
        part.getPrimaryObject()->setTargetRotationEulerDegrees(glm::vec3(j["targetRotation"][0].get<float>(),
                                                     j["targetRotation"][1].get<float>(),
                                                     j["targetRotation"][2].get<float>()));
    }

    // Legacy faceColors
    if (j.contains("faceColors")) {
        const auto& faces = j["faceColors"];
        for (size_t f = 0; f < faces.size() && f < 6; ++f) {
            part.getPrimaryObject()->faceColors[f][0] = faces[f][0];
            part.getPrimaryObject()->faceColors[f][1] = faces[f][1];
            part.getPrimaryObject()->faceColors[f][2] = faces[f][2];
        }
    }

    // Face textures (same as Object deserialization)
    // if (j.contains("faceTextures")) {
    //     const auto& arr = j["faceTextures"];
    //     int limit = std::min<int>(static_cast<int>(arr.size()), static_cast<int>(part.faceTextures.size()));
    //     for (int i = 0; i < limit; ++i) {
    //         const auto& ftj = arr[i];
    //         int size = ftj.value("size", (i < static_cast<int>(part.faceTextures.size()) ? part.faceTextures[i].size : 64));
    //         std::string b64 = ftj.value("pixelsB64", std::string());
    //         if (!b64.empty()) {
    //             std::vector<uint8_t> data = base64Decode(b64);
    //             if (size > 0 && static_cast<int>(data.size()) == size * size * 4 &&
    //                 i < static_cast<int>(part.faceTextures.size())) {
    //                 auto& ft = part.faceTextures[i];
    //                 ft.size = size;
    //                 ft.pixels = std::move(data);
    //                 ft.updateWholeGPU();
    //             }
    //         }
    //     }
    // }

    // Composite sub-objects — saved transforms are local offsets
    if (j.contains("subObjects")) {
        while (part.getSubObjectCount() > 0) {
            part.removeSubObject(part.getSubObjectCount() - 1);
        }
        for (const auto& sj : j["subObjects"]) {
            // Extract the local offset from the saved transform
            glm::mat4 localOffset(1.0f);
            std::vector<float> tvals = sj.value("transform", std::vector<float>{});
            if (tvals.size() == 16) {
                localOffset = vectorToMat4(tvals);
            }

            Object* sub = sj.contains("shapeKind")
                ? part.addSubObject(static_cast<Object::ShapeKind>(sj["shapeKind"].get<int>()), localOffset)
                : part.addSubObject(static_cast<Object::ShapeKind>(sj.value("geometryType", 0)), localOffset);
            if (sub) {
                // Load everything except transform (already set by addSubObject)
                glm::mat4 savedWorld = sub->getTransform();
                from_json(sj, *sub);
                sub->setTransform(savedWorld);
            }
        }
    }
}

nlohmann::json bodyToJson(const Body& body) {
    nlohmann::json j;
    j["shape"] = body.shape;
    j["artStyle"] = body.artStyle;
    j["height"] = body.height;

    nlohmann::json partsArr = nlohmann::json::array();
    for (const auto* part : body.parts) {
        if (part) {
            partsArr.push_back(bodyPartToJson(*part));
        }
    }
    j["bodyParts"] = partsArr;

    return j;
}

void bodyFromJson(const nlohmann::json& j, Body& body) {
    if (j.contains("height")) body.height = j["height"].get<float>();

    // Match saved parts to existing parts by name, or create new parts if they don't exist
    if (j.contains("bodyParts")) {
        const auto& partsArr = j["bodyParts"];
        for (const auto& pj : partsArr) {
            std::string name = pj.value("name", std::string());
            if (name.empty()) continue;

            // Find matching part in the body
            BodyPart* existingPart = nullptr;
            for (auto* part : body.parts) {
                if (part && part->getName() == name) {
                    existingPart = part;
                    break;
                }
            }

            if (existingPart) {
                // Update existing part
                bodyPartFromJson(pj, *existingPart);
            } else {
                // Create new part from save data
                BodyPart* newPart = new BodyPart();
                bodyPartFromJson(pj, *newPart);
                body.addPart(newPart);
            }
        }
    }
}

// ------------------------------------------------------------------
// Zone object bag — on disk still `{"objects":[...]}` (the old World shape).
// ------------------------------------------------------------------
nlohmann::json zoneObjectsToJson(const Zone& zone) {
    nlohmann::json j = nlohmann::json{};
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& ptr : zone.objects()) {
        if (ptr) arr.push_back(*ptr);
    }
    j["objects"] = arr;
    return j;
}

void zoneObjectsFromJson(const nlohmann::json& j, Zone& zone) {
    const nlohmann::json* arr = nullptr;
    if (j.contains("objects") && j["objects"].is_array()) arr = &j["objects"];
    else if (j.is_array()) arr = &j;
    if (!arr) return;
    for (const auto& oj : *arr) {
        std::string id;
        if (oj.contains("objectID") && oj["objectID"].is_string()) {
            id = oj["objectID"].get<std::string>();
        } else if (oj.contains("id") && oj["id"].is_string()) {
            id = oj["id"].get<std::string>();
        }
        std::shared_ptr<Object> obj = std::make_shared<Object>(id);
        from_json(oj, *obj);
        zone.addObject(std::move(obj));
    }

    // Re-link composition once every object exists. Elements are remembered by
    // identifier, so this pass is order-independent; an element that is not in
    // the Zone is simply not re-linked (composition is a covenant between
    // beings that are present, never a pointer to something absent).
    auto& owned = zone.getOwnedObjectsMutable();
    for (auto& holder : owned) {
        if (!holder || holder->getPendingElementIds().empty()) continue;
        for (const auto& id : holder->getPendingElementIds()) {
            for (auto& candidate : owned) {
                if (candidate && candidate.get() != holder.get() &&
                    candidate->getIdentifier() == id) {
                    holder->addElement(candidate.get());
                    break;
                }
            }
        }
        holder->getPendingElementIds().clear();
    }
}

namespace {
Zone::Scope scopeFromName(const std::string& name) {
    if (name == "Global") return Zone::Scope::Global;
    if (name == "World") return Zone::Scope::World;
    if (name == "Regional") return Zone::Scope::Regional;
    if (name == "UI") return Zone::Scope::UI;
    return Zone::Scope::Local;
}

void internZoneLexemes(Zone& zone, const nlohmann::json& zj) {
    if (!zj.contains("lexemes") || !zj["lexemes"].is_array()) return;
    auto& language = Singularity::Language::LanguageSystem::instance();
    for (const auto& item : zj["lexemes"]) {
        if (!item.is_object()) continue;
        const std::string id = item.value("id", std::string{});
        const std::string symbol = item.value("symbol", std::string{});
        if (id.empty() || symbol.empty()) continue;
        auto lexeme = language.intern(symbol, id);
        zone.addToFormation(lexeme.get());
    }
}

Singular* resolveZoneEndpoint(Zone& zone, const std::string& id) {
    if (id.empty()) return nullptr;
    if (Singular* member = zone.formation().findMemberByIdentifier(id)) return member;
    for (const auto& obj : zone.getOwnedObjects()) {
        if (obj && obj->getIdentifier() == id) return obj.get();
    }
    if (auto cat = categories.get(id)) return cat.get();
    if (auto mat = materials.get(id)) return mat.get();
    auto& language = Singularity::Language::LanguageSystem::instance();
    if (auto lexeme = language.findById(id)) {
        zone.addToFormation(lexeme.get());
        return lexeme.get();
    }
    if (auto lexeme = language.findBySymbol(id)) {
        zone.addToFormation(lexeme.get());
        return lexeme.get();
    }
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == id) return being;
    }
    return nullptr;
}

void applyFormationRelations(Zone& zone, const nlohmann::json& zj) {
    if (!zj.contains("formationRelations") || !zj["formationRelations"].is_array()) return;
    // MEMBERS BEFORE RELATIONS. Zone::syncFormationMembers does not run
    // until the frame loop, so a relation added here used to find neither
    // of its endpoints. Lexemes are interned first so is_pos / resolves_to
    // edges bind to beings, not leftover name-strings.
    zone.syncFormationMembers();
    internZoneLexemes(zone, zj);

    // Called on every load now (not just when the Zone arrives with no
    // objects — Bug #7), so a relation already present by type + both
    // endpoint ids is skipped rather than duplicated.
    std::set<std::tuple<std::string, std::string, std::string>> existing;
    for (const auto& r : zone.formation().relations().getAll()) {
        if (r) existing.insert({r->type, r->aId(), r->bId()});
    }

    size_t refused = 0;
    size_t unbound = 0;
    for (const auto& relJson : zj["formationRelations"]) {
        auto rel = std::make_shared<Relation>(Relation::fromJson(relJson, [&](const std::string& id) {
            return resolveZoneEndpoint(zone, id);
        }));
        const auto key = std::make_tuple(rel->type, rel->aId(), rel->bId());
        if (existing.count(key)) continue;
        if (!zone.formation().add(rel)) {
            ++refused;
            if (!rel->hasEndpoints()) ++unbound;
        } else {
            existing.insert(key);
        }
    }
    if (refused > 0) {
        // Name the ACTUAL cause. This line used to say "self-ground or a
        // directed cycle" for every refusal, and the common case is neither:
        // an endpoint that is not in the world YET. Zone hydration runs at
        // boot, before a world file's categories and authors are read, so a
        // relation naming one of those finds nothing to bind to. Sending a
        // reader to look for a cycle that is not there is the kind of
        // confidently-wrong diagnostic that costs an afternoon.
        std::cout << "⚠️  Zone '" << zone.name() << "': " << refused
                  << " saved formation relation(s) were REFUSED on load";
        if (unbound > 0) {
            std::cout << " — " << unbound << " because an endpoint is not in "
                      << "the world yet (a being the world file has not "
                      << "loaded, or one nothing authors)";
            if (unbound < refused) std::cout << ", the rest";
        }
        if (unbound < refused) {
            std::cout << " because the edge is a self-ground or closes a "
                      << "directed cycle";
        }
        std::cout << ". They are not in the formation and will not be written "
                  << "back on the next save. Fix them in the save file to keep "
                  << "them." << std::endl;
    }
}
} // namespace

nlohmann::json zoneToJson(const Zone& zone) {
    nlohmann::json zj;
    zj["name"] = zone.name();
    zj["identifier"] = zone.getIdentifier();
    zj["owner"] = zone.owner();
    zj["parentZone"] = zone.getParentZone();
    zj["scope"] = zone.scopeName();
    nlohmann::json qualities = nlohmann::json::object();
    for (const auto& kv : zone.getQualities()) {
        qualities[kv.first] = kv.second;
    }
    zj["qualities"] = qualities;
    nlohmann::json del = nlohmann::json::object();
    for (const auto& kv : zone.getDeletability()) {
        del[kv.first] = kv.second;
    }
    zj["deletable"] = del;
    zj["world"] = zoneObjectsToJson(zone);
    nlohmann::json lexemes = nlohmann::json::array();
    for (Singular* member : zone.formation().getMembers()) {
        auto* lexeme = dynamic_cast<Singularity::Language::Lexeme*>(member);
        if (!lexeme) continue;
        lexemes.push_back({
            {"id", lexeme->getIdentifier()},
            {"symbol", lexeme->getSymbol()}
        });
    }
    zj["lexemes"] = lexemes;
    zj["formationRelations"] = zone.formation().relations().toJson();

    // Paint lives on Material beings. Objects only store a materialId.
    // If those materials stay only in the session bag, loading another
    // save replaces them and Home comes back white. Carry the materials
    // this Zone/Home's objects name, so the identity file is the surface.
    nlohmann::json mats = nlohmann::json::array();
    std::unordered_set<std::string> seen;
    for (const auto& obj : zone.objects()) {
        if (!obj) continue;
        const std::string& mid = obj->materialId();
        if (mid.empty() || !seen.insert(mid).second) continue;
        if (auto m = materials.get(mid)) mats.push_back(m->toJson());
    }
    if (!mats.empty()) zj["materials"] = std::move(mats);

    if (const auto* home = dynamic_cast<const Home*>(&zone)) {
        zj["being"] = "home";
        zj["primary"] = home->isPrimaryHome();
        zj["entryRequiresWill"] = home->entryRequiresWill();
        zj["cannotForceStay"] = home->cannotForceStay();
        zj["stakes"] = home->stakeIds();
        zj["inhabitants"] = home->inhabitantIds();
    }
    return zj;
}

void applyZoneJson(Zone& zone, const nlohmann::json& zj, bool replaceObjects) {
    if (zj.contains("materials")) {
        materials.mergeFromJson(zj["materials"]);
    }
    if (zj.contains("owner")) {
        zone.setOwner(zj.value("owner", std::string{}));
    }
    if (zj.contains("parentZone")) {
        zone.setParentZone(zj.value("parentZone", std::string{}));
    }
    if (zj.contains("scope") && zj["scope"].is_string()) {
        zone.setScope(scopeFromName(zj["scope"].get<std::string>()));
    }
    if (zj.contains("qualities") && zj["qualities"].is_object()) {
        for (auto it = zj["qualities"].begin(); it != zj["qualities"].end(); ++it) {
            if (it.value().is_string()) {
                zone.setQuality(it.key(), it.value().get<std::string>());
            }
        }
    }
    if (zj.contains("deletable") && zj["deletable"].is_object()) {
        for (auto it = zj["deletable"].begin(); it != zj["deletable"].end(); ++it) {
            if (it.value().is_boolean()) {
                zone.setDeletable(it.key(), it.value().get<bool>());
            }
        }
    }
    if (replaceObjects) {
        zone.getOwnedObjectsMutable().clear();
    }
    if (zone.getOwnedObjects().empty()) {
        if (zj.contains("world")) {
            zoneObjectsFromJson(zj["world"], zone);
        } else if (zj.contains("objects")) {
            zoneObjectsFromJson(zj, zone);
        }
    }
    // Bug #7: this used to be nested in the empty-objects branch above, so
    // a Zone that already held objects (kept live, or just hydrated from
    // the store) could never receive its relation graph or lexemes. Now
    // idempotent (see applyFormationRelations), it always runs.
    applyFormationRelations(zone, zj);
    if (auto* home = dynamic_cast<Home*>(&zone)) {
        if (zj.value("primary", false)) home->markPrimaryHome();
        if (zj.contains("stakes") && zj["stakes"].is_array()) {
            std::vector<std::string> ids;
            for (const auto& s : zj["stakes"]) {
                if (s.is_string()) ids.push_back(s.get<std::string>());
            }
            home->loadStakeIds(std::move(ids));
        }
        if (zj.contains("inhabitants") && zj["inhabitants"].is_array()) {
            std::vector<std::string> ids;
            for (const auto& s : zj["inhabitants"]) {
                if (s.is_string()) ids.push_back(s.get<std::string>());
            }
            home->loadInhabitantIds(std::move(ids));
        }
    }
}

std::shared_ptr<Zone> makeZoneFromJson(const nlohmann::json& zj) {
    const std::string name = zj.value("name", zj.value("identifier", "Untitled Zone"));
    std::string kind;
    if (zj.contains("qualities") && zj["qualities"].is_object()) {
        kind = zj["qualities"].value("kind", std::string{});
    }
    if (kind.empty()) kind = zj.value("kind", std::string{});
    const bool dwelling = (kind == Zone::kHomeKind || kind == Zone::kCommunityHomeKind
                           || name == "Home" || zj.value("being", std::string{}) == "home");
    std::shared_ptr<Zone> zone = dwelling
        ? std::shared_ptr<Zone>(std::make_shared<Home>(name, "strict"))
        : std::make_shared<Zone>(name, "strict");
    applyZoneJson(*zone, zj, true);
    return zone;
}
