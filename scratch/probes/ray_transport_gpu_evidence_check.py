"""Independently recompute the six saved numerical gates from raw native rows.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Finite frozen-ray evidence only; this does not certify arbitrary native omissions.
"""
import argparse,gzip,json,math
from pathlib import Path

def read(path):
    data=path.read_bytes()
    if path.suffix=='.gz':data=gzip.decompress(data)
    return json.loads(data)

def three(values):
    if not values or not all(math.isfinite(x) for x in values):raise ValueError('nonfinite/empty errors')
    s=sorted(values)
    return {'mean':sum(values)/len(values),'p99':s[len(s)*99//100],'max':s[-1]}

def check(root,report,rows):
    r=read(root/'docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json')['rays']
    ref=read(root/'docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json')['rows']
    if len(rows)!=len(r) or len(rows)!=len(ref):raise ValueError('row count')
    e={'native_baseline':{'RGB_pooled_channels':[],'T':[]},'unfused_A_star_on_device':{'RGB_pooled_channels':[],'T':[]}}
    ast_rgb=ast_t=0.0
    for a,b,c in zip(rows,r,ref):
        if a[:2]!=b[:2] or a[:2]!=c[:2]:raise ValueError('pixel order')
        if not all(math.isfinite(float(x)) for x in a):raise ValueError('nonfinite native row')
        for name,color,t in [('native_baseline',b[8:11],b[7]),('unfused_A_star_on_device',a[2:5],a[5])]:
            e[name]['RGB_pooled_channels'] += [abs(color[k]-c[3+k]) for k in range(3)]
            e[name]['T'].append(abs(t-c[2]))
        ast_rgb=max(ast_rgb,max(abs(a[2+k]-a[10+k]) for k in range(3)))
        ast_t=max(ast_t,abs(a[5]-a[13]))
    result={name:{ch:three(v) for ch,v in channels.items()} for name,channels in e.items()}
    saved=report['accuracy_vs_65536_step_reference']
    for name,channels in result.items():
        for ch,metrics in channels.items():
            for metric,value in metrics.items():
                if not math.isclose(value,saved[name][ch][metric],rel_tol=1e-10,abs_tol=1e-15):raise ValueError(('saved metric mismatch',name,ch,metric,value,saved[name][ch][metric]))
    gates=all(result['unfused_A_star_on_device'][ch][metric]<=result['native_baseline'][ch][metric]
        for ch in ['RGB_pooled_channels','T'] for metric in ['mean','p99','max'])
    if not gates or ast_rgb>=1e-5 or ast_t>=1e-5:raise ValueError('raw numerical gate failed')
    return {'rows':len(rows),'recomputed_metrics':result,'max_AST_RGB_delta':ast_rgb,'max_AST_T_delta':ast_t,'all_six_gates_pass':gates}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--rows',type=Path,required=True)
    a=p.parse_args();print(json.dumps(check(a.root.resolve(),read(a.report),read(a.rows)),indent=2))

if __name__=='__main__':main()
