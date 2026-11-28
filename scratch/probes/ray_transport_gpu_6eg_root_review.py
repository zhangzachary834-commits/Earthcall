"""§6eg native helper counterexamples and conservative reference.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
No renderer mutation, image qualification, or performance claim.
"""
import importlib.util,json,re,struct
from pathlib import Path
def load(root,name):
 s=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def function(text,name):
 a=text.index('fn '+name+'(');b=text.index('{',a);depth=1;i=b+1
 while depth:
  depth+=(text[i]=='{')-(text[i]=='}');i+=1
 return text[a:i]
def shaders(root):
 cpp=load(root,'ray_transport_gpu_6ef_isolated_root_probe').generate(root)
 source=re.search(r'static const char\* kSbiS1ProducerWgsl = R"WGSL\((.*?)\)WGSL";',cpp,re.S)[1]
 helper=function(source,'isolateMonotone');put=function(source,'putBand')+'\n'+function(source,'putBandTagged')
 signature=helper[:helper.index('{')+1];loop=load(root,'ray_transport_gpu_6ef_isolated_root_probe').OUTWARD_SAFE_SBI_LOOP
 start=loop.index('   if (abs(v) <= error)');end=loop.index('\n   if ((fl <',start)
 loop=loop[:start]+'''   if (!finiteValue(v)) { *reason |= 4u; break; }
   // Keep the incoming bracket when the sign is uncertain. This is a
   // conservative safety reference, not an optimized interval-radius rule.
   if (abs(v) <= error) { break; }'''+loop[end:]
 safe=signature+'''
 if (!(finiteValue(a) && finiteValue(b) && finiteValue(c) && finiteValue(va) && finiteValue(vb) && finiteValue(error) && finiteValue(minSlope)) || !(a < b) || error < 0.0 || !(minSlope > 0.0)) { *reason |= 4u; return; }
 if (abs(va) <= error || abs(vb) <= error) { putBandTagged(a,b,bounds,nb,bands,nr,reason,zeroBands,isZeroLevel); return; }
 if ((va > error && vb < -error) || (va < -error && vb > error)) {
'''+loop+'''\n  putBandTagged(l,r,bounds,nb,bands,nr,reason,zeroBands,isZeroLevel);
 }
}'''
 header='''struct CurtainSpec {origin:vec4f,half:vec4f,limits:vec4f,curv:vec4f,amps:vec4f,freqs:vec4f,phases:vec4f};
struct Config {budget:u32}; var<private> cfg:Config;
struct Input {ab:vec4f,rest:vec4f,oracle:vec4f,flags:vec4u};
@group(0) @binding(0) var<storage,read> inputs:array<Input>;
@group(0) @binding(1) var<storage,read_write> output:array<vec4u>;
var<private> index:u32;
fn finiteValue(x:f32)->bool{return (bitcast<u32>(x)&0x7f800000u)!=0x7f800000u;}
fn evalPhi(m:CurtainSpec,o:vec3f,d:vec3f,s:f32)->f32 {
 let data=inputs[index];
 if(data.flags.x==1u){return bitcast<f32>(data.flags.y);}
 if(data.flags.x==2u){return 1.0;}
 return data.oracle.y*(s-data.oracle.x);
}
'''
 kernel='''
@compute @workgroup_size(1) fn witness(@builtin(global_invocation_id) gid:vec3u){
 index=gid.x;let data=inputs[index];cfg.budget=data.flags.z;
 var m:CurtainSpec;var bounds:array<f32,96>;var nb=0u;var bands:array<vec2f,32>;var nr=0u;var reason=0u;var work=0u;var zero=0u;
 isolateMonotone(m,vec3f(0),vec3f(0),data.ab.x,data.ab.y,data.rest.x,data.ab.z,data.ab.w,data.rest.y,data.rest.z,&bounds,&nb,&bands,&nr,&reason,&work,&zero,false);
 output[2u*index]=vec4u(nr,reason,work,zero);
 output[2u*index+1u]=vec4u(bitcast<u32>(bands[0].x),bitcast<u32>(bands[0].y),nb,0u);
}
'''
 return {'legacy':header+put+'\n'+helper+kernel,'safe':header+put+'\n'+safe+kernel}
