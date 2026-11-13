"""Scratch scalar-depth run batching experiment, never production activation.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
OntoMath proves raw E/σ coefficients; Screen emits their GPU arithmetic. Keeps
all original field samples/channels, floors and mixed-medium fallback. Numeric
constants are embedded only for this frozen-fixture experiment: edit/lifecycle
admission is explicitly unfinished. Output fidelity and native cost decide it.
"""
import argparse,json,shlex,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--variant',choices=['batch','dark','lazy'],default='batch');a=p.parse_args()
r=Path(__file__).resolve().parents[2];b=r/'build';s=a.output.resolve();assert r not in s.parents and s!=r;s.mkdir(parents=True,exist_ok=False)
source=(r/'src/Singularity/Screen/WebGPU/SdfWgsl.cpp').read_text()
# Isolated coefficient emitter: no new authored parameters, payload slots, or
# recipes in production. Numeric coefficients are revision-frozen in scratch.
helper=r'''
    auto runPiece=[](const OntoMath::Piecewise* p)->const OntoMath::MathNode* {
        if(!p || p->pieces.size()!=1)return nullptr;const auto& v=p->pieces.front();
        if(v.hasLo||v.hasHi||v.guard||v.whereLEZero||v.call||v.fold)return nullptr;
        return v.mathNode.get();
    };
    const auto* runLight=runPiece(lightRadianceExpr);
    const auto* runSigma=runPiece(extinctionExpr);
    const auto* runEmission=runPiece(emissionExpr);
    bool runDark=runLight && runLight->op==OntoMath::MathNode::Op::ScalarLeaf;
    if(runDark)for(const auto& t:runLight->scalarForm.terms)
        runDark=runDark && t.coefficient==0.0 && t.factors.empty() && t.trans.empty();
    if(runDark && runSigma && runEmission){
        Emit ce;ce.bindTime=true;ce.timeExpression="instances[g_instIdx].time.x";ce.parameterOffset="__run";
        bool ok=true;std::string result;
        for(const char* axis:{"x","y","z"}){
            OntoMath::MathNode component;component.op=OntoMath::MathNode::Op::Component;component.stringArg=axis;
            component.children.push_back(std::make_unique<OntoMath::MathNode>(*runEmission));
            std::string why;auto coefficient=OntoMath::deriveProportionalFieldCoefficient(*runSigma,component,&why);
            if(!coefficient){ok=false;break;}
            std::set<std::string> deps;coefficient->collectDependencies(deps);
            for(const auto& d:deps)if(d!=OntoMath::kTimeVar)ok=false;
            if(!ok)break;
            if(!result.empty())result+=",";result+=emitMathNode(*coefficient,ce,"vec3<f32>(0.0)");
        }
        if(ok && !ce.refused){
            result=ce.body+"    return vec3<f32>("+result+");\n";
            for(size_t k=0;k<ce.params.size();++k){
                if(!std::isfinite(ce.params[k])){ok=false;break;}
                char val[64];std::snprintf(val,sizeof(val),"%.9g",double(ce.params[k]));std::string literal(val);
                if(literal.find_first_of(".eE")==std::string::npos)literal+=".0";
                const auto token="P.v[__run + "+std::to_string(k)+"]";size_t at=0;
                while((at=result.find(token,at))!=std::string::npos){result.replace(at,token.size(),"("+literal+")");at+=literal.size()+2;}
            }
            if(ok)prog.wgsl+="\nfn volumeRunCoefficient() -> vec3<f32> {\n"+result+"}\n";
        }
    }
'''
start=source.index('static Program lowerVolumeFragment(');idx=source.index('    prog.params = std::move(e.params);',start);source=source[:idx]+helper+source[idx:]
needle='        replaceAll(evalBlock, "volumeEmissionEval", "volumeEmissionEval" + suffix);';assert source.count(needle)==1;source=source.replace(needle,needle+'\n        replaceAll(evalBlock, "volumeRunCoefficient", "volumeRunCoefficient" + suffix);')
(s/'batch-SdfWgsl.cpp').write_text(source)
transform=r'''
static std::string rayBatchShader(std::string wgsl){
 for(unsigned i=0;i<4u;++i)if(wgsl.find("fn volumeRunCoefficient_"+std::to_string(i)+"(")==std::string::npos)return wgsl;
 auto replace=[&](const std::string& a,const std::string& b){size_t at=0;unsigned n=0;while((at=wgsl.find(a,at))!=std::string::npos){wgsl.replace(at,a.size(),b);at+=b.size();++n;}if(!n){std::fprintf(stderr,"batch missing hook: %s\n",a.c_str());std::abort();}};
 const std::string flush=R"WGSL(
 if(pendingMask!=0u){
   let nextT=transmittance*exp(-pendingTau);
   integratedRadiance+=pendingColor*(transmittance-nextT);
   transmittance=nextT;pendingMask=0u;pendingTau=0.0;
 }
)WGSL";
 std::string init=R"WGSL(
    var pendingMask=0u;
    var pendingTau=0.0;
    var pendingColor=vec3<f32>(0.0);
)WGSL";
 for(unsigned i=0;i<4u;++i){const auto n=std::to_string(i);init+="g_instIdx="+std::to_string(i+1)+"u; let runColor"+n+"=volumeRunCoefficient_"+n+"();\n";}
 replace("var integratedRadiance = vec3<f32>(0.0);","var integratedRadiance = vec3<f32>(0.0);\n"+init);
 replace("let worldP = ro + rd * sampleT;","let worldP = ro + rd * sampleT; var runMask=0u;var runFloor=false;var runIncident=false;var sampleRunColor=vec3<f32>(0.0);");
 for(unsigned i=0;i<4u;++i){const auto n=std::to_string(i);
  replace("if (density"+n+" > 0.0) {","if (density"+n+" > 0.0) { runMask|="+std::to_string(1u<<i)+"u;sampleRunColor=runColor"+n+";");
  replace("totalExtinction += extinction"+n+";","totalExtinction += extinction"+n+";runFloor=runFloor || !(extinction"+n+">1e-6 && extinction"+n+"<1e30);");
  replace("if (any(incidentLi"+n+" != vec3<f32>(0.0))) {","if (any(incidentLi"+n+" != vec3<f32>(0.0))) {runIncident=true;");
 }
 replace("        let oldT = transmittance;",R"WGSL(
        if(totalExtinction>0.0){
          let dtau=totalExtinction*stepLength;
          let colorOK=all(sampleRunColor>=vec3<f32>(0.0))&&all(sampleRunColor<vec3<f32>(1e20));
          let single=countOneBits(runMask)==1u&&!runFloor&&!runIncident&&colorOK;
          if(single && pendingMask!=0u && pendingMask!=runMask){
)WGSL"+flush+R"WGSL(
          }
          // Work grouping only while T>0.1 and pending optical depth<=1:
          // actual pending T remains far above the legacy 0.01 cutoff.
          if(single && transmittance>0.1 && pendingTau+dtau<=1.0){
            pendingMask=runMask;pendingTau+=dtau;pendingColor=sampleRunColor;
            continue;
          }
)WGSL"+flush+R"WGSL(
        }
        let oldT = transmittance;
)WGSL");
 // Flush exactly once at the final answer. Pending zero gaps have no effect;
 // mixed/floor/large-depth samples already flushed before the original update.
 replace("    let alpha = 1.0 - transmittance;",flush+"\n    let alpha = 1.0 - transmittance;");
 std::fprintf(stderr,"BATCH active: four OntoMath coefficients, sample-run transport\n");
 if(const char* dump=std::getenv("EC_BATCH_WGSL")){if(FILE* f=std::fopen(dump,"w")){std::fwrite(wgsl.data(),1,wgsl.size(),f);std::fclose(f);}}
 return wgsl;
}
'''
# Independently screen literal-zero source specialization, and then the
# lower-liveness batch form, rather than attribute both changes to batching.
if a.variant in ('dark','lazy'):
    dark=r"""
 for(unsigned i=0;i<4u;++i){
  const auto begin=wgsl.find("fn lightRadianceEval_"+std::to_string(i)+"(");
  const auto body=wgsl.find('{',begin);const auto end=wgsl.find("\n}",body);
  if(begin==std::string::npos||body==std::string::npos||end==std::string::npos)std::abort();
  wgsl.replace(body+1,end-body-1,"\n    return 0.0;");
 }
 std::fprintf(stderr,"DARK active: literal-zero authored source specialization\n");
"""
    if a.variant=='dark':dark+=' return wgsl;\n'
    transform=transform.replace(' const std::string flush=',dark+' const std::string flush=',1)
