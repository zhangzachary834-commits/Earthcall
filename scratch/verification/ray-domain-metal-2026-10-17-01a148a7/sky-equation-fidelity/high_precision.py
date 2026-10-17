from pathlib import Path
import json,sys
sys.path.insert(0,str(Path(__file__).parent/'pydeps'))
import mpmath as mp
mp.mp.dps=80
ROOT=Path('/Users/zacharyzhang/Documents/GitHub/Earthcall')
OUT=Path(__file__).parent
zone=json.loads((ROOT/'saves/zones/Sky-Island Sanctuary/zone.json').read_text(),parse_float=mp.mpf)
fields={f['id']:f for f in [zone['spatialRoot']]+zone['spatialFields']}
def binary(a,b,fn):
    if isinstance(a,tuple) and isinstance(b,tuple):return tuple(fn(x,y) for x,y in zip(a,b))
    if isinstance(a,tuple):return tuple(fn(x,b) for x in a)
    if isinstance(b,tuple):return tuple(fn(a,y) for y in b)
    return fn(a,b)
def evaluate(node,variables):
    op=node['op']
    if op==0:
        total=mp.mpf(0)
        for term in node['scalarForm']['terms']:
            value=mp.mpf(term['c'])
            for v,e in term.get('factors',{}).items():value*=variables[v]**(int(e) if e==int(e) else e)
            for tr in term.get('trans',[]):
                a=mp.mpf(tr.get('scale',1))*variables[tr['var']]+mp.mpf(tr.get('shift',0))
                value*={0:mp.sin,1:mp.cos,2:mp.exp,3:mp.log}[tr['kind']](a)
            total+=value
        return total
    if op==1:return variables[node.get('var',node.get('variableName'))]
    c=[evaluate(x,variables) for x in node.get('children',[])]
    if op==2:return tuple(c)
    if op==4:return binary(c[0],c[1],lambda a,b:a+b)
    if op==5:return binary(c[0],c[1],lambda a,b:a-b)
    if op==6:return binary(c[0],c[1],lambda a,b:a*b)
    if op==11:return mp.sqrt(sum(x*x for x in c[0]))
    if op==15:return mp.sqrt(sum((a-b)**2 for a,b in zip(c[0],c[1])))
    if op==23:return binary(c[0],c[1],lambda a,b:a/b)
    if op==25:return abs(c[0])
    if op==26:return min(max(c[0],c[1]),c[2])
    if op==27:return mp.sqrt(c[0])
    raise ValueError('Unimplemented source op '+str(op))
before={x['id']:x for x in json.loads((OUT/'before.json').read_text())}
after={x['id']:x for x in json.loads((OUT/'after.json').read_text())}
summary=[];sampled=[]
for name in fields:
    if name not in after:continue
    pieces=fields[name]['field']['astDefinition']['pieces']
    assert len(pieces)==1 and not pieces[0].get('hasLo',False) and not pieces[0].get('hasHi',False)
    node=pieces[0]['mathNode']
    errors={'before_gpu':mp.mpf(0),'after_gpu':mp.mpf(0),'cpu':mp.mpf(0)}
    worst={}
    for old,new in zip(before[name]['samples'],after[name]['samples']):
        assert old['p']==new['p']
        p=tuple(mp.mpf(x) for x in new['p']);v=dict(zip(['x','y','z'],p));v.update(p=p,t=mp.mpf(0))
        exact=evaluate(node,v)
        for label,value in [('before_gpu',old['gpu']),('after_gpu',new['gpu']),('cpu',new['cpu'])]:
            error=abs(mp.mpf(value)-exact)
            if error>errors[label]:errors[label]=error;worst[label]={'p':new['p'],'reference':mp.nstr(exact,35),'observed':value}
        if name.endswith('sun-of-dawn') and new['p']==[0.0,-24.0,-50.0]:
            sampled.append({'id':name,'local_p':new['p'],'world_p':[-20,0,-32],'reference':mp.nstr(exact,50),'before':old['gpu'],'after':new['gpu']})
    row={'id':name,'samples':len(after[name]['samples']),'max_absolute_error':{k:float(v) for k,v in errors.items()},'worst_samples':worst}
    summary.append(row);print(name,len(after[name]['samples']),row['max_absolute_error'],flush=True)
report={'reference':'Independent recursive evaluation of saved JSON, mpmath at 80 decimal digits; original saved decimal coefficients and native source-local sample inputs; clock 0','mpmath_version':mp.__version__,'summary':summary,'examples':sampled}
(OUT/'high-precision-report.json').write_text(json.dumps(report,indent=2))
print('Examples',sampled)
