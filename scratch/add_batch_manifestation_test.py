import os

with open("tests/singularity/frame_lag_test.cpp", "r") as f:
    content = f.read()

test_code = """
// ===========================================================================
// 5. BATCH MANIFESTATION — does region elevation and property mutation avoid
//    Rete fact explosion and lag spikes?
// ===========================================================================
namespace {

void checkBatchManifestation(Zone& zone, LawManager& lawManager, double& worldTime) {
    std::printf("\\n--- RUNNING BATCH MANIFESTATION ---\\n");
    
    // 1. Create a macro image
    auto macro = std::make_shared<Object>("image.test_batch");
    auto mat = std::make_shared<Material>("material.test_batch");
    mat->initFaceTextures(1);
    // 256x256 image
    mat->faceTextures[0].width = 256;
    mat->faceTextures[0].height = 256;
    mat->faceTextures[0].pixels.resize(256 * 256 * 4, 255);
    macro->_materialId = mat->getIdentifier();
    zone.addBeing(macro);
    // register material so resolveRenderMaterial doesn't fail if needed, though this is a headless test
    // We can't directly add to materials here without accessing Universe, but Universe::instance().setProvider handles it.
    
    // 2. Elevate a large region
    OntoMath::Piecewise selector;
    selector.inputVariable = "u";
    OntoMath::Piecewise::Piece regionPiece;
    regionPiece.hasHi = true;
    regionPiece.hi = 0.5;
    regionPiece.includeHi = false;
    regionPiece.mathNode = OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::constant(1.0));
    selector.pieces.push_back(regionPiece);
    
    std::string reason;
    bool elevated = macro->elevateSurfaceRegionProperty("authored.left_half", 0, selector, reason);
    if (!elevated) {
        std::printf("FAIL: ElevatePixels failed: %s\\n", reason.c_str());
        gFailures++;
        return;
    }
    
    // 3. Mutate the region property many times
    double maxMs = 0.0;
    for (int i=0; i<10; i++) {
        double t0 = glfwGetTime();
        
        PropertyValue color(glm::vec3(0.0f, static_cast<float>(i)/10.0f, 1.0f));
        macro->setDynamicProperty("authored.left_half", color);
        lawManager.tick();
        
        double t1 = glfwGetTime();
        double ms = (t1 - t0) * 1000.0;
        if (ms > maxMs) maxMs = ms;
    }
    
    std::printf("Max write-batch tick time: %.3f ms\\n", maxMs);
    if (maxMs > 5.0) { // Should easily be < 5.0 ms
        std::printf("FAIL: Batch manifestation lag exceeds sub-frame bounds!\\n");
        gFailures++;
    } else {
        std::printf("PASS: Batch manifestation executes in sub-frame bounds.\\n");
    }
}

} // namespace
"""

# Insert before "int main"
main_idx = content.find("int main(int argc, char** argv) {")
content = content[:main_idx] + test_code + "\n" + content[main_idx:]

# Call checkBatchManifestation inside main
call_str = "    checkBatchManifestation(*active, lawManager, worldTime);\n"
call_idx = content.find("    checkLoadTime(filename, loadMs, active->getOwnedObjects().size());")
content = content[:call_idx] + call_str + content[call_idx:]

with open("tests/singularity/frame_lag_test.cpp", "w") as f:
    f.write(content)
