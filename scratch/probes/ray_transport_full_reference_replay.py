"""Replay the full-workload council comparison; CPU only, no native timing claim.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
"""
import argparse,gzip,hashlib,importlib.util,json,zipfile
from pathlib import Path
def digest(b):return hashlib.sha256(b).hexdigest()
def need(ok,message):
    if not ok:raise ValueError(message)
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['root','record','archive','reference','output']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();meta=json.loads(a.record.read_text())
    need(digest(a.archive.read_bytes())==meta['archive_sha256'],'archive drift');need(digest(a.reference.read_bytes())==meta['reference_sha256'],'reference drift')
    for f,h in meta['source_sha256'].items():need(digest((a.root/f).read_bytes())==h,'source drift: '+f)
    spec=importlib.util.spec_from_file_location('full_gate',a.root/'scratch/probes/ray_transport_full_workload_accuracy.py');gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate);reference=gate.read(a.reference)
    with zipfile.ZipFile(a.archive) as z:
        def read(name):
            b=z.read(name);need(digest(b)==meta['artifacts'][name]['sha256'],'artifact drift: '+name);return b
        need(set(z.namelist())==set(meta['artifacts']),'archive membership')
        for name in meta['artifacts']:read(name)
        baseline=json.loads(gzip.decompress(read('baseline.json.gz')));summary=json.loads(read('summary.json'));actual={}
        for label,name in meta['case_artifacts'].items():actual[label]=gate.evaluate(json.loads(gzip.decompress(read(name))),baseline,reference,meta['rays_sha256'])
        need(actual==summary['results'],'full-reference metrics differ')
        need(actual['bisection']['both_resolutions_pass'] and actual['terminal_S2']['both_resolutions_pass'],'bisection/S2 failed')
        need(not actual['conservative_6eg']['both_resolutions_pass'] and actual['conservative_6eg']['finest']['rays_beyond_current_max']==316,'missed conservative regression')
        tests=gate.self_tests(reference,baseline,meta['rays_sha256'])
    a.output.write_text(json.dumps({'pass':True,'results':actual,'checker_tests':tests,'scope':'full sparse workload at one pose/clock; numerical reference only; no fresh GPU execution, scale timing or generic certification','shipping_gates_open':[1,2,3]},indent=2)+'\n');print('PASS:all32,784 rows/both resolutions; bisection/S2 pass, conservative316-ray failure preserved; checker controls pass. No GPU work.')
if __name__=='__main__':main()