def cases():
 f=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
 return [
 {'name':'interior sub-ULP','ab':[f(9.99),f(10.01),-10.,10.],'rest':[0.,1e-5,999.,0.],'oracle':[10.,1000.,0.,0.],'flags':[0,0,32,0],'ideal_root':10.-1e-9},
 {'name':'endpoint sub-ULP','ab':[10.,11.,0.,1000.],'rest':[0.,1e-5,999.,0.],'oracle':[10.,1000.,0.,0.],'flags':[0,0,32,0],'ideal_root':10.+1e-9},
 {'name':'radius underflow','ab':[f(-.0001),f(.0001),f(-.0004),f(.0004)],'rest':[0.,2**-149,4.,0.],'oracle':[0.,4.,0.,0.],'flags':[0,0,32,0],'ideal_root':-2**-152},
 {'name':'NaN oracle','ab':[0.,1.,-.3,.7],'rest':[0.,.001,.99,0.],'oracle':[.3,1.,0.,0.],'flags':[1,0x7fc00000,32,0]},
 {'name':'infinite oracle','ab':[0.,1.,-.3,.7],'rest':[0.,.001,.99,0.],'oracle':[.3,1.,0.,0.],'flags':[1,0x7f800000,32,0]},
 {'name':'budget zero','ab':[0.,1.,-.3,.7],'rest':[0.,.001,.99,0.],'oracle':[.3,1.,0.,0.],'flags':[0,0,0,0],'ideal_root':.3},
 {'name':'positive adjacent','ab':[1.,f(1.+2**-23),-2**-24,2**-24],'rest':[0.,0.,1.,0.],'oracle':[1.,1.,0.,0.],'flags':[0,0,32,0],'ideal_root':1.+2**-24},
 {'name':'negative adjacent','ab':[-1.,f(-1.+2**-24),-2**-25,2**-25],'rest':[0.,0.,1.,0.],'oracle':[-1.,1.,0.,0.],'flags':[0,0,32,0],'ideal_root':-1.+2**-25},
 {'name':'cap mechanics only','ab':[0.,1e20,-1.,1.],'rest':[0.,0.,1.,0.],'oracle':[0.,1.,0.,0.],'flags':[2,0,32,0],'scope':'control-flow witness; this oracle deliberately has no supplied mathematical root contract'}]
