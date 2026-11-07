"""Section6ep: exact worker-local binding reuse for Opus's CPU reference.
Derived scratch source only; canonical source and all mathematical expressions
remain unchanged. Proposal: Aetherion / The Firmament, commissioned by Zach.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. No renderer, save, GPU or frame-performance change.
"""
import argparse
import datetime as dt
import hashlib
import json
import math
import os
from pathlib import Path
import shlex
import struct
import subprocess
import time
import zipfile

from ray_transport_6eo_reference_pilot import ROOT, GENERATOR, ZONE, IMPORTED_TEST, digest, git, need, read, write, uniform

from ray_transport_6ep_reference_derivation import CANONICAL_SHA, derive_source
from ray_transport_6ep_reference_derivation import derivation_identity as source_derivation_identity


def sha(b):
    return hashlib.sha256(b).hexdigest()


def derivation_identity():
    return source_derivation_identity(GENERATOR.read_text())

def same_values(a, b):
    return a['columns'] == b['columns'] and len(a['rows']) == len(b['rows']) and all(
        len(x)==len(y) and all(struct.pack('<d',u)==struct.pack('<d',v) for u,v in zip(x,y))
        for x,y in zip(a['rows'],b['rows']))


def prepare(out):
    import sonnet_oracle_capture_gate as G
    need(not out.exists(), 'fresh work required')
    prior = ROOT/'docs/audits/GPU_RAY_TRANSPORT_6EO_REFERENCE_PILOT_2026-10-28.json'
    arc = prior.with_suffix('.zip')
    rec = read(prior)
    need(digest(arc) == rec['archive_sha256'], 'canonical control archive identity')
    out.mkdir()
    jobs = []
    with zipfile.ZipFile(arc) as z:
        for name, source_name, positions in [
             ('continuity','continuity',list(range(18))),
             ('stress','uniform-stress',list(range(74))),
             ('pilot256','cost-cohort',uniform(1024,256)),
             ('cost1024','cost-cohort',list(range(1024)))]:
            ray_member='rays/'+source_name+'.json'
            ref_member='optimized-first/'+source_name+'-reference.json'
            rb, fb = z.read(ray_member), z.read(ref_member)
            need(sha(rb)==rec['members_sha256'][ray_member] and
                 sha(fb)==rec['members_sha256'][ref_member], 'canonical member identity')
            cap, ref = json.loads(rb), json.loads(fb)
            selected = dict(cap, ray_count=len(positions), rays=[cap['rays'][i] for i in positions],
                selection=dict(kind='6ep-fixed-pilot-not-population',
                    parent_rays_sha256=cap['selection']['parent_rays_sha256'],
                    indices=[cap['selection']['indices'][i] for i in positions]))
            G.validate_capture(selected, zone_sha256=digest(ZONE), expected_clock=7.25,
                               expected_origin=[0,50,75], expected_count=len(positions))
            expected = dict(columns=ref['columns'],rows=[ref['rows'][i] for i in positions])
            ray_path=out/(name+'-rays.json');ref_path=out/(name+'-canonical.json')
            write(ray_path,selected);write(ref_path,expected)
            jobs.append(dict(name=name, count=len(positions), rays=str(ray_path),
                 rays_sha256=digest(ray_path), canonical=str(ref_path), canonical_sha256=digest(ref_path),
                 prior_ray_member=ray_member, prior_reference_member=ref_member, positions=positions))
    selection=dict(schema='6ep-fixed-reference-selection-1', prior_record_sha256=digest(prior),
        prior_archive_sha256=digest(arc), generator_derivation=derivation_identity(),
        zone_sha256=digest(ZONE), per_process_timeout_seconds=300, log2N=16, jobs=jobs)
    write(out/'selection.json',selection)
    print(json.dumps({j['name']:j['count'] for j in jobs}))


