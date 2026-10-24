"""Replay bitwise streaming/fallback, raw interval census and cost evidence.
No GPU. Restore prior pose fixtures before invoking this verifier.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
"""
import argparse,copy,gzip,hashlib,importlib.util,json,math,statistics,struct
from pathlib import Path

def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py')
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def bits(v):return b''.join(struct.pack('<f',x) for x in v)

def census(rows):
    histogram={};examples=[];multi=maximum=0
    for px,py,flags,singles,overlaps in rows:
        if flags&0x80000000:continue
        spans=[[] for _ in range(4)]
        for lo,hi,member in singles:
            if not (0<=member<4 and math.isfinite(lo) and math.isfinite(hi) and hi>lo):raise ValueError('single census packet')
            spans[member].append((lo,hi))
        for lo,hi,mask in overlaps:
            if not (0<mask<16 and math.isfinite(lo) and math.isfinite(hi) and hi>lo):raise ValueError('overlap census packet')
            for m in range(4):
                if mask&(1<<m):spans[m].append((lo,hi))
        for member,v in enumerate(spans):
            merged=[]
            for lo,hi in sorted(v):
                if merged and lo<=merged[-1][1]:merged[-1][1]=max(hi,merged[-1][1])
                else:merged.append([lo,hi])
            n=len(merged);histogram[str(n)]=histogram.get(str(n),0)+1;maximum=max(maximum,n)
            if n>1:
                multi+=1
                if len(examples)<8:examples.append({'pixel':[px,py],'member':member,'retained_disjoint_intervals':merged})
    return {'pairs_with_multiple_retained_spans':multi,'maximum_retained_spans_per_member':maximum,'span_count_histogram':histogram,'examples':examples}

def check_record(report,raw,baseline,rays,refs,q,capacities=(176,512,1024,8192)):
    q.independent_check(report,raw,rays,refs)
    n=len(rays['rays']);stream=report['streamed_schedule'];cases=stream['cases'];summaries=[]
    if stream['unique_input_rays']!=n or stream['full_viewport'] is not False:raise ValueError('stream input scope')
    if [c['capacity'] for c in cases]!=list(capacities):raise ValueError('capacities')
    if len(baseline)!=n or len(report['schedule_census_inputs'])!=n:raise ValueError('native coverage')
    result=census(report['schedule_census_inputs'])
    for k,v in result.items():
        if report['single_span_compression_preflight'][k]!=v:raise ValueError('saved span census mismatch')
    for rows in [baseline,report['schedule_census_inputs']]:
        if [v[:2] for v in rows]!=[v[:2] for v in rays['rays']]:raise ValueError('source pixel order')
    for c in cases:
        cap=c['capacity'];tiles=(n+cap-1)//cap;tail=n-(tiles-1)*cap
        if (c['tiles'],c['tail_rays'],c['schedule_storage_bytes'],c['uniform_buffer_bytes'])!=(tiles,tail,cap*1280,tiles*288):raise ValueError('memory/tail accounting')
        if c['forced_fallback_rays']!=n:raise ValueError('forced fallback coverage')
        for kind,expected in [('normal_rows',[v[:2]+v[2:6] for v in raw]),('forced_rows',baseline)]:
            rows=c[kind]
            if len(rows)!=n:raise ValueError('stream output coverage')
            for actual,want in zip(rows,expected):
                if len(actual)!=6 or actual[:2]!=want[:2] or not all(math.isfinite(x) for x in actual) or bits(actual[2:])!=bits(want[2:]):raise ValueError('stream output bit mismatch')
        if [s['repeats'] for s in c['series']]!=[1,2,4]:raise ValueError('unqualified command batch')
        means=[]
        for s in c['series']:
            samples=s['samples'];mode_counts={m:sum(x['mode']==m for x in samples) for m in range(3)}
            if len(samples)!=24 or any(v!=8 for v in mode_counts.values()):raise ValueError('paired timing coverage')
            for x in samples:
                if any(not math.isfinite(x[k]) or x[k]<0 for k in ['encode_ms','completion_ms','total_ms']):raise ValueError('invalid timing')
                if not math.isclose(x['encode_ms']+x['completion_ms'],x['total_ms'],rel_tol=1e-12,abs_tol=1e-10):raise ValueError('clock accounting')
            per_frame={m:{k:statistics.mean(x[k] for x in samples if x['mode']==m)/s['repeats'] for k in ['encode_ms','completion_ms','total_ms']} for m in range(3)}
            means.append({'repeats':s['repeats'],'per_query_batch_ms':per_frame})
        summaries.append({'capacity':cap,'tiles':tiles,'tail':tail,'schedule_bytes':cap*1280,'cost':means})
    return {'census':result,'cases':summaries,'pass':True}

