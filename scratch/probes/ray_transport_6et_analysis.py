"""Numerical pilot and paired complete resident economics for distinct rays.
Never a substitute for the missing all-row reference or native Screen/FPS gates.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. Follows Zach's current-or-better accuracy performance checkpoint.
"""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import statistics
import struct
import zipfile

from ray_transport_6eo_reference_pilot import ROOT,ZONE,read,write,digest,need
import ray_transport_full_workload_accuracy as G


def pilot_rows(rays_path):
    rays=read(rays_path);parent_sha=digest(rays_path)
    record=ROOT/'docs/audits/GPU_RAY_TRANSPORT_6EP_REFERENCE_BINDINGS_2026-10-28.json';rec=read(record);archive=record.with_suffix('.zip')
    need(digest(archive)==rec['archive_sha256'],'reference evidence identity');by_index={};bindings=[]
    with zipfile.ZipFile(archive) as z:
        def take(name):
            data=z.read(name);need(hashlib.sha256(data).hexdigest()==rec['members_sha256'][name],'reference member identity '+name)
            return json.loads(data),hashlib.sha256(data).hexdigest()
        for input_name,tag in [('stress','hoist-stress-4'),('cost1024','hoist-cost1024-8')]:
            cap,cap_sha=take('inputs/'+input_name+'-rays.json');ref,ref_sha=take('runs/'+tag+'-reference.json');receipt,receipt_sha=take('runs/'+tag+'-receipt.json')
            need(cap['selection']['parent_rays_sha256']==parent_sha and cap['time']==rays['time']==7.25 and cap['origin']==rays['origin']==[0,50,75],'reference parent clock/origin')
            need(receipt['before']==receipt['after'] and receipt['exit_code']==0 and receipt['status']=='EXACT_PILOT_PASS_NOT_POPULATION_ACCEPTANCE' and receipt['exact_canonical_match'],'reference execution/canonical control')
            need(receipt['rays_sha256']==receipt['input_sha256_after']==cap_sha and receipt['reference_sha256']==ref_sha and receipt['zone_sha256']==digest(ZONE),'reference receipt binding')
            need(ref['columns']==['T','Lr','Lg','Lb','T_half','Lr_half','Lg_half','Lb_half'] and len(ref['rows'])==len(cap['rays'])==len(cap['selection']['indices']),'pilot reference coverage')
            for index,input_row,row in zip(cap['selection']['indices'],cap['rays'],ref['rows']):
                need(input_row==rays['rays'][index] and len(row)==8 and all(math.isfinite(v) for v in row),'pilot input binding/order')
                need(max(abs(row[k]-row[k+4]) for k in range(4))<=1e-6,'pilot reference convergence')
                if index in by_index:need(by_index[index]==row,'overlapping pilot disagreement')
                by_index[index]=row
            bindings.append(dict(tag=tag,input_sha256=cap_sha,reference_sha256=ref_sha,receipt_sha256=receipt_sha))
    indices=sorted(by_index);need(len(indices)==1094,'deduplicated pilot extent')
    selected=dict(parent_rays_sha256=parent_sha,indices=indices,rows=[rays['rays'][i] for i in indices])
    cohort_sha=hashlib.sha256((json.dumps(selected,sort_keys=True,allow_nan=False)+'\n').encode()).hexdigest()
    reference=dict(rays_sha256=cohort_sha,columns=['px','py','T','Lr','Lg','Lb','T_half','Lr_half','Lg_half','Lb_half'],rows=[[*rays['rays'][i][:2],*by_index[i]] for i in indices])
    return rays,indices,reference,cohort_sha,bindings


def values(path):
    raw=path.read_bytes();need(len(raw)==230400*16,'complete actual-output extent')
    rows=list(struct.iter_unpack('<4f',raw));need(all(math.isfinite(x) for r in rows for x in r),'actual output finite')
    return raw,rows


