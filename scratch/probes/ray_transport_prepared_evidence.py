"""Replay prepared-coefficient evidence and lifecycle corruption controls, no GPU.
First restore the §6cj fixtures with ray_transport_gpu_pose_evidence.py.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
"""
import argparse,copy,gzip,hashlib,importlib.util,json
from pathlib import Path

def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py')
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def verify(root,record,fixtures):
    doc=json.loads(record.read_text());q=load(root,'ray_transport_gpu_pose_qualification');clock=load(root,'ray_transport_gpu_clock_lifecycle');data={}
    for key,a in doc['archives'].items():
        compressed=(record.parent/a['file']).read_bytes()
        if hashlib.sha256(compressed).hexdigest()!=a['compressed_sha256']:raise ValueError('archive hash: '+key)
        raw=gzip.decompress(compressed)
        if hashlib.sha256(raw).hexdigest()!=a['sha256']:raise ValueError('raw hash: '+key)
        data[key]=json.loads(raw) if a['kind']=='json' else raw
    zh=q.sha(root/'saves/zones/Northern Veil/zone.ecform')
    if zh!=doc['zone_sha256']:raise ValueError('changed source')
    cases={n:q.verify_case(root,fixtures/n/'manifest.json',zh) for n in ['inside','inside-zero-clock','inside-full-control']}
    for label,case in [('full','inside-full-control'),('clock','inside')]:
        report=data[label+'_report'];raw=data[label+'_rows'];q.independent_check(report,raw,cases[case][1],cases[case][2])
        if hashlib.sha256(data['recipe_header']).hexdigest()!=report['authored_coefficient_binding']['recipe_header_sha256']:raise ValueError('compiled recipe provenance')
        if hashlib.sha256(data[label+'_cpp']).hexdigest()!=data[label+'_source']['cpp_sha256']:raise ValueError('compiled source provenance')
    first,pair=cases['inside'],cases['inside-zero-clock'];report=data['clock_report'];frames=clock.verify_frames(report,first,pair,q)
    if frames!=data['clock_independent']['frames']:raise ValueError('saved clock metric mismatch')
    if data['cpu']['pass'] is not True or len(data['cpu']['checks'])!=52:raise ValueError('CPU controls')
    if b'PASS 33 shared-factor' not in data['ctest_log']:raise ValueError('registered test result')
    if len(data['pose_controls']['checks'])!=13:raise ValueError('pose controls')
    def rejected(name,mutate):
        v=copy.deepcopy(report);mutate(v)
        try:clock.verify_frames(v,first,pair,q)
        except (ValueError,KeyError):return name
        raise ValueError('corruption accepted: '+name)
    F=lambda v:v['same_device_clock_replay']['frames']
    controls=[rejected('missing frame',lambda v:F(v).pop()),
        rejected('wrong clock sequence',lambda v:F(v)[1].update(clock=first[1]['time'])),
        rejected('stale native baseline',lambda v:F(v)[1].update(native_current_rows=F(v)[0]['native_current_rows'])),
        rejected('nonfinite output',lambda v:F(v)[1]['rows'][0].__setitem__(2,float('nan'))),
        rejected('inexact fallback',lambda v:F(v)[3]['rows'][0].__setitem__(2,F(v)[3]['rows'][0][2]+.1)),
        rejected('missing fallback',lambda v:F(v)[3].update(fallback_rays=0)),
        rejected('inexact return',lambda v:F(v)[2]['rows'][0].__setitem__(2,F(v)[2]['rows'][0][2]+.1))]
    try:clock.verify_frames(data['failed_partial_preparation'],first,pair,q)
    except ValueError:controls.append('expected partial-preparation failure')
    else:raise ValueError('failed native witness admitted')
    return {'archives_checked':len(data),'primary_records_checked':2,'clock_frames_checked':len(frames),'refusal_controls':controls,'pass':True}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--record',type=Path,required=True);p.add_argument('--fixtures',type=Path,required=True);p.add_argument('--output',type=Path)
    a=p.parse_args();result=verify(a.root.resolve(),a.record.resolve(),a.fixtures.resolve())
    if a.output:
        if a.output.exists():raise ValueError('output exists')
        a.output.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',result['archives_checked'],'archive hashes, 2 primary / 5 clock records, 8 lifecycle refusals')
if __name__=='__main__':main()
