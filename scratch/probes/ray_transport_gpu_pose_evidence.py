"""Verify archived multi-view evidence and optionally restore pinned cases.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
No GPU or authored-save writes. Restored external cases avoid repeating dense AST work.
"""
import argparse,gzip,hashlib,importlib.util,json
from pathlib import Path

def digest(b):return hashlib.sha256(b).hexdigest()
def blob(base,entry):
    p=Path(entry['archive'])
    if p.name!=str(p):raise ValueError('archive path must be local basename')
    b=(base/p).read_bytes()
    if digest(b)!=entry['archive_sha256']:raise ValueError('compressed archive hash mismatch')
    raw=gzip.decompress(b)
    if digest(raw)!=entry['uncompressed_sha256']:raise ValueError('uncompressed archive hash mismatch')
    return raw

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--record',type=Path,required=True);p.add_argument('--restore',type=Path)
    a=p.parse_args();root=a.root.resolve();record=a.record.resolve();d=json.loads(record.read_text());base=record.parent
    spec=importlib.util.spec_from_file_location('pose_evidence_math',root/'scratch/probes/ray_transport_gpu_pose_qualification.py')
    q=importlib.util.module_from_spec(spec);spec.loader.exec_module(q)
    if q.sha(root/'saves/zones/Northern Veil/zone.ecform')!=d['zone_sha256']:raise ValueError('authored source changed')
    fixtures={}
    for name,f in d['fixtures'].items():
        r=blob(base,f['rays']);ref=blob(base,f['reference']);fixtures[name]=(r,ref)
        rays=json.loads(r);refs=json.loads(ref)['rows']
        if len(rays['rays'])!=len(refs) or any(x[:2]!=y[:2] for x,y in zip(rays['rays'],refs)):raise ValueError('fixture pixel alignment')
    for name,e in d.get('native_captures',{}).items():
        raw=blob(base,e)
        if digest(raw)!=json.loads(fixtures[name][0])['capture_sha256']:raise ValueError('native capture/ray provenance mismatch')
    checked=0
    for case in d['runs']:
        r,ref=fixtures[case['fixture']];raw=json.loads(blob(base,case['rows']))
        result=q.independent_check(case['report'],raw,json.loads(r),json.loads(ref)['rows'])
        if result!=case['independent_checks']:raise ValueError('independent record mismatch')
        if case['expected_numerical_pass']!=result['both_resolutions_pass']:raise ValueError('recorded pass/refusal mismatch')
        if case['expected_numerical_pass']:q.load(root,'ray_transport_gpu_producer_repaired').validate_report(case['report'])
        checked+=1
    for s in d['sources']:blob(base,s)
    if a.restore:
        work=a.restore.resolve()
        if root==work or root in work.parents:raise ValueError('external fresh restore directory required')
        work.mkdir(parents=True,exist_ok=False)
        for name,(r,ref) in fixtures.items():
            if Path(name).name!=name:raise ValueError('invalid fixture name')
            directory=work/name;directory.mkdir();(directory/'rays.json').write_bytes(r);(directory/'reference.json').write_bytes(ref)
            m={'case':name,'zone_sha256':d['zone_sha256'],'rays':str(directory/'rays.json'),'rays_sha256':digest(r),
                'reference':str(directory/'reference.json'),'reference_sha256':digest(ref),'scope':'Restored pinned native camera/reference witness only'}
            manifest=directory/'manifest.json';manifest.write_text(json.dumps(m,indent=2)+'\n');q.verify_case(root,manifest,d['zone_sha256'])
        print('Restored',len(fixtures),'validated external cases:',work)
    print('PASS',checked,'raw numerical recomputations;',len(fixtures),'fixtures;',len(d['sources']),'source archive hashes')

if __name__=='__main__':main()
