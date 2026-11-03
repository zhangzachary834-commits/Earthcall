"""Portable §6ea corrective replay of §6dz producer-pass coalescing.
Codex / GPT-6.1 Sol / session 01a148a7-bd85-7550-a416-0e859a729c1a.
Offline evidence replay is distinct from native execution and shipping admission.
"""
import argparse,base64,copy,hashlib,importlib.util,json,math,re,statistics,zipfile
from pathlib import Path

def digest(data):return hashlib.sha256(data).hexdigest()
def require(ok,message):
    if not ok:raise ValueError(message)
def load(root,name):
    s=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m

def control_cases(base,mode):
    cases=[]
    for original in base['cases']:
        c=dict(original,mode=mode)
        if mode in [2,4]:
            c.update(completed_s1=0,terminal_endpoint_checks=0,terminal_endpoints=[],flags=[f&~0x40000000 for f in c['flags']])
        cases.append(c)
    return cases

def controls_digest(base,modes,scope):
    # Exact nlohmann dump(2) bytes, reconstructed in bounded case chunks.
    h=hashlib.sha256();h.update(b'{\n  "cases": [\n');first=True
    for mode in modes:
        for c in control_cases(base,mode):
            if not first:h.update(b',\n')
            first=False
            chunk=json.dumps(c,sort_keys=True,ensure_ascii=False,indent=2)
            h.update(('\n'.join('    '+line for line in chunk.splitlines())).encode())
    h.update(('\n  ],\n  "clean_timing": false,\n  "scope": '+json.dumps(scope,ensure_ascii=False)+',\n  "shipping_qualified": false\n}\n').encode())
    return h.hexdigest()

def verify_timing(report):
    table=report['clean_timing_series'];ordered=report['clean_timing_ordered_samples']
    result={}
    for rep in ['1','2','4']:
        per={str(m):{metric:[] for metric in ['encode_ms','completion_ms','total_ms']} for m in range(6)}
        for block in ['series_1','series_2']:
            rows=ordered[block][rep]
            forward=[0,1,2,3,4,5,5,4,3,2,1,0]
            backward=[5,4,3,2,1,0,0,1,2,3,4,5]
            # Both internal series contain the same four alternating inner blocks.
            expected=forward+backward+forward+backward
            require([row['mode'] for row in rows]==expected,'six-mode ordering differs')
            for row in rows:
                require(all(math.isfinite(row[k]) and row[k]>0 for k in ['encode_ms','completion_ms','total_ms']),'invalid clock')
                require(math.isclose(row['encode_ms']+row['completion_ms'],row['total_ms'],rel_tol=1e-10,abs_tol=2e-6),'encode/completion/total disagree')
                for metric in per[str(row['mode'])]:per[str(row['mode'])][metric].append(row[metric])
            for m in range(6):
                values=[row['completion_ms'] for row in rows if row['mode']==m];actual=table[block][rep][str(m)]
                require(len(values)==8 and sorted(values)==actual['samples'],'block samples differ')
                for key,expected_value in [('mean_ms',statistics.mean(values)),('median_ms',statistics.median(values)),('min_ms',min(values)),('max_ms',max(values))]:
                    require(math.isclose(actual[key],expected_value,rel_tol=1e-12,abs_tol=1e-12),'block summary mismatch')
        result[rep]={}
        for m,v in per.items():
            pooled=table['pooled'][rep][m]
            require(sorted(v['completion_ms'])==pooled['samples'],'pooled samples mismatch')
            for k,expected in [('mean_ms',statistics.mean(v['completion_ms'])),('median_ms',statistics.median(v['completion_ms']))]:require(math.isclose(pooled[k],expected,rel_tol=1e-12),'pooled summary mismatch')
            result[rep][m]={metric:{'mean_ms':statistics.mean(values),'median_ms':statistics.median(values),'observations':len(values)} for metric,values in v.items()}
    return result

def early_screen_cpp(cpp):
    filtered = '\n'.join([
        '                    J legSamples = J::array();',
        '                    for (const auto& s : series1Samples) {',
        '                        if (s["mode"].get<int>() < 4) legSamples.push_back(s);',
        '                    }',
        '                    series.push_back({{"repeats", rep}, {"samples", legSamples}});'])
    require(cpp.count(filtered)==1,'missing legacy compatibility projection')
    return cpp.replace(filtered,'                    series.push_back({{"repeats", rep}, {"samples", series1Samples}});')

