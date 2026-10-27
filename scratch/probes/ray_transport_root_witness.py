"""§6ee operation-rounded HOST oracle witnesses, not Metal execution proof.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
The SBI strategy is unchanged. Every host step is rounded to f32; callbacks
have explicit ideal/observed roles. Known nearest-rounded band endpoints can
miss a real root: this suite exposes that shared baseline/candidate gap rather
than hiding it behind a1e-4 acceptance padding. No production code is changed.
"""
import argparse,json,math,struct
from pathlib import Path

def to_f32(x):
    try:return struct.unpack('<f',struct.pack('<f',float(x)))[0]
    except OverflowError:return math.copysign(math.inf,x)
def next_f32(x,up=True):
    x=to_f32(x)
    if math.isnan(x):raise ValueError('NaN has no ordered adjacent float')
    if x==0:return struct.unpack('<f',struct.pack('<I',1 if up else 0x80000001))[0]
    if x==(math.inf if up else -math.inf):return x
    b=struct.unpack('<I',struct.pack('<f',x))[0];b+=(1 if up else -1)*(1 if x>0 else -1)
    return struct.unpack('<f',struct.pack('<I',b))[0]
def add(a,b):return to_f32(a+b)
def sub(a,b):return to_f32(a-b)
def mul(a,b):return to_f32(a*b)
def div(a,b):return to_f32(a/b)

def isolate(kind,f,a,b,va,vb,error,min_slope,max_budget=32):
    a,b,va,vb,error,min_slope=map(to_f32,[a,b,va,vb,error,min_slope])
    if not all(math.isfinite(x) for x in [a,b,va,vb,error,min_slope]) or not a<b or error<0 or min_slope<=0:raise ValueError('invalid finite host-oracle contract')
    l,r,fl,fr=a,b,va,vb;bands=[];work=evals=stuck=last_side=0;reason=0
    if abs(va)<=error:bands.append((a,min(b,add(a,div(mul(2,error),min_slope)))))
    if abs(vb)<=error:bands.append((max(a,sub(b,div(mul(2,error),min_slope))),b))
    bracket=(va>error and vb<-error) or (va<-error and vb>error)
    if bracket:
        for _ in range(32):
            if work>=max_budget:reason|=1;break
            work+=1;h=sub(r,l);mid=add(l,mul(.5,h))
            if mid<=l or mid>=r or h<=to_f32(.0001):break
            s=mid
            if kind=='sbi':
                denominator=sub(fr,fl)
                if abs(denominator)>to_f32(1e-30) and stuck<2:
                    weight=div(-fl,denominator)
                    if 0<weight<1:
                        candidate=add(l,mul(min(max(weight,.125),.875),h))
                        if l<candidate<r:s=candidate
            if not l<s<r:s=mid
            if not l<s<r:break
            v=to_f32(f(s));evals+=1
            if not math.isfinite(v):reason|=4;break
            if abs(v)<=error:
                w=div(mul(2,error),min_slope);l=max(l,sub(s,w));r=min(r,add(s,w));break
            if (fl<0 and v>0) or (fl>0 and v<0):
                r=s;fr=v;stuck=stuck+1 if last_side==1 else 0;last_side=1
            else:
                l=s;fl=v;stuck=stuck+1 if last_side==2 else 0;last_side=2
        bands.append((l,r))
    return {'bands':bands,'evals':evals,'work':work,'reason':reason,'bracket_entered':bracket}

class BisectionIsolator:
    @staticmethod
    def isolate(f,a,b,va,vb,error,min_slope,max_budget=32):return isolate('bisection',f,a,b,va,vb,error,min_slope,max_budget)
class SafeguardedInterpolationIsolator:
    @staticmethod
    def isolate(f,a,b,va,vb,error,min_slope,max_budget=32):return isolate('sbi',f,a,b,va,vb,error,min_slope,max_budget)

def reference_interval(f,a,b):
    a=float(a);b=float(b);fa=f(a);fb=f(b)
    if fa==0:return (a,a)
    if fb==0:return (b,b)
    if fa*fb>0:raise ValueError('reference interval does not bracket')
    for _ in range(160):
        mid=a+.5*(b-a)
        if mid<=a or mid>=b:break
        v=f(mid)
        if v==0:return (mid,mid)
        if (fa<0 and v>0) or (fa>0 and v<0):b=mid
        else:a=mid;fa=v
    return (a,b)
def contains(result,root):return any(lo<=root[0] and hi>=root[1] for lo,hi in result['bands'])

