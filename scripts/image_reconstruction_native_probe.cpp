// Zach commissioned image pixels -> editable world -> WebGPU pixels.
// Standalone desktop witness linked by scripts/verify_image_reconstruction.py.
// Codex / GPT-6 / 01a0fe15-4fe2-7dc0-a2d0-7d823e4ad26c / 2026-10-02.
#include "Singularity/Screen/ImageCodecChannel.hpp"
#include "Singularity/Screen/ScreenRecorder.hpp"
#include "Singularity/Screen/WebGPU/WebGpuContext.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "Person/Person.hpp"
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <fstream>
#include <cstdio>
#include <thread>
#include <filesystem>
extern MaterialManager materials;
int main(int argc, char** argv) {
    if (argc != 4) return 2;
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    nlohmann::json seed; std::ifstream(argv[2]) >> seed;
    Zone sourceZone("Source witness", "source-witness");
    auto imported = ImageCodecChannel::ingestPng(argv[1], "native-witness", sourceZone);
    auto source = imported.first; auto texture = imported.second;
    if (!source || !texture) return 3;
    lawSetValue(*source,PropertyPath::parse("x2D"),PropertyValue(0.0));
    lawSetValue(*source,PropertyPath::parse("y2D"),PropertyValue(0.0));
    int width = texture->faceTextures[0].width, height = texture->faceTextures[0].height;
    const auto expected = texture->faceTextures[0].pixels;
    materials.mergeFromJson(seed["zones"][0]["materials"]);
    std::vector<std::shared_ptr<Object>> objects;
    for (const auto& record : seed["zones"][0]["world"]["objects"]) {
        auto obj = std::make_shared<Object>(); from_json(record, *obj);
        nlohmann::json roundtrip; to_json(roundtrip, *obj);
        Object restored; from_json(roundtrip, restored);
        PropertyValue border, author;
        if (restored.getIdentifier() != obj->getIdentifier() ||
            !restored.getDynamicProperty("border.visible", border) || std::get<bool>(border) ||
            !restored.getDynamicProperty("reconstruction.author", author) ||
            std::get<std::string>(author) != seed["authors"][0].get<std::string>() ||
            restored.materialId() != obj->materialId() ||
            restored.getX2D() != obj->getX2D() || restored.getY2D() != obj->getY2D() ||
            restored.getShapeParams().width2D != obj->getShapeParams().width2D ||
            restored.getShapeParams().height2D != obj->getShapeParams().height2D ||
            !materials.get(obj->materialId())->faceTextures.empty()) return 4;
        objects.push_back(obj);
    }
    if (!glfwInit()) return 5;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto* window = glfwCreateWindow(width/2, height/2, "Earthcall image reconstruction witness", nullptr, nullptr);
    if (!window) return 6;
    auto ctx = wgpu::createWindowContext(window);
    if (!ctx.valid()) return 7;
    int w,h; glfwGetFramebufferSize(window,&w,&h);
    if (w != width || h != height) { std::printf("Physical size mismatch %dx%d vs %dx%d\n",w,h,width,height); return 8; }
    wgpu::configureSurface(ctx,w,h);
    wgpu::Device gpu; gpu.instance=ctx.instance; gpu.adapter=ctx.adapter;
    gpu.device=ctx.device; gpu.queue=ctx.queue;
    WebGpuRenderer renderer;
    if (!renderer.init(gpu,wgpu::kSurfaceFormat)) return 9;
    renderer.attachSurface(ctx.surface,ctx.instance); setCurrentRenderer(&renderer);
    std::filesystem::create_directories(argv[3]);
    auto frame = [&](bool faithful, const char* name) {
        std::vector<uint8_t> pixels(size_t(w)*h*4);
        for (int attempt=0;attempt<10;++attempt) {
            glfwPollEvents(); renderer.beginFrame(w,h,{0,0,0,1}); renderer.begin2D(w,h);
            if (faithful) source->draw2DObject(w,h);
            else for (auto& obj:objects) obj->draw2DObject(w,h);
            renderer.end2D(); renderer.endFrame();
            bool ready=renderer.readPixels(pixels.data(),w,h); renderer.present();
            if (ready) {
                auto path=std::filesystem::path(argv[3])/name;
                if (!Singularity::Screen::ScreenRecorder::writePng(path.string(),pixels.data(),w,h)) throw std::runtime_error("PNG write failed");
                return pixels;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        throw std::runtime_error("Native surface unavailable");
    };
    auto faithful=frame(true,"faithful.png");
    auto reconstructed=frame(false,"reconstructed.png");
    size_t faithfulErrors=0, geometryErrors=0;
    for(size_t i=0;i<expected.size();++i) {
        faithfulErrors += faithful[i]!=expected[i]; geometryErrors += reconstructed[i]!=expected[i];
    }
    // An actual authored Law changes a region's geometry through the shared
    // property mechanism. No test-only rendering or direct geometry setter.
    Soul soul("Witness author"); Body body("humanoid","default");
    Person person(std::move(soul),std::move(body),"default");
    Law edit("reconstruction-edit-witness"); edit.addAuthor(person);
    auto target=objects.front(); float chosenArea=0;
    for(auto& obj:objects) {
        const auto& sp=obj->getShapeParams(); const float* c=obj->faceColors[0];
        if(*std::max_element(c,c+3)-*std::min_element(c,c+3)>0.2f && sp.width2D*sp.height2D>chosenArea) {
            target=obj; chosenArea=sp.width2D*sp.height2D;
        }
    }
    const int x0=int(target->getX2D()), y0=int(target->getY2D());
    const int x1=x0+int(target->getShapeParams().width2D), y1=y0+int(target->getShapeParams().height2D);
    target->removeDynamicProperty("border.visible");
    auto defaultBorder=frame(false,"default-border.png");
    target->setDynamicProperty("border.visible",PropertyValue(false));
    const bool oldBordersPreserved=defaultBorder!=reconstructed;
    PropertyValue oldWidth; lawGetValue(*target,PropertyPath::parse("shape.width2D"),oldWidth);
    edit.setActionModel(ActionNode::set("shape.width2D",PropertyValue(0.0)));
    auto result=edit.applyTo(*target);
    PropertyValue changedWidth; lawGetValue(*target,PropertyPath::parse("shape.width2D"),changedWidth);
    auto edited=frame(false,"edited.png");
    size_t changed=0, outside=0; for(size_t i=0;i<edited.size();i+=4) {
        bool differs=!std::equal(edited.begin()+i,edited.begin()+i+4,reconstructed.begin()+i);
        changed+=differs; int x=int((i/4)%w), y=int((i/4)/w);
        outside+=differs && !(x>=x0 && x<x1 && y>=y0 && y<y1);
    }
    edit.setActionModel(ActionNode::set("shape.width2D",oldWidth)); edit.applyTo(*target);
    edit.setActionModel(ActionNode::set("color",PropertyValue(glm::vec3(0,1,0)))); edit.applyTo(*target);
    auto painted=frame(false,"painted.png"); size_t paintErrors=0;
    for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
        size_t i=(size_t(y)*w+x)*4; bool inside=x>=x0 && x<x1 && y>=y0 && y<y1;
        if(inside) paintErrors+=painted[i]!=0 || painted[i+1]!=255 || painted[i+2]!=0 || painted[i+3]!=255;
        else paintErrors+=!std::equal(painted.begin()+i,painted.begin()+i+4,reconstructed.begin()+i);
    }
    bool ok=!faithfulErrors && !geometryErrors && changed>0 &&
        result==Law::ApplicationResult::Applied && std::get<float>(changedWidth)==0.0f &&
        !outside && !paintErrors && oldBordersPreserved && changed==size_t((x1-x0)*(y1-y0));
    std::printf("RECONSTRUCTION %s physical=%dx%d regions=%zu faithful_byte_errors=%zu geometry_byte_errors=%zu law_changed_pixels=%zu outside_changes=%zu paint_errors=%zu legacy_border=%d roundtrip=PASS texture_free_geometry=PASS\n",
        ok?"PASS":"FAIL",w,h,objects.size(),faithfulErrors,geometryErrors,changed,outside,paintErrors,int(oldBordersPreserved));
    setCurrentRenderer(nullptr); renderer.shutdown(); wgpu::destroyWindowContext(ctx);
    glfwDestroyWindow(window); glfwTerminate(); return ok?0:10;
}
