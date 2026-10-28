"""Replay 6em native evidence without GPU execution.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
Checks stored observations; does not repeat native publication/invalidation execution.
"""
import argparse,gzip,hashlib,importlib.util,json,math,re,statistics,zipfile
from pathlib import Path


def replay(root,record,archive):
    need=lambda v,msg: (_ for _ in ()).throw(ValueError(msg)) if not v else None
    index=json.loads(record.read_text())
    need(hashlib.sha256(archive.read_bytes()).hexdigest()==index['archive_sha256'],'archive drift')
    spec=importlib.util.spec_from_file_location('gate',root/'scratch/probes/ray_transport_full_workload_accuracy.py');gate=importlib.util.module_from_spec(spec);spec.loader.exec_module(gate)
    reference=gate.read(root/'docs/audits/NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27_ROWS.json.gz')
    need(gate.digest(root/'docs/audits/NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27_ROWS.json.gz')==index['reference_sha256'],'reference drift')
    with zipfile.ZipFile(archive) as z:
        for name,sha in index['members_sha256'].items():need(hashlib.sha256(z.read(name)).hexdigest()==sha,'member drift '+name)
        read=lambda name:json.loads(z.read(name))
        native=read('native/index.json');rays=read('inputs/rays.json');rays_sha=hashlib.sha256(z.read('inputs/rays.json')).hexdigest();need(rays_sha==native['rays_sha256'],'ray identity')
        positive=read('native/positive.json')['cases'][0]
        need(positive['refused']==10068 and len(positive['outputs'])==len(positive['flags'])==32784,'positive population')
        for i,(row,flag) in enumerate(zip(positive['outputs'],positive['flags'])):
            need(row[0]==i and row[1]==row[2],'positive Current/bisection comparator equality')
            for word in row[1]:need(math.isfinite(gate.decode(word)),'nonfinite positive')
        need(sum(bool(f&0x80000000) for f in positive['flags'])==10068,'positive refused population')
        need(all(positive[x]==0 for x in ['producer_entry_errors','final_output_errors','output_differences']),'positive ownership')
        for key,name in [('partial_negative','partial-negative.log'),('missing_publication_negative','missing-publication-negative.log')]:
            neg=native[key];need(neg['returncode']!=0 and neg['expected_error'] in z.read('native/'+name).decode(),'targeted rejection')
        comparisons=0;all_accuracy={};summary={}
        for style in (1,0):
            summary[style]={}
            for K in (1,2,4,8,16):
                d=read(f'native/clean-w{style}-K{K}.json')
                need(d['preflight_poisoned'] and d['preflight_parity_pass'] and d['rays']==32784*K,'clean preflight scope')
                need(len(d['preflight_checks'])==4 and all(x['finite'] and x['differing_rays']==0 for x in d['preflight_checks']),'clean actual parity')
                medians={};series_ratios={}
                for mode in range(6):
                    vals=[x for series in d['clean_timing_ordered_samples'].values() for x in series if x['mode']==mode]
                    need(len(vals)==16,'timing count')
                    for field in ('encode','completion','total'):
                        v=[x[field+'_ms'] for x in vals];need(all(math.isfinite(x) and x>=0 for x in v),'invalid clock')
                        stored=d['clean_timing_series']['pooled'][str(mode)][field]
                        need(abs(statistics.median(v)-stored['median_ms'])<1e-9,'median drift')
                        need(abs(statistics.mean(v)-stored['mean_ms'])<1e-9,'mean drift');comparisons+=1
                    medians[mode]=statistics.median(x['total_ms'] for x in vals)
                for series,vals in d['clean_timing_ordered_samples'].items():
                    med=lambda mode:statistics.median(x['total_ms'] for x in vals if x['mode']==mode)
                    series_ratios[series]={'routed_vs_Current':med(2)/med(0),'indirect_vs_Current':med(5)/med(0),'indirect_vs_routed':med(5)/med(2)}
                summary[style][K]={'total_median_ms':medians,'routed_vs_Current':medians[2]/medians[0],'indirect_vs_Current':medians[5]/medians[0],'indirect_vs_routed':medians[5]/medians[2],'series_ratios':series_ratios}
                if K==1:
                    actual={m:json.loads(gzip.decompress(z.read(name))) for m,name in d['preflight_actual_rows_files'].items()}
                    norm={m:[[ *rays['rays'][i][:2],*row[1:]] for i,row in enumerate(values)] for m,values in actual.items()}
                    for mode,rows in norm.items():
                        q=gate.evaluate(rows,norm['0'],reference,rays_sha);need(q['both_resolutions_pass'],'native accuracy '+mode);all_accuracy[f'w{style}-m{mode}']=q
        clean=z.read('native/clean.cpp').decode();old=z.read('source/6el-original.cpp').decode()
        shaders=lambda s:re.findall(r'R"WGSL\((.*?)\)WGSL"',s,re.S)
        need(shaders(clean)==shaders(old),'shader arithmetic changed')
        need('auditWritesBuf' not in clean and 'wgpuCommandEncoderClearBuffer' not in clean,'diagnostics in clean source')
        return {'scope':'stored native evidence replay only; no new GPU work; all three shipping gates OPEN','accuracy':all_accuracy,'summary':summary,'timing_stat_pairs_verified':comparisons,'positive_rows':32784,'refused_rows':10068,'negative_controls_verified':2,'shader_literals_identical':True}


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,default=Path.cwd());p.add_argument('--record',type=Path,required=True);p.add_argument('--archive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    d=replay(a.root.resolve(),a.record,a.archive);a.output.write_text(json.dumps(d,indent=2)+'\n');print('PASS: complete native actual rows/both resolutions; 180 timing mean/median pairs; positive refusal and two targeted negatives; shader arithmetic unchanged. Stored-evidence replay only.')

if __name__=='__main__':main()
