// Scratch probe: where does the horizon frame actually go?
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/RenderMaterial.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <chrono>
#include <cstdio>
namespace { constexpr uint32_t W=512,H=512; struct MapR{bool done=false;};
void onMap(WGPUMapAsyncStatus,WGPUStringView,void*u,void*){static_cast<MapR*>(u)->done=true;} }
int main(int argc, char** argv){
    // A/B toggle retained only to exercise the renderer's optional cache-pointer
    // boundary. It is NOT a Perlin acceleration switch: the saved expression
    // reads p.y inside noise, so isHeightfieldExpr correctly refuses its grid;
    // native parity also quarantined DDA traversal for every expression until
    // its grazing-root proof is repaired. `--no-grid` therefore changes neither
    // the saved field's path nor its meaning.
    bool gridEnabled = true;
    for (int i = 1; i < argc; ++i) if (std::string(argv[i]) == "--no-grid") gridEnabled = false;
    std::setvbuf(stdout,nullptr,_IONBF,0);
    wgpu::Device gpu; if(!gpu.init()){printf("no device\n");return 1;}
    WebGpuRenderer r; if(!r.init(gpu)){printf("no renderer\n");return 1;}
    setCurrentRenderer(&r);
    WGPUTextureDescriptor td={}; td.usage=WGPUTextureUsage_RenderAttachment|WGPUTextureUsage_CopySrc;
    td.dimension=WGPUTextureDimension_2D; td.size={W,H,1}; td.format=WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount=1; td.sampleCount=1;
    WGPUTexture tex=wgpuDeviceCreateTexture(gpu.device,&td);
    WGPUTextureView view=wgpuTextureCreateView(tex,nullptr);

    // The noise floor as authored: y - 40*noise(p*0.008) over [1000,30,1000].
    // The field `perlin-ground-plane` actually carries, lifted verbatim out of
    // saves/zones/Perlin Noise Floor Zone/zone.json -- it is an Op::Noise tree,
    // and a sin/cos stand-in would understate the ALU cost enormously.
    geom::SdfNode field;
    field.op = geom::SdfOp::Leaf;
    field.prim = geom::SdfPrim::Expr;
    field.mathNode = std::shared_ptr<OntoMath::MathNode>(
        OntoMath::MathNode::fromJson(nlohmann::json::parse(std::string("{\"children\": [{\"op\": 1, \"var\": \"y\"}, {\"children\": [{\"op\": 0, \"scalarForm\": {\"terms\": [{\"c\": 40.0, \"factors\": {}}]}}, {\"children\": [{\"children\": [{\"op\": 0, \"scalarForm\": {\"terms\": [{\"c\": 0.008, \"factors\": {}}]}}, {\"children\": [{\"op\": 1, \"var\": \"p\"}, {\"children\": [{\"op\": 0, \"scalarForm\": {\"terms\": [{\"c\": 100.0, \"factors\": {}}]}}, {\"op\": 0, \"scalarForm\": {\"terms\": [{\"c\": 0.0, \"factors\": {}}]}}, {\"op\": 0, \"scalarForm\": {\"terms\": [{\"c\": 100.0, \"factors\": {}}]}}], \"op\": 2}], \"op\": 4}], \"op\": 6}], \"op\": 29}], \"op\": 6}], \"op\": 5}"))).release());
    const glm::vec3 extent(1000.f,30.f,1000.f);

    auto timeFrames=[&](const char* name, glm::vec3 eye, glm::vec3 target, int frames){
        const glm::mat4 proj=glm::perspectiveZO(glm::radians(60.f),1.f,0.1f,1000.f);
        const glm::mat4 viewM=glm::lookAt(eye,target,glm::vec3(0,1,0));
        RenderMaterial mat; mat.baseColor=glm::vec3(0.3f,0.7f,0.3f);
        // This direct renderer probe builds the same optional cache an object
        // would pass. For the saved Perlin field it is deliberately empty: the
        // AST reads ambient y through p, therefore it is not y-h(x,z). Keeping
        // the calculation here verifies the structural eligibility boundary;
        // it does not turn an empirical or syntactic approximation into a skip.
        geom::HeightGrid heightGrid;
        const OntoMath::MathNode* h = nullptr;
        if (geom::isHeightfieldExpr(field, &h) && h) {
            const int dimX = std::clamp(static_cast<int>(extent.x/5.0f), 24, 128);
            const int dimZ = std::clamp(static_cast<int>(extent.z/5.0f), 24, 128);
            heightGrid = geom::computeHeightGrid(*h, extent, dimX, dimZ);
        }
        const geom::HeightGrid* hgArg = (gridEnabled && heightGrid.dimX > 0) ? &heightGrid : nullptr;
        // warm up (pipeline compile) outside the timing
        for(int w=0;w<2;++w){
            r.setCamera(viewM,proj,eye); r.setModel(glm::mat4(1.f));
            r.beginFrameOffscreen(view,W,H,glm::vec4(0,0,0,1));
            r.drawImplicit(field,extent,mat,nullptr,0,0,hgArg); r.endFrame();
        }
        wgpuDevicePoll(gpu.device,true,nullptr);
        auto t0=std::chrono::high_resolution_clock::now();
        for(int i=0;i<frames;++i){
            r.setCamera(viewM,proj,eye); r.setModel(glm::mat4(1.f));
            r.beginFrameOffscreen(view,W,H,glm::vec4(0,0,0,1));
            r.drawImplicit(field,extent,mat,nullptr,0,0,hgArg); r.endFrame();
            wgpuDevicePoll(gpu.device,true,nullptr);
        }
        auto t1=std::chrono::high_resolution_clock::now();
        double ms=std::chrono::duration<double,std::milli>(t1-t0).count()/frames;
        printf("  %-28s %7.2f ms / frame at %ux%u\n",name,ms,W,H);
        return ms;
    };
    // Floor: the same frame with no implicit draw at all. Anything the three
    // camera cases share with this is harness cost, not marcher cost.
    {
        const glm::mat4 proj=glm::perspectiveZO(glm::radians(60.f),1.f,0.1f,1000.f);
        const glm::vec3 eye(0,120,0);
        const glm::mat4 viewM=glm::lookAt(eye,glm::vec3(0),glm::vec3(0,1,0));
        for(int w=0;w<2;++w){ r.setCamera(viewM,proj,eye); r.setModel(glm::mat4(1.f));
            r.beginFrameOffscreen(view,W,H,glm::vec4(0,0,0,1)); r.endFrame(); }
        wgpuDevicePoll(gpu.device,true,nullptr);
        auto t0=std::chrono::high_resolution_clock::now();
        for(int i=0;i<30;++i){ r.setCamera(viewM,proj,eye); r.setModel(glm::mat4(1.f));
            r.beginFrameOffscreen(view,W,H,glm::vec4(0,0,0,1)); r.endFrame();
            wgpuDevicePoll(gpu.device,true,nullptr); }
        auto t1=std::chrono::high_resolution_clock::now();
        printf("  %-28s %7.2f ms / frame  (EMPTY FRAME -- harness floor)\n","no draw at all",
               std::chrono::duration<double,std::milli>(t1-t0).count()/30);
    }
    // Same extents, same cameras, TRIVIAL field (a flat plane, one op). Whatever
    // this still costs is the march and the fragments, not the mathematics.
    {
        geom::SdfNode flat = geom::makeImplicit("y");
        geom::SdfNode save = field;
        field = flat;
        printf("=== TRIVIAL field (y), same extents ===\n");
        timeFrames("looking straight down", glm::vec3(0,120,0), glm::vec3(0,0,0), 30);
        timeFrames("grazing the horizon",   glm::vec3(0,60,-900), glm::vec3(0,55,900), 30);
        timeFrames("45 degrees down",       glm::vec3(0,120,-200), glm::vec3(0,0,0), 30);
        field = save;
    }
    printf("=== implicit terrain (the authored Perlin field), 512x512, 60 deg FOV ===\n");
    double down = timeFrames("looking straight down", glm::vec3(0,120,0), glm::vec3(0,0,0), 30);
    double horiz= timeFrames("grazing the horizon",   glm::vec3(0,60,-900), glm::vec3(0,55,900), 30);
    double tilt = timeFrames("45 degrees down",       glm::vec3(0,120,-200), glm::vec3(0,0,0), 30);
    printf("  horizon / down = %.1fx\n", horiz/(down>1e-9?down:1e-9));
    (void)tilt;
    return 0;
}
