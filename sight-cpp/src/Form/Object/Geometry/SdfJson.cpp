#include "Form/Object/Geometry/SdfJson.hpp"

namespace geom {

nlohmann::json sdfToJson(const SdfNode& n) {
    nlohmann::json j{
        {"op", static_cast<int>(n.op)},
        {"prim", static_cast<int>(n.prim)},
        {"dims", {n.dims.x, n.dims.y, n.dims.z}},
        {"offset", {n.offset.x, n.offset.y, n.offset.z}},
        {"p0", n.p0},
        {"p1", n.p1},
        {"t", n.t}
    };
    if (!n.expr.empty()) j["expr"] = n.expr;
    if (!n.planes.empty()) {
        nlohmann::json planes = nlohmann::json::array();
        for (const auto& p : n.planes) planes.push_back({p.x, p.y, p.z, p.w});
        j["planes"] = planes;
    }
    if (!n.children.empty()) {
        nlohmann::json children = nlohmann::json::array();
        for (const auto& c : n.children) {
            children.push_back(c ? sdfToJson(*c) : nlohmann::json());
        }
        j["children"] = children;
    }
    return j;
}

SdfNode sdfFromJson(const nlohmann::json& j) {
    SdfNode n;
    n.op = static_cast<SdfOp>(j.value("op", 0));
    n.prim = static_cast<SdfPrim>(j.value("prim", 0));
    if (j.contains("dims") && j["dims"].is_array() && j["dims"].size() == 3) {
        n.dims = glm::vec3(j["dims"][0].get<float>(), j["dims"][1].get<float>(), j["dims"][2].get<float>());
    }
    if (j.contains("offset") && j["offset"].is_array() && j["offset"].size() == 3) {
        n.offset = glm::vec3(j["offset"][0].get<float>(), j["offset"][1].get<float>(), j["offset"][2].get<float>());
    }
    n.p0 = j.value("p0", 0.0f);
    n.p1 = j.value("p1", 0.0f);
    n.t = j.value("t", 0.5f);
    if (j.contains("expr")) {
        n.expr = j["expr"].get<std::string>();
        n.rpn = compileExpr(n.expr);   // derived, never serialized
    }
    if (j.contains("planes")) {
        for (const auto& p : j["planes"]) {
            if (p.is_array() && p.size() == 4) {
                n.planes.emplace_back(p[0].get<float>(), p[1].get<float>(),
                                      p[2].get<float>(), p[3].get<float>());
            }
        }
    }
    if (j.contains("children")) {
        for (const auto& c : j["children"]) {
            n.children.push_back(c.is_null() ? nullptr
                                             : std::make_shared<SdfNode>(sdfFromJson(c)));
        }
    }
    return n;
}

} // namespace geom