def build(base, out):
    need(not out.exists() and ROOT not in out.parents, 'fresh external build required')
    m=read(base/'source.json');build_root=Path(m['build_root']);b=build_root/'build'
    need(m['cpp_sha256']==CANONICAL_SHA==digest(GENERATOR), 'canonical base build')
    need(m.get('optimized_weak_definitions_preferred') is True, 'use optimized-first canonical control')
    src=git(ROOT,'rev-parse','HEAD:src')
    need(src==m['engine_src_tree']==git(build_root,'rev-parse','HEAD:src'), 'engine source mismatch')
    for root in (ROOT,build_root):
        need(not git(root,'status','--porcelain','--','src'), 'dirty engine source')
    for item in m['optimized_engine_objects']:
        need(digest(item['source'])==item['source_sha256'] and
             digest(item['object'])==item['object_sha256'], 'optimized evaluator changed')
    out.mkdir();source=out/'veil_reference_worker_bindings.cpp'
    source.write_text(derive_source(GENERATOR.read_text()))
    db=read(b/'compile_commands.json')
    entry=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    cmd=shlex.split(entry['command']);cmd[cmd.index('-c')+1]=str(source);cmd[cmd.index('-o')+1]=str(out/'probe.o')
    cmd+=['-std=c++20','-O3','-I'+str(ROOT/'scratch/probes')]
    need(not any(x.startswith(('-ffast-math','-Ofast','-funsafe-math')) for x in cmd), 'relaxed arithmetic forbidden')
    subprocess.run(cmd,cwd=b,check=True)
    link=list(m['link_argv']);need(link.count(str(base/'probe.o'))==1, 'base main link identity')
    link[link.index(str(base/'probe.o'))]=str(out/'probe.o');link[link.index('-o')+1]=str(out/'probe')
    subprocess.run(link,cwd=b,check=True)
    m.update(source=str(source),cpp_sha256=digest(source),generator_derivation=derivation_identity(),
             canonical_reference_binary_sha256=digest(base/'probe'),compile_argv=cmd,
             probe_object_sha256=digest(out/'probe.o'),link_argv=link)
    write(out/'source.json',m);print(out/'probe')


def fingerprint(binary):
    m=read(binary.parent/'source.json');source=Path(m['source'])
    actual=digest(source)
    need(actual==m['cpp_sha256'], 'compiled generator source changed')
    if actual!=CANONICAL_SHA:
        need(actual==derivation_identity()['derived_generator_sha256'] and
             m.get('generator_derivation')==derivation_identity(), 'unapproved derived generator')
    need(digest(GENERATOR)==CANONICAL_SHA, 'canonical source changed')
    src=git(ROOT,'rev-parse','HEAD:src')
    need(src==m['engine_src_tree']==git(m['build_root'],'rev-parse','HEAD:src'), 'engine mismatch')
    for root in (ROOT,Path(m['build_root'])):
        need(not git(root,'status','--porcelain','--','src'), 'dirty engine source')
    for item in m.get('optimized_engine_objects',[]):
        need(digest(item['source'])==item['source_sha256'] and
             digest(item['object'])==item['object_sha256'], 'optimized evaluator changed')
    return dict(binary_sha256=digest(binary),build_receipt_sha256=digest(binary.parent/'source.json'),
        generator_sha256=actual,canonical_generator_sha256=CANONICAL_SHA,
        zone_sha256=digest(ZONE),engine_src_tree=src,imported_test_sha256=digest(IMPORTED_TEST),
        compile_database_sha256=digest(Path(m['build_root'])/'build/compile_commands.json'))


