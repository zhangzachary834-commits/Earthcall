"""Capture a denser native pixel selection and independent, preselected holdout.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
No upsampling or cloned rays. Instrumented raw output, not a viewport timing.
"""
import argparse,hashlib,json,math,os,struct,subprocess
from pathlib import Path

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def ranks(n,k):return [int((i+.5)*n/k) for i in range(k)]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--work',type=Path,required=True);p.add_argument('--capture-bin',type=Path,required=True);p.add_argument('--reference-bin',type=Path,required=True);p.add_argument('--reference-only',action='store_true');a=p.parse_args()
 root=a.root.resolve();work=a.work.resolve();zone=root/'saves/zones/Northern Veil/zone.ecform';zh=sha(zone)
 if root==work or root in work.parents:raise ValueError('external work required')
 if not a.reference_only:
  work.mkdir(parents=True,exist_ok=False);capture=work/'capture.bin'
  env=dict(os.environ,EC_CAM='0,50,75,40,62,110',EC_VOLUME_SOURCE_MODE='authored',EC_CENSUS=str(capture),EC_CF4_BASELINE='1')
  for key in ['EC_FULL_LATTICE','EC_REFERENCE_SAMPLES','EC_VOLUME_OUTPUT_ERROR_BUDGET']:env.pop(key,None)
  with (work/'capture.log').open('w') as log:subprocess.run([str(a.capture_bin.resolve()),str(zone),'0123','2560','1440','7.25',str(work/'capture.rgba'),'1'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
  raw=capture.read_bytes()
  if len(raw)!=256*144*64*4:raise ValueError('wide capture size')
  u=struct.unpack('<'+'I'*(len(raw)//4),raw);f=struct.unpack('<'+'f'*(len(raw)//4),raw);rows=[];origins=[]
  for i in range(256*144):
   at=i*64
   if u[at]!=1 or u[at+3]!=1:continue
   row=[u[at+1],u[at+2],*f[at+8:at+11],f[at+7],f[at+11],*f[at+12:at+16]]+[0.]*12
   if not all(math.isfinite(x) for x in row) or row[5]>row[6]:raise ValueError('invalid capture')
   rows.append(row);origins.append(list(f[at+4:at+7]))
  if len(rows)<32784 or any(o!=origins[0] for o in origins):raise ValueError('insufficient/mixed origin native queries')
  selected=[rows[i] for i in ranks(len(rows),32784)];indices=ranks(len(selected),192)
  if len(set(tuple(x[:2]) for x in selected))!=len(selected) or len(set(tuple(x[2:7]) for x in selected))!=len(selected):raise ValueError('duplicated queries')
  rays={'scope':'Distinct instrumented native pixel-centre transport inputs; not full viewport/frame evidence','view':'wide-inside','viewport':[2560,1440],'time':7.25,'origin':origins[0],'camera':env['EC_CAM'],'capture_grid':[256,144],'native_volume_rays':len(rows),'selection':'Uniform rank selection of32784 captured native rays;192 reference indices chosen uniformly before candidate/error evaluation','capture_sha256':sha(capture),'rays':selected}
  (work/'rays.json').write_text(json.dumps(rays)+'\n');hold=dict(rays);hold['view']='wide-inside-holdout';hold['rays']=[selected[i] for i in indices];hold['wide_ray_indices']=indices;(work/'holdout-rays.json').write_text(json.dumps(hold)+'\n')
  print('CAPTURE',len(rows),'valid;32784 distinct selected;192 predetermined holdout',flush=True)
 rays=json.loads((work/'rays.json').read_text());hold=json.loads((work/'holdout-rays.json').read_text());out=work/'reference-raw.json'
 if out.exists():raise ValueError('existing reference output')
 with (work/'reference.log').open('w') as log:subprocess.run([str(a.reference_bin.resolve()),str(work/'holdout-rays.json'),'16','192','8',str(out)],cwd=root,stdout=log,stderr=subprocess.STDOUT,check=True)
 rawref=json.loads(out.read_text());rows=rawref['rows']
 if len(rows)!=192 or any(len(x)!=8 or not all(math.isfinite(v) for v in x) for x in rows):raise ValueError('reference coverage')
 ref={'scope':'Original-AST independent Simpson holdout only; no dense reference for other wide rays','finest_intervals':65536,'half_intervals':32768,'ray_indices':hold['wide_ray_indices'],'rows':[[*hold['rays'][i][:2],*row] for i,row in enumerate(rows)]};(work/'reference.json').write_text(json.dumps(ref)+'\n')
 manifest={'case':'wide-inside','zone_sha256':zh,'rays':str(work/'rays.json'),'rays_sha256':sha(work/'rays.json'),'reference':str(work/'reference.json'),'reference_sha256':sha(work/'reference.json'),'capture_binary_sha256':sha(a.capture_bin.resolve()),'reference_binary_sha256':sha(a.reference_bin.resolve()),'reference_scope':'192 predetermined indices of32784 unique native rays'};(work/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 if sha(zone)!=zh:raise ValueError('source changed')
 print('REFERENCE complete',flush=True)
if __name__=='__main__':main()
