"""§6dy: independently replay §6dx proof, native controls and compiler/cost evidence.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-26.
No native GPU run or timing occurs here.
"""
import argparse,copy,hashlib,importlib.util,json,math,re,subprocess,sys,zipfile
from pathlib import Path

def sha(data):return hashlib.sha256(data).hexdigest()
def load(root,name):
    s=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m

def require_spill(rows,value):
    if len(rows)!=2 or any(r['spilled_bytes']!=value for r in rows):raise ValueError('compiler claim differs from native tables')

def replay(root,index,archive,donor_path):
    record=json.loads(index.read_text())
    assert sha(archive.read_bytes())==record['archive_sha256']
    assert sha(donor_path.read_bytes())==record['donor_archive_sha256']
    for path,digest in record['source_sha256'].items():assert sha((root/path).read_bytes())==digest,'source drift: '+path
    dx=load(root,'ray_transport_gpu_6dx_stack_bound');dw=load(root,'ray_transport_gpu_6dw_review_replay');dv=load(root,'ray_transport_gpu_6dv_review_replay')
    models={'baseline':dx.prove_stack_bound(),'cutoff15':dx.prove_stack_bound(max_depth=15),'seed3':dx.prove_stack_bound(seed_count=3)}
    assert models['baseline']==(15,32767,49148) and models['cutoff15'][0]==models['seed3'][0]==16
    source_mutations=dx.verify_proof_sources(root);assert len(source_mutations)==14
    optimized=subprocess.run([sys.executable,'-O','-B',str(root/'scratch/probes/ray_transport_gpu_6dx_stack_bound.py'),'--root',str(root),'--prove-only'],check=True,capture_output=True,text=True)
    assert '14 emitted-source mutations rejected' in optimized.stdout
    with zipfile.ZipFile(archive) as z,zipfile.ZipFile(donor_path) as donor:
        assert set(z.namelist())==set(record['artifacts'])
        def read(name):
            data=z.read(name);assert sha(data)==record['artifacts'][name]['sha256'],'artifact drift: '+name;return data
        for name in record['artifacts']:read(name)
        witness=json.loads(read('counsel/intrinsic-monotonicity-counterexample.json'))
        expected=[math.sin(u)-u/2 for u in witness['u']]
        assert expected==witness['phi_values']
        assert expected[0]<witness['level'] and expected[2]<witness['level']<expected[1]
        assert math.isclose(witness['normalized_ray_dx']**2+witness['normalized_ray_dz']**2,1.0,abs_tol=1e-15)
        old_namespace={'__name__':'archived_6dx'}
        exec(compile(read('original/ray_transport_gpu_6dx_stack_bound.py'),'archived_6dx','exec'),old_namespace)
        counterexamples=[]
        original_shaders=dx.load(root,'ray_transport_gpu_6du_true_terminal_s1').make_terminal_s1_shaders(root)
        for shader_name in ['kS1UnfusedProducerWgsl','kS1TerminalProducerWgsl']:
            shader=original_shaders[shader_name]
            for mutation,changed in {
                'missing-pop':shader.replace('top--; let cell = stack[top];','let cell = stack[top];'),
                'ineffective-cutoff':shader.replace('if (f32(depth) >= 14.0 ||','if ((false && f32(depth) >= 14.0) ||'),
                'seed-comment-decoy':shader.replace('top = 2u;','top = 3u; // top = 2u;'),
            }.items():
                old_namespace['verify_source_matches_proof'](changed)
                try:dx.verify_source_matches_proof(changed)
                except ValueError:counterexamples.append({'shader':shader_name,'mutation':mutation,'old_guard_admitted':True,'new_guard_refused':True})
                else:raise AssertionError('repair admits known source counterexample')
        for entry in record['native_sources']:
            variant=entry['variant'];kind=entry['kind'];expected=dx.controls_cpp(root,variant) if kind=='controls' else dx.clean_cpp(root,variant)
            if kind=='capture':expected=dx.capture_cpp(expected,Path(entry['original_work']))
            assert expected.encode()==read(entry['artifact']),'native source reconstruction differs'
        controls=json.loads(dw.restore_controls(donor.read('controls/review.json'),json.loads(read('controls-delta.json'))))
        assert len(controls['cases'])==60
        checked_controls={str(mode):dv.verify_controls({'cases':[c for c in controls['cases'] if c['mode']==mode]}) for mode in [2,3]}
        timings={}
        for variant in ['canonical','bound-stack-15']:
            report=json.loads(dw.restore_timing(donor.read('benchmark/second.json'),json.loads(read('timing-'+variant+'-delta.json'))))
            assert sha(donor.read('benchmark/rows.json'))==record['timing_rows_sha256'][variant]
            assert sha(donor.read('benchmark/baseline.json'))==record['timing_baseline_sha256'][variant]
            accuracy=load(root,'ray_transport_gpu_6dd_continuation').validate_6dd(root,report,json.loads(donor.read('benchmark/rows.json')),json.loads(donor.read('benchmark/baseline.json')),json.loads(donor.read('fixture/rays.json')),json.loads(donor.read('fixture/reference.json')))
            dv.verify_timing(report['clean_timing_series'])
            comparisons={}
            for rep,t in report['clean_timing_series']['pooled'].items():
                comparisons[rep]={mode:{'mean_percent_vs_current':100*(t[mode]['mean_ms']/t['0']['mean_ms']-1),'median_percent_vs_current':100*(t[mode]['median_ms']/t['0']['median_ms']-1)} for mode in ['2','3']}
            timings[variant]={'accuracy':accuracy,'pooled':report['clean_timing_series']['pooled'],'comparisons':comparisons,'native_invocations':1,'internal_blocks':2,'observations_per_mode_per_block':8}
        rows=[]
        for name in record['artifacts']:
            if name.startswith('tables/'):
                row=dw.decode_table(read(name))
                if row:rows.append(dict(row,variant=name.split('/')[1],file=name.split('/')[-1]))
        s1={v:[r for r in rows if r['variant']==v and r['function']=='s1ProducerMain'] for v in ['canonical','bound-stack-15']}
        require_spill(s1['canonical'],2000);require_spill(s1['bound-stack-15'],1760)
        assert sorted((r['temporary_registers'],r['uniform_registers'],r['instructions']) for r in s1['canonical'])==[(79,72,5153),(83,88,6435)]
        assert sorted((r['temporary_registers'],r['uniform_registers'],r['instructions']) for r in s1['bound-stack-15'])==[(79,72,5093),(83,88,6375)]
        for variant in ['canonical','bound-stack-15']:
            facts=[json.loads(line) for line in read('capture/'+variant+'-facts.jsonl').decode().splitlines()]
            expected={sha(shader.encode()) for name,shader in re.findall(r'static const char\* (\w+) = R"WGSL\((.*?)\)WGSL";',dx.clean_cpp(root,variant),re.S) if name in ['kS1UnfusedProducerWgsl','kS1TerminalProducerWgsl']}
            assert expected=={r['wgslSHA256'] for r in facts if r.get('wgslEntry')=='s1ProducerMain'}
            device=next(r for r in facts if r.get('kind')=='device');assert device['name']=='Apple M5' and not device['timestampSamplingRequested'] and not device['counterSamplingRequested']
        corruptions=[]
        def refuses(name,fn):
            try:fn()
            except (AssertionError,ValueError,KeyError):corruptions.append(name);return
            raise AssertionError('corruption admitted: '+name)
        bad=copy.deepcopy(report['clean_timing_series']);bad['pooled']['1']['0']['mean_ms']+=1
        refuses('false pooled timing mean',lambda:dv.verify_timing(bad))
        refuses('false zero-spill claim',lambda:require_spill(s1['bound-stack-15'],0))
        delta=json.loads(read('controls-delta.json'));delta['native_sha256']='0'*64
        refuses('changed native control digest',lambda:dw.restore_controls(donor.read('controls/review.json'),delta))
        return {'pass':True,'scope':'CPU proof and source-bound replay of saved native evidence; no fresh GPU run','models':models,'intrinsic_wave_monotonicity_counterexample':witness,'original_guard_counterexamples':counterexamples,'source_mutations_rejected':source_mutations,'python_optimized_guard_passes':True,'native_case_summaries':checked_controls,'compiler_s1_rows':s1,'timing_invocations':timings,'corruptions_rejected':corruptions,'native_negative_variants_not_rerun':True,'performance_retained':False,'all_shipping_gates_open':True}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--record',type=Path,required=True);p.add_argument('--archive',type=Path,required=True);p.add_argument('--donor-archive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    result=replay(a.root.resolve(),a.record,a.archive,a.donor_archive);a.output.write_text(json.dumps(result,indent=2)+'\n');print('PASS: finite bound, 14 source mutations, Python -O, 60 native cases, both saved timing invocations, compiler tables and 3 corruption refusals')

if __name__=='__main__':main()
