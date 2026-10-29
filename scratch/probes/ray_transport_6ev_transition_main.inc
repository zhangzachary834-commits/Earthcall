// Stateful native app-renderer witness. Diagnostic fixture only; never writes a save.
// Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
using ECJson=nlohmann::json;
static void ecNeed(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
static OntoMath::Piecewise ecCopy(const OntoMath::Piecewise& p){auto out=p;for(auto& x:out.pieces)if(x.mathNode)x.mathNode=std::make_shared<OntoMath::MathNode>(*x.mathNode);return out;}
static OntoMath::Piecewise ecConstant(double v){OntoMath::Piecewise p;OntoMath::Piecewise::Piece x;x.mathNode=std::make_shared<OntoMath::MathNode>();x.mathNode->scalarForm=OntoMath::ScalarForm::constant(v);p.pieces.push_back(x);return p;}
static void ecMultiply(OntoMath::Piecewise& p,double v){auto& x=p.pieces.at(0);auto n=std::make_shared<OntoMath::MathNode>();n->op=OntoMath::MathNode::Op::Scale;n->children.push_back(std::make_unique<OntoMath::MathNode>(*ecConstant(v).pieces[0].mathNode));n->children.push_back(std::make_unique<OntoMath::MathNode>(*x.mathNode));x.mathNode=n;}
struct ECTransition {
 std::string name,fixture="base";bool on=true,opaque=false,force=false,outside=false,recreate=false;
 uint32_t w=334,h=190;unsigned members=4,source=0;std::array<double,4> clocks{7.25,7.25,7.25,7.25};
};
struct ECImage {std::vector<uint8_t> rgba;std::vector<float> publication;ECJson stats;};
static void ecSave(const std::string& path,const void* data,size_t bytes){std::ofstream out(path,std::ios::binary);ecNeed(bool(out),"output open "+path);out.write(static_cast<const char*>(data),bytes);ecNeed(bool(out),"output write "+path);}
static std::vector<uint8_t> ecRead(wgpu::Device& gpu,WGPUBuffer source,uint64_t bytes){
 WGPUBufferDescriptor d={};d.size=bytes;d.usage=WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead;auto b=wgpuDeviceCreateBuffer(gpu.device,&d);
 auto e=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(e,source,0,b,0,bytes);auto c=wgpuCommandEncoderFinish(e,nullptr);wgpuQueueSubmit(gpu.queue,1,&c);wgpuCommandBufferRelease(c);wgpuCommandEncoderRelease(e);
 MapResult r;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=mapped;cb.userdata1=&r;wgpuBufferMapAsync(b,WGPUMapMode_Read,0,bytes,cb);while(!r.done)wgpuInstanceProcessEvents(gpu.instance);ecNeed(r.ok,"readback map");
 auto p=static_cast<const uint8_t*>(wgpuBufferGetConstMappedRange(b,0,bytes));std::vector<uint8_t> out(p,p+bytes);wgpuBufferUnmap(b);wgpuBufferRelease(b);return out;
}
static uint64_t ecPoison(WebGpuRenderer& renderer,wgpu::Device& gpu){
 const auto* k=renderer.routedTransportKernelForDiagnostics();if(!k||!k->publication)return 0;uint64_t total=0;
 auto fill=[&](WGPUBuffer b,uint64_t bytes,uint32_t bits){std::vector<uint32_t> data(bytes/4,bits);wgpuQueueWriteBuffer(gpu.queue,b,0,data.data(),bytes);total+=bytes;};
 fill(k->publication,uint64_t(k->width)*k->height*16,0x7fc000a5u);fill(k->rays,uint64_t(k->capacity)*48,0xcdcdcdcdu);
 fill(k->singles,uint64_t(k->capacity)*64*16,0xcdcdcdcdu);fill(k->overlaps,uint64_t(k->capacity)*16*16,0xcdcdcdcdu);
 for(auto b:k->lists)fill(b,uint64_t(k->tiles)*4,0xcdcdcdcdu);
 // Bounded poison keeps a missing-reset control below indirect dispatch limits.
 fill(k->counts,16,1u);fill(k->args,36,1u);
 // Offsets and indices are immutable viewport tables, not per-frame outputs.
 return total;
}
static ECImage ecFrame(WebGpuRenderer& renderer,wgpu::Device& gpu,const ECTransition& state,
 std::vector<geom::FieldNode*>& active,const Rendering::RadianceSourceBinding& sourceZero,
 const OntoMath::Piecewise& sourceOne,const std::string& mutation=""){
 setCurrentRenderer(&renderer);
 if(mutation.empty())unsetenv("EC_6EV_STAGE");else setenv("EC_6EV_STAGE",mutation.c_str(),1);
 renderer.setVolumeRoutedTransportEnabled(state.on);renderer.setRoutedWorkLimitForDiagnostics(state.force?0:1024);
 std::vector<Rendering::VolumeDensityBinding> media;std::string identity;
 for(unsigned i=0;i<state.members;++i){Rendering::VolumeDensityBinding m;ecNeed(Rendering::readVolumeDensity(*active[i],state.clocks[i],1.0/60.0,m),"project actual medium");Rendering::appendVolumeSetIdentity(identity,m.producerId,m);media.push_back(m);}
 renderer.setVolumeDensitySources(std::move(media),std::hash<std::string>{}(identity));
 auto light=sourceZero;light.temporalCoordinate=state.clocks[0]+3.0;
 if(state.source==1){light.radianceExpr=&sourceOne;light.radianceRevision=0xf00d001;}
 if(state.source==2)renderer.setRadianceSources({},2);else renderer.setRadianceSources({light},state.source+1);
 auto eye=state.outside?glm::vec3(0,2,0):glm::vec3(0,50,75);auto look=state.outside?glm::vec3(0,60,90):glm::vec3(40,62,110);
 auto proj=glm::perspectiveZO(glm::radians(60.0f),float(state.w)/state.h,.1f,1000.f);auto view=glm::lookAt(eye,look,glm::vec3(0,1,0));renderer.setCamera(view,proj,eye);renderer.setModel(glm::mat4(1));
 WGPUTextureDescriptor td={};td.usage=WGPUTextureUsage_RenderAttachment|WGPUTextureUsage_CopySrc;td.dimension=WGPUTextureDimension_2D;td.size={state.w,state.h,1};td.format=WGPUTextureFormat_RGBA8Unorm;td.mipLevelCount=1;td.sampleCount=1;
 auto target=wgpuDeviceCreateTexture(gpu.device,&td);auto targetView=wgpuTextureCreateView(target,nullptr);renderer.beginFrameOffscreen(targetView,state.w,state.h,glm::vec4(0,0,0,1));
 if(state.opaque){auto inv=glm::inverse(proj*view);auto point=[&](float x,float y){auto p=inv*glm::vec4(x,y,.97f,1);return glm::vec3(p)/p.w;};auto a=point(-1,-1),b=point(0,-1),c=point(0,1),d=point(-1,1);renderer.drawSolid({a,b,c,a,c,d},glm::vec4(.1f,.2f,.3f,1),Blend::Opaque,true);}
 renderer.composeVolumes();renderer.endFrame();unsetenv("EC_6EV_STAGE");
 ECImage result;const auto s=renderer.frameStats();result.stats={{"routed_sets",s.volumeRoutedSets},{"pixels",s.volumeRoutedPixels},{"program_refusals",s.volumeProgramRefusals},{"refusal",s.volumeLastProgramRefusal},{"compiles",s.volumeProgramCompiles},{"lowerings",s.volumeLowerings},{"parameter_bytes",s.volumeParameterBytesUploaded}};
 const uint32_t stride=(state.w*4+255)&~255u;WGPUBufferDescriptor bd={};bd.size=uint64_t(stride)*state.h;bd.usage=WGPUBufferUsage_CopyDst|WGPUBufferUsage_CopySrc;auto b=wgpuDeviceCreateBuffer(gpu.device,&bd);
 auto e=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);WGPUTexelCopyTextureInfo src={};src.texture=target;src.aspect=WGPUTextureAspect_All;WGPUTexelCopyBufferInfo dst={};dst.buffer=b;dst.layout.bytesPerRow=stride;dst.layout.rowsPerImage=state.h;WGPUExtent3D extent={state.w,state.h,1};wgpuCommandEncoderCopyTextureToBuffer(e,&src,&dst,&extent);auto command=wgpuCommandEncoderFinish(e,nullptr);wgpuQueueSubmit(gpu.queue,1,&command);wgpuCommandBufferRelease(command);wgpuCommandEncoderRelease(e);
 auto padded=ecRead(gpu,b,bd.size);result.rgba.reserve(state.w*state.h*4);for(unsigned y=0;y<state.h;++y)result.rgba.insert(result.rgba.end(),padded.begin()+y*stride,padded.begin()+y*stride+state.w*4);
 wgpuBufferRelease(b);wgpuTextureViewRelease(targetView);wgpuTextureRelease(target);
 if(s.volumeRoutedSets){const auto* k=renderer.routedTransportKernelForDiagnostics();ecNeed(k&&k->width==state.w&&k->height==state.h,"viewport resource reconstruction");auto bytes=ecRead(gpu,k->publication,uint64_t(state.w)*state.h*16);result.publication.resize(bytes.size()/4);std::memcpy(result.publication.data(),bytes.data(),bytes.size());}
 return result;
}
static ECJson ecCensus(const ECImage& image){unsigned accepted=0,refused=0,nonfinite=0,nonempty=0;for(size_t i=0;i<image.publication.size();i+=4){for(unsigned j=0;j<4;++j)nonfinite+=!std::isfinite(image.publication[i+j]);if(image.publication[i+3]<0)++refused;else{++accepted;nonempty+=image.publication[i+3]>1e-5f||std::max({std::abs(image.publication[i]),std::abs(image.publication[i+1]),std::abs(image.publication[i+2])})>1e-6f;}}return {{"accepted",accepted},{"refused",refused},{"nonfinite",nonfinite},{"nonempty",nonempty}};}