def legacy_accuracy_view(report):
    # The first native screen predates the legacy 4-mode report projection.
    # Preserve native bytes/all six clocks and adapt only that legacy reader view.
    projected = dict(report)
    streamed = dict(report['streamed_schedule'])
    cases = []
    for case in streamed['cases']:
        updated = dict(case)
        updated['series'] = [dict(row,samples=[v for v in row['samples'] if v['mode'] < 4]) for row in case['series']]
        cases.append(updated)
    streamed['cases'] = cases
    projected['streamed_schedule'] = streamed
    return projected

def source_binding(root,read,record):
    factory=load(root,'ray_transport_gpu_6dz_producer_coalescing')
    generated=factory.clean_cpp(root,'bound-stack-15')
    require(generated.encode()==read('native/clean.cpp'),'clean source reconstruction differs')
    require(early_screen_cpp(generated).encode()==read('native/screen.cpp'),'initial screen source reconstruction differs')
    require(factory.controls_cpp(root,'bound-stack-15').encode()==read('native/controls.cpp'),'120-case source reconstruction differs')
    shader_blocks=lambda s:dict(re.findall(r'static const char\* (\w+) = R"WGSL\((.*?)\)WGSL";',s,re.S))
    prior=load(root,'ray_transport_gpu_6dx_stack_bound').clean_cpp(root,'bound-stack-15')
    require(shader_blocks(generated)==shader_blocks(prior),'coalescing changed a shader')
    # Require exactly the pipelines, bindings and indirect offsets from each old branch,
    # while moving their producer commands under one begin/end pair.
    body=generated.split('        auto execute=',1)[1].split('        auto correctness=',1)[0]
    for candidate,control in [(4,2),(5,3)]:
        chunk=body.split(f'else if(mode=={candidate})',1)[1].split('else if(mode==',1)[0]
        original=body.split(f'else if(mode=={control})',1)[1].split('else if(mode==',1)[0]
        calls=lambda x:re.findall(r'wgpuComputePassEncoder(?:SetPipeline|SetBindGroup|DispatchWorkgroupsIndirect)\([^;]+;',x)
        require(calls(chunk)==calls(original),'candidate pipeline/binding/offset sequence differs')
        require(chunk.count('wgpuCommandEncoderBeginComputePass')==4,'candidate must have reset/classify/args/one producer pass')
        require(original.count('wgpuCommandEncoderBeginComputePass')==6,'control pass structure changed')
    for row in record['fresh_native']:
        source=read(row['source_artifact'])
        require(factory.controls_cpp(root,'bound-stack-15',row['mutation'],row['mode']).encode()==source,'fresh native source reconstruction differs')
        meta=json.loads(read(row['metadata_artifact']))
        require(meta['cpp_sha256']==digest(source) and meta['review_mode']==row['mode'],'fresh source metadata mismatch')
        log=read(row['log_artifact']).decode(errors='replace')
        require(f'REVIEW EXECUTE mode={row["mode"]}' in log,'candidate mode never executed')
        if row['mutation']!='fixed':
            require(row['wrapper_exit_code']==250 and row['expected_failure'] in log,'mutation did not fail the required assertion')
    return {'embedded_shader_blocks_unchanged':len(shader_blocks(generated)),'candidate_dispatch_sequences_match_controls':True}

