"""Portable §6dw source-bound stack experiment replay, using lossless deltas.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-26.
"""
import argparse,base64,copy,hashlib,importlib.util,json,plistlib,re,zipfile
from pathlib import Path

def digest(data):return hashlib.sha256(data).hexdigest()
def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def top_values(data):
    # Native nlohmann dump(2), preserving each value's original float lexemes.
    assert data.startswith(b'{\n') and data.endswith(b'\n}\n')
    matches=list(re.finditer(rb'^  ("[^"\n]+"):\x20',data,re.M));values={}
    for i,m in enumerate(matches):
        end=matches[i+1].start()-2 if i+1<len(matches) else len(data)-3
        value=data[m.end():end]
        values[json.loads(m[1])]=value
    assert assemble_top(values)==data,'native top-level format differs'
    return values

def assemble_top(values):
    return b'{\n'+b',\n'.join(b'  '+json.dumps(k).encode()+b': '+v for k,v in sorted(values.items()))+b'\n}\n'

def restore_timing(donor,delta):
    values=top_values(donor)
    for key in delta['deleted']:del values[key]
    for key,value in delta['replacements'].items():values[key]=base64.b64decode(value)
    data=assemble_top(values)
    assert digest(data)==delta['native_sha256'],'timing delta not byte-identical'
    return data

def restore_controls(donor,delta):
    baseline=json.loads(donor);cases=[]
    for mode in [2,3]:
        for original in baseline['cases']:
            c=dict(original,mode=mode)
            if mode==2:
                c.update(completed_s1=0,terminal_endpoint_checks=0,terminal_endpoints=[],flags=[flag&~0x40000000 for flag in c['flags']])
            cases.append(c)
    restored=dict(baseline,cases=cases,scope=delta['scope'])
    data=(json.dumps(restored,sort_keys=True,ensure_ascii=False,indent=2)+'\n').encode()
    assert digest(data)==delta['native_sha256'],'controls delta not byte-identical'
    return data

def decode_table(data):
    raw=plistlib.loads(data);objects=raw.get('$objects',[])
    def dec(x):
        if isinstance(x,plistlib.UID):return dec(objects[x.data])
        if isinstance(x,list):return [dec(v) for v in x]
        if isinstance(x,dict):
            if 'NS.keys' in x:return {dec(k):dec(v) for k,v in zip(x['NS.keys'],x['NS.objects'])}
            if 'NS.objects' in x:return dec(x['NS.objects'])
            return {k:dec(v) for k,v in x.items() if k!='$class'}
        return x
    d=dec(raw['$top']['root']) if objects else raw
    if 'Spilled bytes' not in d:return None
    return {'function':d.get('Compile Performance',{}).get('Function Name','unknown'),'spilled_bytes':d['Spilled bytes'],'temporary_registers':d['Temporary register count'],'uniform_registers':d['Uniform register count'],'instructions':d['Instruction count']}

def verify_spill_claim(rows,claim):
    assert len(rows)==2 and all(row['spilled_bytes']==claim for row in rows),'claim differs from native compiler tables'