def generate(root):
 shaders_=shaders(root);rows=cases()
 prefix=load(root,'ray_transport_gpu_6ef_isolated_root_probe').generate(root).split('int main(int argc, char** argv) {')[0]
 # Inputs and function body come from independently stored records; results
 # are read from native storage, not inferred from a host implementation.
 main=r'''
int main(int argc,char** argv){
 J cases=J::parse(R"JSON(CASES)JSON");wgpu::Device gpu;if(!gpu.init(false))throw std::runtime_error("native device refused");
 auto buffer=[&](WGPUBufferUsage usage,uint64_t size,const void* data){WGPUBufferDescriptor desc{};desc.usage=usage;desc.size=size;auto b=wgpuDeviceCreateBuffer(gpu.device,&desc);if(data)wgpuQueueWriteBuffer(gpu.queue,b,0,data,size);return b;};
 struct Input{float ab[4],rest[4],oracle[4];uint32_t flags[4];};std::vector<Input> inputs(cases.size());
 for(size_t i=0;i<cases.size();++i)for(int k=0;k<4;++k){inputs[i].ab[k]=cases[i]["ab"][k];inputs[i].rest[k]=cases[i]["rest"][k];inputs[i].oracle[k]=cases[i]["oracle"][k];inputs[i].flags[k]=cases[i]["flags"][k];}
 auto ib=buffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopyDst,inputs.size()*sizeof(Input),inputs.data());size_t bytes=cases.size()*32;
 auto ob=buffer(WGPUBufferUsage_Storage|WGPUBufferUsage_CopySrc|WGPUBufferUsage_CopyDst,bytes,nullptr);auto rb=buffer(WGPUBufferUsage_MapRead|WGPUBufferUsage_CopyDst,bytes,nullptr);J result=J::object();
 for(auto [name,code]:std::vector<std::pair<std::string,const char*>>{{"legacy",LEGACY},{"safe",SAFE}}){
 wgpuDevicePushErrorScope(gpu.device,WGPUErrorFilter_Validation);WGPUShaderSourceWGSL s{};s.chain.sType=WGPUSType_ShaderSourceWGSL;s.code=wgpu::Device::str(code);WGPUShaderModuleDescriptor md{};md.nextInChain=&s.chain;auto sm=wgpuDeviceCreateShaderModule(gpu.device,&md);WGPUComputePipelineDescriptor pd{};pd.compute.module=sm;pd.compute.entryPoint=wgpu::Device::str("witness");auto cp=wgpuDeviceCreateComputePipeline(gpu.device,&pd);PopContext err;WGPUPopErrorScopeCallbackInfo cb{};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=onPopError;cb.userdata1=&err;wgpuDevicePopErrorScope(gpu.device,cb);while(!err.done)wgpuDevicePoll(gpu.device,true,nullptr);if(err.type!=WGPUErrorType_NoError||!cp)throw std::runtime_error("witness shader refused");
 WGPUBindGroupEntry e[2]{};e[0].binding=0;e[0].buffer=ib;e[0].size=inputs.size()*sizeof(Input);e[1].binding=1;e[1].buffer=ob;e[1].size=bytes;auto layout=wgpuComputePipelineGetBindGroupLayout(cp,0);WGPUBindGroupDescriptor bd{};bd.layout=layout;bd.entryCount=2;bd.entries=e;auto bg=wgpuDeviceCreateBindGroup(gpu.device,&bd);
 WGPUCommandEncoderDescriptor ed{};auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,&ed);WGPUComputePassDescriptor passd{};auto pass=wgpuCommandEncoderBeginComputePass(enc,&passd);wgpuComputePassEncoderSetPipeline(pass,cp);wgpuComputePassEncoderSetBindGroup(pass,0,bg,0,nullptr);wgpuComputePassEncoderDispatchWorkgroups(pass,inputs.size(),1,1);wgpuComputePassEncoderEnd(pass);wgpuCommandEncoderCopyBufferToBuffer(enc,ob,0,rb,0,bytes);WGPUCommandBufferDescriptor cmdD{};auto cmd=wgpuCommandEncoderFinish(enc,&cmdD);wgpuQueueSubmit(gpu.queue,1,&cmd);wgpuDevicePoll(gpu.device,true,nullptr);
 MapContext mapped;WGPUBufferMapCallbackInfo mc{};mc.mode=WGPUCallbackMode_AllowProcessEvents;mc.callback=onMapped;mc.userdata1=&mapped;wgpuBufferMapAsync(rb,WGPUMapMode_Read,0,bytes,mc);while(!mapped.done)wgpuDevicePoll(gpu.device,true,nullptr);if(!mapped.ok)throw std::runtime_error("witness map refused");auto data=static_cast<const uint32_t*>(wgpuBufferGetConstMappedRange(rb,0,bytes));J rows=J::array();for(size_t i=0;i<inputs.size();++i){J words=J::array();for(int k=0;k<8;++k)words.push_back(data[i*8+k]);rows.push_back({{"case",cases[i]},{"words",words}});}result[name]=rows;wgpuBufferUnmap(rb);
 }
 std::ofstream(argv[1])<<J{{"scope","source-extracted isolateMonotone helper on actual Metal, controlled oracles; conservative safety reference has no performance/image admission"},{"results",result}}.dump(2)<<"\n";
}
'''.replace('CASES',json.dumps(rows)).replace('LEGACY','R"WGSL('+shaders_['legacy']+')WGSL"').replace('SAFE','R"WGSL('+shaders_['safe']+')WGSL"')
 return prefix+main

