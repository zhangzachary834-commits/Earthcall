"""Portable §6ec census corrective review replay.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
Replays saved native evidence; this is neither fresh GPU timing nor promotion.
"""
import argparse,array,copy,hashlib,importlib.util,json,struct,tempfile,zipfile
from pathlib import Path

def sha(data):return hashlib.sha256(data).hexdigest()
def need(ok,message):
    if not ok:raise ValueError(message)
def load(root,name):
    s=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m

def tree_delta(base,changed,path=()):
    if type(base)!=type(changed):return [[list(path),changed]]
    if isinstance(base,dict):
        need(set(base)==set(changed),'dictionary shape differs; use explicit schema patch')
        return [row for key in base for row in tree_delta(base[key],changed[key],path+(key,))]
    if isinstance(base,list):
        if len(base)!=len(changed):return [[list(path),changed]]
        return [row for i,(a,b) in enumerate(zip(base,changed)) for row in tree_delta(a,b,path+(i,))]
    return [] if base==changed else [[list(path),changed]]

def apply_delta(base,changes):
    out=copy.deepcopy(base)
    for path,value in changes:
        need(bool(path),'root patch is not supported')
        at=out
        for key in path[:-1]:at=at[key]
        at[path[-1]]=value
    return out

def control_hash(data):
    # Native C++ dump(2) has only integer/bool/text values in this record.
    h=hashlib.sha256();h.update(b'{\n  "cases": [\n')
    for i,c in enumerate(data['cases']):
        if i:h.update(b',\n')
        chunk=json.dumps(c,indent=2,sort_keys=True,ensure_ascii=False)
        h.update(('\n'.join('    '+line for line in chunk.splitlines())).encode())
    h.update(('\n  ],\n  "clean_timing": false,\n  "scope": '+json.dumps(data['scope'],ensure_ascii=False)+',\n  "shipping_qualified": false\n}\n').encode())
    return h.hexdigest()

def fresh_controls(base,header_counts):
    out=copy.deepcopy(base);need(len(out['cases'])==len(header_counts)==30,'incomplete census metadata')
    for c,n in zip(out['cases'],header_counts):c.update(census_events=n,census_overflow=0)
    return out

