"""Scratch GPU switch-root proposal and CPU coverage/cost witness. No production admission.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
"""
import argparse,ast,hashlib,json,os,shlex,subprocess
from pathlib import Path
CPP=r'''
#include <stdexcept>
#include <functional>
#include <cstring>
struct RootRay {glm::vec4 origin_a,direction_b;};
struct RootSpec {glm::vec4 origin,half;};
struct RootRecord {uint32_t count=0,flags=0,evaluations=0,id=0;float windows[8][2]{};};
static_assert(sizeof(RootRecord)==80);
int main(int argc,char**argv) {
 const std::string path=argc>1?argv[1]:"";
 const uint32_t repeat=std::getenv("EC_ROOT_REPEAT")?std::atoi(std::getenv("EC_ROOT_REPEAT")):1;
 const uint32_t bins=std::getenv("EC_ROOT_BINS")?std::atoi(std::getenv("EC_ROOT_BINS")):8;
 if(repeat<1||repeat>16||bins<1||bins>32)throw std::runtime_error("invalid repeat/bins");
 const auto coldStart=std::chrono::steady_clock::now();
 std::ifstream in("saves/zones/Northern Veil/zone.ecform",std::ios::binary);
 std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),{});
 const J zone=J::parse(J::from_msgpack(bytes).at("MigrationRoot").get<std::string>());
 std::vector<Medium> media;
 for(const auto& authored:zone.at("spatialFields")) {
  auto field=geom::FieldNode::fromJson(authored);Rendering::VolumeDensityBinding b;
  if(!field||!Rendering::readVolumeDensity(*field,0,0,b))continue;
  Medium m;m.owner=field;m.id=b.producerId;m.origin=glm::dvec3(b.origin);m.half=glm::abs(glm::dvec3(b.scale));
  m.density=single(b.densityExpr);if(!m.density)throw std::runtime_error("density is not a single AST");
  m.envAST=cloneEnvelope(*m.density);auto f=OntoMath::restrictAlgebraicToLine(*m.envAST,false,"p");
  if(!f)throw std::runtime_error(f.refusal);m.envForm=std::move(*f.form);media.push_back(std::move(m));
 }
 std::vector<float> params;std::vector<RootSpec> specs;std::vector<std::pair<size_t,uint32_t>> identities;
 std::string functions,dispatch="fn switchSign(k:u32,p:vec3<f32>)->f32 {\n";
 for(size_t m=0;m<media.size();++m) {
  OntoMath::Piecewise pw;pw.pieces.emplace_back();pw.pieces[0].mathNode=std::make_unique<M>(*media[m].envAST);
  for(uint32_t k=0;k<media[m].envForm.switches.size();++k) {
   const std::vector<uint32_t> only{k};auto p=sdfwgsl::lowerVolumeSupportKillers(&pw,&only);
   if(!p.ok||p.switches.size()!=1||p.switches[0]!=k||p.formSwitches!=media[m].envForm.switches.size())throw std::runtime_error("switch emitter refused: "+p.error);
   if(p.wgsl.find("instances[")!=std::string::npos)throw std::runtime_error("unbound time context");
   const auto& sw=media[m].envForm.switches[k];const auto& node=media[m].envForm.nodes[sw.node];
   const size_t offset=params.size();params.insert(params.end(),{1,0,1,float(sw.role==2?node.lo:sw.role==3?node.hi:0)});
   params.insert(params.end(),p.params.begin(),p.params.end());
   const size_t id=specs.size();const std::string name="switchSign_"+std::to_string(id);
   const auto at=p.wgsl.find("volumeSupportReach");if(at==std::string::npos)throw std::runtime_error("emitter function changed");
   p.wgsl.replace(at,std::strlen("volumeSupportReach"),name);functions+=p.wgsl;
   dispatch+=" if(k=="+std::to_string(id)+"u){return "+name+"(p,"+std::to_string(offset)+"u);}\n";
   specs.push_back({glm::vec4(media[m].origin,0),glm::vec4(media[m].half,0)});identities.push_back({m,k});
  }
 }
 dispatch+=" return bitcast<f32>(0x7fc00000u);\n}\n";
 const double coldMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-coldStart).count();
 J fixture;{std::ifstream f("docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json");f>>fixture;}
 std::vector<RootRay> rays;for(const auto& r:fixture["rays"])rays.push_back({{fixture["origin"][0],fixture["origin"][1],fixture["origin"][2],r[5]}, {r[2],r[3],r[4],r[6]}});
 const uint32_t n=rays.size(),execution=n*repeat,ns=specs.size();const size_t recordCount=size_t(execution)*ns,outputBytes=recordCount*sizeof(RootRecord);
 if(outputBytes>128u*1024u*1024u)throw std::runtime_error("output exceeds 128MiB diagnostic cap");
 const std::string header=R"WGSL(
struct Config { base:u32,execution:u32,switches:u32,bins:u32 };
struct Params {v:array<f32>};
struct Ray {o:vec4<f32>,d:vec4<f32>};struct Spec {o:vec4<f32>,h:vec4<f32>};
struct Record {data:vec4<u32>,windows:array<vec2<f32>,8>};
@group(0) @binding(0) var<uniform> cfg:Config;
@group(0) @binding(1) var<storage,read> P:Params;
@group(0) @binding(2) var<storage,read> rays:array<Ray>;
@group(0) @binding(3) var<storage,read> specs:array<Spec>;
@group(0) @binding(4) var<storage,read_write> rootOutput:array<Record>;
)WGSL";
 const std::string kernel=R"WGSL(
fn addRoot(v:ptr<function,Record>,a:f32,b:f32) {
 if((*v).data.x>=8u){(*v).data.y|=1u;return;}
 (*v).windows[(*v).data.x]=vec2<f32>(a,b);(*v).data.x+=1u;
}
@compute @workgroup_size(64)
fn rootMain(@builtin(global_invocation_id) gid:vec3<u32>) {
 if(gid.x>=cfg.execution||gid.y>=cfg.switches){return;}
 let index=gid.y*cfg.execution+gid.x;let ray=rays[gid.x%cfg.base];let spec=specs[gid.y];
 let o=ray.o.xyz-spec.o.xyz;let d=ray.d.xyz;var lo=ray.o.w;var hi=ray.d.w;
 var result:Record;result.data.w=index;
 for(var k=0u;k<3u;k+=1u) {
  if(abs(d[k])<1e-15) {if(abs(o[k])>spec.h[k]){result.data.y=4u;rootOutput[index]=result;return;}}
  else {let a=(-spec.h[k]-o[k])/d[k];let b=(spec.h[k]-o[k])/d[k];lo=max(lo,min(a,b));hi=min(hi,max(a,b));}
 }
 if(!(hi>lo)){result.data.y=4u;rootOutput[index]=result;return;}
 let pad=1e-4*(hi-lo);var previous=lo;var pv=switchSign(gid.y,o+lo*d);result.data.z=1u;
 for(var bin=0u;bin<cfg.bins;bin+=1u) {
  let end=lo+(hi-lo)*f32(bin+1u)/f32(cfg.bins);let ev=switchSign(gid.y,o+end*d);result.data.z+=1u;
  if(!(abs(ev)<=3.4e38 && abs(pv)<=3.4e38)){result.data.y|=2u;}
  else if(pv==0.0 && ev!=0.0) {addRoot(&result,max(lo,previous-pad),min(hi,previous+pad));}
  else if(ev==0.0 && pv!=0.0) {addRoot(&result,max(lo,end-pad),min(hi,end+pad));}
  else if((pv<0.0)!=(ev<0.0)) {
   var a=previous;var b=end;var av=pv;
   for(var it=0u;it<16u;it+=1u) {
    if(b-a<=pad*.125){break;}let mid=(a+b)*.5;let mv=switchSign(gid.y,o+mid*d);result.data.z+=1u;
    if(!(abs(mv)<=3.4e38)){result.data.y|=2u;break;}
    if((mv<0.0)==(av<0.0)){a=mid;av=mv;}else{b=mid;}
   }
   addRoot(&result,max(lo,a-pad),min(hi,b+pad));
  }
  previous=end;pv=ev;
 }
 rootOutput[index]=result;
}
@compute @workgroup_size(64)
fn emptyMain(@builtin(global_invocation_id) gid:vec3<u32>){
 if(gid.x==0u&&gid.y==0u){var v:Record;v.data.w=cfg.base+u32(P.v[0])+u32(rays[0].o.x)+u32(specs[0].h.x);rootOutput[0]=v;}
}
)WGSL";
 const std::string wgsl=header+functions+dispatch+kernel;
 if(!path.empty()){std::ofstream f(path+"-kernel.wgsl");f<<wgsl;}
 wgpu::Device gpu;if(!gpu.init(false))throw std::runtime_error("native device unavailable");
 WGPUAdapterInfo info={};if(wgpuAdapterGetInfo(gpu.adapter,&info)!=WGPUStatus_Success)throw std::runtime_error("adapter query");
 J adapter={{"device",std::string(info.device.data,info.device.length)},{"backend",int(info.backendType)},{"metal",info.backendType==WGPUBackendType_Metal},{"timestamps",gpu.timestampQueries}};wgpuAdapterInfoFreeMembers(info);
 auto buffer=[&](uint64_t size,WGPUBufferUsage usage,const void* data=nullptr){WGPUBufferDescriptor d={};d.size=size;d.usage=usage;auto b=wgpuDeviceCreateBuffer(gpu.device,&d);if(data)wgpuQueueWriteBuffer(gpu.queue,b,0,data,size);return b;};
 struct Config {uint32_t base,execution,switches,bins;}config{n,execution,ns,bins};
 auto c=buffer(sizeof(config),WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst,&config);
 auto p=buffer(params.size()*4,WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst,params.data());
 auto r=buffer(rays.size()*sizeof(RootRay),WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst,rays.data());
 auto s=buffer(specs.size()*sizeof(RootSpec),WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst,specs.data());
 auto out=buffer(outputBytes,WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc);
 auto staging=buffer(outputBytes,WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead);
 WGPUShaderSourceWGSL code={};code.chain.sType=WGPUSType_ShaderSourceWGSL;code.code={wgsl.data(),wgsl.size()};WGPUShaderModuleDescriptor md={};md.nextInChain=&code.chain;
 wgpuDevicePushErrorScope(gpu.device,WGPUErrorFilter_Validation);auto module=wgpuDeviceCreateShaderModule(gpu.device,&md);
 auto build=[&](const char* name){WGPUComputePipelineDescriptor pd={};pd.compute.module=module;pd.compute.entryPoint={name,WGPU_STRLEN};return wgpuDeviceCreateComputePipeline(gpu.device,&pd);};
 auto real=build("rootMain"),empty=build("emptyMain");
 PopContext pop;WGPUPopErrorScopeCallbackInfo pc={};pc.mode=WGPUCallbackMode_AllowSpontaneous;pc.callback=onPopError;pc.userdata1=&pop;wgpuDevicePopErrorScope(gpu.device,pc);while(!pop.done)wgpuInstanceProcessEvents(gpu.instance);if(pop.type!=WGPUErrorType_NoError)throw std::runtime_error("shader refused");
 auto group=[&](WGPUComputePipeline cp){auto layout=wgpuComputePipelineGetBindGroupLayout(cp,0);WGPUBindGroupEntry es[5]={};const WGPUBuffer bs[]={c,p,r,s,out};const uint64_t sizes[]={sizeof(config),params.size()*4,rays.size()*sizeof(RootRay),specs.size()*sizeof(RootSpec),outputBytes};for(int k=0;k<5;++k){es[k].binding=k;es[k].buffer=bs[k];es[k].size=sizes[k];}WGPUBindGroupDescriptor bd={};bd.layout=layout;bd.entryCount=5;bd.entries=es;return wgpuDeviceCreateBindGroup(gpu.device,&bd);};
 auto rg=group(real),eg=group(empty);
 auto batch=[&](WGPUComputePipeline cp,WGPUBindGroup bg,int count){auto start=std::chrono::steady_clock::now();WGPUCommandEncoderDescriptor ed={};auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,&ed);for(int k=0;k<count;++k){WGPUComputePassDescriptor desc={};auto pass=wgpuCommandEncoderBeginComputePass(enc,&desc);wgpuComputePassEncoderSetPipeline(pass,cp);wgpuComputePassEncoderSetBindGroup(pass,0,bg,0,nullptr);wgpuComputePassEncoderDispatchWorkgroups(pass,(execution+63)/64,ns,1);wgpuComputePassEncoderEnd(pass);wgpuComputePassEncoderRelease(pass);}auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuDevicePoll(gpu.device,true,nullptr);wgpuCommandBufferRelease(cmd);wgpuCommandEncoderRelease(enc);return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();};
 for(int k=0;k<4;++k)batch(real,rg,1);
 auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(enc,out,0,staging,0,outputBytes);auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuDevicePoll(gpu.device,true,nullptr);
 MapContext map;WGPUBufferMapCallbackInfo mc={};mc.mode=WGPUCallbackMode_AllowSpontaneous;mc.callback=onMapped;mc.userdata1=&map;wgpuBufferMapAsync(staging,WGPUMapMode_Read,0,outputBytes,mc);while(!map.done){wgpuDevicePoll(gpu.device,true,nullptr);wgpuInstanceProcessEvents(gpu.instance);}if(!map.ok)throw std::runtime_error("map refused");
 std::vector<RootRecord> records(recordCount);std::memcpy(records.data(),wgpuBufferGetConstMappedRange(staging,0,outputBytes),outputBytes);wgpuBufferUnmap(staging);
 uint64_t roots=0,flags=0,evals=0;J raw=J::array();
 for(uint32_t sw=0;sw<ns;++sw)for(uint32_t ray=0;ray<n;++ray){const auto& a=records[size_t(sw)*execution+ray];roots+=a.count;flags+=bool(a.flags&3);evals+=a.evaluations;J spans=J::array();for(uint32_t k=0;k<a.count;++k){if(!(std::isfinite(a.windows[k][0])&&std::isfinite(a.windows[k][1])&&a.windows[k][1]>=a.windows[k][0]))throw std::runtime_error("invalid bracket");spans.push_back({a.windows[k][0],a.windows[k][1]});}raw.push_back({ray,sw,a.flags,spans});for(uint32_t rep=1;rep<repeat;++rep){const auto& b=records[size_t(sw)*execution+ray+n*rep];if(a.count!=b.count||a.flags!=b.flags||a.evaluations!=b.evaluations||std::memcmp(a.windows,b.windows,sizeof(a.windows)))throw std::runtime_error("replicated endpoints differ");}}
 if(!path.empty()){std::ofstream f(path+"-roots.json");f<<raw.dump()<<"\n";}
 // Independent CPU interval coverage: gaps outside proposals must be classified.
 // It is an exact-mathematics check, not a native execution certificate.
 J perSwitch=J::array(),missedExamples=J::array();uint64_t missedCrossings=0,gapQueries=0,unresolved=0,undefined=0,refusedJobs=0,checkedJobs=0;double unresolvedWidth=0;
 std::vector<std::vector<std::pair<double,double>>> memberWindows(media.size()*n);
 for(uint32_t sw=0;sw<ns;++sw)for(uint32_t ray=0;ray<n;++ray){const auto& rec=records[size_t(sw)*execution+ray];auto& windows=memberWindows[identities[sw].first*n+ray];for(uint32_t k=0;k<rec.count;++k)windows.push_back({rec.windows[k][0],rec.windows[k][1]});}
 for(auto& windows:memberWindows){std::sort(windows.begin(),windows.end());std::vector<std::pair<double,double>> merged;for(auto v:windows){if(!merged.empty()&&v.first<=merged.back().second)merged.back().second=std::max(merged.back().second,v.second);else merged.push_back(v);}windows.swap(merged);}
 std::vector<bool> refusedRays(n,false);const auto verifyStart=std::chrono::steady_clock::now();
 for(uint32_t sw=0;sw<ns;++sw){const auto beforeRefused=refusedJobs,beforeUnknown=unresolved;auto [member,local]=identities[sw];const auto& m=media[member];
  for(uint32_t ray=0;ray<n;++ray){const auto& record=records[size_t(sw)*execution+ray];if(record.flags&4)continue;++checkedJobs;
   const glm::dvec3 o=glm::dvec3(rays[ray].origin_a)-m.origin,d=glm::dvec3(rays[ray].direction_b);double a=rays[ray].origin_a.w,b=rays[ray].direction_b.w;
   for(int k=0;k<3;++k)if(std::abs(d[k])>=1e-15){double x=(-m.half[k]-o[k])/d[k],y=(m.half[k]-o[k])/d[k];a=std::max(a,std::min(x,y));b=std::min(b,std::max(x,y));}
   std::map<std::string,double> bind;for(int k=0;k<3;++k){bind[OntoMath::kLineOriginVars[k]]=o[k];bind[OntoMath::kLineDirectionVars[k]]=d[k];}
   OntoMath::LineAlgebraicEvaluator ev(m.envForm,bind,true);if(!ev.ok())throw std::runtime_error(ev.refusal());std::vector<int8_t> signs(m.envForm.switches.size(),0);
   const double resolution=(b-a)*1e-4;bool refused=bool(record.flags&3);uint32_t budget=0;
   std::function<void(double,double)> verify=[&](double lo,double hi){if(!(hi>lo))return;if(++budget>8192){refused=true;++unresolved;unresolvedWidth+=hi-lo;return;}++gapQueries;bool u=false;ev.switchRange(local,lo,hi,signs,&u);if(u){++undefined;refused=true;return;}if(ev.classifySwitch(local,lo,hi,signs))return;if(hi-lo<=resolution){++unresolved;unresolvedWidth+=hi-lo;refused=true;
    const auto& split=m.envForm.switches[local];const auto& node=m.envForm.nodes[split.node];
    auto at=[&](double t){auto x=ev.value(node.a,t);if(!x)throw std::runtime_error("undefined switch point");if(split.role==1){auto y=ev.value(node.b,t);if(!y)throw std::runtime_error("undefined second switch point");return *x-*y;}return *x-(split.role==2?node.lo:split.role==3?node.hi:0.);};
    const double va=at(lo),vb=at(hi);
    if(std::isfinite(va)&&std::isfinite(vb)&&va*vb<0){++missedCrossings;if(missedExamples.size()<20)missedExamples.push_back({{"ray",ray},{"member",member},{"switch",local},{"lo",lo},{"hi",hi},{"value_lo",va},{"value_hi",vb}});}
    return;}double mid=(lo+hi)*.5;verify(lo,mid);verify(mid,hi);};
   double end=a;for(auto [lo,hi]:memberWindows[member*n+ray]){lo=std::max(a,lo);hi=std::min(b,hi);verify(end,lo);end=std::max(end,hi);}verify(end,b);
   if(refused){++refusedJobs;refusedRays[ray]=true;}
  }
  const auto& form=media[member].envForm;const auto& ls=form.switches[local];perSwitch.push_back({{"member",member},{"switch",local},{"role",ls.role},{"source",form.nodes[ls.node].sourceA->toJson()},{"refused_jobs",refusedJobs-beforeRefused},{"unresolved_leaves",unresolved-beforeUnknown}});
 }
 uint32_t originalPointWitnesses=0;
 for(auto& x:missedExamples) {
  const size_t member=x["member"],ray=x["ray"],k=x["switch"];const auto& m=media[member];const auto& sw=m.envForm.switches[k];const auto& node=m.envForm.nodes[sw.node];
  auto originalAt=[&](double t) {
   const glm::vec3 p=glm::vec3(glm::dvec3(rays[ray].origin_a)+t*glm::dvec3(rays[ray].direction_b)-m.origin);
   const std::map<std::string,PropertyValue> vars{{"p",PropertyValue(p)},{"x",PropertyValue(double(p.x))},{"y",PropertyValue(double(p.y))},{"z",PropertyValue(double(p.z))}};
   auto v=node.sourceA->evaluate(vars);double a=0;if(!v||!propertyValueToNumber(*v,a))throw std::runtime_error("original switch point refused");
   if(sw.role==1){v=node.sourceB->evaluate(vars);double b=0;if(!v||!propertyValueToNumber(*v,b))throw std::runtime_error("original second switch point refused");return a-b;}
   return a-(sw.role==2?node.lo:sw.role==3?node.hi:0.);
  };
  const double a=originalAt(x["lo"]),b=originalAt(x["hi"]);const bool pass=std::isfinite(a)&&std::isfinite(b)&&a*b<0;
  x["authored_AST_value_lo"]=a;x["authored_AST_value_hi"]=b;x["authored_AST_sign_change"]=pass;originalPointWitnesses+=pass;
 }
 const double verificationMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-verifyStart).count();
 J timings=J::array();
 for(int size:{1,4,16}){for(int k=0;k<4;++k){batch(real,rg,size);batch(empty,eg,size);}std::vector<double> a,b;for(int block=0;block<6;++block){a.push_back(batch(real,rg,size));b.push_back(batch(empty,eg,size));b.push_back(batch(empty,eg,size));a.push_back(batch(real,rg,size));}timings.push_back({{"dispatches_per_fence",size},{"root_batch_host_ms",stats(a)},{"empty_batch_host_ms",stats(b)}});}
 J ids=J::array();for(auto [m,k]:identities)ids.push_back({m,k});
 J report={{"scope","GPU root proposals with independent CPU mathematical gap verification; no production or native omission admission"},{"adapter",adapter},{"cold_decode_restrict_emit_ms",coldMs},{"base_rays",n},{"execution_rays",execution},{"switches",ns},{"repeat",repeat},{"bins",bins},{"identities",ids},{"root_windows",roots},{"switch_evaluations",evals},{"device_refusal_jobs",flags},{"root_packet_bytes",outputBytes},{"base_packet_bytes",n*ns*sizeof(RootRecord)},{"params_bytes",params.size()*4},{"replicated_endpoints_identical",true},{"cpu_gap_queries",gapQueries},{"cpu_unresolved_leaves",unresolved},{"cpu_undefined",undefined},{"cpu_unresolved_width_sum",unresolvedWidth},{"cpu_refused_jobs",refusedJobs},{"cpu_checked_jobs",checkedJobs},{"cpu_refused_rays",std::count(refusedRays.begin(),refusedRays.end(),true)},{"cpu_verification_ms",verificationMs},{"timing",timings},{"coverage_contract","Every gap outside the UNION of all member switch proposals is checked for every switch; per-ray unresolved cases refuse."},{"production_admitted",false},{"mathematical_gap_check_pass",refusedJobs==0},{"real_sign_changes_outside_proposals",missedCrossings},{"independent_authored_AST_sign_change_examples",originalPointWitnesses},{"missed_examples",missedExamples},{"per_switch",perSwitch}};
 std::ofstream f(path);f<<report.dump(2)<<"\n";std::cout<<report.dump(2)<<"\n";return 0;
}
'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--work',type=Path,required=True);p.add_argument('--run-only',action='store_true');p.add_argument('--repeat',type=int,default=1);p.add_argument('--bins',type=int,default=8);p.add_argument('--label',default='series1');a=p.parse_args();root=a.root.resolve();w=a.work.resolve();assert root!=w and root not in w.parents
 base=root/'scratch/probes/ray_transport_metal_diagnostic.py';v={x.targets[0].id:ast.literal_eval(x.value) for x in ast.parse(base.read_text()).body if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
 for k,rel in [('RAYS_HASH','docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json'),('ROWS_HASH','docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json'),('ZONE_HASH','saves/zones/Northern Veil/zone.ecform')]:assert hashlib.sha256((root/rel).read_bytes()).hexdigest()==v[k]
 original=v['CPP_SOURCE'];prefix=original[:original.index('// WGSL Header with common math')];stat=original[original.index('static J stats('):original.index('int main(')]
 cpp=prefix+stat+CPP
 if not a.run_only:
  w.mkdir(parents=True,exist_ok=False);(w/'probe.cpp').write_text(cpp);(w/'source.json').write_text(json.dumps({'generated_cpp_sha256':hashlib.sha256(cpp.encode()).hexdigest(),'driver_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'base_script_sha256':hashlib.sha256(base.read_bytes()).hexdigest()},indent=2))
  build=root/'build';db=json.loads((build/'compile_commands.json').read_text());c=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command']);args=shlex.split(c['command']);args[args.index('-c')+1]=str(w/'probe.cpp');args[args.index('-o')+1]=str(w/'probe.o');args+=['-std=c++20','-O3'];subprocess.run(args,cwd=build,check=True)
  args=shlex.split((build/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());args[next(i for i,x in enumerate(args) if x.endswith('/src/entry.cpp.o'))]=str(w/'probe.o');args[args.index('-o')+1]=str(w/'probe');subprocess.run(args,cwd=build,check=True)
 else:assert (w/'probe.cpp').read_text()==cpp
 with (w/(a.label+'.log')).open('w') as log:r=subprocess.run([str(w/'probe'),str(w/(a.label+'.json'))],cwd=root,env=dict(os.environ,EC_ROOT_REPEAT=str(a.repeat),EC_ROOT_BINS=str(a.bins)),stdout=log,stderr=subprocess.STDOUT)
 if r.returncode:print((w/(a.label+'.log')).read_text()[-3000:]);raise SystemExit(r.returncode)
 print(w/(a.label+'.json'))
if __name__=='__main__':main()
