#include "Util/SaveSystem.hpp"
#include "Util/Serialization.hpp"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <iostream>

using json = nlohmann::json;

void test_msgpack_roundtrip() {
    json j;
    j["schema_version"] = 1;
    j["nodes"] = {
        {"id", "law_gravity"},
        {"type", "physics"},
        {"nested", {
            {"x", 1.0},
            {"y", 2.0},
            {"array", {1, 2, 3}}
        }}
    };
    
    // Save to message pack
    std::string filename = SaveSystem::writeBinary(j, "test_compat", SaveSystem::SaveType::CUSTOM);
    assert(!filename.empty());
    
    // Load from message pack (fallback logic in readSaveData should detect binary)
    json loaded = SaveSystem::readSaveData(filename);
    
    assert(loaded["schema_version"] == 1);
    assert(loaded["nodes"]["id"] == "law_gravity");
    assert(loaded["nodes"]["nested"]["array"][1] == 2);
    
    // Test Frontier versioning pipeline
    JsonReader reader(loaded);
    auto frontierState = Frontier::load_frontier<EarthcallSaveState_V1>(reader);
    
    assert(frontierState.payload["schema_version"] == 1);
    
    std::filesystem::remove(filename);
    std::puts("serialization_compat_test: MessagePack roundtrip OK");
}

void test_json_fallback() {
    json j;
    j["schema_version"] = 1;
    j["legacy"] = true;
    
    // Write traditional JSON
    std::string filename = SaveSystem::writeJson(j, "test_legacy", SaveSystem::SaveType::CUSTOM);
    assert(!filename.empty());
    
    // Load it (fallback logic should detect plain json and parse it properly)
    json loaded = SaveSystem::readSaveData(filename);
    assert(loaded["legacy"] == true);
    
    std::filesystem::remove(filename);
    
    // Remove the parallel .ecsave produced by dry-run
    std::string binaryFile = filename;
    if (binaryFile.length() > 5) {
        binaryFile.replace(binaryFile.length() - 5, 5, ".ecsave");
        std::filesystem::remove(binaryFile);
    }
    
    std::puts("serialization_compat_test: JSON fallback OK");
}

// Face textures: a flat (never-painted) face must serialize as a colour rather
// than as 16 KB of that colour repeated, a painted one must still round-trip
// pixel-exact, and v1 saves written before "fillRGBA" existed must still load.
//
// This is the check behind an 80 MB -> 0.81 MB save on a real 290-object world:
// every one of its 1,740 stored face textures was a uniform fill.
void test_face_texture_flat_fill() {
    Object flat;
    flat.setGeometryType(Object::GeometryType::Cube);
    assert(!flat.faceTextures.empty());

    // FaceTexture::create fills the buffer with one RGBA colour — this is the
    // state every unpainted face is in.
    const size_t faceBytes = static_cast<size_t>(flat.faceTextures[0].size) *
                             static_cast<size_t>(flat.faceTextures[0].size) * 4;
    for (auto& ft : flat.faceTextures) ft.create(0xFF8000FFu);

    json jf = flat;
    assert(jf.contains("faceTextures"));
    assert(jf["textureVersion"] == 2);
    assert(jf["faceTextures"][0].contains("fillRGBA"));
    assert(!jf["faceTextures"][0].contains("pixelsB64"));

    // The whole point: the encoded face is tiny next to its pixel count.
    assert(jf["faceTextures"][0].dump().size() < faceBytes / 100);

    Object flatBack;
    flatBack.setGeometryType(Object::GeometryType::Cube);
    from_json(jf, flatBack);
    assert(flatBack.faceTextures[0].pixels.size() == faceBytes);
    assert(flatBack.faceTextures[0].pixels == flat.faceTextures[0].pixels);

    // A painted face is not uniform, so it must keep going out as pixels and
    // come back byte-identical.
    Object painted;
    painted.setGeometryType(Object::GeometryType::Cube);
    for (auto& ft : painted.faceTextures) ft.create(0x000000FFu);
    painted.faceTextures[0].pixels[0]   = 0x12;
    painted.faceTextures[0].pixels[1]   = 0x34;
    painted.faceTextures[0].pixels[4 * 7 + 2] = 0x56;

    json jp = painted;
    assert(jp["faceTextures"][0].contains("pixelsB64"));
    assert(!jp["faceTextures"][0].contains("fillRGBA"));

    Object paintedBack;
    paintedBack.setGeometryType(Object::GeometryType::Cube);
    from_json(jp, paintedBack);
    assert(paintedBack.faceTextures[0].pixels == painted.faceTextures[0].pixels);

    // A v1 save (pixelsB64 only, textureVersion 1) still loads: the reader
    // accepts both spellings, so existing saves are unaffected.
    json legacy = jp;
    legacy["textureVersion"] = 1;
    Object legacyBack;
    legacyBack.setGeometryType(Object::GeometryType::Cube);
    from_json(legacy, legacyBack);
    assert(legacyBack.faceTextures[0].pixels == painted.faceTextures[0].pixels);

    std::puts("serialization_compat_test: face texture flat-fill OK");
}

int main() {
    std::puts("serialization_compat_test: Starting...");
    test_msgpack_roundtrip();
    test_json_fallback();
    test_face_texture_flat_fill();
    std::puts("serialization_compat_test: ALL OK");
    return 0;
}
