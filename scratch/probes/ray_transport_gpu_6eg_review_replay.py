"""§6eg corrective replay: immutable native evidence, truthful numerical scope.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
The native verifier below retains §6ee controls; changed-method accuracy is an
observation evaluated against declared gates, never forced true by a comparator.
"""
import argparse,copy,hashlib,importlib.util,json,math,struct,subprocess,zipfile
from pathlib import Path
def sha(b):return hashlib.sha256(b).hexdigest()
def need(ok,message):
    if not ok:raise ValueError(message)
def load(root,name):
    s=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def f32(w):return struct.unpack('<f',struct.pack('<I',w))[0]
def analyze(root,data,rays,refs,baseline,before,bindings):
    dv=load(root,'ray_transport_gpu_6dv_review_replay');metrics=load(root,'ray_transport_sliver_review');out={}
    need(len(data['cases'])==90,'missing source-isolated native cases')
    selected=dict(rays,rays=[rays['rays'][i] for i in refs['ray_indices']]);current=dict(selected,rays=[list(r) for r in selected['rays']])
    for row,i in zip(current['rays'],refs['ray_indices']):row[7:11]=[baseline[i][5],*baseline[i][2:5]]
    for mode in [2,3,5]:
        cases=[c for c in data['cases'] if c['mode']==mode];need(len(cases)==30,'native mode coverage')
        if mode in [2,3]:dv.verify_controls({'cases':cases})
        else:
            need([(c['case'],c['repeats']) for c in cases]==[(c['case'],c['repeats']) for c in data['cases'][:30]],'candidate reordered')
            for c in cases:
                need(c['producer_entry_errors']==c['final_output_errors']==0,'candidate ownership error')
                need(len(c['flags'])==32784,'candidate coverage')
                need(sum(bool(f&0x40000000) for f in c['flags'])==c['completed_s1'],'terminal count')
                need(sum(bool(f&0x80000000) for f in c['flags'])==c['refused'],'fallback count')
                need(all(f&0xc0000000!=0xc0000000 for f in c['flags']),'two final owners')
                if c['repeats']==1:need(sum(len(x[1]) for x in c['terminal_endpoints'])==c['terminal_endpoint_checks'],'terminal endpoints incomplete')
                need(all(math.isfinite(f32(w)) for _,v,_ in c['outputs'] for w in v),'nonfinite native output')
                need(len(c['outputs'])==(32784 if c['repeats']==1 else 0),'exported output coverage')
                if c['repeats']==1 and c['output_differences'] is not None:need(c['output_differences']==sum(v!=e for _,v,e in c['outputs']),'false difference count')
        c=cases[0];raw=[[*rays['rays'][i][:2],*[f32(w) for w in actual]] for i,actual,expected in c['outputs']]
        sample=[raw[i] for i in refs['ray_indices']]
        packet=[[*rays['rays'][i][:2],flag,[[f32(a),f32(b),m] for a,b,m in ss],[[f32(a),f32(b),m] for a,b,m in oo]] for i,flag,ss,oo in c['actual_packets']]
        need(len(packet)==32784,'actual packet coverage')
        out[str(mode)]={'cases':len(cases),'strict_reference_cases':sum(c['strict_reference_parity'] for c in cases),'normal_case':{k:v for k,v in c.items() if k not in ['outputs','flags','terminal_endpoints','actual_packets']},'reference_captured':metrics.reference_metrics(sample,selected,refs['rows']),'reference_contemporary':metrics.reference_metrics(sample,current,refs['rows']),'affine_endpoints':metrics.affine_endpoints(before,packet,rays,bindings)}
    need(out['2']['strict_reference_cases']==out['3']['strict_reference_cases']==30,'unchanged controls lost parity')
    return out