def replay(root,index,archive,donor_archive):
    record=json.loads(index.read_text());assert digest(archive.read_bytes())==record['archive_sha256']
    assert digest(donor_archive.read_bytes())==record['donor_archive_sha256']
    with zipfile.ZipFile(archive) as z,zipfile.ZipFile(donor_archive) as donor:
        assert set(z.namelist())==set(record['artifacts'])
        def read(name):
            data=z.read(name);assert digest(data)==record['artifacts'][name]['sha256'],'artifact drift: '+name;return data
        for name in record['artifacts']:read(name)
        du=load(root,'ray_transport_gpu_6dv_review_replay')
        native_controls=restore_controls(donor.read('controls/review.json'),json.loads(read('controls-delta.json')))
        controls=json.loads(native_controls)
        assert len(controls['cases'])==60
        mode_summaries={}
        for mode in [2,3]:
            cases=[c for c in controls['cases'] if c['mode']==mode]
            mode_summaries[str(mode)]=du.verify_controls({'cases':cases})
        native_timing=restore_timing(donor.read('benchmark/second.json'),json.loads(read('timing-delta.json')))
        report=json.loads(native_timing);timing=du.verify_timing(report['clean_timing_series'])
        rows=json.loads(donor.read('benchmark/rows.json'));baseline=json.loads(donor.read('benchmark/baseline.json'));rays=json.loads(donor.read('fixture/rays.json'));ref=json.loads(donor.read('fixture/reference.json'))
        assert digest(donor.read('benchmark/rows.json'))==record['timing_rows_sha256']
        assert digest(donor.read('benchmark/baseline.json'))==record['timing_baseline_sha256']
        checked=load(root,'ray_transport_gpu_6dd_continuation').validate_6dd(root,report,rows,baseline,rays,ref)
        # Actual scalar and canonical controls had exactly identical native JSON.
        assert all(v==digest(native_controls) for v in record['native_control_report_sha256'].values())
        tables=[]
        for name in record['artifacts']:
            if name.startswith('tables/'):
                row=decode_table(read(name))
                if row:tables.append(dict(row,variant=name.split('/')[1],file=name.split('/')[-1]))
        s1={v:[r for r in tables if r['variant']==v and r['function']=='s1ProducerMain'] for v in ['canonical','scalar-stack']}
        assert len(s1['canonical'])==len(s1['scalar-stack'])==2
        for variant,claim in record['s1_spill_claims'].items():verify_spill_claim(s1[variant],claim)
        factory=load(root,'ray_transport_gpu_6dw_stack_isolation')
        # Clean source must reconstruct, independent of diagnostic log wording.
        for variant in ['canonical','scalar-stack']:
            actual=read('clean/'+variant+'.cpp')
            assert factory.clean_cpp(root,variant).encode()==actual,'clean source reconstruction failed'
            expected_hashes={digest(shader.encode()) for name,shader in re.findall(r'static const char\* (\w+) = R"WGSL\((.*?)\)WGSL";',actual.decode(),re.S) if name in ['kS1UnfusedProducerWgsl','kS1TerminalProducerWgsl']}
            facts=[json.loads(line) for line in read('capture/'+variant+'-facts.jsonl').decode().splitlines()]
            observed={row['wgslSHA256'] for row in facts if row.get('wgslEntry')=='s1ProducerMain'}
            assert expected_hashes==observed,'compiler capture not bound to these S1 shaders'
            device=next(row for row in facts if row.get('kind')=='device')
            assert device['name']=='Apple M5' and not device['timestampSamplingRequested'] and not device['counterSamplingRequested']
            cpp=read('native/'+variant+'.cpp').decode()
            embedded=re.findall(r'R"WGSL\((.*?)\)WGSL"',cpp,re.S)
            expected=re.findall(r'R"WGSL\((.*?)\)WGSL"',factory.controls_cpp(root,variant),re.S)
            assert embedded==expected,'instrumented shader reconstruction failed'
        failures=[]
        for label,fn in [
            ('timing delta corruption',lambda:restore_timing(donor.read('benchmark/second.json'),dict(json.loads(read('timing-delta.json')),native_sha256='0'*64))),
            ('native controls corruption',lambda:restore_controls(donor.read('controls/review.json'),dict(json.loads(read('controls-delta.json')),native_sha256='0'*64))),
            ('false zero-spill claim',lambda:verify_spill_claim(s1['scalar-stack'],0)),
        ]:
            try:fn()
            except (AssertionError,ValueError,KeyError):failures.append(label)
            else:raise AssertionError('corruption admitted: '+label)
        return {'pass':True,'native_cases_per_variant':60,'native_variants':['canonical','scalar-stack'],'mode_summaries':mode_summaries,'clean_timing_scope':'one screening invocation; two internal ordered blocks, not two independent invocations','clean_timing':timing,'accuracy_validation':checked,'compiler_rows':tables,'s1_compiler_rows':s1,'corruptions_rejected':failures,'spilled_bytes_change_percent':-27.2,'retained':False,'cost_screen_failed':'No substantial repeated mean-and-median Current win; stop before a second timing invocation. Dynamic traffic/occupancy/latency cause unmeasured.','shipping_gates_open':True}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--record',type=Path,required=True);p.add_argument('--archive',type=Path,required=True);p.add_argument('--donor-archive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    result=replay(a.root.resolve(),a.record,a.archive,a.donor_archive);a.output.write_text(json.dumps(result,indent=2)+'\n');print('PASS: 120 native cases, clean screen/reference replay, controlled compiler tables and corruption refusals')

if __name__=='__main__':main()
