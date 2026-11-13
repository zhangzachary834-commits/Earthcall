// Scratch read-only scalar fidelity witness, following Zach's exact-equation criterion.
// Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-17.
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"
#include <webgpu/wgpu.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <array>
#include <cmath>
struct Mapping {bool done=false,ok=false;};
void mapped(WGPUMapAsyncStatus s,WGPUStringView,void* u,void*){auto& m=*static_cast<Mapping*>(u);m.done=true;m.ok=s==WGPUMapAsyncStatus_Success;}
struct Error {bool done=false,ok=true;};
void popped(WGPUPopErrorScopeStatus,WGPUErrorType type,WGPUStringView msg,void* u,void*){auto& e=*static_cast<Error*>(u);e.done=true;e.ok=type==WGPUErrorType_NoError;if(!e.ok)std::cerr<<std::string(msg.data,msg.length)<<'\n';}
int main(int argc,char** argv){
    if(argc!=3)return 2;
    std::ifstream in(argv[1]);nlohmann::json zone;in>>zone;
    std::vector<nlohmann::json> fields={zone["spatialRoot"]};for(auto& f:zone["spatialFields"])fields.push_back(f);
    wgpu::Device gpu;if(!gpu.init()){std::cerr<<"No device\n";return 3;}
    WGPUAdapterInfo ai={};wgpuAdapterGetInfo(gpu.adapter,&ai);std::cout<<"backend="<<int(ai.backendType)<<" adapter="<<std::string(ai.device.data,ai.device.length)<<'\n';
    nlohmann::json report=nlohmann::json::array();
    for(const auto& saved:fields){
        auto f=geom::FieldNode::fromJson(saved);Rendering::AuthorableLightState light;
        if(!Rendering::readAuthorableLight(*f,light)||f->field->astDefinition.pieces.empty())continue;
        geom::SdfNode root;
        auto program=sdfwgsl::compile(root,nullptr,nullptr,&f->field->astDefinition);
        if(!program.ok){std::cerr<<program.error<<'\n';return 4;}
        std::vector<glm::vec4> points;
        for(float y:{0.f,2.f,5.f,12.f})for(int x=-32;x<=32;x+=4)for(int z=-32;z<=32;z+=4)points.emplace_back(glm::vec3(float(x),y,float(z))-f->origin,0.f);
        const size_t n=points.size();
        auto buffer=[&](uint64_t size,WGPUBufferUsage usage,const void* data){WGPUBufferDescriptor d={};d.size=size;d.usage=usage|(data?WGPUBufferUsage_CopyDst:0);auto b=wgpuDeviceCreateBuffer(gpu.device,&d);if(data)wgpuQueueWriteBuffer(gpu.queue,b,0,data,size);return b;};
        auto input=buffer(n*sizeof(glm::vec4),WGPUBufferUsage_Storage,points.data());
        auto output=buffer(n*sizeof(float),WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc,nullptr);
        auto readback=buffer(n*sizeof(float),WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead,nullptr);
        auto params=buffer(program.params.size()*sizeof(float),WGPUBufferUsage_Storage,program.params.data());
        std::array<float,72> uniformValues{};auto uniform=buffer(sizeof(uniformValues),WGPUBufferUsage_Uniform,uniformValues.data());
        std::array<uint32_t,72> instance{};auto instances=buffer(sizeof(instance),WGPUBufferUsage_Storage,instance.data());
        const std::string code=program.wgsl+R"WGSL(
@group(2) @binding(0) var<storage,read> fidelityPoints: array<vec4<f32>>;
@group(2) @binding(1) var<storage,read_write> fidelityValues: array<f32>;
@compute @workgroup_size(64)
fn fidelityMain(@builtin(global_invocation_id) id:vec3<u32>){
 if(id.x>=arrayLength(&fidelityPoints)){return;}
 g_instIdx=0u;
 fidelityValues[id.x]=lightRadiance(fidelityPoints[id.x].xyz);
}
)WGSL";
        std::ofstream(std::string(argv[2])+"-"+f->getIdentifier()+".wgsl")<<code;
        wgpuDevicePushErrorScope(gpu.device,WGPUErrorFilter_Validation);
        WGPUShaderSourceWGSL ws={};ws.chain.sType=WGPUSType_ShaderSourceWGSL;ws.code=wgpu::Device::str(code.c_str());WGPUShaderModuleDescriptor md={};md.nextInChain=&ws.chain;
        auto module=wgpuDeviceCreateShaderModule(gpu.device,&md);WGPUComputePipelineDescriptor pd={};pd.compute.module=module;pd.compute.entryPoint=wgpu::Device::str("fidelityMain");auto pipeline=wgpuDeviceCreateComputePipeline(gpu.device,&pd);
        auto group=[&](uint32_t i,const std::vector<std::pair<uint32_t,std::pair<WGPUBuffer,uint64_t>>>& values){std::vector<WGPUBindGroupEntry> entries;for(auto& v:values){WGPUBindGroupEntry e={};e.binding=v.first;e.buffer=v.second.first;e.size=v.second.second;entries.push_back(e);}WGPUBindGroupDescriptor gd={};gd.layout=wgpuComputePipelineGetBindGroupLayout(pipeline,i);gd.entryCount=entries.size();gd.entries=entries.data();return wgpuDeviceCreateBindGroup(gpu.device,&gd);};
        const auto functionStart=program.wgsl.find("fn lightRadiance(");const auto functionEnd=program.wgsl.find("fn lightChroma(",functionStart);const bool usesTime=program.wgsl.substr(functionStart,functionEnd-functionStart).find("u.radianceTime")!=std::string::npos;
        std::vector<std::pair<uint32_t,std::pair<WGPUBuffer,uint64_t>>> bindings0={{1,{params,program.params.size()*sizeof(float)}}};if(usesTime)bindings0.push_back({0,{uniform,sizeof(uniformValues)}});
        auto g0=group(0,bindings0);
        auto g1=group(1,{{0,{instances,sizeof(instance)}}});
        auto g2=group(2,{{0,{input,n*sizeof(glm::vec4)}},{1,{output,n*sizeof(float)}}});
        Error bindingError;WGPUPopErrorScopeCallbackInfo bindingCallback={};bindingCallback.mode=WGPUCallbackMode_AllowProcessEvents;bindingCallback.callback=popped;bindingCallback.userdata1=&bindingError;wgpuDevicePopErrorScope(gpu.device,bindingCallback);while(!bindingError.done)wgpuDevicePoll(gpu.device,true,nullptr);if(!bindingError.ok){std::cerr<<"Source refused: "<<f->getIdentifier()<<'\n';return 5;}
        wgpuDevicePushErrorScope(gpu.device,WGPUErrorFilter_Validation);
        auto encoder=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);auto pass=wgpuCommandEncoderBeginComputePass(encoder,nullptr);wgpuComputePassEncoderSetPipeline(pass,pipeline);
        wgpuComputePassEncoderSetBindGroup(pass,0,g0,0,nullptr);wgpuComputePassEncoderSetBindGroup(pass,1,g1,0,nullptr);wgpuComputePassEncoderSetBindGroup(pass,2,g2,0,nullptr);wgpuComputePassEncoderDispatchWorkgroups(pass,(n+63)/64,1,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);
        wgpuCommandEncoderCopyBufferToBuffer(encoder,output,0,readback,0,n*sizeof(float));auto command=wgpuCommandEncoderFinish(encoder,nullptr);wgpuQueueSubmit(gpu.queue,1,&command);
        Error error;WGPUPopErrorScopeCallbackInfo ec={};ec.mode=WGPUCallbackMode_AllowProcessEvents;ec.callback=popped;ec.userdata1=&error;wgpuDevicePopErrorScope(gpu.device,ec);while(!error.done)wgpuDevicePoll(gpu.device,true,nullptr);if(!error.ok)return 5;
        Mapping map;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=mapped;cb.userdata1=&map;wgpuBufferMapAsync(readback,WGPUMapMode_Read,0,n*sizeof(float),cb);while(!map.done)wgpuDevicePoll(gpu.device,true,nullptr);if(!map.ok)return 6;
        const auto* values=static_cast<const float*>(wgpuBufferGetConstMappedRange(readback,0,n*sizeof(float)));
        nlohmann::json record={{"id",f->getIdentifier()},{"origin",{f->origin.x,f->origin.y,f->origin.z}},{"samples",nlohmann::json::array()}};
        double worst=0;
        for(size_t i=0;i<n;++i){auto p=glm::vec3(points[i]);auto cpu=f->field->astDefinition.evaluate({{"p",p},{"x",double(p.x)},{"y",double(p.y)},{"z",double(p.z)},{"t",0.}});double value=0;if(!cpu||!propertyValueToNumber(*cpu,value)||!std::isfinite(values[i]))return 7;worst=std::max(worst,std::abs(value-values[i]));record["samples"].push_back({{"p",{p.x,p.y,p.z}},{"cpu",value},{"gpu",values[i]}});}
        wgpuBufferUnmap(readback);std::cout<<f->getIdentifier()<<" samples="<<n<<" max_gpu_cpu_error="<<worst<<'\n';report.push_back(record);std::ofstream(argv[2])<<report.dump(2);
        for(auto b:{input,output,readback,params,instances,uniform})wgpuBufferRelease(b);for(auto g:{g0,g1,g2})wgpuBindGroupRelease(g);wgpuComputePipelineRelease(pipeline);wgpuShaderModuleRelease(module);wgpuCommandBufferRelease(command);wgpuCommandEncoderRelease(encoder);
    }
    std::ofstream(argv[2])<<report.dump(2);return 0;
}
