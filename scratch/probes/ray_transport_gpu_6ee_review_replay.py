"""§6ee source-isolated review replay; saved native evidence, no new GPU timing.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
"""
import argparse,hashlib,importlib.util,json,math,re,struct,subprocess,zipfile
from pathlib import Path

def sha(data):return hashlib.sha256(data).hexdigest()
def need(ok,message):
    if not ok:raise ValueError(message)
def load(root,name):
    s=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def factor(data,put):
    # Preserve native numeric lexemes and indentation; share whole repeated
    # field values without storing 478MB of repeated native JSON in Git.
    rows=[];start=0
    pattern=rb'^      ("[^"\n]+"):\x20'
    for match in re.finditer(pattern,data,re.M):
        if match.start()>start:rows.append(put(data[start:match.start()]))
        end=data.find(b'\n      "',match.end())
        closing=data.find(b'\n    }',match.end())
        end=min(x for x in [end,closing] if x>=0)
        rows.append(put(data[match.start():end]));start=end
    if start<len(data):rows.append(put(data[start:]))
    return rows
def restore(parts,read):return b''.join(read(part) for part in parts)
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
    need(out['5']['normal_case']['output_differences']==32736,'isolated SBI counterexample changed')
    need(out['5']['affine_endpoints']['missing']==3,'protected-endpoint observation changed')
    need(out['5']['reference_captured']['both_resolutions_pass'] and out['5']['reference_contemporary']['both_resolutions_pass'],'selected accuracy observation changed')
    return out

def replay(root,index,archive,donor):
    meta=json.loads(index.read_text());need(sha(archive.read_bytes())==meta['archive_sha256'],'archive drift');need(sha(donor.read_bytes())==meta['donor_archive_sha256'],'donor drift')
    historical_host=None
    for name,h in meta['source_sha256'].items():
        data=(root/name).read_bytes()
        if sha(data)!=h and name=='scratch/probes/ray_transport_root_witness.py':
            data=subprocess.check_output(['git','show','aaac44267:'+name],cwd=root)
            historical_host=data
        need(sha(data)==h,'source drift: '+name)
    with zipfile.ZipFile(archive) as z,zipfile.ZipFile(donor) as dz:
        need(set(z.namelist())==set(meta['artifacts']),'archive membership')
        def read(name):
            b=z.read(name);need(sha(b)==meta['artifacts'][name]['sha256'],'artifact drift: '+name);return b
        for name in meta['artifacts']:read(name)
        restored={}
        for name,r in meta['native_reports'].items():
            b=restore(r['parts'],read);need(sha(b)==r['sha256'],'native bytes differ: '+name);restored[name]=json.loads(b)
        producer=load(root,'ray_transport_gpu_6ee_isolated_root_probe')
        for item in meta['native_sources']:
            need(producer.generate(root,item['mutation'],item['mode'],item['strict']).encode()==read(item['artifact']),'native source not reconstructed')
        for item in meta['negatives']:
            need(item['exit_code']==250 and item['assertion'] in read(item['log']).decode(errors='replace'),'negative did not reach assertion')
        refs=json.loads(dz.read('fixture/reference.json'));rays=json.loads(dz.read('fixture/rays.json'));original=json.loads(dz.read('benchmark/second.json'));baseline=json.loads(read('fresh/baseline.json'))
        analysis=analyze(root,restored['fresh'],rays,refs,baseline,original['schedule_census_inputs'],original['guarded_sliver_policy']['curtain_bindings'])
        need(analysis==json.loads(read('analysis/native.json')),'analysis drift')
        if historical_host is not None:
            namespace={'__name__':'source_bound_6ee_host'};exec(compile(historical_host,'source_bound_6ee_host','exec'),namespace);host=namespace['evaluate_suite']()
        else:host=load(root,'ray_transport_root_witness').evaluate_suite()
        need(json.loads(json.dumps(host))==json.loads(read('analysis/cpu.json')),'host witness drift');need(host['checks_pass'] and not host['mathematical_qualification'],'host scope drift')
        legacy=restored['legacy-matrix'];need(len(legacy['cases'])==30 and all(c['output_differences']==0 and c['packet_differences'] in [0,None] for c in legacy['cases']),'legacy self-comparison observation drift')
        old=load(root,'ray_transport_gpu_6dv_review_replay').verify_timing(restored['legacy-clean-timing']['clean_timing_series'])
        published=json.loads(read('legacy/published.json'));need(published['clean_timing_series']==restored['legacy-clean-timing']['clean_timing_series'],'legacy timing provenance differs')
        try:load(root,'ray_transport_gpu_6ed_guarded_root_probe').generate_guarded_root_cpp(root)
        except RuntimeError:pass
        else:raise ValueError('unsafe legacy entrypoint admitted')
        # These are diagnostic failures, not approvals of the candidate.
        return {'pass':True,'strict_native_control_cases':60,'candidate_observation_cases':30,'negative_native_controls':3,'strict_comparison_counterexample':True,'candidate_math_qualified':False,'analysis':analysis,'host_checks_pass':host['checks_pass'],'shared_host_rounding_gap_detected':host['shared_nearest_rounding_gap']['both_miss_without_padding'],'legacy_timing_recomputed':old,'fresh_clean_SBI_timing':False,'shipping_gates_open':[1,2,3]}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['root','record','archive','donor','output']:p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();r=replay(a.root,a.record,a.archive,a.donor);a.output.write_text(json.dumps(r,indent=2)+'\n');print('PASS:60 strict control cases;30 SBI observations;3 native safety negatives plus strict-comparison rejection; selected-reference/endpoint and host-gap evidence replay. Candidate unqualified; no new shipping gain.')
if __name__=='__main__':main()