def argument_permutation(residual,arguments):
    need(residual[:16]==arguments[:16],'observer headers differ')
    r=array.array('I');r.frombytes(residual[16:]);a=array.array('I');a.frombytes(arguments[16:])
    lookup={(r[i]<<32)|r[i+3]:i//8 for i in range(0,len(r),8)}
    need(len(lookup)==len(r)//8,'residual event identity duplicated')
    perm=array.array('I')
    for i in range(0,len(a),8):
        idx=lookup[(a[i]<<32)|a[i+3]];j=8*idx
        expected=list(r[j:j+8]);expected[5]=0;expected[7]=(expected[7]&~255)|2
        need(list(a[i:i+8])==expected,'observer semantic trace differs')
        perm.append(idx)
    need(len(perm)==len(lookup) and len(set(perm))==len(perm),'not a full event permutation')
    return perm.tobytes()

def restore_arguments(residual,permutation):
    count=struct.unpack_from('<I',residual)[0];need(len(permutation)==4*count,'argument permutation length differs')
    words=array.array('I');words.frombytes(residual[16:]);perm=array.array('I');perm.frombytes(permutation)
    need(len(set(perm))==count and all(i<count for i in perm),'invalid argument permutation')
    out=bytearray(residual[:16]);out.extend(b'\0'*(32*count))
    for ordinal,index in enumerate(perm):
        i=8*index;event=list(words[i:i+8]);event[5]=0;event[7]=(event[7]&~255)|2
        struct.pack_into('<8I',out,16+32*ordinal,*event)
    return bytes(out)

def schema_tests(parser):
    def event(seq,sbits,site=3,level=0,tag=(3<<8)|1,value=0):return [0,0,(site<<4)|level,seq,sbits,value,0,tag]
    records=[event(0,0)]+[event(i,1065353216) for i in range(1,10)]+[event(10,0)]
    def blob(rows,cap=None):return struct.pack('<4I',len(rows),0,len(rows) if cap is None else cap,0)+b''.join(struct.pack('<8I',*r) for r in rows)
    with tempfile.TemporaryDirectory(prefix='earthcall-census-replay-') as d:
        p=Path(d)/'events.bin';p.write_bytes(blob(records));v=parser.analyze_events(p,1)
        need(v['actual_route_counts']=={'General':11} and v['position_only_LRU_oracle_hits']['4']==9,'family/cache witness changed')
        refused=[]
        def reject(label,data):
            p.write_bytes(data)
            try:parser.analyze_events(p,1)
            except ValueError:refused.append(label)
            else:raise ValueError('corruption admitted: '+label)
        reject('truncated events',blob(records)[:-1]);reject('trailing events',blob(records)+b'x');reject('event capacity',blob(records,1))
        for label,word,value in [('ray outside fixture',0,1),('medium outside schema',1,4),('unknown site',2,112),('sequence gap',3,2),('nonfinite argument',4,0x7f800000),('missing actual route',7,1),('mixed route',7,257)]:
            rows=[list(r) for r in records];rows[-1][word]=value;reject(label,blob(rows))
        reject('residual at coarse site',blob([event(0,0,site=0,level=3,tag=257)]))
        reject('unavailable value claim',blob([event(0,0,tag=258,value=1)]))
        return {'corruptions_rejected':refused,'family_counterexample':'General may touch only medium0','cache_counterexample':'a10-call first-use gap can require only2 distinct cache entries','LRU_oracle':v['position_only_LRU_oracle_hits']}

def replay(root,record_path,archive_path,donor_path):
    record=json.loads(record_path.read_text());need(sha(archive_path.read_bytes())==record['archive_sha256'],'archive bytes changed');need(sha(donor_path.read_bytes())==record['donor_archive_sha256'],'donor changed')
    for name,digest in record['source_sha256'].items():need(sha((root/name).read_bytes())==digest,'source drift: '+name)
    with zipfile.ZipFile(archive_path) as z,zipfile.ZipFile(donor_path) as dz:
        need(set(z.namelist())==set(record['artifacts']),'archive membership differs')
        def read(name):
            data=z.read(name);need(sha(data)==record['artifacts'][name]['sha256'],'artifact changed: '+name);return data
        for name in record['artifacts']:read(name)
        base=json.loads(dz.read('controls/review.json'));dv=load(root,'ray_transport_gpu_6dv_review_replay')
        old=apply_delta(base,json.loads(read('legacy/control-delta.json')))
        need(control_hash(old)==record['legacy_control_sha256'],'legacy native bytes not reconstructed')
        try:dv.verify_controls(old)
        except AssertionError:legacy_rejected=True
        else:raise ValueError('old relaxed record must fail strict parity')
        first=old['cases'][0];different=[(i,v,e) for i,v,e in first['outputs'] if v!=e]
        need(first['output_differences']==len(different)==21 and first['packet_differences']==76,'legacy counterexample drift')
        max_ulp=max(abs(a-b) for _,actual,expected in different for a,b in zip(actual,expected));need(max_ulp==145,'legacy tolerance was not limited to1ULP')
        fresh=fresh_controls(base,record['fresh_control_header_counts']);need(control_hash(fresh)==record['fresh_control_sha256'],'fresh native controls bytes differ');case_summaries=dv.verify_controls(fresh)
        need(all(c['census_overflow']==0 for c in fresh['cases']),'census overflow admitted')
        factory=load(root,'ray_transport_gpu_6eb_phi_demand_census')
        for row in record['native_sources']:
            cpp=read(row['cpp_artifact']);meta=json.loads(read(row['metadata_artifact']))
            need(factory.generate_census_cpp(root,row['mutation'],row['capture_values']).encode()==cpp,'fresh source reconstruction differs')
            need(meta['cpp_sha256']==sha(cpp),'compiled source metadata differs')
            log=read(row['log_artifact']).decode(errors='replace')
            if row['mutation']!='fixed':need(row['exit_code']==250 and row['expected_failure'] in log,'native negative did not reach assertion')
        namespace={'__name__':'archived_6eb','__file__':str(root/'scratch/probes/ray_transport_gpu_6eb_phi_demand_census.py')}
        exec(compile(read('legacy/generator.py'),'archived-6eb','exec'),namespace)
        need(namespace['generate_census_cpp'](root).encode()==read('legacy/probe.cpp'),'old native source not reconstructible')
        parser=load(root,'ray_transport_gpu_6ec_census_review')
        rdata=read('events/residual.bin');adata=restore_arguments(rdata,read('events/arguments-order.bin'))
        need(sha(adata)==record['argument_native_sha256'],'argument native bytes not restored')
        analyses={}
        with tempfile.TemporaryDirectory(prefix='earthcall-census-events-') as d:
            for name,data,legacy in [('legacy',read('events/legacy.bin'),True),('residual',rdata,False),('arguments',adata,False)]:
                p=Path(d)/(name+'.bin');p.write_bytes(data);a=parser.analyze_events(p,32784,legacy);analyses[name]=a
                if name!='legacy':need(json.loads(json.dumps(a))==json.loads(read('analysis/'+name+'.json')),'analysis differs from raw events')
        need(analyses['residual']['canonical_demand_sha256']==analyses['arguments']['canonical_demand_sha256'],'observers disagree about demand')
        need(analyses['residual']['total_evaluations']==2142243 and analyses['residual']['position_repeats']==40941,'observed demand changed')
        tests=schema_tests(parser)
        f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
        phi,c=f32(.1),f32(.65);need(phi!=f32(f32(phi-c)+c),'value reconstruction counterexample failed')
        return {'pass':True,'legacy_strict_admission_rejected':legacy_rejected,'legacy_baseline':{'differing_outputs':21,'packet_differences':76,'max_component_ulp':max_ulp},'fresh_native_cases':60,'negative_native_count':4,'fresh_case_summaries':case_summaries,'analyses':analyses,'schema_and_constructive_tests':tests,'scope':'fresh native correctness plus portable raw evidence replay; no new GPU timing/capture','shipping_gates_open':True}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['root','record','archive','donor-archive','output']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();d=replay(a.root.resolve(),a.record,a.archive,a.donor_archive);a.output.write_text(json.dumps(d,indent=2)+'\n');print('PASS: strict60 native cases/4 negatives, both observer traces, rejected legacy parity,12 schema refusals and source-bound raw replay')
if __name__=='__main__':main()
