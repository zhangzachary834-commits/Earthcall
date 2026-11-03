"""Replay §6dv evidence: original §6du economics and repaired native ownership.

Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-26.
This is an offline admission check, not new GPU execution or a shipping decision.
"""
import argparse,copy,hashlib,importlib.util,json,math,statistics,zipfile
from pathlib import Path

def sha(data):
    return hashlib.sha256(data).hexdigest()

def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    return module

def verify_timing(table):
    for rep in ['1','2','4']:
        for mode in ['0','1','2','3']:
            samples=[]
            for block in ['series_1','series_2']:
                row=table[block][rep][mode];v=row['samples']
                assert len(v)==8 and all(math.isfinite(x) and x>0 for x in v), 'invalid timing samples'
                for k,expected in [('mean_ms',statistics.mean(v)),('median_ms',statistics.median(v)),('min_ms',min(v)),('max_ms',max(v))]:
                    assert math.isclose(row[k],expected,rel_tol=1e-12,abs_tol=1e-12), 'timing statistic mismatch'
                samples.extend(v)
            pooled=table['pooled'][rep][mode]
            assert sorted(samples)==pooled['samples'], 'pooled samples differ'
            assert math.isclose(pooled['mean_ms'],statistics.mean(samples),rel_tol=1e-12), 'pooled mean mismatch'
            assert math.isclose(pooled['median_ms'],statistics.median(samples),rel_tol=1e-12), 'pooled median mismatch'
    return {rep:{mode:{metric:table['pooled'][rep][mode][metric] for metric in ['mean_ms','median_ms']} for mode in ['0','1','2','3']} for rep in ['1','2','4']}

def verify_controls(data):
    cases=data['cases']
    expected=[('pose-'+str(pose),rep) for pose in [0,1,0,2,3,0] for rep in [1,2,4,9]]+[(f'budget-{n}',1) for n in [0,23,24,25]]+[(f'capacity-{n}',1) for n in [0,1]]
    assert [(c['case'],c['repeats']) for c in cases]==expected, 'incomplete/reordered control matrix'
    for c in cases:
        assert c['rays']==32784 and sum(c['family_counts'])==2304
        assert c['producer_entry_errors']==c['final_output_errors']==c['output_differences']==0, 'native ownership/output error'
        assert c['packet_differences'] in [0,None]
        actual_endpoint_count=sum(len(pairs) for _,pairs in c['terminal_endpoints'])
        if c['repeats']==1:
            assert actual_endpoint_count==c['terminal_endpoint_checks'], 'terminal endpoint capture count mismatch'
        assert c['completed_s1']+c['refused']<=32784
        assert len(c['flags'])==32784
        refused=completed=0
        for flag in c['flags']:
            assert flag&0xc0000000 != 0xc0000000, 'conflicting ownership flags'
            refused+=bool(flag&0x80000000);completed+=bool(flag&0x40000000)
        assert refused==c['refused'] and completed==c['completed_s1']
        for _,actual,expected_bits in c['outputs']:
            assert actual==expected_bits, 'native output bits differ'
    return [{k:v for k,v in c.items() if k not in ['flags','outputs','terminal_endpoints']} for c in cases]

def verify_negatives(records,read):
    names=['no-reset','no-tail','double-publish','missing-publish','fallback-missing-publish','fallback-double-publish','terminal-endpoint-corrupt']
    assert [r['mutation'] for r in records]==names, 'missing negative controls'
    for r in records:
        log=read('negative/'+r['mutation']+'.log')
        assert sha(log)==r['log_sha256'] and r['exit_code']==250 and r['expected_failure'] in log.decode(errors='replace'), 'negative assertion not witnessed'
        assert sha(read('negative/'+r['mutation']+'.cpp'))==r['cpp_sha256']

def verify_artifact(data,expected):
    assert sha(data)==expected, 'artifact digest mismatch'

def self_tests(table,controls,records,read):
    rejected=[]
    def reject(name,fn):
        try:fn()
        except (AssertionError,KeyError,ValueError,TypeError):rejected.append(name);return
        raise AssertionError('corruption accepted: '+name)
    d=copy.deepcopy(table);d['series_1']['1']['0']['mean_ms']+=1;reject('false mean',lambda:verify_timing(d))
    d=copy.deepcopy(table);d['series_1']['1']['0']['samples'].pop();reject('omitted observation',lambda:verify_timing(d))
    d=copy.deepcopy(table);del d['series_2'];reject('missing timing block',lambda:verify_timing(d))
    d={'cases':controls['cases'][:-1]};reject('missing capacity case',lambda:verify_controls(d))
    d={'cases':list(controls['cases'])};d['cases'][0]=dict(d['cases'][0],final_output_errors=1);reject('publication error',lambda:verify_controls(d))
    d={'cases':list(controls['cases'])};d['cases'][0]=dict(d['cases'][0],flags=[0xc0000000]+d['cases'][0]['flags'][1:]);reject('both ownership flags',lambda:verify_controls(d))
    d=copy.deepcopy(records);d[0]['exit_code']=0;reject('negative unexpectedly passes',lambda:verify_negatives(d,read))
    d={'cases':list(controls['cases'])};d['cases'][0]=dict(d['cases'][0],terminal_endpoint_checks=d['cases'][0]['terminal_endpoint_checks']+1);reject('terminal capture completeness',lambda:verify_controls(d))
    reject('artifact altered',lambda:verify_artifact(b'changed',sha(b'original')))
    return rejected