AUTHORED_MAIN = 'int main(int argc,char** argv){\n std::ifstream in("saves/zones/Northern Veil/zone.ecform",std::ios::binary);std::vector<uint8_t>b((std::istreambuf_iterator<char>(in)),{});\n J zone=J::parse(J::from_msgpack(b).at("MigrationRoot").get<std::string>());J query;{std::ifstream f(argv[1]);f>>query;}\n std::vector<Medium> media;std::vector<std::vector<float>> params;\n for(auto& j:zone.at("spatialFields")){auto field=geom::FieldNode::fromJson(j);Rendering::VolumeDensityBinding bnd;if(!field||!Rendering::readVolumeDensity(*field,0,0,bnd))continue;Medium m;m.owner=field;m.id=bnd.producerId;m.origin=bnd.origin;m.half=glm::abs(bnd.scale);m.density=single(bnd.densityExpr);m.sigma=single(bnd.extinctionExpr);m.emission=single(bnd.emissionExpr);m.envAST=cloneEnvelope(*m.density);auto restricted=OntoMath::restrictAlgebraicToLine(*m.envAST,false,"p");if(!restricted)throw std::runtime_error(restricted.refusal);m.envForm=std::move(*restricted.form);params.push_back(sdfwgsl::collectVolumeParams(bnd.densityExpr).values);media.push_back(std::move(m));}\n J result=J::array();double clock=query.at("clock");glm::dvec3 origin(query["origin"][0],query["origin"][1],query["origin"][2]);\n for(auto& q:query.at("queries")){int member=q["member"];auto& m=media.at(member);auto&v=params.at(member);glm::dvec3 rd(q["direction"][0],q["direction"][1],q["direction"][2]);J samples=J::array();double a=q["old_start"],b=q["new_start"];double minD=INFINITY,maxD=0,maxEnv=0;int positive=0;\n for(int k=0;k<=1024;++k){double s=a+(b-a)*k/1024.0;auto local=origin+s*rd-m.origin;glm::vec3 p(local);std::map<std::string,PropertyValue> vars{{"p",PropertyValue(p)},{"x",PropertyValue(double(p.x))},{"y",PropertyValue(double(p.y))},{"z",PropertyValue(double(p.z))},{OntoMath::kTimeVar,PropertyValue(clock)},{OntoMath::kOmegaXVar,PropertyValue(-rd.x)},{OntoMath::kOmegaYVar,PropertyValue(-rd.y)},{OntoMath::kOmegaZVar,PropertyValue(-rd.z)}};double D=0;auto value=m.density->evaluate(vars);if(!value||!propertyValueToNumber(*value,D))throw std::runtime_error("authored AST density refused");double env=evalEnvelope_cpu(p,v.data());float f1=std::clamp(v[1]-std::abs(p.x)/v[2],v[3],v[4]);float f2=std::clamp(v[5]*(p.y+v[6]),v[7],v[8]);float f3=std::clamp(v[9]-(p.y+v[11])/v[10],v[12],v[13]);minD=std::min(minD,D);maxD=std::max(maxD,D);maxEnv=std::max(maxEnv,env);positive+=D>0;if(k%128==0)samples.push_back({{"s",s},{"local",{p.x,p.y,p.z}},{"f1",f1},{"f2",f2},{"f3",f3},{"envelope",env},{"authored_D",D}});}\n std::map<std::string,double> bind;auto localOrigin=origin-m.origin;for(int j=0;j<3;++j){bind[OntoMath::kLineOriginVars[j]]=localOrigin[j];bind[OntoMath::kLineDirectionVars[j]]=rd[j];}bind[OntoMath::kTimeVar]=clock;bind[OntoMath::kOmegaXVar]=-rd.x;bind[OntoMath::kOmegaYVar]=-rd.y;bind[OntoMath::kOmegaZVar]=-rd.z;\n OntoMath::LineAlgebraicEvaluator evaluator(m.envForm,bind,true);if(!evaluator.ok())throw std::runtime_error(evaluator.refusal());std::vector<int8_t>signs(m.envForm.switches.size(),0);J proof=J::array();bool allZero=true;std::vector<std::tuple<double,double,int>> pending{{a,b,0}};\n while(!pending.empty()){auto [lo,hi,depth]=pending.back();pending.pop_back();bool undefined=false;auto range=evaluator.range(m.envForm.roots[0],lo,hi,signs,&undefined);bool zero=!undefined&&range.lo==0&&range.hi==0;if(!zero&&depth<12){double mid=lo+.5*(hi-lo);pending.emplace_back(mid,hi,depth+1);pending.emplace_back(lo,mid,depth+1);continue;}proof.push_back({{"lo",lo},{"hi",hi},{"range",{range.lo,range.hi}},{"undefined",undefined},{"zero",zero}});allZero=allZero&&zero;}\n result.push_back({{"ideal_math_all_zero",allZero},{"ideal_math_interval_leaves",proof},{"query",q},{"param34",v},{"id",m.id},{"positive_count",positive},{"sample_count",1025},{"min_density",minD},{"max_density",maxD},{"max_envelope",maxEnv},{"samples",samples}});\n }\n std::ofstream(argv[2])<<J{{"scope","executed original authored AST at1025 host points per omitted interval; sampling is not an interval proof"},{"queries",result}}.dump(2)<<"\\n";\n}\n'