def controls(report,raw,baseline,rays,refs,q):
    passed=[]
    def refuse(name,change):
        v=copy.deepcopy(report);change(v)
        try:check_record(v,raw,baseline,rays,refs,q)
        except (ValueError,KeyError):passed.append(name);return
        raise ValueError('corruption accepted: '+name)
    C=lambda v:v['streamed_schedule']['cases'][0]
    refuse('missing tail output',lambda v:C(v)['normal_rows'].pop())
    refuse('changed pixel order',lambda v:C(v)['normal_rows'][0].__setitem__(0,-1))
    refuse('nonfinite result',lambda v:C(v)['normal_rows'][0].__setitem__(2,float('nan')))
    refuse('inexact fallback',lambda v:C(v)['forced_rows'][0].__setitem__(2,C(v)['forced_rows'][0][2]+.01))
    refuse('wrong fallback count',lambda v:C(v).update(forced_fallback_rays=0))
    refuse('wrong bounded storage',lambda v:C(v).update(schedule_storage_bytes=1))
    refuse('missing timing pair',lambda v:C(v)['series'][0]['samples'].pop())
    refuse('invalid complete clock',lambda v:C(v)['series'][0]['samples'][0].update(total_ms=-1))
    refuse('wrong retained span count',lambda v:v['single_span_compression_preflight'].update(pairs_with_multiple_retained_spans=-1))
    # Signed zero is different at the bit level even though numeric == accepts it.
    zero=next(((i,k) for i,row in enumerate(C(report)['normal_rows']) for k in range(2,6) if row[k]==0),None)
    if zero:
        i,k=zero;refuse('signed-zero bit difference',lambda v:C(v)['normal_rows'][i].__setitem__(k,-C(v)['normal_rows'][i][k]))
    return passed

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--record',type=Path,required=True);p.add_argument('--fixtures',type=Path,required=True);p.add_argument('--output',type=Path)
    a=p.parse_args();root=a.root.resolve();record=a.record.resolve();doc=json.loads(record.read_text());data={}
    for key,meta in doc['archives'].items():
        compressed=(record.parent/meta['file']).read_bytes();raw=gzip.decompress(compressed)
        if hashlib.sha256(compressed).hexdigest()!=meta['compressed_sha256'] or hashlib.sha256(raw).hexdigest()!=meta['sha256']:raise ValueError('archive identity')
        data[key]=json.loads(raw) if meta['kind']=='json' else raw
    if hashlib.sha256(data['cpp']).hexdigest()!=data['source']['cpp_sha256']:raise ValueError('compiled source identity')
    q=load(root,'ray_transport_gpu_pose_qualification');zh=q.sha(root/'saves/zones/Northern Veil/zone.ecform');results={}
    for label,case in [('full','inside-full-control'),('spawn','spawn')]:
        manifest,rays,refs=q.verify_case(root,a.fixtures.resolve()/case/'manifest.json',zh)
        results[label]=check_record(data[label+'_report'],data[label+'_rows'],data[label+'_baseline'],rays,refs,q)
        if label=='full':results['negative_controls']=controls(data[label+'_report'],data[label+'_rows'],data[label+'_baseline'],rays,refs,q)
    if b'Parent device is lost' not in data['failed_log']:raise ValueError('missing refused device-loss evidence')
    results['pass']=True
    if a.output:
        if a.output.exists():raise ValueError('output exists')
        a.output.write_text(json.dumps(results,indent=2)+'\n')
    print('PASS',len(data),'archives, 8 streamed normal/fallback cases, independent span census and',len(results['negative_controls']),'refusal controls')
if __name__=='__main__':main()
