"""Target-only support producer experiment. Scratch diagnostic, no production admission.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
"""
import argparse,ast,hashlib,importlib.util,json,os,shlex,subprocess
from pathlib import Path

def load(path,name):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def once(s,a,b):
 assert s.count(a)==1,(s.count(a),a[:100]);return s.replace(a,b)

def source(s):
 helper=r'''
// Mathematical support only: no internal switch signs are needed by the consumer.
// Unknown, undefined and budget-limited spans remain represented, never dropped.
struct SupportStats {
 uint64_t queries=0,positive=0,empty=0,uncertain=0,budget=0,undefined=0;
};
static std::vector<OntoMath::LineSegmentationPiece> supportSegments(
 const OntoMath::LineAlgebraicEvaluator& ev,uint32_t root,double a,double b,
 double resolution,SupportStats& stats,uint64_t budget=65536) {
 std::vector<OntoMath::LineSegmentationPiece> result;
 std::vector<int8_t> signs(ev.form().switches.size(),0);
 const uint64_t initial=stats.queries;
 auto emit=[&](double lo,double hi,int8_t state) {
  if(!result.empty() && result.back().targetSigns[0]==state && result.back().s1==lo) {
   result.back().s1=hi;return;
  }
  result.push_back({lo,hi,state!=0,{signs},{state}});
 };
 std::function<void(double,double)> visit=[&](double lo,double hi) {
  if(stats.queries-initial>=budget) {++stats.budget;emit(lo,hi,0);return;}
  ++stats.queries;bool undefined=false;
  const auto bound=ev.range(root,lo,hi,signs,&undefined);
  const double slack=ev.slack(root,lo,hi,signs);
  if(undefined || !bound.bounded() || !std::isfinite(slack)) {
   ++stats.undefined;emit(lo,hi,0);return;
  }
  if(double(bound.hi)+slack<=0) {++stats.empty;emit(lo,hi,-1);return;}
  if(double(bound.lo)-slack>0) {++stats.positive;emit(lo,hi,1);return;}
  const double mid=(lo+hi)*.5;
  if(hi-lo<=resolution || !(mid>lo && mid<hi)) {++stats.uncertain;emit(lo,hi,0);return;}
  visit(lo,mid);visit(mid,hi);
 };
 // Proposals are cuts, never proofs. All spans, including root windows, visited.
 std::vector<double> roots;ev.proposeRoots(a,b,roots);std::sort(roots.begin(),roots.end());
 double cur=a;
 for(double r:roots) {
  if(!std::isfinite(r)||r<a||r>b)continue;
  const double lo=std::max(cur,r-.5*resolution),hi=std::min(b,r+.5*resolution);
  if(lo>cur)visit(cur,lo);if(hi>lo)visit(lo,hi);cur=std::max(cur,hi);
 }
 if(cur<b || result.empty())visit(cur,b);
 if(result.empty()||result.front().s0!=a||result.back().s1!=b)throw std::runtime_error("support coverage gap");
 for(size_t i=1;i<result.size();++i)if(result[i-1].s1!=result[i].s0)throw std::runtime_error("support partition gap");
 return result;
}
'''
 checks=r'''
static J supportChecks() {
 J checks=J::array();
 auto record=[&](const char* name,bool pass) {checks.push_back({{"case",name},{"pass",pass}});if(!pass)throw std::runtime_error(name);};
 auto scalar=[](double c) {auto n=std::make_unique<M>();n->op=M::Op::ScalarLeaf;n->scalarForm=OntoMath::ScalarForm::constant(c);return n;};
 auto variable=[](){auto n=std::make_unique<M>();n->op=M::Op::ValueLeaf;n->variableName="x";return n;};
 auto make=[&](double c,bool island) {
  if(!island)return scalar(c);
  auto square=std::make_unique<M>();square->op=M::Op::Scale;square->children.push_back(variable());square->children.push_back(variable());
  auto n=std::make_unique<M>();n->op=M::Op::Sub;n->children.push_back(scalar(c));n->children.push_back(std::move(square));return n;
 };
 std::map<std::string,double> bind;
 for(int i=0;i<3;++i){bind[OntoMath::kLineOriginVars[i]]=0;bind[OntoMath::kLineDirectionVars[i]]=i==0?1:0;}
 for(auto [name,c]:std::vector<std::pair<const char*,double>>{{"constant empty",-1},{"constant positive",1},{"zero density",0}}) {
  auto n=make(c,false);auto form=OntoMath::restrictAlgebraicToLine(*n,false,"p");if(!form)throw std::runtime_error("witness restriction");
  OntoMath::LineAlgebraicEvaluator ev(*form.form,bind,true);SupportStats st;
  const auto pieces=supportSegments(ev,form.form->roots[0],-1,1,.0001,st);
  record(name,pieces.size()==1 && pieces[0].targetSigns[0]==(c>0?1:-1));
 }
 auto n=make(.25,true);auto form=OntoMath::restrictAlgebraicToLine(*n,false,"p");if(!form)throw std::runtime_error("island restriction");
 OntoMath::LineAlgebraicEvaluator ev(*form.form,bind,true);SupportStats st;
 const auto pieces=supportSegments(ev,form.form->roots[0],-1,1,.0001,st);
 bool covered=true,empty=true;
 for(int k=0;k<=2000;++k){const double x=-1+k*.001;bool kept=false;for(const auto& p:pieces)if(p.targetSigns[0]>=0&&x>=p.s0&&x<=p.s1)kept=true;if(.25-x*x>0&&!kept)covered=false;}
 for(const auto& p:pieces)if(p.targetSigns[0]<0)for(int k=0;k<=10;++k){const double x=p.s0+(p.s1-p.s0)*k/10;if(.25-x*x>1e-12)empty=false;}
 record("interior positive island retained despite negative endpoints",covered&&empty);
 SupportStats zero,one;
 const auto z=supportSegments(ev,form.form->roots[0],-1,1,.0001,zero,0);
 record("zero query budget retains entire interval",z.size()==1&&z[0].s0==-1&&z[0].s1==1&&z[0].targetSigns[0]==0&&zero.budget>0);
 const auto q=supportSegments(ev,form.form->roots[0],-1,1,.0001,one,1);
 bool budgetCoverage=true;double end=-1;for(const auto& p:q){budgetCoverage=budgetCoverage&&p.s0==end;end=p.s1;}
 record("partial budget exhaustion retains complete coverage",budgetCoverage&&end==1&&one.budget>0);
 OntoMath::LineAlgebraicForm unknown;unknown.nodes.emplace_back();unknown.nodes[0].kind=OntoMath::LineAlgebraicNode::Kind::Undefined;unknown.roots.push_back(0);
 OntoMath::LineAlgebraicEvaluator u(unknown,std::map<std::string,double>{},false);SupportStats us;const auto up=supportSegments(u,0,-1,1,.0001,us);
 record("undefined expression retained instead of proving empty",up.size()==1&&up[0].targetSigns[0]==0&&us.undefined==1);
 return checks;
}
'''
 s=once(s,'int main(int argc, char** argv) {',helper+checks+'\nint main(int argc, char** argv) {\n const J supportWitnesses=supportChecks();')
 s=once(s,'            if (!m.hasEnv) continue;', '            if (!m.hasEnv) throw std::runtime_error("envelope restriction refused");')
 s=once(s,'            if (!ev.ok()) continue;', '            if (!ev.ok()) throw std::runtime_error("envelope binding refused");')
 s=once(s,'    uint32_t maxSubpanelsPerRay = 0;',r'''
    uint32_t maxSubpanelsPerRay = 0;
    const bool supportMode=!std::getenv("EC_SUPPORT_LEGACY");
    const double panelWidth=std::getenv("EC_SUPPORT_WIDTH")?std::atof(std::getenv("EC_SUPPORT_WIDTH")):4.;
    if(!(panelWidth>0 && panelWidth<=4))throw std::runtime_error("panel width outside (0,4]");
    SupportStats supportStats;uint64_t legacyQueries=0,boxHits=0,allRetained=0;
    std::vector<double> producerRayUs;
    const auto producerStart=std::chrono::steady_clock::now();
''')
 s=once(s,'        std::vector<Piece> allPieces;', '        const auto rayProducerStart=std::chrono::steady_clock::now();\n        std::vector<Piece> allPieces;')
 s=once(s,'            OntoMath::LineAlgebraicEvaluator ev(m.envForm, bind, true);', '            ++boxHits;\n            OntoMath::LineAlgebraicEvaluator ev(m.envForm, bind, true);')
 old='''            const auto pieces = OntoMath::segmentLine({&ev}, ta, tb, resolution, &segStats, 1u << 16, true, nullptr, &target);'''
 new='''            const auto pieces = supportMode ? supportSegments(ev,m.envForm.roots[0],ta,tb,resolution,supportStats) : OntoMath::segmentLine({&ev}, ta, tb, resolution, &segStats, 1u << 16, true, nullptr, &target);
            legacyQueries+=segStats.rangeEvaluations;'''
 s=once(s,old,new)
 s=once(s,'                bool undefinedD = false;', '                if(supportMode) {allPieces.push_back(Piece{int(j),p.s0,p.s1,p.signs[0]});continue;}\n                ++legacyQueries;\n                bool undefinedD = false;')
 s=once(s,'        std::vector<double> events;', '        allRetained+=allPieces.size();\n        std::vector<double> events;')
 s=once(s,'int(std::ceil(h / 4.0))','int(std::ceil(h / panelWidth))')
 s=once(s,'                gpuOverlaps.push_back(GpuOverlap{float(s0), float(s1), mask, 0.0f});', '''                const int panels=supportMode?std::max(1,int(std::ceil(h/panelWidth))):1;
                for(int k=0;k<panels;++k)gpuOverlaps.push_back(GpuOverlap{float(s0+h*k/panels),float(s0+h*(k+1)/panels),mask,0.0f});''')
 hook='''        gpuRays[i].overlapCount = rayOverlapCount;
    }

    std::vector<GpuMediumParams> gpuMedia(media.size());'''
 replacement='''        gpuRays[i].overlapCount = rayOverlapCount;
        producerRayUs.push_back(std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-rayProducerStart).count());
    }
    const double producerMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-producerStart).count();
    J producer={{"support_refusal_witnesses",supportWitnesses},{"mode",supportMode?"target-only support":"legacy branch segmentation"},
      {"host_total_ms",producerMs},{"host_ray_us",stats(producerRayUs)},{"rays",count},{"box_hits",boxHits},
      {"target_queries",supportStats.queries},{"legacy_queries_including_prune",legacyQueries},
      {"positive_leaves",supportStats.positive},{"empty_leaves",supportStats.empty},{"uncertain_leaves_retained",supportStats.uncertain},
      {"budget_refusals_retained",supportStats.budget},{"undefined_refusals_retained",supportStats.undefined},
      {"retained_medium_pieces",allRetained},{"single_subpanels",gpuSubpanels.size()},{"overlap_intervals",gpuOverlaps.size()},
      {"schedule_bytes",gpuRays.size()*sizeof(GpuRayHeader)+gpuSubpanels.size()*sizeof(GpuSubpanel)+gpuOverlaps.size()*sizeof(GpuOverlap)},
      {"panel_width",panelWidth},{"native_empty_certificate",false},{"cross_ray_answer_reuse",false}};
    if(std::getenv("EC_SUPPORT_CPU_ONLY")) {std::ofstream out(outputPath);out<<producer.dump(2)<<"\\n";return 0;}

    std::vector<GpuMediumParams> gpuMedia(media.size());'''
 s=once(s,hook,replacement)
 s=once(s,'    std::cout << report.dump(2) << std::endl;', '    report["producer"]=producer;\n    std::cout << report.dump(2) << std::endl;')
 return s

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--root',type=Path,required=True);p.add_argument('--work',type=Path,required=True)
 p.add_argument('--run-only',action='store_true');p.add_argument('--label',default='support')
 p.add_argument('--mode',choices=['support','legacy'],default='support');p.add_argument('--cpu-only',action='store_true')
 p.add_argument('--repeat',type=int,default=128);p.add_argument('--width',type=float,default=1.)
 a=p.parse_args();root=a.root.resolve();work=a.work.resolve();assert root!=work and root not in work.parents
 base=root/'scratch/probes/ray_transport_metal_diagnostic.py'
 v={x.targets[0].id:ast.literal_eval(x.value) for x in ast.parse(base.read_text()).body if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
 for key,rel in [('RAYS_HASH','docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json'),('ROWS_HASH','docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json'),('ZONE_HASH','saves/zones/Northern Veil/zone.ecform')]:assert hashlib.sha256((root/rel).read_bytes()).hexdigest()==v[key]
 repair=load(root/'scratch/probes/ray_transport_metal_overlap_repair.py','repair')
 match=load(root/'scratch/probes/ray_transport_native_march_comparison.py','match')
 cpp=source(match.source(repair.repaired_source(v['CPP_SOURCE'])))
 if not a.run_only:
  work.mkdir(parents=True,exist_ok=False);(work/'probe.cpp').write_text(cpp)
  (work/'source.json').write_text(json.dumps({'generated_cpp_sha256':hashlib.sha256(cpp.encode()).hexdigest(),'driver_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()},indent=2))
  build=root/'build';db=json.loads((build/'compile_commands.json').read_text());c=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command']);args=shlex.split(c['command']);args[args.index('-c')+1]=str(work/'probe.cpp');args[args.index('-o')+1]=str(work/'probe.o');args+=['-std=c++20','-O3'];subprocess.run(args,cwd=build,check=True)
  args=shlex.split((build/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());args[next(i for i,x in enumerate(args) if x.endswith('/src/entry.cpp.o'))]=str(work/'probe.o');args[args.index('-o')+1]=str(work/'probe');subprocess.run(args,cwd=build,check=True)
 else:assert hashlib.sha256((work/'probe.cpp').read_bytes()).hexdigest()==hashlib.sha256(cpp.encode()).hexdigest()
 env=dict(os.environ,EC_MATCH_REPEAT=str(a.repeat),EC_SUPPORT_WIDTH=str(a.width))
 for k in ['EC_SUPPORT_CPU_ONLY','EC_SUPPORT_LEGACY']:env.pop(k,None)
 if a.cpu_only:env['EC_SUPPORT_CPU_ONLY']='1'
 if a.mode=='legacy':env['EC_SUPPORT_LEGACY']='1'
 with (work/(a.label+'.log')).open('w') as log:
  r=subprocess.run([str(work/'probe'),str(work/(a.label+'.json'))],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
 if r.returncode:print((work/(a.label+'.log')).read_text()[-3000:]);raise SystemExit(r.returncode)
 print(work/(a.label+'.json'))
if __name__=='__main__':main()