if a.variant=='lazy':
    begin=transform.index(' for(unsigned i=0;i<4u;++i){const auto n=std::to_string(i);init+=')
    end=transform.index('\n',begin)
    transform=transform[:begin]+transform[end:]
    old='sampleRunColor=runColor"+n+";'
    new='sampleRunColor=select(volumeRunCoefficient_"+n+"(),pendingColor,pendingMask=="+std::to_string(1u<<i)+"u);'
    # A true branch avoids executing the coefficient evaluator on continuation;
    # WGSL select eagerly evaluates its operands, so do not use it here.
    new='if(pendingMask=="+std::to_string(1u<<i)+"u){sampleRunColor=pendingColor;}else{sampleRunColor=volumeRunCoefficient_"+n+"();}'
    assert transform.count(old)==1
    transform=transform.replace(old,new)

renderer=(r/'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp').read_text();renderer=renderer.replace('namespace {\n',transform+'\nnamespace {\n',1);start=renderer.index('WebGpuRenderer::volumePipeline(');idx=renderer.index('    const auto& wgsl = program.wgsl;',start);renderer=renderer[:idx]+renderer[idx:].replace('    const auto& wgsl = program.wgsl;','    const auto wgsl = rayBatchShader(program.wgsl);',1);(s/'batch-renderer.cpp').write_text(renderer)
db=json.loads((b/'compile_commands.json').read_text())
def compile_one(suffix,src,obj):
 c=next(x for x in db if x['file'].endswith(suffix) and 'earthcall_webgpu.dir' in x['command']);args=shlex.split(c['command']);args[args.index('-c')+1]=str(src);args[args.index('-o')+1]=str(obj);subprocess.run(args,cwd=b,check=True)