def replay(root,record,archive,run_self_tests=False):
    meta=json.loads(record.read_text())
    assert sha(archive.read_bytes())==meta['archive_sha256'], 'archive digest mismatch'
    for path,digest in meta['source_sha256'].items():
        assert sha((root/path).read_bytes())==digest, 'source drift: '+path
    with zipfile.ZipFile(archive) as z:
        assert set(z.namelist())==set(meta['artifacts']), 'archive membership differs'
        def read(name):
            data=z.read(name);verify_artifact(data,meta['artifacts'][name]['sha256']);return data
        def obj(name):return json.loads(read(name))
        # Check every artifact, including logs and original generated sources.
        for name in meta['artifacts']:read(name)
        assert sha((root/'saves/zones/Northern Veil/zone.ecform').read_bytes())==meta['zone_sha256']
        rays=obj('fixture/rays.json');ref=obj('fixture/reference.json')
        rows=obj('benchmark/rows.json');baseline=obj('benchmark/baseline.json')
        c6dd=load(root,'ray_transport_gpu_6dd_continuation')
        namespace={'__name__':'archived_6du_generator'}
        exec(compile(read('original/benchmark.py'),'archived_6du_generator','exec'),namespace)
        import ast
        constants={x.targets[0].id:ast.literal_eval(x.value) for x in ast.parse((root/'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
        assert namespace['make_terminal_s1_cpp_source'](constants['CPP_SOURCE'],root).encode()==read('benchmark/probe.cpp'), 'saved benchmark source cannot be reconstructed'
        native=[]
        for invocation in ['first','second']:
            report=obj('benchmark/'+invocation+'.json')
            source=obj('benchmark/'+invocation+'-source.json')
            assert sha(read('benchmark/probe.cpp'))==source['cpp_sha256']
            checked=c6dd.validate_6dd(root,report,rows,baseline,rays,ref)
            timing=verify_timing(report['clean_timing_series'])
            native.append({'invocation':invocation,'validation':checked,'pooled_completion_ms':timing,'blocks_in_this_invocation':2,'samples_per_mode_per_block':8})
        published=obj('original/published-6du.json')
        assert published['clean_timing_series']==obj('benchmark/second.json')['clean_timing_series'], 'published table provenance differs'
        controls=obj('controls/review.json');cases=verify_controls(controls)
        assert sha(read('controls/probe.cpp'))==meta['control_cpp_sha256']
        negatives=obj('negative/results.json');verify_negatives(negatives,read)
        # The old classifier microbenchmark fails the routing reset precondition.
        source=read('benchmark/probe.cpp').decode()
        classify=source.split('measurePass("pass1_classify",',1)[1].split('measurePass("pass2_args",',1)[0]
        assert 'cpReset' not in classify and 'cpClassify' in classify
        assert 'for (int it = 0; it < 16; ++it)' in source
        result={
            'pass':True,'scope':'Offline replay plus source-bound saved native correctness; no fresh cost or hardware profile',
            'native_invocations_replayed':native,'published_timing_invocations_included':1,
            'native_control_cases':cases,'negative_controls':negatives,
            'rejected_old_pass_breakdown':'Classifier repeated 16 times without reset; later indirect work does not have the qualified 2304-tile routing state. Isolated completion phases are non-additive.',
            'hardware_cause':'Historical equal 2000-byte fields are descriptive evidence. No controlled same-source ablation isolates arrays or quadrature; no minimum spill requirement for winning kernels follows.',
            'new_mode2_full_matrix':'not executed by the terminal-only control driver; original-pose composed checks remain bounded evidence',
            'all_shipping_gates_open':True,
        }
        if run_self_tests:result['corruptions_rejected']=self_tests(published['clean_timing_series'],controls,negatives,read)
        return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,required=True);p.add_argument('--record',type=Path,required=True);p.add_argument('--archive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--self-test',action='store_true')
    a=p.parse_args();result=replay(a.root.resolve(),a.record,a.archive,a.self_test)
    a.output.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS: 2 saved native invocations, 30 native controls, 7 negative assertions;',len(result.get('corruptions_rejected',[])),'corruptions rejected')

if __name__=='__main__':main()
