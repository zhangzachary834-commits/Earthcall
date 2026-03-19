// GamePolyhedron.cpp – Polyhedron building & generation helpers
// Split from Game.cpp during refactor.

#include "Game.hpp"
#include "Form/Object/Object.hpp"

#include <glm/glm.hpp>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Core {

// Helper to build a regular polygon with n sides as a list of 2D points
static std::vector<glm::vec2> makeRegularPolygon2D(int n, float radius = 0.5f) {
    std::vector<glm::vec2> pts;
    pts.reserve(n);
    for (int i = 0; i < n; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * i / n;
        pts.push_back(glm::vec2(std::cos(angle) * radius, std::sin(angle) * radius));
    }
    return pts;
}

Object::PolyhedronData Game::buildCurrentPolyhedron() const {
    Object::PolyhedronData data;

    // Step 1: build base shape
    if (_useCustomPolyhedron && !_customPolyhedronVertices.empty()) {
        data = Object::PolyhedronData::createCustomPolyhedron(
            _customPolyhedronVertices, _customPolyhedronFaces);
    } else if (_irregularType > 0) {
        auto basePoly = makeRegularPolygon2D(_irregularBaseSides, 0.5f);
        switch (_irregularType) {
            case 1: // Prism
                data = Object::PolyhedronData::createPrism(basePoly, _irregularHeight);
                break;
            case 2: // Antiprism
                data = Object::PolyhedronData::createAntiprism(_irregularBaseSides, 0.5f, _irregularHeight);
                break;
            case 3: // Pyramid
                data = Object::PolyhedronData::createPyramid(basePoly, _irregularHeight);
                break;
            case 4: // Bipyramid
                data = Object::PolyhedronData::createBipyramid(_irregularBaseSides, 0.5f, _irregularHeight);
                break;
            case 5: // Frustum
                data = Object::PolyhedronData::createFrustum(basePoly, _irregularHeight, _frustumTopScale);
                break;
            default:
                data = Object::PolyhedronData::createRegularPolyhedron(_currentPolyhedronType);
                break;
        }
    } else {
        switch (_currentConcaveType) {
            case 0:
                data = Object::PolyhedronData::createRegularPolyhedron(_currentPolyhedronType);
                break;
            case 1:
                data = Object::PolyhedronData::createConcavePolyhedron(
                    _currentPolyhedronType, 0.5f, _concavityAmount);
                break;
            case 2:
                data = Object::PolyhedronData::createStarPolyhedron(
                    _currentPolyhedronType, 0.5f, _spikeLength);
                break;
            case 3:
                data = Object::PolyhedronData::createCraterPolyhedron(
                    _currentPolyhedronType, 0.5f, _craterDepth);
                break;
            default:
                data = Object::PolyhedronData::createRegularPolyhedron(_currentPolyhedronType);
                break;
        }
    }

    // Step 2: apply modifiers
    if (_applyTruncation) {
        data = Object::PolyhedronData::truncate(data, _truncationAmount);
    }
    if (_applyDual) {
        data = Object::PolyhedronData::createDual(data);
    }

    return data;
}

void Game::_generateCustomPolyhedron() {
    _customPolyhedronVertices.clear();
    _customPolyhedronFaces.clear();

    // Generate vertices on a sphere
    float radius = 0.5f;
    for (int i = 0; i < _customPolyhedronVertexCount; ++i) {
        // Use spherical coordinates for even distribution
        float phi = acos(1.0f - 2.0f * (i + 0.5f) / _customPolyhedronVertexCount);
        float theta = M_PI * (1.0f + sqrt(5.0f)) * (i + 0.5f);

        float x = radius * sin(phi) * cos(theta);
        float y = radius * sin(phi) * sin(theta);
        float z = radius * cos(phi);

        _customPolyhedronVertices.push_back(glm::vec3(x, y, z));
    }

    // Generate faces using convex hull approximation (triangular faces)
    int facesCreated = 0;
    for (int i = 0; i < _customPolyhedronVertexCount && facesCreated < _customPolyhedronFaceCount; ++i) {
        for (int j = i + 1; j < _customPolyhedronVertexCount && facesCreated < _customPolyhedronFaceCount; ++j) {
            for (int k = j + 1; k < _customPolyhedronVertexCount && facesCreated < _customPolyhedronFaceCount; ++k) {
                std::vector<int> face = {i, j, k};
                _customPolyhedronFaces.push_back(face);
                facesCreated++;
            }
        }
    }

    // If we need more faces, create some with more vertices
    while (facesCreated < _customPolyhedronFaceCount && _customPolyhedronVertexCount >= 4) {
        std::vector<int> face;
        for (int v = 0; v < 4 && v < _customPolyhedronVertexCount; ++v) {
            face.push_back(v);
        }
        _customPolyhedronFaces.push_back(face);
        facesCreated++;
    }
}

} // namespace Core
