"""§6dc: portable strict evidence replay; no GPU, temporary case or build required.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-23.
Historical omissions are reported as omissions, never reconstructed observations.
"""
import argparse, copy, hashlib, importlib.util, json
from pathlib import Path


def load(root, name):
    s=importlib.util.spec_from_file_location(name,root/'scratch/probes'/(name+'.py'))
    m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m


def historical(root,r,rows,baseline,rays,ref,guarded,guarded_rows):
    c=load(root,'ray_transport_gpu_continuation_review');p=load(root,'ray_transport_sliver_review');p.clocks=c.clocks
    q=load(root,'ray_transport_root_review');n=len(rays['rays'])
    result=p.check(root,r,rows,baseline,rays,ref['rows'],ref['ray_indices'])
    def same(a,b):
        return len(a)==len(b)==n and all(x[:2]==y[:2] and p.bits(x[2:6])==p.bits(y[2:6]) for x,y in zip(a,b))
    if not same(rows,guarded_rows) or not same(r['trace_output_rows'],rows):raise ValueError('historical normal/trace output parity')
    packets=r['fused_packet_census_inputs'];original=guarded['schedule_census_inputs']
    if len(packets)!=n or any(not q.packets_equal(a,b,p) for a,b in zip(packets,original)):raise ValueError('historical packet parity')
    controls=r['synthetic_budget_controls']
    if [x['budget'] for x in controls]!=[23,24,25]:raise ValueError('historical budget coverage')
    for x in controls:
        expected=16 if x['budget']==23 else 0
        if x['fallbacks']!=expected or x['expected']!=expected or x['pass'] is not True or len(x['flags'])!=16 or any(f!=(0x80000001 if expected else 0) for f in x['flags']):raise ValueError('historical budget flags')
    caps=r['synthetic_capacity_controls']
    if [x['control'] for x in caps]!=['maxSubPerRay=4','maxOverPerRay=1']:raise ValueError('historical capacity coverage')
    for x,reason in zip(caps,[16,32]):
        flags=x['flags'];actual=x['output_rows']
        if len(flags)!=n or len(actual)!=n or x['fallbacks']!=sum(bool(f&0x80000000) for f in flags) or not x['fallbacks'] or x['fallback_bit_exact_pass'] is not True:raise ValueError('historical raw capacity coverage')
        for f,a,b,cur in zip(flags,actual,rows,baseline):
            if f&0x80000000 and f&0x7fffffff!=reason:raise ValueError('historical capacity reason')
            if a[:2]!=b[:2] or p.bits(a[2:6])!=p.bits((cur if f&0x80000000 else b)[2:6]):raise ValueError('historical capacity output')
    result['evidence_gaps']={'mode2_normal_and_forced_rows_not_exported':True,'trace_shader_own_headers_not_read_back':True}
    result['strict_new_experiment_admission']=False
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--record',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if a.output.exists():raise ValueError('refusing output overwrite')
    root=a.root.resolve();record=json.loads(a.record.read_text());q=load(root,'ray_transport_root_review');c=load(root,'ray_transport_gpu_continuation_review');p=load(root,'ray_transport_sliver_review')
    for path,digest in record['dependencies'].items():
        if hashlib.sha256((root/path).read_bytes()).hexdigest()!=digest:raise ValueError('replay dependency drift: '+path)
    bundle=q.Bundle(root/record['bundle'],record['bundle_sha256'])
    for name in bundle.index:bundle.read(name)
    rays=bundle.read('rays');ref=bundle.read('reference');guarded=bundle.read('guarded_report');guarded_rows=bundle.read('guarded_rows');results={}
    for name in ['onfly','coarse','coarse_wide']:
        r=bundle.read(name+'_report');rows=bundle.read(name+'_rows');baseline=bundle.read(name+'_baseline')
        identity=bundle.read(name+'_identity');source=bundle.read(name+'_source')
        if hashlib.sha256(source).hexdigest()!=identity['cpp_sha256'] or identity['zone_sha256']!=record['zone_sha256']:raise ValueError('compiled source/fixture identity')
        if name=='onfly':result=historical(root,r,rows,baseline,rays,ref,guarded,guarded_rows)
        else:
            if 'coarse_peel_experiment' not in r:raise ValueError('coarse native evidence marker')
            result=c.validate(root,r,rows,baseline,rays,ref)
            if len(rows)!=len(guarded_rows) or any(p.bits(x[2:6])!=p.bits(y[2:6]) for x,y in zip(rows,guarded_rows)):raise ValueError('coarse guarded output parity')
            result['affine']=p.affine_endpoints(guarded['schedule_census_inputs'],r['coarse_packet_census_inputs'],rays,guarded['guarded_sliver_policy']['curtain_bindings'])
            if result['affine']['missing']:raise ValueError('coarse affine preservation')
        results[name]=result
    ce=bundle.read('parallel_counterexample')
    if ce['ray_index']!=5 or ce['actual_header']!=ce['baseline_header'] or ce['actual_words'][3]!=ce['expected_words'][3] or [abs(x-y) for x,y in zip(ce['actual_words'][:3],ce['expected_words'][:3])]!=[2,3,2]:raise ValueError('parallel counterexample identity')
    ps=bundle.read('parallel_source');pi=bundle.read('parallel_identity')
    if hashlib.sha256(ps).hexdigest()!=pi['cpp_sha256']:raise ValueError('parallel compiled source identity')
    # Independent negative controls exercise omissions, case coverage, both flag
    # and result corruption, real packet loss, and erroneous clock accounting.
    refusals=[]
    def refuse(name,stage,change):
        r=copy.deepcopy(bundle.read(stage+'_report'));rows=bundle.read(stage+'_rows');baseline=bundle.read(stage+'_baseline');change(r)
        try:
            if stage=='onfly':historical(root,r,rows,baseline,rays,ref,guarded,guarded_rows)
            else:c.validate(root,r,rows,baseline,rays,ref)
        except (ValueError,KeyError,IndexError,TypeError):refusals.append(name)
        else:raise ValueError('corruption accepted: '+name)
    refuse('empty four-mode cases','onfly',lambda r:r['streamed_schedule'].update(cases=[]))
    refuse('missing repeat series','onfly',lambda r:r['streamed_schedule']['cases'][0]['series'].pop())
    refuse('missing trace results','onfly',lambda r:r.pop('trace_output_rows'))
    refuse('missing synthetic budget','onfly',lambda r:r['synthetic_budget_controls'].pop())
    refuse('missing raw capacity flags','onfly',lambda r:r['synthetic_capacity_controls'][0].pop('flags'))
    refuse('false trace endpoint','onfly',lambda r:r['fused_packet_census_inputs'][0][3][0].__setitem__(0,.125))
    refuse('missing mode2 rows','coarse',lambda r:r['streamed_schedule']['cases'][0].pop('mode2_normal_rows'))
    refuse('missing coarse budget','coarse',lambda r:r['coarse_budget_controls'].pop())
    refuse('wrong exact-budget flag','coarse',lambda r:r['coarse_budget_controls'][1]['flags'].__setitem__(0,0x80000001))
    refuse('missing coarse packet','coarse',lambda r:r['coarse_packet_census_inputs'].pop())
    refuse('missing coarse capacity flags','coarse',lambda r:r['coarse_capacity_controls'][0].pop('flags'))
    refuse('poisoned mixed capacity output','coarse',lambda r:r['coarse_capacity_controls'][0]['output_rows'][0].__setitem__(2,1000))
    refuse('wrong four-mode order','coarse_wide',lambda r:r['streamed_schedule']['cases'][0]['series'][0]['samples'][0].update(mode=9))
    refuse('false clock accounting','coarse_wide',lambda r:r['streamed_schedule']['cases'][0]['series'][0]['samples'][0].update(total_ms=999))
    out={'pass':True,'verified_documents':len(bundle.index),'results':results,'parallel_strict_gate_failed':ce,'corruption_refusals':refusals,
         'limits':'Lossless JSON values and exact source bytes; original JSON whitespace not retained. Native AST assertions are recorded, not independently rerun offline. Onfly missing Mode2/raw trace headers remain gaps. Dense reference covers192 holdouts at two resolutions; no full frame, hardware cause, generic source admission or shipped performance.'}
    a.output.write_text(json.dumps(out,indent=2)+'\n');print('PASS continuation bundle',len(bundle.index),'documents;',len(refusals),'corruption refusals',a.output)


if __name__=='__main__':main()
