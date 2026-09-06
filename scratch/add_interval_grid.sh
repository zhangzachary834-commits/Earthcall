sed -i '' 's/HeightGrid computeHeightGrid(const OntoMath::MathNode& h, const glm::vec3& halfExtent,/HeightGrid computeRegionalHeightGrid(const SdfNode\& n, const glm::vec3\& halfExtent, int dimX, int dimZ);\nHeightGrid computeHeightGrid(const OntoMath::MathNode\& h, const glm::vec3\& halfExtent,/' src/ConstructedBeing/Singular/Object/Geometry/Sdf.hpp

cat << 'INNER_EOF' >> src/ConstructedBeing/Singular/Object/Geometry/Sdf.cpp

HeightGrid computeRegionalHeightGrid(const SdfNode& n, const glm::vec3& halfExtent, int dimX, int dimZ) {
    HeightGrid grid;
    if (dimX <= 0 || dimZ <= 0) return grid;

    grid.dimX = dimX;
    grid.dimZ = dimZ;
    grid.cells.resize(static_cast<size_t>(dimX) * static_cast<size_t>(dimZ));

    const float cellSizeX = (2.0f * halfExtent.x) / static_cast<float>(dimX);
    const float cellSizeZ = (2.0f * halfExtent.z) / static_cast<float>(dimZ);
    const int Y_BINS = 64;
    const float cellSizeY = (2.0f * halfExtent.y) / static_cast<float>(Y_BINS);

    for (int iz = 0; iz < dimZ; ++iz) {
        float zMin = -halfExtent.z + iz * cellSizeZ;
        float zMax = zMin + cellSizeZ;
        for (int ix = 0; ix < dimX; ++ix) {
            float xMin = -halfExtent.x + ix * cellSizeX;
            float xMax = xMin + cellSizeX;
            
            float cellYMin = halfExtent.y;
            float cellYMax = -halfExtent.y;
            bool hitAny = false;

            for (int iy = 0; iy < Y_BINS; ++iy) {
                float yMin = -halfExtent.y + iy * cellSizeY;
                float yMax = yMin + cellSizeY;
                OntoMath::Interval range = evalRange(n, glm::vec3(xMin, yMin, zMin), glm::vec3(xMax, yMax, zMax));
                if (range.lo <= 0.0f && range.hi >= 0.0f) {
                    if (!hitAny) {
                        cellYMin = yMin;
                        hitAny = true;
                    }
                    cellYMax = yMax;
                }
            }

            auto& cell = grid.cells[iz * dimX + ix];
            if (hitAny) {
                cell.first = cellYMin;
                cell.second = cellYMax;
            } else {
                cell.first = halfExtent.y; // Empty cell
                cell.second = -halfExtent.y;
            }
        }
    }
    return grid;
}
INNER_EOF

