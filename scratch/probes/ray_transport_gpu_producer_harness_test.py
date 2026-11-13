"""GPU diagnostic runner negative controls; no GPU invocation or live-save writes.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
"""
import argparse,ast,copy,json,subprocess,sys,tempfile
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args();root=a.root.resolve()
    if a.output.exists():raise SystemExit('refusing output overwrite')
    probes=root/'scratch/probes';sys.path.insert(0,str(probes))
    import ray_transport_gpu_producer_repaired as runner
    source_path=probes/'ray_transport_metal_diagnostic.py'
    v={x.targets[0].id:ast.literal_eval(x.value) for x in ast.parse(source_path.read_text()).body
       if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
    source=runner.source(v['CPP_SOURCE'],root)
    work=Path(tempfile.mkdtemp(prefix='earthcall-gpu-negative-',dir='/private/tmp'));checks=[]
    def refuse(label,target,where,*options):
        result=subprocess.run([sys.executable,str(probes/'ray_transport_gpu_producer_repaired.py'),
            '--root',str(target),'--work',str(where),*options],capture_output=True,text=True)
        if result.returncode==0:raise AssertionError(label+' incorrectly succeeded')
        checks.append({'case':label,'refused':True,'exit_code':result.returncode})
        return result.stderr+result.stdout
    existing=work/'existing';existing.mkdir();(existing/'marker').write_text('keep')
    refuse('existing directory',root,existing);assert (existing/'marker').read_text()=='keep'
    refuse('in-repository directory',root,root/'scratch/probes/would-build')
    for label,opts in [('negative budget',['--budget','-1']),('zero sub capacity',['--sub-cap','0']),
        ('huge overlap capacity',['--over-cap','65']),('NaN margin',['--margin','nan']),
        ('negative margin',['--margin','-1']),('unsafe label',['--label','../elsewhere'])]:
        refuse(label,root,work/'invalid',*opts)
    replay=work/'replay';replay.mkdir();(replay/'probe.cpp').write_text('different source')
    assert 'source mismatch' in refuse('source mismatch',root,replay,'--run-only')
    replay.joinpath('probe.cpp').write_text(source);fake=replay/'probe'
    fake.write_text('#!/bin/sh\nexit 7\n');fake.chmod(0o755)
    assert 'returned non-zero exit status 7' in refuse('failing native child',root,replay,'--run-only')
    fake.write_text('#!/bin/sh\nexit 0\n')
    refuse('missing report after zero exit',root,replay,'--run-only')
    good={'qualification':{k:True for k in runner.QUALIFICATION_KEYS}}
    good['qualification']['shipping_performance_qualified']=False
    runner.validate_report(good)
    for label,key,value in [('false AST','independent_AST_agreement_pass',False),
        ('false fallback','fallback_native_baseline_identical',False),
        ('false write completeness','poisoned_connected_output_fully_written',False),
        ('truthy string','mean_p99_max_reference_gates_pass','false'),
        ('truthy integer','device_fused_unfused_parity_pass',1),
        ('unearned shipping claim','shipping_performance_qualified',True)]:
        d=copy.deepcopy(good);d['qualification'][key]=value
        fake.write_text('#!'+sys.executable+'\nimport sys\nfrom pathlib import Path\nPath(sys.argv[1]).write_text('+repr(json.dumps(d))+')\n')
        outcome=refuse(label,root,replay,'--run-only','--label',label.replace(' ','-'))
        expected='scratch result must not claim shipping' if key=='shipping_performance_qualified' else 'qualification failed'
        assert expected in outcome
    fake_root=work/'edited-fixture';(fake_root/'scratch/probes').mkdir(parents=True)
    (fake_root/'scratch/probes/ray_transport_metal_diagnostic.py').write_text(source_path.read_text())
    for rel in ['docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json',
                'docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json']:
        path=fake_root/rel;path.parent.mkdir(parents=True,exist_ok=True);path.symlink_to(root/rel)
    zone=fake_root/'saves/zones/Northern Veil/zone.ecform';zone.parent.mkdir(parents=True)
    zone.write_bytes((root/'saves/zones/Northern Veil/zone.ecform').read_bytes()+b'edited-fixture')
    assert 'Pinned fixture changed' in refuse('authored fixture edit',fake_root,work/'would-build')
    assert not (work/'would-build').exists()
    prior=replay/'prior.json';prior.write_text('keep')
    assert 'refusing output overwrite' in refuse('existing report',root,replay,'--run-only','--label','prior')
    assert prior.read_text()=='keep'
    a.output.write_text(json.dumps({'scope':__doc__,'checks':checks,'pass':True},indent=2)+'\n')
    print('PASS',len(checks),'negative controls;',a.output)

if __name__=='__main__':main()