def evaluate_suite():
    cases=[
      ('affine',lambda s:2.5*(s-4.2),0,10,.001,2.4),
      ('curved monotone',lambda s:.5*math.sin(.3*s)+1.2*s-5,2,6,.002,1.05),
      ('decreasing',lambda s:-3*(s-1.5),0,3,.001,2.9),
      ('small slope',lambda s:.005*(s-10),0,20,.0005,.004),
      ('steep',lambda s:150*(s-7.3),7,7.5,.01,140),
      ('endpoint uncertainty',lambda s:1.5*(s-1.0002),1,3,.001,1.4),
      ('cubic',lambda s:(s-1)**3+.1*(s-1),.5,3,.001,.09),
      ('high curvature',lambda s:math.sin(2*s)+2.1*s-4,1,3,.001,.1),
      ('overflowing secant denominator',lambda s:1e38*(s-.3),-2,2,1e32,9e37),
      ('tiny secant denominator',lambda s:1e-35*(s-.3),0,1,1e-40,9e-36)]
    rows=[]
    for name,ideal,a,b,error,slope in cases:
        a,b=to_f32(a),to_f32(b);truth=reference_interval(ideal,a,b);observed=lambda x,f=ideal:to_f32(f(x))
        methods={kind:isolate(kind,observed,a,b,observed(a),observed(b),error,slope) for kind in ['bisection','sbi']}
        rows.append({'case':name,'host_double_reference_interval':truth,'results':methods,'both_enclose_without_padding':all(contains(v,truth) and not v['reason'] for v in methods.values())})
    # Real adjacent-float branch: the root lies INSIDE the two f32 endpoints.
    for a in [1.,-1.]:
        b=next_f32(a);truth=(float(a)+float(b))*.5;f=lambda x:x-truth
        result=isolate('sbi',f,a,b,f(a),f(b),0,1)
        rows.append({'case':'adjacent floats '+str(a),'results':{'sbi':result},'both_enclose_without_padding':contains(result,(truth,truth)) and result['bracket_entered'] and result['work']==1 and result['evals']==0})
    ideal=lambda s:s-.3;observed=lambda s:to_f32(ideal(s)+.0004)
    result=isolate('sbi',observed,0,1,observed(0),observed(1),.001,.99)
    rows.append({'case':'explicit bounded oracle bias','results':{'sbi':result},'both_enclose_without_padding':contains(result,(.3,.3))})
    budget=isolate('sbi',lambda s:s-.3,0,1,-.3,.7,.001,.99,max_budget=0)
    # A positive error radius smaller than an ULP can round both ends INWARD.
    true_root=10-1e-9;f=lambda s:1000*(s-true_root);a,b=to_f32(9.99),to_f32(10.01)
    gap={kind:isolate(kind,f,a,b,to_f32(f(a)),to_f32(f(b)),1e-5,999) for kind in ['bisection','sbi']}
    detected=all(not contains(v,(true_root,true_root)) for v in gap.values())
    positive=all(r['both_enclose_without_padding'] for r in rows)
    return {'scope':'finite operation-rounded HOST model with ideal/observed oracle roles; not a GPU arithmetic or universal enclosure proof','checks_pass':positive and detected and budget['reason']==1 and budget['evals']==0,'mathematical_qualification':False,'cases':rows,'budget_refusal':budget,'shared_nearest_rounding_gap':{'true_root':true_root,'error':1e-5,'min_slope':999,'results':gap,'both_miss_without_padding':detected,'requires_contract_repair_before_generic_proof':True},'reference_scope':'independent binary64 bracketing of the stated ideal functions; no acceptance padding or device-execution equivalence claim','source_mismatch_note':'host model explicitly rounds each step and refuses nonfinite oracle values; actual Metal FMA/library behavior still needs its execution envelope'}

def run_witness_suite():
    d=evaluate_suite();print(json.dumps({'host_checks_pass':d['checks_pass'],'mathematical_qualification':False,'shared_rounding_gap_detected':d['shared_nearest_rounding_gap']['both_miss_without_padding']}));return d['checks_pass']
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path);a=p.parse_args();d=evaluate_suite()
    if a.output:a.output.write_text(json.dumps(d,indent=2)+'\n')
    print('HOST CHECKS',d['checks_pass'],'GENERIC/DEVICE ENCLOSURE QUALIFICATION',d['mathematical_qualification'])
    raise SystemExit(0 if d['checks_pass'] else 1)
