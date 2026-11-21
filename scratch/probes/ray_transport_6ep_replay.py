"""CPU replay of the worker-binding reference derivation, identity and parity.
Stored observations only; no whole-population, GPU or frame-performance claim.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. Proposal from Aetherion / The Firmament; commissioned by Zach.
"""
import argparse
import copy
import json
import math
from pathlib import Path
import zipfile

import ray_transport_6ep_reference_bindings as B
import sonnet_oracle_capture_gate as G


def controls():
    checks=[]
    def refuses(label,fn,reason):
        try:fn()
        except ValueError as e:
            B.need(reason in str(e),'wrong refusal for '+label+': '+str(e));checks.append(label);return
        raise ValueError('admitted '+label)
    original=B.GENERATOR.read_text();derived=B.derive_source(original);identity=B.derivation_identity()
    G.validate_reference_generator({'generator_sha256':B.CANONICAL_SHA});checks.append('canonical identity retained')
    G.validate_reference_generator({'generator_sha256':identity['derived_generator_sha256'],'generator_derivation':identity});checks.append('exact closed derived identity accepted')
    refuses('arbitrary generated source hash',lambda:G.validate_reference_generator({'generator_sha256':'f'*64,'generator_derivation':identity}),'generator identity mismatch')
    refuses('derived source without derivation receipt',lambda:G.validate_reference_generator({'generator_sha256':identity['derived_generator_sha256']}),'derivation identity mismatch')
    wrong=dict(identity,canonical_generator_sha256='f'*64)
    refuses('stale canonical parent',lambda:G.validate_reference_generator({'generator_sha256':identity['derived_generator_sha256'],'generator_derivation':wrong}),'derivation identity mismatch')
    refuses('canonical hash cannot impersonate derived execution',lambda:G.validate_reference_generator({'generator_sha256':B.CANONICAL_SHA,'generator_derivation':identity}),'canonical generator cannot claim')
    refuses('canonical edit cannot inherit old transformation',lambda:B.derive_source(original+'\n'),'unreviewed canonical')
    mutated=derived.replace('vOx=PropertyValue(double(wo.x))','vOx=PropertyValue(0.0)')
    refuses('missing refresh source cannot enter gate',lambda:G.validate_reference_generator({'generator_sha256':B.sha(mutated.encode()),'generator_derivation':identity}),'generator identity mismatch')
    B.need(not B.same_values({'columns':['T'],'rows':[[0.]]},{'columns':['T'],'rows':[[-0.]]}),'signed-zero comparison');checks.append('exact comparator distinguishes signed zero')
    B.need(not B.same_values({'columns':['T'],'rows':[[1.]]},{'columns':['T'],'rows':[]}),'missing reference comparison');checks.append('exact comparator refuses missing rows')
    B.need(derived.count('vp=PropertyValue(pf)')==1 and derived.count('const std::map<std::string,PropertyValue> vars')==1,'derivation scope');checks.append('normal loop reuses storage; separate diagnostic map remains unchanged')
    return checks


