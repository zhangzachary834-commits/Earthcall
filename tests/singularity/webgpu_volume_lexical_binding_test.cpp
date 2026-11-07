// Codex / GPT-6 / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-11-02.
// Retained volume lowering and actual WGSL versus independently substituted
// CPU expressions. No Engine/world/save mutation.
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
using M=OntoMath::MathNode;using O=M::Op;using N=std::unique_ptr<M>;
static unsigned checks=0,failed=0;
#define CHECK(c) do{++checks;if(!(c)){++failed;std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);}}while(0)
static N value(const char* s){auto n=std::make_unique<M>();n->op=O::ValueLeaf;n->variableName=s;return n;}
static N num(double v){auto n=std::make_unique<M>();n->op=O::ScalarLeaf;n->scalarForm=OntoMath::ScalarForm::constant(v);return n;}
template<class...Kids>static N op(O o,Kids...kids){auto n=std::make_unique<M>();n->op=o;(n->children.push_back(std::move(kids)),...);return n;}
static N let(const char* name,N v,N b){auto n=op(O::Let,std::move(v),std::move(b));n->variableName=name;return n;}
static OntoMath::Piecewise pw(N n){OntoMath::Piecewise p;p.inputVariable="x";OntoMath::Piecewise::Piece piece;piece.mathNode=std::move(n);p.pieces.push_back(std::move(piece));return p;}
static bool same(const sdfwgsl::KernelVolumeEvaluator&a,const sdfwgsl::KernelVolumeEvaluator&b){return a.program.ok&&b.program.ok&&a.program.wgsl==b.program.wgsl&&a.program.params==b.program.params;}
static double cpu(const M& n,float x,float y,float z){auto v=n.evaluate({{"x",PropertyValue(double(x))},{"y",PropertyValue(double(y))},{"z",PropertyValue(double(z))},{"t",PropertyValue(7.0)},{"p",PropertyValue(glm::vec3(x,y,z))}});CHECK(v&&std::holds_alternative<double>(*v));return v&&std::holds_alternative<double>(*v)?std::get<double>(*v):NAN;}
static std::vector<float> device(wgpu::Device& g,const sdfwgsl::KernelVolumeEvaluator&e){
 auto p=sdfwgsl::composeVolumeEvaluator(e,false);CHECK(p.ok);if(!p.ok)return {};
 std::vector<float> params=p.params;params.push_back(0);float uniform[64]={},instance[20]={};instance[8]=7;
 const float points[16]={.25f,.125f,.375f,0,.5f,.375f,.125f,0,.75f,.25f,.5f,0,-.25f,.5f,.125f,0};
 auto buffer=[&](size_t bytes,WGPUBufferUsage usage,const void*d){WGPUBufferDescriptor bd={};bd.size=bytes;bd.usage=usage;auto b=wgpuDeviceCreateBuffer(g.device,&bd);if(d)wgpuQueueWriteBuffer(g.queue,b,0,d,bytes);return b;};
 auto u=buffer(sizeof(uniform),WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst,uniform),ib=buffer(sizeof(instance),WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst,instance),pb=buffer(params.size()*4,WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst,params.data()),pts=buffer(sizeof(points),WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst,points),out=buffer(16,WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc,nullptr),rb=buffer(16,WGPUBufferUsage_MapRead|WGPUBufferUsage_CopyDst,nullptr);
 std::string code=p.wgsl+"\n@group(2) @binding(0) var<storage,read> pts:array<vec4f>;\n@group(2) @binding(1) var<storage,read_write> answer:array<f32>;\n@compute @workgroup_size(1) fn evaluate(@builtin(global_invocation_id) id:vec3u){g_instIdx=0u;let q=pts[id.x].xyz;\n";
 if(e.hasSharing)code+="volumeSharedEval(q);\n";
 code+="answer[id.x]=volumeDensityEval(q)+0.0*(u.eyePos.x+P.v[0]+instances[0].time.x);}\n";
 wgpuDevicePushErrorScope(g.device,WGPUErrorFilter_Validation);
 WGPUShaderSourceWGSL wg={};wg.chain.sType=WGPUSType_ShaderSourceWGSL;wg.code=wgpu::Device::str(code.c_str());WGPUShaderModuleDescriptor md={};md.nextInChain=&wg.chain;auto shader=wgpuDeviceCreateShaderModule(g.device,&md);WGPUComputePipelineDescriptor cd={};cd.compute.module=shader;cd.compute.entryPoint=wgpu::Device::str("evaluate");auto pipe=wgpuDeviceCreateComputePipeline(g.device,&cd);
 WGPUBindGroup groups[3]={};WGPUBindGroupLayout layouts[3]={};
 for(unsigned i=0;i<3;++i){layouts[i]=wgpuComputePipelineGetBindGroupLayout(pipe,i);std::vector<WGPUBindGroupEntry> entries;auto add=[&](unsigned b,WGPUBuffer buf){WGPUBindGroupEntry e={};e.binding=b;e.buffer=buf;e.size=wgpuBufferGetSize(buf);entries.push_back(e);};if(i==0){add(0,u);add(1,pb);}if(i==1)add(0,ib);if(i==2){add(0,pts);add(1,out);}WGPUBindGroupDescriptor gd={};gd.layout=layouts[i];gd.entryCount=entries.size();gd.entries=entries.data();groups[i]=wgpuDeviceCreateBindGroup(g.device,&gd);}
 auto enc=wgpuDeviceCreateCommandEncoder(g.device,nullptr);auto pass=wgpuCommandEncoderBeginComputePass(enc,nullptr);wgpuComputePassEncoderSetPipeline(pass,pipe);for(unsigned i=0;i<3;++i)wgpuComputePassEncoderSetBindGroup(pass,i,groups[i],0,nullptr);wgpuComputePassEncoderDispatchWorkgroups(pass,4,1,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);wgpuCommandEncoderCopyBufferToBuffer(enc,out,0,rb,0,16);auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuQueueSubmit(g.queue,1,&cmd);
 struct Error{bool done=false,ok=false;}err;WGPUPopErrorScopeCallbackInfo ec={};ec.mode=WGPUCallbackMode_AllowProcessEvents;ec.userdata1=&err;ec.callback=[](WGPUPopErrorScopeStatus,WGPUErrorType t,WGPUStringView m,void*u,void*){auto&e=*static_cast<Error*>(u);e.ok=t==WGPUErrorType_NoError;e.done=true;if(!e.ok)std::fprintf(stderr,"WGSL: %.*s\n",int(m.length),m.data);};wgpuDevicePopErrorScope(g.device,ec);while(!err.done)wgpuDevicePoll(g.device,false,nullptr);CHECK(err.ok);
 std::vector<float> result;
 if(err.ok){Error map;WGPUBufferMapCallbackInfo mc={};mc.mode=WGPUCallbackMode_AllowProcessEvents;mc.userdata1=&map;mc.callback=[](WGPUMapAsyncStatus s,WGPUStringView,void*u,void*){auto&e=*static_cast<Error*>(u);e.ok=s==WGPUMapAsyncStatus_Success;e.done=true;};wgpuBufferMapAsync(rb,WGPUMapMode_Read,0,16,mc);while(!map.done)wgpuInstanceProcessEvents(g.instance);CHECK(map.ok);if(map.ok){auto*f=static_cast<const float*>(wgpuBufferGetConstMappedRange(rb,0,16));result.assign(f,f+4);wgpuBufferUnmap(rb);}}
 for(auto b:{u,ib,pb,pts,out,rb})wgpuBufferRelease(b);for(unsigned i=0;i<3;++i){wgpuBindGroupRelease(groups[i]);wgpuBindGroupLayoutRelease(layouts[i]);}wgpuComputePipelineRelease(pipe);wgpuShaderModuleRelease(shader);wgpuCommandBufferRelease(cmd);wgpuCommandEncoderRelease(enc);return result;
}
int main(int argc,char**argv){
 auto density=pw(let("a",op(O::Add,value("x"),num(1)),op(O::Scale,value("a"),value("a"))));sdfwgsl::VolumeProgramInput input{&density};auto first=sdfwgsl::lowerVolumeEvaluator(input);CHECK(first.program.ok);
 density.pieces[0].mathNode->children[0]->op=O::Sub;auto structural=sdfwgsl::repairVolumeEvaluator(input,first,false);CHECK(same(structural,sdfwgsl::lowerVolumeEvaluator(input)));
 density.pieces[0].mathNode->children[0]->children[1]->scalarForm=OntoMath::ScalarForm::constant(3);auto numeric=sdfwgsl::repairVolumeEvaluator(input,structural,true);CHECK(same(numeric,sdfwgsl::lowerVolumeEvaluator(input)));
 auto shadow=pw(let("a",num(2),let("b",value("x"),value("a"))));sdfwgsl::VolumeProgramInput si{&shadow};auto before=sdfwgsl::lowerVolumeEvaluator(si);shadow.pieces[0].mathNode->children[1]->variableName="a";auto after=sdfwgsl::repairVolumeEvaluator(si,before,false);CHECK(same(after,sdfwgsl::lowerVolumeEvaluator(si)));
 auto integral=op(O::Integral,op(O::Add,value("x"),value("t")),num(0),num(1));integral->variableName="x";auto ip=pw(std::move(integral));sdfwgsl::VolumeProgramInput ii{&ip};auto oldIntegral=sdfwgsl::lowerVolumeEvaluator(ii);ip.pieces[0].mathNode->variableName="t";CHECK(same(sdfwgsl::repairVolumeEvaluator(ii,oldIntegral,false),sdfwgsl::lowerVolumeEvaluator(ii)));
 auto sine=std::make_unique<M>();sine->op=O::ScalarLeaf;sine->scalarForm=OntoMath::ScalarForm::transcendental(OntoMath::TransFactor::Kind::Sin,"t",1,0);auto time=pw(let("t",op(O::Add,value("x"),num(1)),std::move(sine)));sdfwgsl::VolumeProgramInput ti{&time};auto te=sdfwgsl::lowerVolumeEvaluator(ti);CHECK(te.program.ok&&te.program.kernelVolumeTimeSlots.empty());
 auto noise=pw(let("x",value("y"),op(O::Noise,op(O::VectorConstruct,value("x"),value("y"),value("z")))));auto extinction=pw(std::make_unique<M>(*noise.pieces[0].mathNode));sdfwgsl::VolumeProgramInput ni{&noise,&extinction};auto ne=sdfwgsl::lowerVolumeEvaluator(ni);CHECK(ne.program.ok);
 if(argc>1&&std::string(argv[1])=="--cpu-only"){std::printf("lexical CPU: %u checks, %u failures\n",checks,failed);return failed?1:0;}
 wgpu::Device gpu;CHECK(gpu.init());if(!gpu.device)return 1;
 const float points[4][3]={{.25f,.125f,.375f},{.5f,.375f,.125f},{.75f,.25f,.5f},{-.25f,.5f,.125f}};
 auto reference=op(O::Noise,op(O::VectorConstruct,value("y"),value("y"),value("z")));
 for(auto* e:{&structural,&numeric,&after,&te,&ne}){auto got=device(gpu,*e);CHECK(got.size()==4);for(size_t i=0;i<got.size();++i){auto q=points[i];double expected=e==&structural?(q[0]-1)*(q[0]-1):e==&numeric?(q[0]-3)*(q[0]-3):e==&after?q[0]:e==&te?std::sin(q[0]+1):cpu(*reference,q[0],q[1],q[2]);CHECK(std::isfinite(got[i])&&std::abs(got[i]-expected)<1e-5);}}
 std::printf("lexical CPU/native: %u checks, %u failures\n",checks,failed);return failed?1:0;
}