def generate_authored(root):
 return load(root,"ray_transport_gpu_6ef_isolated_root_probe").generate(root).split("int main(int argc, char** argv) {")[0]+"\n"+AUTHORED_MAIN

def generate_matrix(root,mutation='fixed',mode=None,strict=False):
 cpp=load(root,'ray_transport_gpu_6ef_isolated_root_probe').generate(root,mutation,mode,strict)
 safe=function(shaders(root)['safe'],'isolateMonotone')
 for name in ['kSbiS1ProducerWgsl','kSbiS2ProducerWgsl','kSbiGeneralProducerWgsl']:
  p=re.search(r'static const char\* '+name+r' = R"WGSL\((.*?)\)WGSL";',cpp,re.S)
  old=p[1];helper=function(old,'isolateMonotone');new=old.replace(helper,'fn finiteValue(x:f32)->bool{return (bitcast<u32>(x)&0x7f800000u)!=0x7f800000u;}\n'+safe)
  cpp=cpp[:p.start(1)]+new+cpp[p.end(1):]
 return cpp.replace('§6ef outward-safe SBI observation; modes0-3 frozen bisection, mode5 candidate; no premature shipping claim','§6eg conservative uncertain-bracket candidate; frozen controls; finite numerical observations only')

def main():
 import argparse,hashlib,os,subprocess,sys
 p=argparse.ArgumentParser(description=__doc__)
 for n in ['root','build-root','work']:p.add_argument('--'+n,type=Path,required=True)
 p.add_argument('--kind',choices=['micro','authored','matrix'],required=True);p.add_argument('--queries',type=Path);p.add_argument('--mutation',choices=['fixed','terminal-missing','fallback-missing','no-reset'],default='fixed');p.add_argument('--mode',type=int,choices=[2,3,5]);p.add_argument('--strict',action='store_true');p.add_argument('--run',action='store_true');a=p.parse_args();root=a.root.resolve();work=a.work.resolve()
 if work.exists() or root==work or root in work.parents:raise ValueError('fresh external output required')
 work.mkdir();cpp=generate_matrix(root,a.mutation,a.mode,a.strict) if a.kind=='matrix' else {'micro':generate,'authored':generate_authored}[a.kind](root);source=work/'probe.cpp';source.write_text(cpp)
 (work/'source.json').write_text(json.dumps({'scope':'6eg corrective diagnostic, no mathematical/performance promotion','kind':a.kind,'mutation':a.mutation,'mode':a.mode,'strict':a.strict,'cpp_sha256':hashlib.sha256(cpp.encode()).hexdigest()},indent=2)+'\n')
 subprocess.run([sys.executable,'-B',str(root/'scratch/probes/ray_transport_probe_build.py'),'--root',str(root),'--build-root',str(a.build_root.resolve()),'--source',str(source),'--work',str(work/'build')],check=True)
 if not a.run:return
 env=dict(os.environ);args=[str(work/'build/probe')]
 if a.kind=='authored':
  if not a.queries:raise ValueError('authored mode requires frozen queries JSON')
  args+=[str(a.queries.resolve()),str(work/'review.json')]
 else:args+=[str(work/'review.json')]
 zone=root/'saves/zones/Northern Veil/zone.ecform';before=hashlib.sha256(zone.read_bytes()).hexdigest()
 if a.kind=='matrix':
  wide=load(root,'ray_transport_gpu_wide_stream');meta,rays,_=wide.verify_case(root,Path('/private/tmp/earthcall-wide-case/manifest.json'),before)
  env.update(EC_POSE_RAYS=meta['rays'],EC_POSE_REFERENCE=meta['reference'],EC_POSE_CLOCK=str(rays['time']),EC_MATCH_REPEAT='1',EC_GPU_BUDGET='1024',EC_GPU_SUB_CAP='64',EC_GPU_OVER_CAP='16',EC_GPU_MARGIN='.001');env.pop('EC_GPU_GEOMETRY_CASE',None)
 env.pop('METAL_CAPTURE_ENABLED',None)
 with (work/'native.log').open('xb') as f:r=subprocess.run(args,cwd=root,env=env,stdout=f,stderr=subprocess.STDOUT)
 if hashlib.sha256(zone.read_bytes()).hexdigest()!=before:raise ValueError('fixture drift')
 raise SystemExit(r.returncode)
if __name__=='__main__':main()