def micro_check(data):
    legacy=data['results']['legacy'];safe=data['results']['safe']
    need(len(legacy)==len(safe)==9,'helper case coverage')
    def band(row):return [f32(w) for w in row['words'][4:6]]
    def covers(row):
        root=row['case'].get('ideal_root');a,b=band(row)
        return root is None or (a<=root<=b)
    need(not covers(legacy[1]) and not covers(legacy[2]),'lost original endpoint/radius counterexample')
    need(legacy[3]['words'][1]==legacy[4]['words'][1]==0,'original nonfinite behavior changed')
    for old,row in zip(legacy,safe):
        need(row['case']==old['case'],'helper inputs differ')
        need(row['words'][0]==1 and all(math.isfinite(x) for x in band(row)),'invalid reference band')
        need(covers(row),'reference lost an ideal root')
    need(safe[3]['words'][1]==safe[4]['words'][1]==4,'reference failed nonfinite refusal')
    need(safe[5]['words'][1:3]==[1,0],'budget refusal semantics')
    need(safe[-1]['words'][2]==32,'cap mechanics not exercised')
    return {'native_helper_observations':18,'legacy_endpoint_root_loss':True,'legacy_underflow_root_loss':True,'legacy_nonfinite_refusal_missing':True,'conservative_reference_pass':True,'scope':'conditional helper behavior; no whole-renderer theorem or cost qualification'}

def authored_check(data):
    rows=data['queries'];need([r['query']['ray'] for r in rows]==[4167,29382,30133],'authored query coverage')
    leaves=0
    for row,named_factor in zip(rows,['f3','f1','f2']):
        q=row['query'];proof=row['ideal_math_interval_leaves'];cursor=q['old_start']
        for p in proof:
            need(p['lo']==cursor and p['hi']>cursor,'interval coverage gap')
            need(p['range']==[0.,0.] and p['undefined'] is False and p['zero'],'not an exact zero range')
            cursor=p['hi'];leaves+=1
        need(cursor==q['new_start'] and row['ideal_math_all_zero'],'incomplete interval proof')
        need(row['positive_count']==0 and row['sample_count']==1025,'authored execution summary differs')
        need(all(s[named_factor]>0 for s in row['samples']),'named-factor refutation changed')
    return {'ideal_authored_zero_intervals':3,'range_leaves':leaves,'executed_AST_points':3075,'exported_point_rows':sum(len(r['samples']) for r in rows),'source_bound_OntoMath_range_execution':True,'native_arithmetic_enclosure_proved':False}

def omission_subsets(matrix,authored):
    case=next(c for c in matrix['cases'] if c['mode']==5)
    result=[]
    for row in authored['queries']:
        q=row['query'];i=q['ray'];m=q['member'];packets=case['actual_packets'][i]
        candidates=[f32(a) for a,b,member in packets[2] if member==m]+[f32(a) for a,b,mask in packets[3] if mask&(1<<m)]
        need(bool(candidates),'no candidate member interval')
        end=min(candidates)
        need(end<=q['new_start'],'new omitted interval escapes proved zero region')
        result.append({'ray':i,'old_start':q['old_start'],'new_start':end,'proved_zero_through':q['new_start'],'omission_is_subset':True})
    return result

def checker_negatives(micro,authored):
    rejected=[]
    def reject(name,fn):
        try:fn()
        except (ValueError,KeyError,IndexError):rejected.append(name);return
        raise ValueError('corruption admitted: '+name)
    d=copy.deepcopy(micro);d['results']['safe'].pop();reject('missing native helper case',lambda:micro_check(d))
    d=copy.deepcopy(micro);d['results']['safe'][3]['words'][1]=0;reject('nonfinite refusal removed',lambda:micro_check(d))
    d=copy.deepcopy(micro);d['results']['safe'][1]['words'][5]=d['results']['safe'][1]['words'][4];reject('reference endpoint enclosure collapsed',lambda:micro_check(d))
    a=copy.deepcopy(authored);a['queries'][0]['ideal_math_interval_leaves'][0]['range'][1]=1.;reject('nonzero interval called zero',lambda:authored_check(a))
    a=copy.deepcopy(authored);a['queries'][1]['ideal_math_interval_leaves'].pop();reject('omitted interval coverage',lambda:authored_check(a))
    return rejected