def run(selection_path, binary, job_name, threads, tag):
    import sonnet_oracle_capture_gate as G
    s=read(selection_path);j=next(j for j in s['jobs'] if j['name']==job_name)
    cap=read(j['rays']);expected=read(j['canonical'])
    need(digest(j['rays'])==j['rays_sha256'] and digest(j['canonical'])==j['canonical_sha256'], 'selected control identity')
    need(s['generator_derivation']==derivation_identity() and s['zone_sha256']==digest(ZONE), 'frozen derivation identity')
    G.validate_capture(cap,zone_sha256=digest(ZONE),expected_clock=7.25,expected_origin=[0,50,75],expected_count=j['count'])
    before=fingerprint(binary)
    out=selection_path.parent/'runs';out.mkdir(exist_ok=True)
    need(tag.replace('-','').replace('_','').isalnum(), 'simple result tag required')
    result=out/(tag+'-reference.json');receipt_path=out/(tag+'-receipt.json');log_path=out/(tag+'.log')
    need(not result.exists() and not receipt_path.exists() and not log_path.exists(), 'no result overwrite')
    argv=[str(binary),j['rays'],'16',str(j['count']),str(threads),str(result)]
    env={k:v for k,v in os.environ.items() if not k.startswith('EC_')}
    started=dt.datetime.now(dt.timezone.utc).isoformat();t0=time.monotonic()
    with log_path.open('w') as log:
        try:code=subprocess.run(argv,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=300).returncode
        except subprocess.TimeoutExpired:code='timeout'
    elapsed=time.monotonic()-t0;after=fingerprint(binary)
    receipt=dict(schema='6ep-worker-bindings-execution-1',status='REFUSED',job=job_name,threads=threads,
        log2N=16,started_utc=started,elapsed_seconds=elapsed,exit_code=code,argv=argv,
        before=before,after=after,selection_sha256=digest(selection_path),
        rays_sha256=j['rays_sha256'],input_sha256_after=digest(j['rays']),canonical_control_sha256=digest(j['canonical']),
        generator_sha256=before['generator_sha256'],zone_sha256=digest(ZONE),time=cap['time'],
        origin=cap['origin'],medium_clocks=cap['medium_clocks'])
    if before['generator_sha256']!=CANONICAL_SHA:receipt['generator_derivation']=derivation_identity()
    write(receipt_path,receipt)
    need(code==0 and before==after and receipt['rays_sha256']==receipt['input_sha256_after'], 'bounded execution failed/changed')
    raw=read(result);receipt['reference_sha256']=digest(result)
    need(len(raw['rows'])==j['count'] and all(len(r)==8 and all(math.isfinite(x) for x in r) for r in raw['rows']), 'reference coverage/finite')
    receipt['exact_values_compared']=8*j['count'];receipt['exact_canonical_match']=same_values(raw,expected)
    write(receipt_path,receipt)
    need(receipt['exact_canonical_match'], 'worker reuse differs from independent canonical control')
    ref=G.reference_to_checker_form(raw,cap,j['rays_sha256'],reference_identity=receipt,reference_sha256=digest(result))
    G.check_convergence(ref)
    receipt['convergence_max']=dict(T=max(abs(r[0]-r[4]) for r in raw['rows']),
        RGB=max(abs(r[c]-r[c+4]) for r in raw['rows'] for c in (1,2,3)))
    log=log_path.read_text()
    need('SOURCE authored radiance identically zero: yes' in log and 'MEDIA 4, clock 7.250' in log and
         f'RAYS {j["count"]}, finest 2^16 intervals,' in log, 'reference model/log count')
    receipt.update(status='EXACT_PILOT_PASS_NOT_POPULATION_ACCEPTANCE',
        full_population_minutes_linear_estimate=elapsed/j['count']*230400/60)
    write(receipt_path,receipt)
    print(json.dumps(dict(tag=tag,rays=j['count'],threads=threads,seconds=elapsed,
        canonical_match=True,convergence=receipt['convergence_max'],full_minutes_estimate=receipt['full_population_minutes_linear_estimate'])))


def main():
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='command',required=True)
    q=sub.add_parser('prepare');q.add_argument('--out',type=Path,required=True)
    q=sub.add_parser('build');q.add_argument('--base-build',type=Path,required=True);q.add_argument('--out',type=Path,required=True)
    q=sub.add_parser('run');q.add_argument('--selection',type=Path,required=True);q.add_argument('--binary',type=Path,required=True)
    q.add_argument('--job',choices=['continuity','stress','pilot256','cost1024'],required=True)
    q.add_argument('--threads',type=int,choices=[4,8],default=4);q.add_argument('--tag',required=True)
    a=p.parse_args()
    if a.command=='prepare':prepare(a.out.resolve())
    elif a.command=='build':build(a.base_build.resolve(),a.out.resolve())
    else:run(a.selection.resolve(),a.binary.resolve(),a.job,a.threads,a.tag)


if __name__=='__main__':main()