def verify_execution(cap,raw,receipt,job,ray_bytes,ref_bytes,selection_sha,build,log,expected):
    B.need(receipt['status']=='EXACT_PILOT_PASS_NOT_POPULATION_ACCEPTANCE' and receipt['exit_code']==0,
           'incomplete execution')
    B.need(receipt['before']==receipt['after'] and
           receipt['rays_sha256']==receipt['input_sha256_after']==job['rays_sha256']==B.sha(ray_bytes), 'execution input/source changed')
    B.need(receipt['selection_sha256']==selection_sha and
           receipt['reference_sha256']==B.sha(ref_bytes), 'execution byte identity')
    B.need(receipt['canonical_control_sha256']==job['canonical_sha256'], 'canonical control identity')
    B.need(receipt['log2N']==16 and receipt['threads'] in (4,8) and
           math.isfinite(receipt['elapsed_seconds']) and 0<receipt['elapsed_seconds']<=300, 'bounded execution')
    B.need(receipt['argv'][2:5]==['16',str(job['count']),str(receipt['threads'])], 'executed work arguments')
    B.need(f'RAYS {job["count"]}, finest 2^16 intervals,' in log and
           f'on {receipt["threads"]} threads,' in log and
           'SOURCE authored radiance identically zero: yes' in log and 'MEDIA 4, clock 7.250' in log,
           'executed model/work log')
    G.validate_capture(cap,zone_sha256=receipt['before']['zone_sha256'],
                       expected_count=job['count'],expected_clock=7.25,expected_origin=[0,50,75])
    B.need(len(raw['rows'])==job['count'] and
           all(len(r)==8 and all(math.isfinite(x) for x in r) for r in raw['rows']), 'reference coverage/finite')
    B.need(B.same_values(raw,expected), 'exact original-AST numerical parity')
    ref=G.reference_to_checker_form(raw,cap,job['rays_sha256'],reference_identity=receipt,reference_sha256=B.sha(ref_bytes))
    G.check_convergence(ref)
    conv=dict(T=max(abs(r[0]-r[4]) for r in raw['rows']),RGB=max(abs(r[c]-r[c+4]) for r in raw['rows'] for c in (1,2,3)))
    B.need(conv==receipt['convergence_max'], 'convergence report')
    B.need(receipt['exact_values_compared']==8*job['count'] and receipt['exact_canonical_match'] is True,
           'exact comparison report')
    B.need(build['cpp_sha256']==receipt['generator_sha256']==receipt['before']['generator_sha256'] and
           build['engine_src_tree']==receipt['before']['engine_src_tree'], 'compiled source identity')
    if receipt['generator_sha256']!=B.CANONICAL_SHA:
        B.need(build['generator_derivation']==B.derivation_identity(), 'compiled derivation identity')
    return dict(rays=job['count'],threads=receipt['threads'],seconds=receipt['elapsed_seconds'],
                convergence_max=conv,full_population_minutes_linear_estimate=receipt['full_population_minutes_linear_estimate'])


