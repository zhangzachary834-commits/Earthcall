"""Wide distinct-native-ray ring reuse with explicitly sparse accuracy holdout.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
No fabricated dense reference. All-query bitwise stream/bulk/native fallback;
original-AST node comparison all rays, converged transport reference 192 rays.
"""
import argparse,importlib.util,json,math
from pathlib import Path

def load(root,name):
 spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py')
 m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
 q=load(root,'ray_transport_gpu_pose_qualification');s=load(root,'ray_transport_gpu_streamed_schedule').source(base,root)
 s=q.once(s,'for(uint32_t capacity:{176u,512u,1024u,8192u})','for(uint32_t capacity:{8192u,16384u,32768u})')
 s=q.once(s,'std::vector<int>{1,64,512}','std::vector<int>{1,2,4}')
 s=q.once(s,'if(n==(repeatFactor==1?512:4))','if(n==(repeatFactor==1?4:4))')
 s=q.once(s,'    for (size_t i = 0; i < count; ++i) {\n        const auto& truth = Rows[i];',
  '    for (size_t referenceIndex = 0; referenceIndex < Rows.size(); ++referenceIndex) {\n        const size_t i=ref.at("ray_indices")[referenceIndex].get<size_t>();\n        const auto& truth = Rows[referenceIndex];')
 meta=r'''
    report["scope"]="32784 distinct native-ray ring-reuse screen; independent dense transport reference only on192 predetermined holdout rays; no full viewport/frame/source generality";
    report["pose_case"]["selected_reference_ray_count"]=Rows.size();
    report["reference_scope"]={{"query_count",count},{"dense_reference_count",Rows.size()},
        {"ray_indices",ref.at("ray_indices")},{"selection","Uniform indices fixed before candidate/error evaluation"},
        {"all_rays_have_dense_reference",false},{"all_rays_original_AST_node_agreement_checked",true}};
    report["regressions_vs_native"]["RGB_worse_pct"]=double(aStarWorseThanNat_RGB)*100/Rows.size();
    report["regressions_vs_native"]["T_worse_pct"]=double(aStarWorseThanNat_T)*100/Rows.size();
    report["timing_protocol"]="Bounded1/2/4 query repetitions; streamed_schedule includes encoding and completion, paired with actual current and bulk candidate; no GPU timestamps";
    report["current_vs_A_star"]["legacy_normalization_used_for_retention"]=false;
'''
 return q.once(s,'    std::cout << report.dump(2) << std::endl;',meta+'\n    std::cout << report.dump(2) << std::endl;')

def verify_case(root,path,zone_hash):
 q=load(root,'ray_transport_gpu_pose_qualification');m=json.loads(path.read_text());r=Path(m['rays']);p=Path(m['reference'])
 if not r.is_absolute() or not p.is_absolute() or m['zone_sha256']!=zone_hash or q.sha(r)!=m['rays_sha256'] or q.sha(p)!=m['reference_sha256']:raise ValueError('wide source/case identity')
 rays=json.loads(r.read_text());ref=json.loads(p.read_text());a=rays['rays'];indices=ref['ray_indices'];rows=ref['rows']
 if len(a)!=32784 or len(rows)!=192 or indices!=[int((k+.5)*len(a)/192) for k in range(192)] or rays['viewport']!=[2560,1440] or rays['capture_grid']!=[256,144]:raise ValueError('wide count/selection/viewport')
 if len(rays['origin'])!=3 or not all(math.isfinite(v) for v in rays['origin']) or not math.isfinite(rays['time']):raise ValueError('wide bindings')
 if len(set(tuple(v[:2]) for v in a))!=len(a) or len(set(tuple(v[2:7]) for v in a))!=len(a):raise ValueError('duplicated native inputs')
 for v in a:
  if len(v)!=23 or not all(math.isfinite(x) for x in v) or v[5]>v[6] or not (0<=v[0]<2560 and 0<=v[1]<1440):raise ValueError('wide row')
 for i,v in zip(indices,rows):
  if len(v)!=10 or v[:2]!=a[i][:2] or not all(math.isfinite(x) for x in v):raise ValueError('wide reference alignment')
 if ref['finest_intervals']!=65536 or ref['half_intervals']!=32768:raise ValueError('reference resolution')
 return m,rays,(rows,indices)

def independent_check(report,raw,rays,reference):
 q=load(Path(__file__).resolve().parents[2],'ray_transport_gpu_pose_qualification');refs,indices=reference
 if len(raw)!=len(rays['rays']) or any(v[:2]!=w[:2] or not all(math.isfinite(x) for x in v) for v,w in zip(raw,rays['rays'])):raise ValueError('all-query output coverage/finiteness')
 selected=dict(rays);selected['rays']=[rays['rays'][i] for i in indices]
 result=q.independent_check(report,[raw[i] for i in indices],selected,refs)
 result['scope']={'all_query_count':len(raw),'dense_reference_count':len(indices),'all_queries_densely_referenced':False}
 return result

def main():
 p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True);a,_=p.parse_known_args();root=a.root.resolve();q=load(root,'ray_transport_gpu_pose_qualification')
 q.source=lambda base,r:source(base,r);q.verify_case=lambda r,p,z:verify_case(r,p,z);q.independent_check=independent_check;q.main()
if __name__=='__main__':main()
