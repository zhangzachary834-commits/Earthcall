"""§6cu: lossless JSON-value bundle replay of root/sweep review evidence.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Standard library; independently uses the portable reference/structure verifier.
"""
import argparse,copy,hashlib,importlib.util,json,zipfile
from pathlib import Path


def canonical(v):return json.dumps(v,sort_keys=True,separators=(',',':'),ensure_ascii=True).encode()
def sha(b):return hashlib.sha256(b).hexdigest()
def helper(root):
 p=root/'scratch/probes/ray_transport_sliver_review.py';spec=importlib.util.spec_from_file_location('portable_sliver_review',p)
 m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


class Bundle:
 def __init__(self,path,expected):
  if sha(path.read_bytes())!=expected:raise ValueError('bundle SHA256')
  self.zip=zipfile.ZipFile(path);self.index=json.loads(self.zip.read('index.json'));self.cache={}
 def read(self,name):
  entry=self.index[name]
  def decode(v):
   if isinstance(v,dict) and set(v)=={'$array'}:
    key=v['$array']
    if key not in self.cache:
     data=self.zip.read('arrays/'+key+'.json')
     if sha(data)!=key:raise ValueError('array SHA256')
     self.cache[key]=json.loads(data)
    return self.cache[key]
   if isinstance(v,dict):return {k:decode(x) for k,x in v.items()}
   if isinstance(v,list):return [decode(x) for x in v]
   return v
  if entry['kind']=='json':
   value=decode(entry['value'])
   if sha(canonical(value))!=entry['canonical_sha256']:raise ValueError('JSON-value identity')
   return value
  data=self.zip.read('text/'+entry['sha256'])
  if sha(data)!=entry['sha256']:raise ValueError('source identity')
  return data


def packets_equal(a,b,m):
 if a[:3]!=b[:3] or len(a[3])!=len(b[3]) or len(a[4])!=len(b[4]):return False
 return all(x[2]==y[2] and m.bits(x[:2])==m.bits(y[:2]) for x,y in zip(a[3]+a[4],b[3]+b[4]))


def qualify(root,report,rows,baseline,rays,ref,before,before_rows):
 m=helper(root);result=m.check(root,report,rows,baseline,rays,ref['rows'],ref['ray_indices'])
 a,b=before['schedule_census_inputs'],report['schedule_census_inputs']
 if len(a)!=len(b) or any(x[:2]!=y[:2] for x,y in zip(a,b)):raise ValueError('source structure alignment')
 result['affine']=m.affine_endpoints(a,b,rays,before['guarded_sliver_policy']['curtain_bindings'])
 result['ray_member_unions_changed']=sum(x!=y for a,b in zip(m.union(a),m.union(b)) for x,y in zip(a,b))
 result['schedule_rays_changed']=sum(not packets_equal(x,y,m) for x,y in zip(a,b))
 result['output_rays_changed']=sum(m.bits(x[2:6])!=m.bits(y[2:6]) for x,y in zip(before_rows,rows))
 result['structural_admitted']=result['affine']['missing']==0 and result['ray_member_unions_changed']==0
 return result


def budget_check(data):
 for c in data['cases']:
  expected=16 if c['budget']<24 else 0
  before=16 if c['budget']<=24 else 0
  if c['expected_original_refusal']!=expected or c['refused_rays']!=(before if c['variant']=='before' else expected):raise ValueError('budget regression witness')
 if len(data['cases'])!=9:raise ValueError('budget case coverage')


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--record',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 if a.output.exists():raise SystemExit('refusing output overwrite')
 record=json.loads(a.record.read_text());bundle=Bundle(a.root/record['bundle'],record['bundle_sha256'])
 # Validate every archived source/JSON document even when a stage does not consume it.
 for key in bundle.index:bundle.read(key)
 before=bundle.read('baseline_report');before_rows=bundle.read('baseline_rows');rays=bundle.read('rays');ref=bundle.read('reference');m=helper(a.root);results={}
 for name in record['stages']:
  report=bundle.read(name+'_report');rows=bundle.read(name+'_rows');baseline=bundle.read(name+'_baseline')
  r=qualify(a.root,report,rows,baseline,rays,ref,before,before_rows)
  if name in ['merge_first','merge_confirm','subcache','fixed'] and (not r['structural_admitted'] or r['schedule_rays_changed'] or r['output_rays_changed']):raise ValueError('strict source parity')
  if name=='secant_all' and (r['structural_admitted'] or r['affine']['missing']!=6 or r['ray_member_unions_changed']!=47084):raise ValueError('all-level refusal not reproduced')
  if name=='secant_zero' and not r['structural_admitted']:raise ValueError('zero-level structural admission')
  results[name]=r
 budget=bundle.read('budget_controls');budget_check(budget)
 fixed=bundle.read('fixed_report');rows=bundle.read('fixed_rows');baseline=bundle.read('fixed_baseline')
 refusals=m.controls(a.root,fixed,rows,baseline,rays,ref['rows'],ref['ray_indices'])
 bad=copy.deepcopy(budget);bad['cases'][-2]['refused_rays']=16
 try:budget_check(bad)
 except ValueError:refusals.append('exact-budget fixed result corrupted')
 else:raise ValueError('corrupted budget witness accepted')
 mutated=copy.deepcopy(fixed);mutated['schedule_census_inputs'][0][3][0][0]+=.01
 r=qualify(a.root,mutated,rows,baseline,rays,ref,before,before_rows)
 if r['structural_admitted']:raise ValueError('support change accepted')
 refusals.append('changed support union refused')
 result={'pass':True,'stages':results,'budget_controls':budget,'corruption_refusals':refusals,'verified_documents':len(bundle.index),
  'limits':'JSON values losslessly reconstructed; original report whitespace is not archived. Native AST1e-5 assertions recorded, not re-evaluated offline. Dense reference192 rays at two resolutions; no frame/FPS admission.'}
 a.output.write_text(json.dumps(result,indent=2)+'\n');print('PASS root review bundle',len(bundle.index),'documents;',len(refusals),'corruption refusals',a.output)


if __name__=='__main__':main()
