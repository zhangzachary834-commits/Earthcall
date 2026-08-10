#include "Form/Object/Tool/PolyhedronSettings.hpp"

#include <cmath>

namespace Core {

namespace {

std::vector<glm::vec2> makeRegularPolygon2D(int n, float radius) {
    std::vector<glm::vec2> pts;
    pts.reserve(n);
    for (int i = 0; i < n; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * i / n;
        pts.push_back(glm::vec2(std::cos(angle) * radius, std::sin(angle) * radius));
    }
    return pts;
}

} // namespace

PolyhedronData PolyhedronSettings::build() const {
    PolyhedronData data;

    if (useCustom && !customVertices.empty()) {
        data = PolyhedronData::createCustomPolyhedron(customVertices, customFaces);
    } else if (irregularType > 0) {
        auto basePoly = makeRegularPolygon2D(irregularBaseSides, 0.5f);
        switch (irregularType) {
            case 1: data = PolyhedronData::createPrism(basePoly, irregularHeight); break;
            case 2: data = PolyhedronData::createAntiprism(irregularBaseSides, 0.5f, irregularHeight); break;
            case 3: data = PolyhedronData::createPyramid(basePoly, irregularHeight); break;
            case 4: data = PolyhedronData::createBipyramid(irregularBaseSides, 0.5f, irregularHeight); break;
            case 5: data = PolyhedronData::createFrustum(basePoly, irregularHeight, frustumTopScale); break;
            default: data = PolyhedronData::createRegularPolyhedron(currentType); break;
        }
    } else {
        switch (concaveType) {
            case 0: data = PolyhedronData::createRegularPolyhedron(currentType); break;
            case 1: data = PolyhedronData::createConcavePolyhedron(currentType, 0.5f, concavityAmount); break;
            case 2: data = PolyhedronData::createStarPolyhedron(currentType, 0.5f, spikeLength); break;
            case 3: data = PolyhedronData::createCraterPolyhedron(currentType, 0.5f, craterDepth); break;
            default: data = PolyhedronData::createRegularPolyhedron(currentType); break;
        }
    }

    if (applyTruncation) {
        data = PolyhedronData::truncate(data, truncationAmount);
    }
    if (applyDual) {
        data = PolyhedronData::createDual(data);
    }

    return data;
}

void PolyhedronSettings::generateCustom() {
    customVertices.clear();
    customFaces.clear();

    const float radius = 0.5f;
    for (int i = 0; i < customVertexCount; ++i) {
        float phi   = std::acos(1.0f - 2.0f * (i + 0.5f) / customVertexCount);
        float theta = static_cast<float>(M_PI) * (1.0f + std::sqrt(5.0f)) * (i + 0.5f);

        float x = radius * std::sin(phi) * std::cos(theta);
        float y = radius * std::sin(phi) * std::sin(theta);
        float z = radius * std::cos(phi);

        customVertices.push_back(glm::vec3(x, y, z));
    }

    int facesCreated = 0;
    for (int i = 0; i < customVertexCount && facesCreated < customFaceCount; ++i) {
        for (int j = i + 1; j < customVertexCount && facesCreated < customFaceCount; ++j) {
            for (int k = j + 1; k < customVertexCount && facesCreated < customFaceCount; ++k) {
                customFaces.push_back({i, j, k});
                ++facesCreated;
            }
        }
    }

    while (facesCreated < customFaceCount && customVertexCount >= 4) {
        std::vector<int> face;
        for (int v = 0; v < 4 && v < customVertexCount; ++v) {
            face.push_back(v);
        }
        customFaces.push_back(std::move(face));
        ++facesCreated;
    }
}

} // namespace Core
