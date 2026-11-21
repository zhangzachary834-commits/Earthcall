"""§6dw: qualify actual routed modes, then isolate pending DFS stack lowering.

Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-26.
Scratch-only diagnostics. No changed equations, budgets, capacities or clean timing.
"""
import argparse,ast,hashlib,importlib.util,json,os,re,shlex,subprocess,sys
from pathlib import Path

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def load(root,name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

def scalar_stack(shader):
    declaration='var stack: array<vec4f, 20>;'
    assert shader.count(declaration)==1
    assert set(re.findall(r'stack\[([^]]+)\]',shader))=={'0','1','top','top + 1u'}
    shader=shader.replace(declaration,'\n'.join(f'var pending{i}: vec4f;' for i in range(20)))
    shader=shader.replace('stack[0] =','pending0 =').replace('stack[1] =','pending1 =')
    read='let cell = stack[top];'
    assert shader.count(read)==1
    select='var cell = vec4f(0.0); switch(top) {\n'+''.join(f'case {i}u: {{cell=pending{i};}}\n' for i in range(20))+'default: {*reason |= 4u;} } if(*reason != 0u){break;}'
    shader=shader.replace(read,select)
    for index,selector in [('top','top'),('top + 1u','top + 1u')]:
        pattern=r'stack\['+re.escape(index)+r'\] = ([^;]+);'
        matches=list(re.finditer(pattern,shader));assert len(matches)==1
        expr=matches[0][1]
        switch='switch('+selector+') {\n'+''.join(f'case {i}u: {{pending{i}={expr};}}\n' for i in range(20))+'default: {*reason |= 4u;} }'
        shader=shader[:matches[0].start()]+switch+shader[matches[0].end():]
    assert 'stack[' not in shader
    return shader

def apply_variant(cpp,variant):
    if variant=='canonical':return cpp
    # Change only S1's pending DFS storage in the unfused and terminal pipelines.
    names=['kS1UnfusedProducerWgsl','kS1TerminalProducerWgsl' if 'kS1TerminalProducerWgsl = R"WGSL(' in cpp else 'kS1ProducerWgsl']
    for name in names:
        match=re.search(r'(static const char\* '+name+r' = R"WGSL\()(.*?)(\)WGSL";)',cpp,re.S);assert match,name
        cpp=cpp[:match.start(2)]+scalar_stack(match[2])+cpp[match.end(2):]
    return cpp

def clean_cpp(root,variant):
    base=load(root,'ray_transport_gpu_6du_true_terminal_s1')
    constants={x.targets[0].id:ast.literal_eval(x.value) for x in ast.parse((root/'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x,ast.Assign) and isinstance(x.value,ast.Constant) and isinstance(x.value.value,str)}
    return apply_variant(base.make_terminal_s1_cpp_source(constants['CPP_SOURCE'],root),variant)

def controls_cpp(root,variant):
    # Reuse the qualified instrumentation over the actual §6du two-mode host,
    # with explicit asserted adaptation points. Neither clean shader math nor
    # the dispatch graph is reimplemented in a separate toy harness.
    path=root/'scratch/probes/ray_transport_gpu_6du_controls.py'
    text=path.read_text()
    target="root / 'scratch/probes/ray_transport_gpu_6dt_true_terminal_s1.py'"
    assert text.count(target)==1
    text=text.replace(target,"root / 'scratch/probes/ray_transport_gpu_6du_true_terminal_s1.py'")
    hook="source = m.make_terminal_s1_cpp_source(v['CPP_SOURCE'], root, wide=True)"
    assert text.count(hook)==1
    text=text.replace(hook,hook+".replace('kS1TerminalProducerWgsl', 'kS1ProducerWgsl')")
    text=text.replace("for name in ['kS2ProducerWgsl', 'kFallbackProducerWgsl']:","for name in ['kS1UnfusedProducerWgsl', 'kS2ProducerWgsl', 'kFallbackProducerWgsl']:")
    text=text.replace('assert source.count(needle_bg_s2_fb) == 4','assert source.count(needle_bg_s2_fb) == 6')
    text=text.replace('if(mode==3) wgpuCommandEncoderClearBuffer','if(mode==2||mode==3) wgpuCommandEncoderClearBuffer')
    ns={'__name__':'adapted_6du_controls'};exec(compile(text,str(path),'exec'),ns)
    branch=ns['BRANCH']
    start='        auto bitsEqual='
    assert branch.count(start)==1
    branch=branch.replace(start,'        for(int reviewMode:{2,3}){\n'+start)
    branch=branch.replace('execute(3,rep,false);','execute(reviewMode,rep,false);')
    branch=branch.replace('routeChecks.push_back({{"case",label}', 'routeChecks.push_back({{"mode",reviewMode},{"case",label}')
    branch=branch.replace('        std::ofstream reviewOut(outputPath);','        }\n        std::ofstream reviewOut(outputPath);')
    branch=branch.replace('§6du True Terminal S1 routed correctness and ownership controls','§6dw actual Mode2 and Mode3 routed correctness and ownership controls')
    branch=branch.replace('std::cerr<<"ROUTED REVIEW "<<label', 'std::cerr<<"ROUTED REVIEW mode="<<reviewMode<<" "<<label')
    branch=branch.replace('(0 sBuf writes verified)', '(completed-S1 poison check only)')
    ns['BRANCH']=branch
    return apply_variant(ns['generate'](root,'fixed'),variant)

def capture_cpp(cpp,work):
    main='int main(int argc, char** argv) {';assert cpp.count(main)==1
    cpp=cpp.replace(main,'extern "C" void rtInstallMetalObserver();\nextern "C" void rtBeginCapture(const char*);\nextern "C" void rtEndCapture();\nextern "C" void rtMetalSetContext(const char*,const char*,size_t);\n'+main+'\n rtInstallMetalObserver();')
    for needle,prefix in [
        ('WGPUComputePipeline cp = wgpuDeviceCreateComputePipeline(gpu.device, &cpd);','rtMetalSetContext("rayMain",fullCode.data(),fullCode.size());\n'),
        ('auto pipeline = wgpuDeviceCreateComputePipeline(gpu.device, &pd);','rtMetalSetContext(entry,text.data(),text.size());\n'),
        ('auto pipeline=wgpuDeviceCreateComputePipeline(gpu.device,&pd);','rtMetalSetContext(entry,text.data(),text.size());\n')]:
        assert cpp.count(needle)==1;cpp=cpp.replace(needle,prefix+needle)
    hook='        auto correctness=';assert cpp.count(hook)==1
    captures=''
    for mode in [0,2,3]:
        captures+=f'        rtBeginCapture("{work}/mode{mode}.gputrace"); execute({mode},1,false); rtEndCapture();\n'
    return cpp.replace(hook,captures+'        return 0;\n'+hook)

def build_capture(root,build,work,source):
    trees=[]
    for checkout in [root,build]:
        assert not subprocess.check_output(['git','-c','core.fsmonitor=false','status','--porcelain','--','src'],cwd=checkout,text=True).strip(),'dirty engine source'
        trees.append(subprocess.check_output(['git','rev-parse','HEAD:src'],cwd=checkout,text=True).strip())
    assert trees[0]==trees[1],'source/build tree mismatch'
    b=build/'build';observer=root/'scratch/probes/ray_transport_metal_pipeline_observer.mm'
    subprocess.run(['/usr/bin/clang++','-std=c++20','-O2','-fobjc-arc','-c',str(observer),'-o',str(work/'observer.o')],check=True)
    db=json.loads((b/'compile_commands.json').read_text());entry=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    cmd=shlex.split(entry['command']);cmd[cmd.index('-c')+1]=str(source);cmd[cmd.index('-o')+1]=str(work/'probe.o');cmd+=['-std=c++20','-O3','-I'+str(root/'scratch/probes')];subprocess.run(cmd,cwd=b,check=True)
    cmd=shlex.split((b/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());cmd[next(i for i,x in enumerate(cmd) if x.endswith('/src/entry.cpp.o'))]=str(work/'probe.o');cmd[cmd.index('-o')+1]=str(work/'probe');cmd += [str(work/'observer.o'),'-framework','Metal','-framework','Foundation'];subprocess.run(cmd,cwd=b,check=True)
    return work/'probe'

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--build-root',type=Path,required=True);p.add_argument('--work',type=Path,required=True);p.add_argument('--variant',choices=['canonical','scalar-stack'],required=True);p.add_argument('--capture',action='store_true');p.add_argument('--timing',action='store_true');p.add_argument('--run',action='store_true');a=p.parse_args()
    root=a.root.resolve();build=a.build_root.resolve();work=a.work.resolve()
    if work.exists() or work==root or root in work.parents:raise ValueError('fresh external work required')
    work.mkdir(parents=True,exist_ok=False)
    if a.capture and a.timing:raise ValueError('capture and clean timing are separate runs')
    cpp=capture_cpp(clean_cpp(root,a.variant),work) if a.capture else clean_cpp(root,a.variant) if a.timing else controls_cpp(root,a.variant)
    source=work/'probe.cpp';source.write_text(cpp)
    (work/'source.json').write_text(json.dumps({'variant':a.variant,'capture':a.capture,'timing':a.timing,'cpp_sha256':sha(source),'engine_src_tree':subprocess.check_output(['git','rev-parse','HEAD:src'],cwd=root,text=True).strip()},indent=2)+'\n')
    if a.capture:binary=build_capture(root,build,work,source)
    else:
        subprocess.run([sys.executable,'-B',str(root/'scratch/probes/ray_transport_probe_build.py'),'--root',str(root),'--build-root',str(build),'--source',str(source),'--work',str(work/'build')],check=True);binary=work/'build/probe'
    if a.run:
        wide=load(root,'ray_transport_gpu_wide_stream');zone=root/'saves/zones/Northern Veil/zone.ecform';before=sha(zone);meta,rays,_=wide.verify_case(root,Path('/private/tmp/earthcall-wide-case/manifest.json'),before)
        env=dict(os.environ,EC_POSE_RAYS=meta['rays'],EC_POSE_REFERENCE=meta['reference'],EC_POSE_CLOCK=str(rays['time']),EC_MATCH_REPEAT='1',EC_GPU_BUDGET='1024',EC_GPU_SUB_CAP='64',EC_GPU_OVER_CAP='16',EC_GPU_MARGIN='.001')
        env.pop('EC_GPU_GEOMETRY_CASE',None);env.pop('METAL_CAPTURE_ENABLED',None)
        if a.capture:env.update(METAL_CAPTURE_ENABLED='1',EC_RAY_PIPELINE_FACTS=str(work/'pipeline-facts.jsonl'))
        with (work/'native.log').open('xb') as f:result=subprocess.run([str(binary),str(work/'result.json')],cwd=root,env=env,stdout=f,stderr=subprocess.STDOUT)
        assert sha(zone)==before,'fixture drift'
        if result.returncode:raise SystemExit(result.returncode)
        if a.timing:
            report=json.loads((work/'result.json').read_text());rows=json.loads((work/'result.json-rows.json').read_text());baseline=json.loads((work/'result.json-baseline-rows.json').read_text());ref=json.loads(Path(meta['reference']).read_text())
            checked=load(root,'ray_transport_gpu_6dd_continuation').validate_6dd(root,report,rows,baseline,rays,ref)
            (work/'independent.json').write_text(json.dumps(checked,indent=2)+'\n')
            for rep,modes in report['clean_timing_series']['pooled'].items():
                current=modes['0'];print('REPEAT',rep,'Current mean/median',current['mean_ms'],current['median_ms'])
                for mode in ['2','3']:
                    candidate=modes[mode];print('MODE',mode,'mean/median percent',100*(candidate['mean_ms']/current['mean_ms']-1),100*(candidate['median_ms']/current['median_ms']-1))
            print('PASS clean fixture/reference gates; one native timing invocation, two internal ordered blocks')
        elif not a.capture:
            data=json.loads((work/'result.json').read_text());assert len(data['cases'])==60 and [c['mode'] for c in data['cases']]==[2]*30+[3]*30
            print('PASS',a.variant,'actual routed modes: 60 native cases')
        else:print('CAPTURED',a.variant,'modes 0/2/3; not a clean cost witness')

if __name__=='__main__':main()
