// Production Screen/compiler/GPU witness; never reads or writes inhabited saves.
// Codex / GPT-6.1 Sol / 01a10a2b-a247-7c11-9d5f-7a8b89df6cfc /
// 2026-10-04 PDT. Zach requested direct mathematical Screen manifestation.
#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "Person/Person.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Screen/ScreenRecorder.hpp"
#include <GLFW/glfw3.h>
#include "../../third_party/stb/stb_image.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <vector>

namespace {
constexpr uint32_t W = 32, H = 24;
void check(bool yes, const char* what) {
    if (!yes) { std::fprintf(stderr, "FAIL: %s\n", what); std::exit(1); }
    std::printf("PASS: %s\n", what);
}
std::shared_ptr<OntoMath::MathNode> scalar(const OntoMath::ScalarForm& value) {
    return OntoMath::MathNode::fromLegacyExpression(value);
}
std::shared_ptr<OntoMath::MathNode> constant(double value) {
    return scalar(OntoMath::ScalarForm::constant(value));
}
std::shared_ptr<OntoMath::MathNode> vector(const OntoMath::ScalarForm& x,
                                        const OntoMath::ScalarForm& y,
                                        const OntoMath::ScalarForm& z) {
    auto n = std::make_shared<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::VectorConstruct;
    for (const auto* f : {&x, &y, &z})
        n->children.push_back(std::make_unique<OntoMath::MathNode>(*scalar(*f)));
    return n;
}
std::shared_ptr<OntoMath::VectorField> field(OntoMath::Piecewise form) {
    auto f = std::make_shared<OntoMath::VectorField>();
    f->mode = OntoMath::VectorField::EvaluationMode::AST;
    f->astDefinition = std::move(form);
    return f;
}
struct Mapping { bool done = false, ok = false; };
void onMap(WGPUMapAsyncStatus status, WGPUStringView, void* data, void*) {
    auto& m = *static_cast<Mapping*>(data);
    m.ok = status == WGPUMapAsyncStatus_Success; m.done = true;
}
}

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--engine") {
        auto& engine = Core::Engine::instance();
        check(engine.init(1,argv), "isolated production Engine initialized");
        auto* channel = Singularity::Screen::ScreenChannel::find(*engine.getLawManager());
        auto* recorder = Singularity::Screen::ScreenRecorder::find(*engine.getLawManager());
        check(channel && recorder && engine.getPerson(), "live Screen channel and Person found");
        auto nativeForm = field(OntoMath::Piecewise::continuous(vector(
            OntoMath::ScalarForm::variable("u"), OntoMath::ScalarForm::variable("v"),
            OntoMath::ScalarForm::constant(.25))));
        Law authoring("native-direct-screen-witness", {engine.getPerson()});
        authoring.setLawIdentifier("law-native-direct-screen-witness");
        authoring.setActionModel(ActionNode::fromJson(
            ActionNode::addProperty("", "output.color", nativeForm).toJson()));
        check(authoring.applyTo(*channel) == Law::ApplicationResult::Applied,
            "live Person-authored Law supplies direct Screen form");
        lawSetValue(*recorder,PropertyPath::parse("outputPath"),std::string("capture"));
        lawSetValue(*recorder,PropertyPath::parse("recordCursor"),false);
        engine.tick(.016f);
        lawSetValue(*recorder,PropertyPath::parse("snapshot"),true);
        engine.tick(.016f);
        PropertyValue drawn,refusal,path;
        lawGetValue(*channel,PropertyPath::parse("output.drawn"),drawn);
        lawGetValue(*channel,PropertyPath::parse("output.lastRefusal"),refusal);
        lawGetValue(*recorder,PropertyPath::parse("lastSnapshotPath"),path);
        check(std::get<bool>(drawn) && std::get<std::string>(refusal).empty(),
            "Engine::tick actually manifests the direct form");
        check(std::holds_alternative<std::string>(path) && !std::get<std::string>(path).empty(),
            "live viewport capture exists for independent pixel verification");
        int width=0,height=0; glfwGetFramebufferSize(engine.window(),&width,&height);
        int decodedWidth=0,decodedHeight=0,channels=0;
        auto* captured=stbi_load(std::get<std::string>(path).c_str(),
            &decodedWidth,&decodedHeight,&channels,4);
        check(captured && decodedWidth==width && decodedHeight==height,
            "production PNG capture decodes at native framebuffer resolution");
        int maxError=0;
        for(int y=0;y<height;++y) for(int x=0;x<width;++x) {
            const int expected[4]={int(std::round(255.0*(x+.5)/width)),
                int(std::round(255.0*(y+.5)/height)),64,255};
            for(int c=0;c<4;++c)
                maxError=std::max(maxError,std::abs(int(captured[(y*width+x)*4+c])-expected[c]));
        }
        stbi_image_free(captured);
        check(maxError<=1,"every production viewport pixel agrees with independent mathematical expectation");
        std::printf("ENGINE_SCREEN_PIXEL_RESULT pixels=%d maxByteError=%d\n",width*height,maxError);
        std::ofstream("engine-screen-witness.json") << nlohmann::json{
            {"capture",std::get<std::string>(path)},{"width",width},{"height",height},
            {"fixtureAuthor",engine.getPerson()->getIdentifier()},
            {"pixelsChecked",width*height},{"maxByteError",maxError}}.dump(2);
        channel->removeDynamicProperty("output.color");
        engine.tick(.016f);
        lawGetValue(*channel,PropertyPath::parse("output.drawn"),drawn);
        check(!std::get<bool>(drawn), "live removal withdraws direct manifestation");
        engine.shutdown();
        std::puts("DIRECT_SCREEN_ENGINE_RESULT PASS");
        return 0;
    }
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    using namespace OntoMath;
    const auto zero = ScalarForm::constant(0);
    const auto one = ScalarForm::constant(1);
    auto gradient = field(Piecewise::continuous(vector(ScalarForm::variable("u"),
        ScalarForm::variable("v"), ScalarForm::constant(.25))));
    auto red = field(Piecewise::continuous(vector(one, zero, zero)));
    check(sdfwgsl::compileScreenForm(gradient->astDefinition).ok, "physical and normalized coordinates compile");
    auto editedGradient = *gradient;
    editedGradient.astDefinition.pieces[0].mathNode = vector(ScalarForm::variable("u"),
        ScalarForm::variable("v"), ScalarForm::constant(.75));
    const auto a = sdfwgsl::compileScreenForm(gradient->astDefinition);
    const auto b = sdfwgsl::compileScreenForm(editedGradient.astDefinition);
    check(a.wgsl == b.wgsl && a.params != b.params, "numeric edits preserve exact pipeline structure");
    auto temporal = field(Piecewise::continuous(vector(ScalarForm::variable("t"), zero, zero)));
    check(!sdfwgsl::compileScreenForm(temporal->astDefinition).ok &&
        sdfwgsl::compileScreenForm(temporal->astDefinition, nullptr, true).ok,
        "time must be explicitly admitted");
    auto invalid = field(Piecewise::continuous(vector(ScalarForm::variable("alien"), zero, zero)));
    check(!sdfwgsl::compileScreenForm(invalid->astDefinition).ok, "unbound coordinates refuse");
    Piecewise scalarColor = Piecewise::continuous(constant(1));
    check(!sdfwgsl::compileScreenForm(scalarColor).ok, "scalar color refuses before GPU submission");
    check(propertyValueToJson(propertyValueFromJson(propertyValueToJson(gradient))) == propertyValueToJson(gradient),
        "typed vector field preserves the entire mathematical payload");
    check(std::holds_alternative<std::monostate>(propertyValueFromJson(nlohmann::json{{"t","vector_field"}})),
        "old tag-only field refuses invented mathematics");
    auto guarded = gradient->astDefinition;
    guarded.pieces[0].guard = std::make_shared<ConditionNode>();
    check(!sdfwgsl::compileScreenForm(guarded).ok, "unresolved world guards refuse");
    auto unknown = gradient->astDefinition;
    unknown.pieces[0].mathNode = std::make_shared<MathNode>(*unknown.pieces[0].mathNode);
    unknown.pieces[0].mathNode->op = static_cast<MathNode::Op>(999);
    check(!sdfwgsl::compileScreenForm(unknown).ok, "unknown serialized operators refuse");
    auto infinite = field(Piecewise::continuous(vector(
        ScalarForm::constant(std::numeric_limits<double>::infinity()), zero, zero)));
    check(!sdfwgsl::compileScreenForm(infinite->astDefinition).ok, "nonfinite parameters refuse");

    wgpu::Device gpu;
    check(gpu.init(), "native WebGPU device acquired");
    WebGpuRenderer renderer;
    check(renderer.init(gpu, WGPUTextureFormat_RGBA8Unorm), "production renderer initialized");
    setCurrentRenderer(&renderer);
    WGPUTextureDescriptor td = {};
    td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    td.dimension = WGPUTextureDimension_2D; td.size = {W,H,1};
    td.format = WGPUTextureFormat_RGBA8Unorm; td.mipLevelCount = 1; td.sampleCount = 1;
    WGPUTexture target = wgpuDeviceCreateTexture(gpu.device, &td);
    WGPUTextureView view = wgpuTextureCreateView(target, nullptr);
    WGPUBufferDescriptor bd = {};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead; bd.size = 256 * H;
    WGPUBuffer readback = wgpuDeviceCreateBuffer(gpu.device, &bd);
    auto read = [&]() {
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
        WGPUTexelCopyTextureInfo src = {};
        src.texture = target; src.aspect = WGPUTextureAspect_All;
        WGPUTexelCopyBufferInfo dst = {};
        dst.buffer = readback; dst.layout.bytesPerRow = 256; dst.layout.rowsPerImage = H;
        WGPUExtent3D extent = {W,H,1};
        wgpuCommandEncoderCopyTextureToBuffer(encoder, &src, &dst, &extent);
        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(gpu.queue, 1, &command);
        wgpuCommandBufferRelease(command); wgpuCommandEncoderRelease(encoder);
        Mapping mapping;
        WGPUBufferMapCallbackInfo callback = {};
        callback.mode = WGPUCallbackMode_AllowProcessEvents;
        callback.callback = onMap; callback.userdata1 = &mapping;
        wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, 256*H, callback);
        while (!mapping.done) wgpuInstanceProcessEvents(gpu.instance);
        check(mapping.ok, "native frame readback succeeds");
        const auto* bytes = static_cast<const unsigned char*>(wgpuBufferGetConstMappedRange(readback, 0, 256*H));
        std::vector<unsigned char> frame(W*H*4);
        for (uint32_t y=0; y<H; ++y)
            std::copy(bytes+y*256, bytes+y*256+W*4, frame.begin()+y*W*4);
        wgpuBufferUnmap(readback);
        return frame;
    };
    Singularity::Screen::ScreenChannel channel;
    Singularity::Language::Lexeme source("authored field", "lexeme.direct-screen-field");
    Person author(Soul("direct-screen-test-person"), Body(), "default");
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) { beings = {&channel, &source, &author}; });
    const auto grant = [&](Singular& subject, const char* name, const PropertyValue& value) {
        Law law("direct-screen-test-grant", {&author});
        law.setActionModel(ActionNode::fromJson(ActionNode::addProperty("", name, value).toJson()));
        check(law.applyTo(subject) == Law::ApplicationResult::Applied, "authored AddProperty round-trip applies through Law");
    };
    grant(source, "color", gradient);
    grant(channel, "output.colorPath", std::string("@lexeme.direct-screen-field.color"));
    auto render = [&]() {
        renderer.beginFrameOffscreen(view, W,H, glm::vec4(0,0,1,1));
        bool ok = channel.manifestOutput(renderer,W,H);
        renderer.endFrame();
        return std::make_pair(ok,read());
    };
    auto [ok, pixels] = render();
    check(ok, "non-Object Singular manifests through the production Screen binding");
    for (uint32_t y=0;y<H;++y) for (uint32_t x=0;x<W;++x) {
        std::map<std::string,PropertyValue> vars{{"u",double(x+.5)/W},{"v",double(y+.5)/H},
            {"x",double(x+.5)},{"y",double(y+.5)},{"z",0.0},{"p",glm::vec3(x+.5,y+.5,0)},
            {"width",double(W)},{"height",double(H)}};
        const auto value = gradient->astDefinition.evaluate(vars);
        if (!value || !std::holds_alternative<glm::vec3>(*value)) check(false, "CPU form is defined");
        const auto rgb = std::get<glm::vec3>(*value);
        for (int c=0;c<3;++c)
            if (std::abs(int(pixels[(y*W+x)*4+c])-int(std::round(rgb[c]*255)))>1)
                { std::fprintf(stderr,"FAIL: native/CPU pixel %u,%u component %d\n",x,y,c); return 2; }
    }
    check(true,"every native gradient pixel agrees with independent CPU evaluation");
    source.setDynamicProperty("color",std::make_shared<VectorField>(editedGradient));
    auto numeric=render(); check(numeric.first && std::abs(int(numeric.second[2])-191)<=1,
        "numeric field edit updates native samples through the shared pipeline structure");
    auto composed = field(Piecewise::continuous(vector(ScalarForm::variable("u"),
        ScalarForm::variable("u").times(ScalarForm::variable("v")), ScalarForm::constant(.25))));
    auto rebound = std::make_unique<MathNode>(); rebound->op=MathNode::Op::SDF;
    rebound->children.push_back(std::make_unique<MathNode>(*scalar(ScalarForm::variable("u"))));
    rebound->children.push_back(std::make_unique<MathNode>(*vector(
        ScalarForm::constant(.1),ScalarForm::constant(.2),zero)));
    composed->astDefinition.pieces[0].mathNode->children[0]=std::move(rebound);
    source.setDynamicProperty("color",composed);
    auto composited=render(); check(composited.first,"composed normalized coordinates and point substitution manifest");
    for(uint32_t y=0;y<H;++y) for(uint32_t x=0;x<W;++x) {
        double u=double(x+.5)/W,v=double(y+.5)/H;
        auto value=composed->astDefinition.evaluate({{"u",u},{"v",v},{"p",glm::vec3(x+.5,y+.5,0)},
            {"x",double(x+.5)},{"y",double(y+.5)},{"z",0.0}});
        if(!value || !std::holds_alternative<glm::vec3>(*value)) check(false,"composed CPU expression is defined");
        auto rgb=std::get<glm::vec3>(*value);
        for(int c=0;c<3;++c)
            if(std::abs(int(composited.second[(y*W+x)*4+c])-int(std::round(rgb[c]*255)))>1)
                check(false,"normalized expression composition preserves CPU meaning");
    }
    check(true,"every composed sample preserves independent scalar bindings across SDF point substitution");
    std::ofstream("direct-screen-gradient.rgba",std::ios::binary).write(reinterpret_cast<const char*>(pixels.data()),pixels.size());

    auto single = *red;
    auto selector = std::make_shared<MathNode>();
    selector->op = MathNode::Op::Sub;
    auto distance = std::make_unique<MathNode>(); distance->op = MathNode::Op::Distance;
    auto p = std::make_unique<MathNode>(); p->op = MathNode::Op::ValueLeaf; p->variableName = "p";
    distance->children.push_back(std::move(p));
    distance->children.push_back(std::make_unique<MathNode>(*vector(ScalarForm::constant(7.5),ScalarForm::constant(9.5),zero)));
    selector->children.push_back(std::move(distance));
    selector->children.push_back(std::make_unique<MathNode>(*constant(.1)));
    single.astDefinition.pieces[0].whereLEZero = selector;
    Law replace("direct-screen-test-set", {&author});
    replace.setActionModel(ActionNode::fromJson(ActionNode::set("@lexeme.direct-screen-field.color",
        std::make_shared<VectorField>(single)).toJson()));
    check(replace.applyTo(channel)==Law::ApplicationResult::Applied, "authored Set replaces the source field");
    auto singleFrame = render(); check(singleFrame.first,"arbitrary pure region manifests");
    for (uint32_t y=0;y<H;++y) for(uint32_t x=0;x<W;++x) {
        auto at=(y*W+x)*4; bool chosen=x==7&&y==9;
        if (!(singleFrame.second[at]==(chosen?255:0) && singleFrame.second[at+1]==0 &&
            singleFrame.second[at+2]==(chosen?0:255))) check(false, "single physical pixel selection preserves all other samples");
    }
    check(true,"exactly one physical pixel changed; every other native sample survived");
    check(Singularity::Screen::ScreenRecorder::writePng("single-pixel.png",singleFrame.second.data(),W,H),
        "single physical pixel frame exported from native readback");
    auto interval = *red; interval.astDefinition.inputVariable="x";
    auto& piece=interval.astDefinition.pieces[0];
    piece.hasLo=piece.hasHi=true; piece.lo=7.5; piece.hi=8.5; piece.includeLo=false;
    source.setDynamicProperty("color",std::make_shared<VectorField>(interval));
    auto intervalFrame=render(); check(intervalFrame.first,"exclusive interval bound manifests");
    for(uint32_t y=0;y<H;++y) for(uint32_t x=0;x<W;++x)
        if (intervalFrame.second[(y*W+x)*4]!=(x==8?255:0)) check(false,"GPU preserves authored endpoint inclusion");

    check(true,"GPU preserves authored endpoint inclusion at every sample");
    ActionNode example = ActionNode::addProperty("@screen-channel", "output.color", gradient);
    std::ofstream("direct-screen-form-action.json") << example.toJson().dump(2);
    source.setDynamicProperty("color",red);
    auto opacity=std::make_shared<ScalarField>(); opacity->mode=ScalarField::EvaluationMode::AST;
    opacity->astDefinition=Piecewise::continuous(constant(.5));
    check(propertyValueToJson(propertyValueFromJson(propertyValueToJson(opacity))) == propertyValueToJson(opacity),
        "typed scalar field preserves its mathematical payload");
    grant(channel,"output.opacity",opacity);
    auto blend=render(); check(blend.first,"authored opacity manifests");
    check(std::abs(int(blend.second[0])-128)<=1 && std::abs(int(blend.second[2])-128)<=1,
        "GPU blends authored red over existing blue");
    channel.removeDynamicProperty("output.opacity");
    source.setDynamicProperty("color",temporal);
    auto refused=render(); check(!refused.first && refused.second[0]==0 && refused.second[2]==255,
        "unbound temporal expression refuses without stale output");
    grant(channel,"output.time",.4);
    auto timed=render(); check(timed.first && std::abs(int(timed.second[0])-102)<=1,
        "explicit temporal coordinate controls native samples");
    channel.setDynamicProperty("output.colorPath",std::string("@missing.color"));
    auto missing=render(); check(!missing.first && missing.second[2]==255,
        "unresolved source binding refuses without stale pixels");
    check(!channel.findProperty("output.lastRefusal")->isStructurallyWritable(),
        "derived refusal telemetry is readable and read-only");
    channel.removeDynamicProperty("output.colorPath");
    auto absent=render(); check(!absent.first && absent.second[2]==255,
        "removing the binding withdraws the direct Screen act");
    Universe::instance().setProvider({});
    check(Singularity::Screen::ScreenRecorder::writePng("direct-screen-gradient.png", pixels.data(), W, H),
        "native gradient exported without synthesizing image content");
    wgpuBufferRelease(readback); wgpuTextureViewRelease(view); wgpuTextureRelease(target);
    renderer.shutdown(); setCurrentRenderer(nullptr);
    std::printf("DIRECT_SCREEN_FORM_RESULT PASS\n");
}
