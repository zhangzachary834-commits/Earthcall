"""§6cu: Exact-budget correction and independently gated zero-level secant trial.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Quarantined fixture experiments. No source theorem or production admission.
"""
import argparse, ast, gzip, importlib.util, json, sys
from pathlib import Path


def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/(name+'.py'))
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def source(base,root,variant):
    q=load(root,'ray_transport_gpu_pose_qualification')
    s=load(root,'ray_transport_gpu_producer_sliver_guarded' if variant=='baseline' else 'ray_transport_gpu_producer_envelope_and_subcache').source(base,root)
    before='if (work + 24u >= cfg.budget) { reason |= 1u; break; }'
    repaired='if (work > cfg.budget || cfg.budget - work < 24u) { reason |= 1u; break; }'
    if variant in ['cs-fixed','secant-zero']:
        if s.count(before)==2:s=s.replace(before,repaired)
        else:assert s.count(repaired)==2
    elif variant=='cs-before':
        # Intentionally preserve the old fault for the executable regression witness.
        if s.count(repaired)==2:s=s.replace(repaired,before)
        else:assert s.count(before)==2
    if variant=='secant-zero':
        sec=load(root,'ray_transport_gpu_producer_secant_guarded')
        tree=ast.parse((root/'scratch/probes/ray_transport_gpu_producer_secant_guarded.py').read_text())
        fn=next(x for x in tree.body if isinstance(x,ast.FunctionDef) and x.name=='source')
        target=next(ast.literal_eval(x.value) for x in fn.body if isinstance(x,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='old_target' for t in x.targets))
        s=q.once(s,target,'    if (isZeroLevel) {\n'+sec.SECANT_WGSL+'\n    } else {\n'+target+'\n    }')
    # Synthetic geometry controls use the actual producer. They return before
    # transport comparisons, because these specs are not the fixture's AST.
    s=q.once(s,'if(geometryCase=="near-parallel"){',r'''
        if(geometryCase=="budget-boundary"){
            spec.limits={2,2,0,.2f};spec.curv={0,.001f,0,0};
            for(auto& ray:gpuRays){ray.ro[0]=-.5f;ray.ro[1]=0;ray.ro[2]=2;
                ray.rd[0]=1;ray.rd[1]=ray.rd[2]=0;ray.clipA=1;ray.clipB=2;}
        }else if(geometryCase=="near-parallel"){
''')
    s=q.once(s,'    if(!geometryCase.empty()){\n        unsigned positiveChecks=0,fallbacks=0;',r'''
    if(geometryCase=="budget-boundary"){
        unsigned refused=0;J flags=J::array();
        for(size_t i=0;i<count;++i){
            if(producerFlags[i]&0x80000000u)++refused;
            if(gpuRays[i].subpanelCount!=0 || (producerFlags[i]&0x7fffffffu)!=((producerFlags[i]&0x80000000u)?1u:0u))throw std::runtime_error("budget control unexpected packet");
            flags.push_back(producerFlags[i]);
        }
        J result={{"scope","Actual native producer exact-budget geometry control only; no transport or speed qualification"},
            {"budget",cfg.pad},{"rays",count},{"refused",refused},{"flags",flags},{"cell_work_required",24}};
        std::ofstream f(outputPath);f<<result.dump(2)<<"\n";std::cout<<result.dump(2)<<std::endl;return 0;
    }
    if(!geometryCase.empty()){
        unsigned positiveChecks=0,fallbacks=0;
''')
    meta='''
    report["budget_boundary_review"]={{"variant","VARIANT"},{"exact24_remaining_allowed",FIXED},
        {"fixture_only",true},{"shipping_performance_qualified",false}};
'''.replace('VARIANT',variant).replace('FIXED','true' if variant in ['baseline','cs-fixed','secant-zero'] else 'false')
    if variant=='secant-zero':
        meta+='''
    report["envelope_and_subcache"]["strict_same_device_bit_exactness_required"]=false;
    report["budget_boundary_review"]["secant_only_on_zero_level"]=true;
'''
    return q.once(s,'    std::cout << report.dump(2) << std::endl;',meta+'\n    std::cout << report.dump(2) << std::endl;')


def admission(root,report,raw,rays,reference,variant):
    wide=load(root,'ray_transport_gpu_wide_stream');review=load(root,'ray_transport_sliver_review')
    result=wide.independent_check(report,raw,rays,reference)
    before=json.loads(gzip.decompress((root/'docs/audits/GPU_RAY_TRANSPORT_SLIVER_REVIEW_2026-10-22.guarded_confirm_report.gz').read_bytes()))
    a,b=before['schedule_census_inputs'],report['schedule_census_inputs']
    if len(a)!=len(b) or any(x[:2]!=y[:2] for x,y in zip(a,b)):raise ValueError('frozen structural baseline alignment')
    affine=review.affine_endpoints(a,b,rays,before['guarded_sliver_policy']['curtain_bindings'])
    changed=sum(x!=y for row,other in zip(review.union(a),review.union(b)) for x,y in zip(row,other))
    result['frozen_structure']={'affine':affine,'ray_member_unions_changed':changed,'schedule_rays_changed':sum(x!=y for x,y in zip(a,b))}
    if affine['missing'] or changed:raise ValueError(('structural admission refused',result['frozen_structure']))
    if variant=='cs-fixed':
        if any(x!=y for x,y in zip(a,b)):raise ValueError('strict frozen schedule equality')
        old=json.loads(gzip.decompress((root/'docs/audits/GPU_RAY_TRANSPORT_SLIVER_REVIEW_2026-10-22.guarded_confirm_rows.gz').read_bytes()))
        if any(review.bits(x[2:6])!=review.bits(y[2:6]) for x,y in zip(old,raw)):raise ValueError('strict frozen output equality')
    return result


def main():
    p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True)
    p.add_argument('--variant',choices=['baseline','cs-before','cs-fixed','secant-zero'],default='cs-fixed')
    a,rest=p.parse_known_args();root=a.root.resolve();sys.path.insert(0,str(root/'scratch/probes'))
    q=load(root,'ray_transport_gpu_pose_qualification');wide=load(root,'ray_transport_gpu_wide_stream')
    q.source=lambda base,r:source(base,r,a.variant)
    q.verify_case=lambda r,p,z:wide.verify_case(r,p,z)
    q.independent_check=lambda report,raw,rays,refs:admission(root,report,raw,rays,refs,a.variant)
    old=sys.argv;sys.argv=[old[0],'--root',str(root),*rest]
    try:q.main()
    finally:sys.argv=old


if __name__=='__main__':main()
