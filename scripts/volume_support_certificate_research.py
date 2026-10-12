"""Research-only backend-bound contract and refusal witnesses; no renderer integration.
Codex / GPT-6.1 Sol / 01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca / 2026-10-12 PDT.
Zach's minimum-work direction; Astra's bounded certificate brief. OPEN AD-30–32
are not resolved here. This bounds one scalar clamp, NOT a whole contribution.
The current production backend/profile is UNRESOLVED: admitted skips stay zero.
Conditional numerical tables are research artifacts, never GPU certificates.
"""
from fractions import Fraction as Q
from dataclasses import dataclass
from functools import lru_cache
from itertools import product
from pathlib import Path
import argparse, math, json, struct, hashlib

class Unknown(ValueError): pass
MIN_NORMAL=Q(1,2**126); SUB_ULP=Q(1,2**149)
MAX_F32=Q((2**24-1)*2**104)


def exact(v):
    if not math.isfinite(v): raise Unknown('nonfinite input')
    return Q(v)


def bits(v): return struct.unpack('<I',struct.pack('<f',v))[0]
def unpack(b): return struct.unpack('<f',struct.pack('<I',b))[0]
def adjacent(v,up):
    if v==0: return unpack(1 if up else 0x80000001)
    return unpack(bits(v)+(1 if (v>0)==up else -1))


def directed(v,up):
    if abs(v)>MAX_F32: raise Unknown('possible f32 overflow')
    f=struct.unpack('<f',struct.pack('<f',float(v)))[0]
    # Compare EXACT rationals, so binary64 conversion/double rounding cannot
    # inward-round an enclosure endpoint. One adjacent f32 repairs it.
    if (up and Q(f)<v) or (not up and Q(f)>v): f=adjacent(f,up)
    if not math.isfinite(f): raise Unknown('nonfinite rounded result')
    return Q(f)


@dataclass(frozen=True)
class Range:
    lo: Q
    hi: Q
    def __post_init__(self):
        if self.lo>self.hi:raise Unknown('empty interval')
    @classmethod
    def point(cls,x): return cls(exact(x),exact(x))
    def hull(self,b):return Range(min(self.lo,b.lo),max(self.hi,b.hi))
    def flush(self):
        # WGSL may flush any subnormal arithmetic input/output to zero.
        if self.lo<MIN_NORMAL and self.hi>-MIN_NORMAL:return self.hull(Range(Q(0),Q(0)))
        return self
    def rounded(self):return Range(directed(self.lo,False),directed(self.hi,True)).flush()
    def __add__(self,b):
        a=self.flush();b=b.flush();return Range(a.lo+b.lo,a.hi+b.hi).rounded()
    def __mul__(self,b):
        a=self.flush();b=b.flush();v=[x*y for x,y in product((a.lo,a.hi),(b.lo,b.hi))]
        return Range(min(v),max(v)).rounded()
    def neg(self):return Range(-self.hi,-self.lo)
    def absolute(self):
        return Range(Q(0) if self.lo<=0<=self.hi else min(abs(self.lo),abs(self.hi)),max(abs(self.lo),abs(self.hi)))
    def divide(self,b):
        a=self.flush();b=b.flush()
        if b.lo<=0<=b.hi:raise Unknown('divisor may be zero')
        if min(abs(b.lo),abs(b.hi))<MIN_NORMAL or max(abs(b.lo),abs(b.hi))>2**126:
            raise Unknown('divisor outside defined f32 accuracy range')
        v=[x/y for x,y in product((a.lo,a.hi),(b.lo,b.hi))];lo,hi=min(v),max(v)
        magnitude=max(abs(lo),abs(hi))
        if magnitude>MAX_F32:raise Unknown('division may overflow')
        # 3*maxULP plus directed endpoint rounding over-encloses WGSL's
        # 2.5-ULP division accuracy, including either rounding direction.
        e=SUB_ULP
        if magnitude>=MIN_NORMAL:
            n=magnitude.numerator.bit_length()-magnitude.denominator.bit_length()
            power=Q(2**n) if n>=0 else Q(1,2**(-n))
            if power>magnitude:n-=1
            e=Q(2**(n-23)) if n>=23 else Q(1,2**(23-n))
        return Range(lo-3*e,hi+3*e).rounded()