def replay(root,record_path,archive_path,donor_path):
    record=json.loads(record_path.read_text());require(digest(archive_path.read_bytes())==record['archive_sha256'],'archive digest mismatch');require(digest(donor_path.read_bytes())==record['donor_archive_sha256'],'donor mismatch')
    for name,expected in record['source_sha256'].items():require(digest((root/name).read_bytes())==expected,'source drift: '+name)
    with zipfile.ZipFile(archive_path) as z,zipfile.ZipFile(donor_path) as donor:
        require(set(z.namelist())==set(record['artifacts']),'archive membership differs')
        def read(name):
            data=z.read(name);require(digest(data)==record['artifacts'][name]['sha256'],'artifact drift: '+name);return data
        for name in record['artifacts']:read(name)
        base=json.loads(donor.read('controls/review.json'));du=load(root,'ray_transport_gpu_6dv_review_replay')
        controls={}
        for row in record['controls']:
            require(controls_digest(base,row['modes'],row['scope'])==row['native_sha256'],'control report does not reconstruct byte-for-byte')
            controls[row['label']]={str(mode):du.verify_controls({'cases':control_cases(base,mode)}) for mode in row['modes']}
        dw=load(root,'ray_transport_gpu_6dw_review_replay');dd=load(root,'ray_transport_gpu_6dd_continuation')
        rows=json.loads(donor.read('benchmark/rows.json'));baseline=json.loads(donor.read('benchmark/baseline.json'));rays=json.loads(donor.read('fixture/rays.json'));ref=json.loads(donor.read('fixture/reference.json'))
        timings={};accuracy={}
        for row in record['timings']:
            raw=dw.restore_timing(donor.read('benchmark/second.json'),json.loads(read(row['delta_artifact'])))
            require(digest(raw)==row['native_sha256'],'native timing bytes differ')
            report=json.loads(raw)
            timings[row['label']]=verify_timing(report)
            accuracy[row['label']]=dd.validate_6dd(root,legacy_accuracy_view(report),rows,baseline,rays,ref)
            accuracy[row['label']]['legacy_four_mode_reader_view']=True
        binding=source_binding(root,read,record)
        facts=[json.loads(line) for line in read('capture/facts.jsonl').decode().splitlines()]
        clean=read('native/clean.cpp').decode();blocks=dict(re.findall(r'static const char\* (\w+) = R"WGSL\((.*?)\)WGSL";',clean,re.S))
        expected={digest(blocks[n].encode()) for n in ['kS1UnfusedProducerWgsl','kS1TerminalProducerWgsl']}
        observed={r['wgslSHA256'] for r in facts if r.get('wgslEntry')=='s1ProducerMain'}
        require(expected==observed,'capture shaders differ')
        device=next(r for r in facts if r.get('kind')=='device');require(device['name']=='Apple M5','wrong native adapter')
        tables=[]
        for name in record['artifacts']:
            if name.startswith('tables/'):
                row=dw.decode_table(read(name));require(row is not None,'missing compiler table');tables.append(dict(row,trace_mode=int(name.split('/')[1]),file=name.split('/')[-1]))
        for mode in [0,2,3,4,5]:
            selected=[r for r in tables if r['trace_mode']==mode and r['function']=='s1ProducerMain']
            require(sorted((r['spilled_bytes'],r['temporary_registers'],r['uniform_registers'],r['instructions']) for r in selected)==[(1760,79,72,5093),(1760,83,88,6375)],'S1 compiler fields differ')
        bad=[]
        sample=json.loads(dw.restore_timing(donor.read('benchmark/second.json'),json.loads(read(record['timings'][0]['delta_artifact']))))
        tests=[]
        x=copy.deepcopy(sample);x['clean_timing_series']['pooled']['1']['4']['mean_ms']+=1;tests.append(('candidate mean corruption',lambda:verify_timing(x)))
        y=copy.deepcopy(sample);y['clean_timing_ordered_samples']['series_1']['1'][4]['mode']=2;tests.append(('candidate dispatch-order relabel',lambda:verify_timing(y)))
        tests.append(('native controls digest corruption',lambda:require(controls_digest(base,[4],record['controls'][0]['scope'])=='0'*64,'digest corruption refused')))
        for label,fn in tests:
            try:fn()
            except (AssertionError,ValueError,KeyError):bad.append(label)
            else:raise ValueError('corruption admitted: '+label)
        return {'pass':True,'source_binding':binding,'controls':controls,'timings':timings,'accuracy':accuracy,'compiler_rows':tables,'fresh_native_negatives':record['fresh_native'],'corruptions_rejected':bad,'shipping_gates_open':['profitable connected execution','generic OntoMath/full-frame integration','inhabited FPS/Person acceptance'],'scope':'offline replay of saved 120 cases/3 timing invocations plus this review’s fresh 60 candidate cases/11 targeted negatives; no fresh timing'}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['root','record','archive','donor-archive','output']:p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();v=replay(a.root.resolve(),a.record,a.archive,a.donor_archive);a.output.write_text(json.dumps(v,indent=2)+'\n');print('PASS: source-bound candidate dispatch, lossless raw native evidence, six-mode ordered clocks, reference gates and targeted negatives')
if __name__=='__main__':main()