compile_one('/SdfWgsl.cpp',s/'batch-SdfWgsl.cpp',s/'batch-sdf.o')
compile_one('/WebGpuRenderer.cpp',s/'batch-renderer.cpp',s/'batch-renderer.o')
compile_one('/entry.cpp',r/'scratch/probes/volume_demand_probe.cpp',s/'probe.o')
args=shlex.split((b/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
for suffix,obj in [('/src/entry.cpp.o',s/'probe.o'),('/src/Singularity/Screen/WebGPU/SdfWgsl.cpp.o',s/'batch-sdf.o'),('/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp.o',s/'batch-renderer.o')]:args[next(i for i,x in enumerate(args) if x.endswith(suffix))]=str(obj)
args[args.index('-o')+1]=str(s/'batch');subprocess.run(args,cwd=b,check=True)
(s/'preflight.txt').write_text('Variant: '+a.variant+'. '+('Source specialization only; no batching. ' if a.variant=='dark' else '')+'Raw proportional coefficient derived by OntoMath with time-only dependencies and literal-zero authored source. All original field/DDA/sample/channel work retained; only homogeneous single-positive-member runs batch optical depth. Mixed/floor/nonfinite/incident/large-depth cases flush and use original transport. T>0.1 and pending tau<=1 avoids crossing legacy cutoff while deferred. Optimistic census exponentials inside 67003 -> 3558, density calls unchanged 147796. More live state/branches/coefficients may erase savings. Accept further work only on repeated >=5% inside native gain; substantial checkpoint target remains >=20% and converged current-or-better reference. Scratch embedded coefficient constants are revision-frozen: edit/device/lifecycle not admitted. No production activation.\n')
print('Scratch '+a.variant+' prototype built; production untouched.')