def uploaded(x):
    try:v=struct.unpack('<f',struct.pack('<f',float(x)))[0]
    except (OverflowError,ValueError):raise Unknown('unrepresentable uploaded parameter')
    return Range.point(v)


def arctan_bounds(inv,n=60):
    x=Q(1,inv);s=sum(((-1)**k*x**(2*k+1)/Q(2*k+1) for k in range(n)),Q(0))
    tail=(-1)**n*x**(2*n+1)/Q(2*n+1)
    return Range(min(s,s+tail),max(s,s+tail))

# Machin identity + alternating-series remainder supplies a rational pi bracket.
a=arctan_bounds(5);b=arctan_bounds(239)
PI=Range(16*a.lo-4*b.hi,16*a.hi-4*b.lo)


def trig(r,kind):
    r=r.flush()
    if kind not in (0,1):raise Unknown('unsupported intrinsic')
    if r.lo < -PI.lo or r.hi>PI.lo:raise Unknown('intrinsic accuracy domain uncertain/outside [-pi,pi]')
    c=(r.lo+r.hi)/2; radius=(r.hi-r.lo)/2
    # Exact rational Taylor midpoint, Lagrange remainder, and |f'|<=1:
    # no libm sin/cos endpoints or empirical epsilon enters this enclosure.
    count=16
    if kind==0:
        s=sum(((-1)**k*c**(2*k+1)/Q(math.factorial(2*k+1)) for k in range(count)),Q(0))
        rem=abs(c)**33/Q(math.factorial(33))
    else:
        s=sum(((-1)**k*c**(2*k)/Q(math.factorial(2*k)) for k in range(count)),Q(0))
        rem=abs(c)**32/Q(math.factorial(32))
    low=max(Q(-1),s-rem-radius);high=min(Q(1),s+rem+radius)
    # Normative intrinsic error; directed conversion additionally over-encloses it.
    return Range(low-Q(1,2048),high+Q(1,2048)).rounded()


def all_sum_trees(values):
    # Closed MODEL profile: every binary sum parenthesization/permutation;
    # rounded products include exact products, so contracted FMA is enclosed.
    @lru_cache(None)
    def rec(mask):
        if mask&(mask-1)==0:return values[mask.bit_length()-1]
        out=None;first=mask&-mask;sub=(mask-1)&mask
        while sub:
            other=mask^sub
            if other and sub&first:
                v=rec(sub)+rec(other);out=v if out is None else out.hull(v)
            sub=(sub-1)&mask
        return out
    if not values or len(values)>8:raise Unknown('sum closure exceeds research scope')
    return rec((1<<len(values))-1)


def const(n):
    if n.get('op')!=0:raise Unknown('expected constant leaf')
    terms=n.get('scalarForm',{}).get('terms',[])
    if len(terms)!=1 or terms[0].get('factors') or terms[0].get('trans'):raise Unknown('not a scalar constant')
    return float(terms[0]['c'])


def recognize(n):
    """Recognize Clamp(1 - Div(Abs(Add(axis, trig terms)), w), 0, 1).
    Reads AST operations/variables only; never Zone/object names or fixed slots.
    """
    if n.get('op')!=26:raise Unknown('not clamp')
    body,low,high=n['children']
    if const(low)!=0 or const(high)!=1 or body.get('op')!=5:raise Unknown('unsupported clamp')
    one,d=body['children']
    if const(one)!=1 or d.get('op')!=23:raise Unknown('unsupported support form')
    numerator,width=d['children']
    if numerator.get('op')!=25:raise Unknown('support numerator is not abs')
    terms=[];axis=[]
    def visit(q):
        if q.get('op')==4:
            for child in q['children']:visit(child)
        elif q.get('op')==1:axis.append(q['var'])
        elif q.get('op')==0:
            t=q['scalarForm']['terms']
            if len(t)!=1 or t[0].get('factors') or len(t[0].get('trans',[]))!=1:raise Unknown('unsupported fold leaf')
            f=t[0]['trans'][0]
            if f['kind'] not in (0,1):raise Unknown('unsupported fold intrinsic')
            terms.append((float(t[0]['c']),float(f['scale']),float(f.get('shift',0)),int(f['kind']),f['var']))
        else:raise Unknown('unsupported fold operation/binding')
    visit(numerator['children'][0])
    if len(axis)!=1 or not terms or len(terms)>7 or any(t[4]!=terms[0][4] for t in terms):raise Unknown('unsupported binding shape')
    if axis[0]==terms[0][4] or axis[0] not in ('x','y','z') or terms[0][4] not in ('x','y','z'):raise Unknown('unsupported evaluation domain')
    return {'axis':axis[0],'fold_axis':terms[0][4],'terms':terms,'width':const(width),'ast':n}