def replay(root,index,archive,donor_archive,fixture_archive):
    meta=json.loads(index.read_text());need(sha(archive.read_bytes())==meta['archive_sha256'],'review archive drift')
    need(sha(donor_archive.read_bytes())==meta['donor_archive_sha256'],'chunk donor drift');need(sha(fixture_archive.read_bytes())==meta['fixture_archive_sha256'],'fixture donor drift')
    for name,h in meta['source_sha256'].items():need(sha((root/name).read_bytes())==h,'source drift: '+name)
    with zipfile.ZipFile(archive) as z,zipfile.ZipFile(donor_archive) as dz,zipfile.ZipFile(fixture_archive) as fz:
        need(set(z.namelist())==set(meta['artifacts']),'archive membership differs')
        def read(name):
            if name.startswith('donor:'):
                name=name[6:];b=dz.read(name);need(name.startswith('chunks/') and sha(b)==name.split('/')[1],'donor chunk drift');return b
            b=z.read(name);need(sha(b)==meta['artifacts'][name]['sha256'],'artifact drift: '+name);return b
        for name in meta['artifacts']:read(name)
        factory=load(root,'ray_transport_gpu_6eg_root_review');ef=load(root,'ray_transport_gpu_6ef_isolated_root_probe')
        for spec in meta['native_sources']:
            kind=spec['kind']
            if kind=='6ef':cpp=ef.generate(root,spec['mutation'],spec['mode'],spec['strict'])
            elif kind=='matrix':cpp=factory.generate_matrix(root,spec['mutation'],spec['mode'],spec['strict'])
            elif kind=='micro':cpp=factory.generate(root)
            else:cpp=factory.generate_authored(root)
            need(cpp.encode()==read(spec['artifact']),'compiled source reconstruction: '+spec['artifact'])
        for spec in meta['negative_sources']:
            need(spec['exit_code']==250 and spec['assertion'] in read(spec['log']).decode(errors='replace'),'fresh negative did not reach assertion')
        # Host witness is deliberately frozen in the evidence rather than
        # coupled to a mutable public path used by subsequent experiments.
        host_ns={'__name__':'source_bound_6eg_host'};exec(compile(read('host/witness.py'),'source_bound_6eg_host','exec'),host_ns);host=host_ns['evaluate_suite']()
        need(json.loads(json.dumps(host))==json.loads(read('host/result.json')) and host['checks_pass'] and not host['mathematical_qualification'],'host witness replay')
        need(host['remaining_contract_gaps']['radius_underflow']['miss_detected'] and host['remaining_contract_gaps']['rounded_nonzero_radius']['inward_gap_detected'],'radius gap disappeared')
        micro_raw=json.loads(read('micro/result.json'));micro=micro_check(micro_raw);authored=json.loads(read('authored/result.json'));proof=authored_check(authored);negative_checks=checker_negatives(micro_raw,authored)
        original=json.loads(fz.read('benchmark/second.json'));rays=json.loads(fz.read('fixture/rays.json'));refs=json.loads(fz.read('fixture/reference.json'))
        summaries={};subsets=None;factor=load(root,'ray_transport_gpu_6ee_review_replay')
        for label,spec in meta['native_reports'].items():
            raw=factor.restore(spec['parts'],read);need(sha(raw)==spec['sha256'],'native JSON bytes differ: '+label);data=json.loads(raw);del raw
            baseline=json.loads(read(spec['baseline']));summary=analyze(root,data,rays,refs,baseline,original['schedule_census_inputs'],original['guarded_sliver_policy']['curtain_bindings'])
            need(summary==json.loads(read(spec['analysis'])),'derived native analysis differs: '+label);summaries[label]=summary
            if label=='conservative':subsets=omission_subsets(data,authored)
            del data
        need(summaries['6ef']['5']['normal_case']['output_differences']==32758,'6ef counterexample drift')
        need(summaries['conservative']['5']['normal_case']['output_differences']==32780,'conservative output observation drift')
        need(summaries['conservative']['5']['reference_captured']['both_resolutions_pass'] and summaries['conservative']['5']['reference_contemporary']['both_resolutions_pass'],'bounded numerical checkpoint gate failed')
        return {'pass':True,'saved_6ef_strict_controls':60,'fresh_conservative_strict_controls':60,'fresh_conservative_candidate_observations':30,'fresh_native_safety_negatives':3,'checker_corruptions_rejected':negative_checks,'micro':micro,'authored':proof,'new_omission_subsets':subsets,'native_analysis':summaries,'full_GPU_arithmetic_theorem':False,'new_clean_candidate_cost_screen':False,'shipping_gates_open':[1,2,3]}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['root','record','archive','donor','fixture','output']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();r=replay(a.root,a.record,a.archive,a.donor,a.fixture);a.output.write_text(json.dumps(r,indent=2)+'\n');print('PASS: original source-bound observations,90 fresh matrix cases,18 native helper observations,3 safety negatives,3 OntoMath zero intervals, historical/source integrity. Bounded accuracy retained; cost/integration/inhabited gates remain open.')
if __name__=='__main__':main()