def qualify(work,label):
    rays,indices,ref,sha,bindings=pilot_rows(work/'rays.json');result={};outputs={}
    for mode in [0,1,2]:outputs[mode]=values(work/(label+'.json.mode'+str(mode)+'.bin'))
    need(outputs[1][0]==outputs[2][0],'full population own-method parity')
    for mode in [0,1,2]:
        rows=[[*rays['rays'][i][:2],*outputs[mode][1][i]] for i in indices]
        baseline=[[*rays['rays'][i][:2],*outputs[0][1][i]] for i in indices]
        result[str(mode)]=G.evaluate(rows,baseline,ref,sha)
    headers={mode:(work/(label+'.json.headers'+str(mode)+'.bin')).read_bytes() for mode in [1,2]}
    need(all(len(v)==230400*48 for v in headers.values()),'complete header coverage')
    prefix_differences=0;header_differences=0;refused={};max_sub={};max_over={}
    for mode,raw in headers.items():
        count=0;subs=[];overs=[]
        for i,h in enumerate(struct.iter_unpack('<8f4I',raw)):
            src=rays['rays'][i];expected=[*rays['origin'],src[5],*src[2:5],src[6]]
            need(struct.pack('<8f',*h[:8])==struct.pack('<8f',*expected),'producer changed captured input')
            count+=bool(h[11]&0x80000000);subs.append(h[9]);overs.append(h[11]&0x3fffffff)
        refused[str(mode)]=count;max_sub[str(mode)]=max(subs);max_over[str(mode)]=max(overs)
    header_differences=sum(headers[1][i:i+48]!=headers[2][i:i+48] for i in range(0,len(headers[1]),48))
    return dict(scope='full own-method parity;1094 reference cohort only, not full230400 numerical closure',population=230400,reference_cohort=1094,
        parent_rays_sha256=digest(work/'rays.json'),cohort_sha256=sha,reference_bindings=bindings,full_mode2_mode1_differing_rays=0,
        header_differing_rays=header_differences,refused=refused,max_subpanels=max_sub,max_overlaps=max_over,pilot_accuracy=result,
        pilot_both_candidate_modes_pass=all(result[str(mode)]['both_resolutions_pass'] for mode in [1,2]),full_reference_qualified=False)


def timings(work,labels):
    observations=[];runs={};ratios=[]
    for label in labels:
        data=read(work/(label+'.json'));need(len(data['observations'])==48 and data['rays']==230400 and data['tiles']==14400,'timing extent')
        actual=data['observations'];rat=[]
        for block in range(8):
            rows=[r for r in actual if r['block']==block];want=[2,1,0,0,1,2] if block%4 in [1,2] else [0,1,2,2,1,0]
            need([r['mode'] for r in rows]==want and [r['position'] for r in rows]==list(range(6)),'paired order')
            need(all(r['total_ms']>0 and r['completion_ms']>=0 and r['encode_ms']>=0 and abs(r['total_ms']-r['completion_ms']-r['encode_ms'])<1e-8 for r in rows),'complete timing scope arithmetic')
            means={mode:statistics.mean(r['total_ms'] for r in rows if r['mode']==mode) for mode in [0,1,2]}
            rat.append(dict(block=block,bulk_over_current=means[1]/means[0],routed_over_current=means[2]/means[0]))
        mode_summary={str(mode):{field:dict(median=statistics.median([r[field] for r in actual if r['mode']==mode]),mean=statistics.mean([r[field] for r in actual if r['mode']==mode])) for field in ['encode_ms','completion_ms','total_ms']} for mode in [0,1,2]}
        runs[label]=dict(wait_style=data['wait_style'],modes=mode_summary,paired_blocks=rat,median_routed_over_current=statistics.median(r['routed_over_current'] for r in rat),routed_blocks_faster=sum(r['routed_over_current']<1 for r in rat))
    return dict(scope='complete resident encoding+submit+completion; frozen input/parameter upload/time preparation/Screen excluded',runs=runs,frame_fps_claim=False)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--work',type=Path,required=True);p.add_argument('--label',default='preflight');p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    result=qualify(a.work,a.label);write(a.output,result)
    print('PILOT',result['reference_cohort'],'PASS' if result['pilot_both_candidate_modes_pass'] else 'FAIL','header differences',result['header_differing_rays'])
    for mode in ['1','2']:print(mode,result['pilot_accuracy'][mode]['finest']['gates'])
    if not result['pilot_both_candidate_modes_pass']:raise SystemExit(2)


if __name__=='__main__':main()