def nodes(n):
    yield n
    for c in n.get('children',[]):yield from nodes(c)


def key(form,binding):
    # Research dependency witness, not a production sharing/cache key.
    parameters=[bits(float(uploaded(x).lo)) for t in form['terms'] for x in t[:3]]+[bits(float(uploaded(form['width']).lo))]
    structure=(form['axis'],form['fold_axis'],tuple((t[3],t[4]) for t in form['terms']))
    return (structure,tuple(parameters),tuple(binding))


def require_same_binding(previous,current):
    if previous!=current:raise Unknown('stale parameters, structure, or evaluation domain')


def fold_atoms(form,x):
    atoms=[]
    for amplitude,scale,shift,kind,_ in form['terms']:
        # Covers separately rounded affine inputs AND contracted FMA.
        arg=uploaded(scale)*x+uploaded(shift)
        atoms.append(uploaded(amplitude)*trig(arg,kind))
    return atoms


def fold_range(form,x):
    return all_sum_trees(fold_atoms(form,x))


def band(form,x,zmax):
    """Directed f32 band for the CLOSED MODEL; never a production certificate."""
    atoms=fold_atoms(form,x)
    w=uploaded(form['width']).flush()
    if w.lo<uploaded(1e-6).lo:raise Unknown('guarded division branch may select zero')
    if w.lo<MIN_NORMAL or w.hi>2**126:raise Unknown('unsupported division accuracy domain')
    magnitude=zmax+sum(max(abs(v.lo),abs(v.hi)) for v in atoms)
    error=Q(0)
    # n terms + z => n additions, in any parenthesization/permutation.
    for _ in atoms:
        m=magnitude+error
        if m>MAX_F32:raise Unknown('sum closure may overflow')
        if m<MIN_NORMAL:ulp=SUB_ULP
        else:
            e=m.numerator.bit_length()-m.denominator.bit_length()
            power=Q(2**e) if e>=0 else Q(1,2**(-e))
            if power>m:e-=1
            ulp=Q(2**(e-23)) if e>=23 else Q(1,2**(23-e))
        # Up to two input flushes and one output flush per addition.
        error+=ulp+3*MIN_NORMAL
    # 2.5 ULP(q) <= 3*2^-23*q for a finite normal q. This threshold
    # guarantees every permitted quotient >= 1, then 1-quotient <= 0.
    threshold=w.hi/(1-Q(3,2**23))
    if (magnitude+error)/w.lo>MAX_F32:raise Unknown('quotient may overflow')
    lo=-sum(v.hi for v in atoms)-error-threshold
    hi=-sum(v.lo for v in atoms)+error+threshold
    return Range(directed(lo,False),directed(hi,True))


def model_zero(form,x,z):
    # MODEL profile only: abs/div/sub/clamp boundaries remain ordered; all
    # additive trees are allowed. Actual unrestricted WGSL is not admitted.
    w=uploaded(form['width']).flush()
    if w.lo<uploaded(1e-6).lo:raise Unknown('guarded division branch may select zero')
    atoms=[]
    for amp,scale,shift,kind,_ in form['terms']:
        arg=uploaded(scale)*x+uploaded(shift)
        atoms.append(uploaded(amp)*trig(arg,kind))
    displacement=all_sum_trees((z,*atoms)).absolute()
    ratio=displacement.divide(w)
    result=uploaded(1)+(ratio.neg())
    return result.hi<=0