def replay(record_path,archive_path):
    rec=B.read(record_path);B.need(B.digest(archive_path)==rec['archive_sha256'], 'archive identity')
    B.need(rec['generator_derivation']==B.derivation_identity(), 'record derivation identity')
    B.need(B.digest(B.ROOT/'scratch/probes/ray_transport_full_workload_accuracy.py')==
           rec['shared_accuracy_checker_sha256'], 'shared mathematical checker changed')
    prior_path=B.ROOT/rec['prior_record'];prior=B.read(prior_path);prior_arc=prior_path.with_suffix('.zip')
    B.need(B.digest(prior_path)==rec['prior_record_sha256'] and B.digest(prior_arc)==prior['archive_sha256'], 'prior control identity')
    results={};distinct=set();first=None
    with zipfile.ZipFile(archive_path) as z,zipfile.ZipFile(prior_arc) as old:
        B.need(len(z.namelist())==len(set(z.namelist()))==len(rec['members_sha256']), 'archive member set')
        for n,h in rec['members_sha256'].items():B.need(B.sha(z.read(n))==h,'member identity '+n)
        B.need(z.read('source/derived-reference.cpp')==B.derive_source(B.GENERATOR.read_text()).encode(),
               'archived compiled source derivation')
        cm=json.loads(z.read('build/canonical-source.json'));dm=json.loads(z.read('build/derived-source.json'))
        B.need(cm['optimized_engine_objects']==dm['optimized_engine_objects'] and
               cm['engine_src_tree']==dm['engine_src_tree'], 'common evaluator build control')
        selection_bytes=z.read('selection.json');selection=json.loads(selection_bytes)
        B.need(selection['prior_record_sha256']==B.digest(prior_path) and
               selection['prior_archive_sha256']==B.digest(prior_arc), 'selection prior identity')
        for entry in rec['executions']:
            tag=entry['tag'];job=next(j for j in selection['jobs'] if j['name']==entry['job'])
            cb=z.read('inputs/'+job['name']+'-rays.json');rb=z.read('runs/'+tag+'-reference.json')
            cap,raw=json.loads(cb),json.loads(rb);receipt=json.loads(z.read('runs/'+tag+'-receipt.json'))
            expected_bytes=z.read('inputs/'+job['name']+'-canonical.json');expected=json.loads(expected_bytes)
            B.need(B.sha(expected_bytes)==job['canonical_sha256'], 'stored control bytes')
            parent_bytes=old.read(job['prior_ray_member']);reference_bytes=old.read(job['prior_reference_member'])
            B.need(B.sha(parent_bytes)==prior['members_sha256'][job['prior_ray_member']] and
                   B.sha(reference_bytes)==prior['members_sha256'][job['prior_reference_member']], 'canonical parent members')
            parent,parent_ref=json.loads(parent_bytes),json.loads(reference_bytes)
            B.need(cap['rays']==[parent['rays'][i] for i in job['positions']] and
                   cap['selection']['indices']==[parent['selection']['indices'][i] for i in job['positions']] and
                   cap['selection']['parent_rays_sha256']==parent['selection']['parent_rays_sha256'], 'actual parent ray selection')
            actual_expected=dict(columns=parent_ref['columns'],rows=[parent_ref['rows'][i] for i in job['positions']])
            B.need(B.same_values(expected,actual_expected), 'independent canonical control')
            build_bytes=z.read(entry['build_receipt']);build=json.loads(build_bytes)
            B.need(B.sha(build_bytes)==receipt['before']['build_receipt_sha256'], 'build receipt bytes')
            log=z.read('runs/'+tag+'.log').decode()
            result=verify_execution(cap,raw,receipt,job,cb,rb,B.sha(selection_bytes),build,log,expected)
            results[tag]=result
            if job['name']!='continuity':distinct.update(tuple(r[:2]) for r in cap['rays'])
            if first is None and job['name']=='pilot256' and receipt['generator_sha256']!=B.CANONICAL_SHA:
                first=(cap,raw,receipt,job,cb,rb,B.sha(selection_bytes),build,log,expected)
        # One-ULP corruption still refuses even after the output hash is re-signed.
        B.need(first is not None,'derived pilot witness missing')
        def reject(case,reason):
            try:verify_execution(*case)
            except ValueError as e:
                B.need(reason in str(e),'wrong artifact mutation refusal: '+str(e));return
            raise ValueError('artifact mutation admitted: '+reason)
        case=list(copy.deepcopy(first));raw=case[1];raw['rows'][0][0]=math.nextafter(raw['rows'][0][0],math.inf)
        case[5]=(json.dumps(raw)+'\n').encode();case[2]['reference_sha256']=B.sha(case[5])
        reject(case,'exact original-AST numerical parity')
        case=list(copy.deepcopy(first));case[2]['threads']=8
        reject(case,'executed work arguments')
        case=list(copy.deepcopy(first));case[2]['exit_code']='timeout'
        reject(case,'incomplete execution')
    B.need(results==rec['results'] and len(distinct)==rec['distinct_new_capture_rays'], 'stored results')
    return dict(executions=results,distinct_new_capture_rays=len(distinct),
                controls=controls()+['re-signed one ULP output corruption refuses exact canonical comparison',
                                     'forged eight-thread receipt refuses actual four-thread arguments',
                                     'timeout cannot become a completed reference result'],
                population_acceptance=False,gpu_execution=False)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--record',type=Path)
    p.add_argument('--archive',type=Path);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    if a.record:
        B.need(a.archive is not None,'archive required');report=replay(a.record,a.archive)
    else:report=dict(controls=controls())
    B.write(a.output,report)
    print('PASS:',len(report['controls']),'identity/exactness controls;',len(report.get('executions',{})),'stored executions')


if __name__=='__main__':main()
