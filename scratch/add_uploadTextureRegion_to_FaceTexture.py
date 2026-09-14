import os

with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "r") as f:
    content = f.read()

# We need to change writeSamples to track bounding box.
new_writeSamples = """
bool FaceTexture::writeSamples(const std::vector<glm::ivec2>& coordinates,
                               const std::vector<glm::vec3>& colors) {
    if (width <= 0 || height <= 0 || coordinates.size() != colors.size()) return false;
    
    int minX = width, minY = height, maxX = -1, maxY = -1;
    
    for (const glm::ivec2& xy : coordinates) {
        if (xy.x < 0 || xy.y < 0 || xy.x >= width || xy.y >= height) return false;
        if (xy.x < minX) minX = xy.x;
        if (xy.y < minY) minY = xy.y;
        if (xy.x > maxX) maxX = xy.x;
        if (xy.y > maxY) maxY = xy.y;
    }
    
    if (minX > maxX || minY > maxY) return false;
    
    for (const glm::vec3& color : colors) {
        if (!std::isfinite(color.r) || !std::isfinite(color.g) ||
            !std::isfinite(color.b)) {
            return false;
        }
    }
    const std::size_t expected = static_cast<std::size_t>(width) * height * 4;
    if (pixels.size() != expected) return false;
    const auto channel = [](float value) {
        return static_cast<uint8_t>(std::lround(value * 255.0f));
    };

    auto write = [&](std::vector<uint8_t>& buffer) {
        if (buffer.size() != expected) return false;
        std::size_t i = 0;
        for (const glm::ivec2& xy : coordinates) {
            const glm::vec3 clamped = glm::clamp(
                colors[i++], glm::vec3(0.0f), glm::vec3(1.0f));
            const std::size_t offset = static_cast<std::size_t>(xy.y * width + xy.x) * 4;
            buffer[offset] = channel(clamped.r);
            buffer[offset + 1] = channel(clamped.g);
            buffer[offset + 2] = channel(clamped.b);
            buffer[offset + 3] = 255;
        }
        return true;
    };

    if (useLayers) {
        if (activeLayer < 0 || activeLayer >= static_cast<int>(layers.size()) ||
            !write(layers[activeLayer])) {
            return false;
        }
        compositeLayers(); // Wait, compositeLayers updates the whole thing! 
    } else if (!write(pixels)) {
        return false;
    }
    
    if (id == 0 || useLayers) {
        uploadToGPU();
    } else {
        uint32_t regionW = maxX - minX + 1;
        uint32_t regionH = maxY - minY + 1;
        currentRenderer().uploadTextureRegion(id, pixels.data(), width, height, minX, minY, regionW, regionH);
    }
    return true;
}
"""

start_idx = content.find("bool FaceTexture::writeSamples(")
end_idx = content.find("void FaceTexture::saveStrokeState()")

content = content[:start_idx] + new_writeSamples + "\n" + content[end_idx:]

with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "w") as f:
    f.write(content)