def possible_bin(x,edges):
    # Proposed runtime selects bins with direct comparisons against resident
    # exact f32 thresholds. No (x+extent)/size float index expression is used.
    # Enclosure touching an edge declines rather than choosing one side.
    if x.lo<edges[0] or x.hi>=edges[-1]:raise Unknown('outside bin partition')
    hits=[i for i in range(len(edges)-1) if x.hi>=edges[i] and x.lo<edges[i+1]]
    if len(hits)!=1:raise Unknown('uncertain bin edge')
    return hits[0]


def production_zero(*args,**kwargs):
    # No verified closed arithmetic/coordinate evaluation profile is available
    # for the retained WGSL module. It permits broader reassociation than the
    # executable MODEL above; no flag in this research tool can assert otherwise.
    raise Unknown('production execution/coordinate profile unresolved')


def witnesses():
    f={'axis':'z','fold_axis':'x','width':1.,'terms':[(1.,1.,0.,0,'x')]}
    cases={}
    def refuse(name,fn):
        try:fn()
        except Unknown:cases[name]=True
        else:raise AssertionError('refusal witness accepted: '+name)
    assert not model_zero(f,Range.point(0.),Range.point(1.0001))
    cases['apparent separation inside intrinsic error retains evaluator']=True
    assert model_zero(f,Range.point(0.),Range.point(1.01))
    envelope=band(f,Range.point(0.),Q(2))
    assert envelope.lo<=Q(1.0001)<=envelope.hi
    assert Q(1.01)>envelope.hi
    cases['separation outside model enclosure certifies scalar zero']=True
    cases['two-endpoint model band refuses uncertainty margin']=True
    refuse('bin boundary uncertainty',lambda:possible_bin(Range(Q(-1,2**25),Q(1,2**25)),[Q(-1),Q(0),Q(1)]))
    refuse('principal domain boundary',lambda:trig(Range(PI.lo-SUB_ULP,PI.hi+SUB_ULP),0))
    refuse('outside intrinsic accuracy range',lambda:trig(Range.point(4.),1))
    refuse('guarded small denominator',lambda:model_zero(dict(f,width=0.),Range.point(0.),Range.point(2.)))
    refuse('nonfinite parameter',lambda:uploaded(float('nan')))
    refuse('overflow',lambda:Range(MAX_F32,MAX_F32)+Range(MAX_F32,MAX_F32))
    refuse('division unsupported range',lambda:Range.point(1.).divide(Range.point(1e-40)))
    assert (Range(SUB_ULP,SUB_ULP)+Range.point(0.)).lo==0
    cases['subnormal flushing encloses zero']=True
    changed=dict(f,terms=[(3.,1.,0.,0,'x')])
    assert key(f,('medium0','point0','clock0'))!=key(changed,('medium0','point0','clock0'))
    refuse('numeric edit refuses stale certificate',lambda:require_same_binding(
        key(f,('medium0','point0','clock0')),key(changed,('medium0','point0','clock0'))))
    assert model_zero(f,Range.point(.5),Range.point(-1.6))
    assert not model_zero(changed,Range.point(.5),Range.point(-1.6))
    cases['numeric edit changes binding key and invalidates prior zero']=True
    for field,domain in [('independent clock','clock1'),('shifted SDF point','point1'),('source coordinate domain','source0')]:
        refuse(field,lambda domain=domain:require_same_binding(
            key(f,('medium0','point0','clock0')),key(f,('medium0','point0',domain))))
    refuse('unknown production profile refuses even model-certified zero',lambda:production_zero(f,Range.point(0.),Range.point(10.)))
    # Independent high-precision libm samples supplement the rational derivation;
    # they do not prove intrinsic error or backend profile closure.
    for kind in (0,1):
        for i in range(-24,25):
            c=i/8; r=Range(exact(c)-Q(1,1000),exact(c)+Q(1,1000));enclosed=trig(r,kind)
            for t in (float(r.lo),c,float(r.hi)):
                v=exact((math.sin if kind==0 else math.cos)(t));assert enclosed.lo<=v<=enclosed.hi
    cases['98 rational trig intervals contain independent endpoint/midpoint samples']=True
    return cases


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--fixture',type=Path,required=True,help='Existing decoded JSON fixture, read-only')
    p.add_argument('--output',type=Path,required=True,help='New JSON research report; no overwrites')
    p.add_argument('--bins',type=int,default=256)
    a=p.parse_args()
    if not 2<=a.bins<=512:p.error('bounded research uses 2..512 bins')
    checks=witnesses(); z=json.loads(a.fixture.read_text());reports=[]
    for f in z.get('spatialFields',[]):
        pw=f.get('volumeDensity',{});pieces=pw.get('pieces',[])
        if len(pieces)!=1 or set(pieces[0])-{'mathNode'}:raise Unknown('guarded/piecewise density excluded from recognition')
        found=[]
        for n in nodes(pieces[0]['mathNode']):
            try:found.append(recognize(n))
            except (Unknown,KeyError,ValueError):pass
        if len(found)!=1:raise Unknown('expected exactly one structural folded clamp per fixture member')
        form=found[0];extent=f['scale'][{'x':0,'y':1,'z':2}[form['fold_axis']]]
        h=uploaded(abs(extent)).lo
        edges=[directed(-h+2*h*Q(i,a.bins),False) for i in range(a.bins+1)]
        if len(set(edges))!=len(edges):raise Unknown('collapsed bin partition')
        axis_extent=uploaded(abs(f['scale'][{'x':0,'y':1,'z':2}[form['axis']]])).lo
        admitted=[];unknown=[];bands={}
        for i in range(a.bins):
            x=Range(edges[i],edges[i+1])
            try:
                bound=band(form,x,axis_extent);bands[i]=bound
                admitted.append({'bin':i,'zero_band_lo':float(bound.lo),'zero_band_hi':float(bound.hi)})
            except Unknown as e:unknown.append({'bin':i,'reason':str(e)})
        # Screen a deterministic 256 x 128 local point grid, with input
        # uncertainty enclosed by BOTH adjacent f32s of each rational centre.
        # This is an explicit MODEL input domain, not a moving GPU camera route.
        axis_extent=uploaded(abs(f['scale'][{'x':0,'y':1,'z':2}[form['axis']]])).lo
        total=numeric=declined=0
        for i in range(a.bins):
            x=Range(edges[i],edges[i+1])
            if i not in bands:continue
            for j in range(128):
                centre=-axis_extent+2*axis_extent*Q(2*j+1,256)
                zz=Range(directed(centre,False),directed(centre,True));total+=1
                safe=zz.hi<bands[i].lo or zz.lo>bands[i].hi
                numeric+=safe
                if safe:assert model_zero(form,x,zz), 'band escaped enumerated model trees'
        reports.append({'fixture_label':f.get('id',f.get('identifier','')), 'recognition':'AST-only folded clamp',
            'domain':{'fold_axis':form['fold_axis'],'displacement_axis':form['axis']},
            'model_enclosed_bins':len(admitted),'unknown_bins':len(unknown),'model_screen_queries':total,
            'model_scalar_zero_queries':numeric,'model_declined_queries':declined,
            'production_admitted_queries':0,'production_reason':'execution/coordinate profile unresolved',
            'table':admitted,'declines':unknown})
    report={'signature':'Codex / GPT-6.1 Sol / 01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca / 2026-10-12 PDT',
        'fixture_sha256':hashlib.sha256(a.fixture.read_bytes()).hexdigest(),'bins':a.bins,
        'scope':'rational closed-model arithmetic research; not a production WGSL/backend certificate or GPU speedup',
        'author_dependencies':'AD-30..32 remain OPEN; no new whole-contribution skips',
        'refusal_witnesses':checks,'model_band_witness':'Every accepted grid query also certifies zero through independently enumerated sum trees',
        'members':reports,'production_admitted_total':0}
    with a.output.open('x') as out:json.dump(report,out,indent=2);out.write('\n')
    print(json.dumps({k:v for k,v in report.items() if k!='members'},indent=2))
    for r in reports:print(json.dumps({k:v for k,v in r.items() if k not in ('table','declines')}))

if __name__=='__main__':main()
