"""Compare actual native polynomial/control outputs with the independent rows.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
Finite numerical evidence, not universal error admission or timing.
"""
import argparse,hashlib,json,struct,statistics
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);args=p.parse_args();root=args.root;out=args.output
inputs=json.loads((root/'docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json').read_text())
assert hashlib.sha256((root/inputs['fixture']).read_bytes()).hexdigest()==inputs['fixture_sha256']
reference=json.loads((root/'docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json').read_text())['rows']
f=lambda x:struct.unpack('<f',struct.pack('<I',x))[0]
def stats(v):
 v=sorted(v);return dict(mean=statistics.mean(v),p99=v[len(v)*99//100],max=max(v))
rows={};result={}
for mode in ['baseline','polynomial']:
 rows[mode]=list(struct.iter_unpack('<64I',(out/(mode+'.bin')).read_bytes()));assert len(rows[mode])==len(reference)==2304
 for r,q in zip(rows[mode],reference):assert [r[1],r[2]]==q[:2]
 result[mode]={'T':stats([abs(f(r[12])-q[2]) for r,q in zip(rows[mode],reference)]),'RGB':stats([abs(f(r[13+k])-q[3+k]) for r,q in zip(rows[mode],reference) for k in range(3)])}
for a,b in zip(rows['baseline'],rows['polynomial']):assert a[4:12]==b[4:12]
for a,r in zip(rows['baseline'],inputs['rays']):assert [f(a[12+k]) for k in range(4)]==r[7:11]
result['candidate_minus_control']={'T':stats([abs(f(a[12])-f(b[12])) for a,b in zip(rows['baseline'],rows['polynomial'])]),'RGB':stats([abs(f(a[13+k])-f(b[13+k])) for a,b in zip(rows['baseline'],rows['polynomial']) for k in range(3)])}
result['reference_finest_half_spread']={'T':stats([abs(q[2]-q[6]) for q in reference]),'RGB':stats([abs(q[3+k]-q[7+k]) for q in reference for k in range(3)])}
regressions=[]
for a,b,q in zip(rows['baseline'],rows['polynomial'],reference):
 old=[abs(f(a[12+k])-q[2+k]) for k in range(4)];new=[abs(f(b[12+k])-q[2+k]) for k in range(4)];regressions.append(dict(pixel=q[:2],T_delta_error=new[0]-old[0],RGB_max_delta_error=max(new[1:])-max(old[1:]),baseline=[f(a[12+k]) for k in range(4)],candidate=[f(b[12+k]) for k in range(4)],reference=q[2:6]))
result['largest_new_RGB_regressions']=sorted(regressions,key=lambda r:r['RGB_max_delta_error'],reverse=True)[:12]
result['largest_new_T_regressions']=sorted(regressions,key=lambda r:r['T_delta_error'],reverse=True)[:12]
r=rows['polynomial'];s=[sum(x[20+k] for x in r) for k in range(4)]
result['polynomial_calls']={'calls':s[0],'hits':s[1],'preparations':s[2],'fallback':s[3],'domain_resets':sum(x[56] for x in r)}
(out/'accuracy.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if not k.startswith('largest')},indent=2))
