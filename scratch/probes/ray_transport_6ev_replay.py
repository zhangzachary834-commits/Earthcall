"""CPU replay of retained §6ev native transitions, not fresh GPU execution.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import zipfile

NAMES=['baseline','baseline-repeat','toggle-off','toggle-on','independent-clocks','clock-recover','emission-edit','emission-recover','density-edit','density-recover','unsupported-structure','structure-recover','move-and-extent','bounds-recover','membership-three','membership-recover','positive-source','source-recover','absent-source-nonwi','phase-source-recover','resize-odd','resize-wide','resize-recover','opaque-front','opaque-recover','forced-refusal','refusal-recover','camera-outside','camera-recover','fresh-device','fresh-device-repeat']


def need(value,message):
    if not value:raise ValueError(message)


def expected(name):
    w,h=(321,181) if name=='resize-odd' else (640,360) if name=='resize-wide' else (334,190)
    on=name!='toggle-off';members=3 if name=='membership-three' else 4;source=1 if name=='positive-source' else 2 if name=='absent-source-nonwi' else 0
    fixture={'emission-edit':'emission','density-edit':'density','unsupported-structure':'unsupported','move-and-extent':'bounds','absent-source-nonwi':'nonwi'}.get(name,'base')
    clocks=[7.25,13.5,-2.,4.75] if name=='independent-clocks' else [7.25]*4
    return dict(width=w,height=h,on=on,members=members,source=source,fixture=fixture,clocks=clocks,force=name=='forced-refusal',opaque=name=='opaque-front',outside=name=='camera-outside',device=int(name.startswith('fresh-device')),expected_routed=on and members==4 and source==0 and fixture!='unsupported')


def replay(path):
    with zipfile.ZipFile(path) as z:
        m=json.loads(z.read('manifest.json'));need(set(z.namelist())=={'manifest.json'}|{'blobs/'+s for s in m['files'].values()},'content-addressed archive inventory')
        for sha in set(m['files'].values()):need(hashlib.sha256(z.read('blobs/'+sha)).hexdigest()==sha,'artifact hash '+sha)
        def data(name):return z.read('blobs/'+m['files'][name])
        def j(name):return json.loads(data(name))
        def eq(a,b):return m['files'][a]==m['files'][b]
        builds={k:j(k+'/build.json') for k in ['actual','negative']}
        need(builds['actual']['negative_derivation'] is False,'positive uses actual renderer object')
        need(isinstance(builds['negative']['negative_derivation'],dict),'labelled negative derivation')
        for key,b in builds.items():
            need(b['before']==b['after'],'fixed build source '+key)
            need(m['files'][key+'/probe.cpp']==b['probe_cpp_sha256'],'generated native main '+key)
            for name,sha in b['tool_sha256'].items():need(m['files']['tools/'+Path(name).name]==sha,'generator bytes')
        need(builds['actual']['after']==builds['negative']['after'],'same parent engine/fixture')
        need(builds['actual']['renderer_object_sha256']==builds['negative']['renderer_object_sha256'],'same original app renderer object')
        deriv=builds['negative']['negative_derivation']
        need(m['files']['negative/derived.inc']==deriv['derived_inc_sha256'] and m['files']['negative/derived-renderer.cpp']==deriv['derived_renderer_sha256'],'negative source bytes')
        need(len(deriv['mutations'])==4,'closed stage-omission derivation')
        need(deriv['parent_inc_sha256']==builds['actual']['after']['source_files']['src/Singularity/Screen/WebGPU/VolumeRoutedTransport.inc'] and deriv['parent_renderer_sha256']==builds['actual']['after']['source_files']['src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp'],'negative parents match executed engine')
        import re
        text=data('negative/derived-renderer.cpp').decode();pattern=r'#include "[^"]+/derived\.inc"';need(len(re.findall(pattern,text))==1,'single derived include');text=re.sub(pattern,'#include "Singularity/Screen/WebGPU/VolumeRoutedTransport.inc"',text)
        need(hashlib.sha256(text.encode()).hexdigest()==deriv['parent_renderer_sha256'],'exact renderer include derivation')
        inc=data('negative/derived.inc').decode();header='#include <cstdlib>\nstatic bool ecOmit(const char* stage){auto* p=std::getenv("EC_6EV_STAGE");return p&&std::string(p)==stage;}\n'
        need(inc.startswith(header),'negative diagnostic prefix');parent=inc[len(header):]
        for old,new in reversed(deriv['mutations']):need(parent.count(new)==1,'unique reversible mutation');parent=parent.replace(new,old)
        need(hashlib.sha256(parent.encode()).hexdigest()==deriv['parent_inc_sha256'],'exact native parent derivation')
        def receipt(prefix,key,code):
            r=j(prefix+'/receipt.json');b=builds[key]
            need(r['before']==r['after']==b['after'] and r['binary_sha256']==b['binary_sha256'],'execution source/binary binding '+prefix)
            need(r['exit_code']==code,'native process '+prefix)
            for name,sha in r['files_sha256'].items():need(m['files'][prefix+'/'+name]==sha,'executed artifact bytes '+prefix)
            log=data(prefix+'/native.log').decode();need(not any(s in log.lower() for s in ['validation error','uncaptured error','device lost','[routed compiler]']),'GPU/compiler '+prefix)
            return r,log
        prefix='actual/transitions';r,log=receipt(prefix,'actual',0);rows=j(prefix+'/states.json');need([s['name'] for s in rows]==NAMES and 'TRANSITION PASS states=31' in log,'native state matrix coverage')
        summary=[];compared=0;poison=0
        for row in rows:
            name=row['name'];spec=expected(name);need(all(row[k]==v for k,v in spec.items()),'independent intended state '+name)
            w,h=spec['width'],spec['height'];base=prefix+'/'+name
            for label in ['first','fresh','poisoned']:need(len(data(base+'-'+label+'.rgba'))==w*h*4,'Screen extent '+name)
            need(eq(base+'-first.rgba',base+'-fresh.rgba') and eq(base+'-poisoned.rgba',base+'-fresh.rgba'),'fresh Screen comparison '+name);compared+=w*h*8
            routed=spec['expected_routed']
            for label in ['first','fresh','poisoned']:need(row[label+'_stats']['routed_sets']==int(routed) and row[label+'_stats']['program_refusals']==0,'actual eligibility '+name)
            need(all(row[k] is True for k in ['Screen_first_equals_fresh','Screen_poisoned_equals_fresh','publication_first_equals_fresh','publication_poisoned_equals_fresh']),'native assertions '+name)
            # Mutable GPU buffers only; static offsets/indices intentionally excluded.
            cap=w*48;tiles=((w+3)//4)*12;bytes_expected=w*h*16+cap*(48+64*16+16*16)+tiles*12+52
            need(row['poison_bytes']==bytes_expected,'actual poisoned descriptor extent '+name);poison+=bytes_expected
            census=dict(accepted=0,refused=0,nonfinite=0,nonempty=0)
            if routed:
                need(eq(base+'-first.publication.bin',base+'-fresh.publication.bin') and eq(base+'-poisoned.publication.bin',base+'-fresh.publication.bin'),'fresh float publication '+name)
                pub=data(base+'-poisoned.publication.bin');need(len(pub)==w*h*16,'full publication extent '+name);compared+=w*h*32
                for v in struct.iter_unpack('<4f',pub):
                    census['nonfinite']+=sum(not math.isfinite(x) for x in v)
                    if v[3]<0:census['refused']+=1
                    else:
                        census['accepted']+=1;census['nonempty']+=int(v[3]>1e-5 or max(abs(x) for x in v[:3])>1e-6)
                need(census['nonfinite']==0 and census['accepted']+census['refused']==w*h,'finite publication ownership '+name)
                if spec['force']:need(census['refused']==w*h and eq(base+'-current.rgba',base+'-poisoned.rgba'),'actual Current whole-ray fallback')
            need(census==row['publication'],'readback census '+name)
            if name in ['independent-clocks','emission-edit','density-edit','move-and-extent']:need(not eq(base+'-fresh.rgba',prefix+'/baseline-fresh.rgba'),'positive field/clock sensitivity '+name)
            if name.endswith('recover') or name.startswith('fresh-device'):need(eq(base+'-fresh.rgba',prefix+'/baseline-fresh.rgba'),'restoration to original state '+name)
            summary.append(dict(name=name,shape=[w,h],routed=routed,poison_bytes=bytes_expected,publication=census))
        negatives=[]
        for mode in ['missing-setup','missing-producer','missing-consumer','missing-publication']:
            pre='negative/'+mode;rec,log=receipt(pre,'negative',42);need(rec['mutation']==mode and 'TRANSITION FAIL' in log,'intended stage rejection')
            need(not eq(pre+'/baseline-poisoned.publication.bin',pre+'/baseline-fresh.publication.bin'),'independent negative float output')
            if mode=='missing-setup':need('TRANSITION FAIL publication finite baseline' in log,'poison detects omitted setup')
            else:need('TRANSITION FAIL Screen stale or missing output baseline' in log or 'TRANSITION FAIL publication stale or missing output baseline' in log,'missing stage output')
            negatives.append(dict(mutation=mode,exit_code=42,rejection=next(x for x in log.splitlines() if x.startswith('TRANSITION FAIL'))))
        return dict(schema='6ev-retained-native-transition-review-1',engine=builds['actual']['after'],states=summary,transition_states=len(rows),compared_bytes=compared,submitted_poison_bytes=poison,negative_controls=negatives,scope='Same-process state/cache/publication validity on actual app renderer, fresh instance comparisons and independent intended-state/refusal checks. No new authored-reference accuracy, full-resolution transition screen, performance/FPS or Person acceptance.')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('archive',type=Path);p.add_argument('--output',type=Path);a=p.parse_args();v=replay(a.archive)
    if a.output:a.output.write_text(json.dumps(v,indent=2,allow_nan=False)+'\n')
    print('PASS retained native transition replay:',v['transition_states'],'states;',len(v['negative_controls']),'native omission rejections;',v['compared_bytes'],'comparison bytes')

if __name__=='__main__':main()
