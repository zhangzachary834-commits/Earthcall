"""Negative controls for the corrected scratch runner: never turn refusal into success.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
No compilation, GPU invocation or live-save mutation in these wrapper checks.
"""
import argparse
import ast
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args();root=a.root.resolve()
    if a.output.exists():raise SystemExit('refusing output overwrite')
    probes=root/'scratch/probes';sys.path.insert(0,str(probes))
    import ray_transport_canonical_producer_metal as runner
    source_path=probes/'ray_transport_metal_diagnostic.py'
    v={x.targets[0].id:ast.literal_eval(x.value) for x in ast.parse(source_path.read_text()).body
       if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
    source=runner.build_canonical_probe_source(v['CPP_SOURCE'],root)
    work=Path(tempfile.mkdtemp(prefix='earthcall-6cf-negative-',dir='/private/tmp'))
    checks=[]
    def invoke(label,target,where,*options):
        result=subprocess.run([sys.executable,str(probes/'ray_transport_canonical_producer_metal.py'),
            '--root',str(target),'--work',str(where),*options],capture_output=True,text=True)
        if result.returncode==0:raise AssertionError(label+' incorrectly succeeded')
        checks.append({'case':label,'refused':True,'exit_code':result.returncode})
        return result.stderr+result.stdout
    existing=work/'existing';existing.mkdir();(existing/'marker').write_text('keep')
    invoke('existing work directory',root,existing)
    assert (existing/'marker').read_text()=='keep'
    replay=work/'replay';replay.mkdir();(replay/'probe.cpp').write_text('different source')
    assert 'replay source differs' in invoke('source mismatch',root,replay,'--run-only')
    invoke('invalid repeat',root,work/'invalid','--repeat','0')
    invoke('unsafe label',root,work/'invalid-label','--label','../elsewhere')
    replay.joinpath('probe.cpp').write_text(source)
    fake=replay/'probe';fake.write_text('#!/bin/sh\nexit 7\n');fake.chmod(0o755)
    assert 'returned non-zero exit status 7' in invoke('failing child',root,replay,'--run-only')
    assert not (replay/'series1.json').exists()
    report={'qualification':{'closed_form_witnesses_pass':True,'mean_p99_max_reference_gates_pass':True,
                            'independent_AST_agreement_pass':False,'device_fused_unfused_parity_pass':True}}
    fake.write_text('#!'+sys.executable+'\nimport json,sys\nfrom pathlib import Path\nPath(sys.argv[1]).write_text('+repr(json.dumps(report))+')\n')
    assert 'qualification failed' in invoke('failed AST qualification despite zero child exit',root,replay,'--run-only','--label','bad-quality')
    fake_root=work/'edited-fixture';(fake_root/'scratch/probes').mkdir(parents=True)
    (fake_root/'scratch/probes/ray_transport_metal_diagnostic.py').write_text(source_path.read_text())
    for rel in ['docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json',
                'docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json']:
        path=fake_root/rel;path.parent.mkdir(parents=True,exist_ok=True);path.symlink_to(root/rel)
    zone=fake_root/'saves/zones/Northern Veil/zone.ecform';zone.parent.mkdir(parents=True)
    zone.write_bytes((root/'saves/zones/Northern Veil/zone.ecform').read_bytes()+b'edited-fixture')
    assert 'Pinned fixture changed' in invoke('authored fixture edit',fake_root,work/'would-build')
    assert not (work/'would-build').exists()
    a.output.write_text(json.dumps({'scope':__doc__,'checks':checks,'pass':True},indent=2)+'\n')
    print('PASS',len(checks),'negative harness controls;',a.output)


if __name__=='__main__':main()
