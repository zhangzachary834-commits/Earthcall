"""Scratch two-Gauss-node CF4 transport screen; never production activation.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
Human scope: Zach permits a substantial native gain at current-or-better accuracy.
Method: https://doi.org/10.1137/05063042 (two-exponential CF4).
Mathematical promotion belongs in OntoMath; this isolated consumer only screens
cost and exposes an unqualified candidate for Opus's independent reference.
Negative CF4 weights, nonsmooth fields, coarse panels and cutoff are NOT admitted.
"""
import argparse, ast, json, math, shlex, subprocess
from pathlib import Path


def flow(y, sigma, source, h):
    x = sigma*h
    gain = h*(-math.expm1(-x)/x if x else 1.0)
    return (y[0]*math.exp(-x), *(y[i+1]+y[0]*source[i]*gain for i in range(3)))


def cf4(field, n):
    y=(1., 0., 0., 0.)
    a=.25-math.sqrt(3)/6
    b=.25+math.sqrt(3)/6
    for k in range(n):
        s1,e1=field((k+.5-math.sqrt(3)/6)/n)
        s2,e2=field((k+.5+math.sqrt(3)/6)/n)
        for w1,w2 in ((b,a),(a,b)):
            y=flow(y,w1*s1+w2*s2,tuple(w1*x+w2*z for x,z in zip(e1,e2)),1/n)
    return y


def witness():
    # Different colour sources and variable attenuation: commuting/proportional
    # tests alone cannot detect reversal of the two exponentials.
    field=lambda s:(.3+.2*s+.1*s*s, (.2+s*s,.1+.4*s,math.exp(.3*s)))
    def rhs(t,y):
        s,e=field(t)
        return (-s*y[0], *(y[0]*v for v in e))
    y=(1.,0.,0.,0.);n=32768;h=1/n
    for k in range(n):
        t=k*h;k1=rhs(t,y)
        k2=rhs(t+h/2,tuple(a+h*b/2 for a,b in zip(y,k1)))
        k3=rhs(t+h/2,tuple(a+h*b/2 for a,b in zip(y,k2)))
        k4=rhs(t+h,tuple(a+h*b for a,b in zip(y,k3)))
        y=tuple(a+h*(b+2*c+2*d+e)/6 for a,b,c,d,e in zip(y,k1,k2,k3,k4))
    errors=[max(abs(a-b) for a,b in zip(cf4(field,n),y)) for n in (2,4,8,16)]
    ratios=[errors[i]/errors[i+1] for i in range(3)]
    assert all(14 < r < 18 for r in ratios), (errors,ratios)
    constant=cf4(lambda s:(.7,(.2,.3,.4)),3)
    exact=flow((1.,0.,0.,0.),.7,(.2,.3,.4),1.)
    assert max(abs(a-b) for a,b in zip(constant,exact))<1e-14
    vacuum=cf4(lambda s:(0.,(s,s*s,1.)),2)
    assert max(abs(a-b) for a,b in zip(vacuum,(1.,.5,1/3,1.)))<1e-14
    # Explicit anti-admission witness: two zero nodes do not prove an empty
    # interval. A thin positive slab between them is missed completely.
    thin=cf4(lambda s:(1.,(.2,.3,.4)) if .48<s<.52 else (0.,(0.,0.,0.)),1)
    thin_exact=math.exp(-.04)
    midpoint=math.exp(-sum(1/96 for k in range(96) if .48<(k+.5)/96<.52))
    assert abs(thin[0]-thin_exact)>20*abs(midpoint-thin_exact)
    return {'smooth_nonproportional_errors':errors,'halving_ratios':ratios,
            'constant_exact':True,'zero_extinction_limit':True,
            'thin_slab_refuses_blind_coarsening':{'cf4_T_error':abs(thin[0]-thin_exact),'midpoint96_T_error':abs(midpoint-thin_exact)},
            'scope':'manufactured CPU rule witnesses, not authored native accuracy'}


def build(root, out, capture=False, transform_filter=None, capture_storage_multiplier=1, capture_grid=(64,36)):
    root=root.resolve();out=out.resolve()
    assert out!=root and root not in out.parents, 'scratch output must be outside repository'
    assert 1<=capture_storage_multiplier<=4, 'bounded scratch capture storage only'
    assert capture_grid in ((64,36),(128,72),(256,144)), 'bounded native capture grids only'
    b=root/'build';out.mkdir(parents=True,exist_ok=False)
    (out/'rule-witness.json').write_text(json.dumps(witness(),indent=2)+'\n')
    transform=r'''
static std::string cf4Shader(std::string wgsl){
 if(wgsl.find("fn volumeShared_3_G0(")==std::string::npos)return wgsl;
 if(std::getenv("EC_CF4_BASELINE"))return wgsl;
 if(const char* dump=std::getenv("EC_CF4_ORIGINAL")){if(FILE* f=std::fopen(dump,"w")){std::fwrite(wgsl.data(),1,wgsl.size(),f);std::fclose(f);}}
 unsigned group=3u;if(const char* v=std::getenv("EC_CF4_GROUP"))group=std::atoi(v);
 if(group<1u || group>8u)std::abort();
 auto replace=[&](const std::string& a,const std::string& b){size_t at=0;unsigned n=0;while((at=wgsl.find(a,at))!=std::string::npos){wgsl.replace(at,a.size(),b);at+=b.size();++n;}if(!n){std::fprintf(stderr,"CF4 missing hook: %s\n",a.c_str());std::abort();}};
 // The old quality setting still defines the candidate panel scale. This is
 // an alternative quadrature algorithm, not a changed persisted sample knob.
 replace("let segmentSamples = clamp(i32(ceil(segmentSpan / segmentSpacing)), 1, samplesPerChord);",
 "let oldSamples = clamp(i32(ceil(segmentSpan / segmentSpacing)), 1, samplesPerChord);\nlet segmentSamples = max(1, (oldSamples + "+std::to_string(group-1)+") / "+std::to_string(group)+");");
 if(wgsl.find("let firstSample = i32(clamp(ceil(")!=std::string::npos)replace("let firstSample = i32(clamp(ceil((supportStart - segmentStart) / stepLength - 0.5) - 1.0,",
         "let firstSample = i32(clamp(floor((supportStart - segmentStart) / stepLength) - 1.0,");
 const auto begin=wgsl.find("        for (var step = firstSample;");
 const auto alt=wgsl.find("        for (var step = 0; step < segmentSamples;");
 const auto at=begin!=std::string::npos?begin:alt;
 if(at==std::string::npos)std::abort();
 const auto world=wgsl.find("            let worldP = ro + rd * sampleT;",at);
 const std::string first=begin!=std::string::npos?"firstSample":"0";
 wgsl.replace(at,world-at,"        for (var step = "+first+R"WGSL(; step < segmentSamples; step = step + 1) {
            let panelStart=segmentStart+f32(step)*stepLength;
            if(panelStart>supportEnd){break;}
            var nodeSigma=vec2<f32>(0.0);
            var nodeSource:array<vec3<f32>,2>;
            var panelProven=true;
            var panelExit=segmentEnd;
            for(var node=0u;node<2u;node+=1u){
            let fraction=select(0.2113248654051871,0.7886751345948129,node==1u);
            let sampleT=panelStart+fraction*stepLength;
)WGSL");
 replace(R"WGSL(            if (walkAllProven) {
                step = max(step, i32(ceil((walkExit - segmentStart) / stepLength - 0.5)) - 1);
                continue;
            })WGSL",R"WGSL(
            panelProven=panelProven && walkAllProven;
            panelExit=min(panelExit,walkExit);
            nodeSigma[node]=totalExtinction;
            nodeSource[node]=totalSource;
            }
            if(panelProven){
                step=max(step,i32(floor((panelExit-segmentStart)/stepLength))-1);
                continue;
            }
)WGSL");
 const auto tb=wgsl.find("        let oldT = transmittance;",at);
 const auto te=wgsl.find("            if (u.volumeOutputError.y == 0.0 && transmittance < 0.01)",tb);
 if(tb==std::string::npos || te==std::string::npos)std::abort();
 wgsl.replace(tb,te-tb,R"WGSL(
        // CF4 on y=(T,Lrgb): A(s)y=(-sigma*T, source*T).
        // Order matters for varying source/sigma. Both sampled sources are
        // already the sum of all projected/gated curtains at that position.
        for(var stage=0u;stage<2u;stage+=1u){
            let w1=select(0.5386751345948129,-0.0386751345948129,stage==1u);
            let w2=0.5-w1;
            let sigma=w1*nodeSigma.x+w2*nodeSigma.y;
            let source=w1*nodeSource[0]+w2*nodeSource[1];
            let x=sigma*stepLength;
            let atten=exp(-x);
            var gain=stepLength;
            if(abs(x)<0.001){
                gain*=1.0+x*(-0.5+x*(0.1666666666666667+x*(-0.0416666666666667+x*0.0083333333333333)));
            }else{gain*=(1.0-atten)/x;}
            integratedRadiance+=transmittance*source*gain;
            transmittance*=atten;
        }
)WGSL");
 if(wgsl.find("var supportEnd")==std::string::npos)replace("if(panelStart>supportEnd)","if(panelStart>segmentEnd)");
 std::fprintf(stderr,"CF4 ACTIVE group=%u; UNQUALIFIED accuracy/fallback/lifecycle\n",group);
 if(const char* dump=std::getenv("EC_CF4_WGSL")){if(FILE* f=std::fopen(dump,"w")){std::fwrite(wgsl.data(),1,wgsl.size(),f);std::fclose(f);}}
 return wgsl;
}
'''
    if transform_filter is not None:
        transform=transform_filter(transform)
    renderer=(root/'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp').read_text()
    if capture:
        old=(root/'scripts/northern_veil_fragment_demand.py').read_text()
        helper=next(ast.literal_eval(n.value) for n in ast.parse(old).body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='helper' for t in n.targets))
        helper=helper[:helper.index('static std::string censusShader')]
        if capture_storage_multiplier != 1:
            helper=helper.replace('64u * 36u * 64u * sizeof(uint32_t)',f'64u * 36u * {64*capture_storage_multiplier}u * sizeof(uint32_t)')
        helper+=r'''
static std::string captureShader(std::string wgsl){
 if(wgsl.find("fn volumeShared_3_G0(")==std::string::npos)return wgsl;
 auto replace=[&](const std::string& a,const std::string& b){const auto at=wgsl.find(a);if(at==std::string::npos)std::abort();wgsl.replace(at,a.size(),b);};
 wgsl="@group(0) @binding(3) var<storage,read_write> census:array<u32>;\nvar<private> captureSlot:u32;\n"+wgsl;
 replace("fn fs(in: VolumeVSOut) -> @location(0) vec4<f32> {",R"WGSL(fn fs(in: VolumeVSOut) -> @location(0) vec4<f32> {
 let dc=vec2<u32>(floor(in.position.xy*vec2<f32>(64.0,36.0)/u.viewport.xy));
 let centre=vec2<u32>(floor((vec2<f32>(dc)+0.5)*u.viewport.xy/vec2<f32>(64.0,36.0)));
 if(any(vec2<u32>(floor(in.position.xy))!=centre)||any(dc>=vec2<u32>(64u,36u))){discard;}
 captureSlot=(dc.x+64u*dc.y)*64u;
 census[captureSlot]=1u;census[captureSlot+1u]=centre.x;census[captureSlot+2u]=centre.y;
 census[captureSlot+12u]=bitcast<u32>(1.0);
)WGSL");
 replace("let rd = normalize(farWorld - ro);",R"WGSL(let rd = normalize(farWorld - ro);
 census[captureSlot+4u]=bitcast<u32>(ro.x);census[captureSlot+5u]=bitcast<u32>(ro.y);census[captureSlot+6u]=bitcast<u32>(ro.z);
 census[captureSlot+8u]=bitcast<u32>(rd.x);census[captureSlot+9u]=bitcast<u32>(rd.y);census[captureSlot+10u]=bitcast<u32>(rd.z);
)WGSL");
 replace("var transmittance = 1.0;",R"WGSL(census[captureSlot+7u]=bitcast<u32>(t0);census[captureSlot+11u]=bitcast<u32>(t1);
 var transmittance = 1.0;)WGSL");
 replace("    let alpha = 1.0 - transmittance;",R"WGSL(
 census[captureSlot+3u]=1u;
 census[captureSlot+12u]=bitcast<u32>(transmittance);
 census[captureSlot+13u]=bitcast<u32>(integratedRadiance.x);census[captureSlot+14u]=bitcast<u32>(integratedRadiance.y);census[captureSlot+15u]=bitcast<u32>(integratedRadiance.z);
 let alpha = 1.0 - transmittance;)WGSL");
 return wgsl;
}
'''
        gx,gy=capture_grid
        helper=helper.replace('64u * 36u * ',f'{gx}u * {gy}u * ')
        helper=helper.replace('vec2<f32>(64.0,36.0)',f'vec2<f32>({gx}.0,{gy}.0)')
        helper=helper.replace('vec2<u32>(64u,36u)',f'vec2<u32>({gx}u,{gy}u)')
        helper=helper.replace('dc.x+64u*dc.y',f'dc.x+{gx}u*dc.y')
        transform=helper+transform
    renderer=renderer.replace('namespace {\n',transform+'\nnamespace {\n',1)
    start=renderer.index('WebGpuRenderer::volumePipeline(')
    renderer=renderer[:start]+renderer[start:].replace('    const auto& wgsl = program.wgsl;',
        '    const auto wgsl = '+('captureShader(cf4Shader(program.wgsl))' if capture else 'cf4Shader(program.wgsl)')+';',1)
    if capture:
        start=renderer.index('WebGpuRenderer::volumePipeline(');end=renderer.index('WebGpuRenderer::',start+30);part=renderer[start:end]
        part=part.replace('globalEntries[3]','globalEntries[4]').replace('globalDesc.entryCount = 3;', 'globalEntries[3].binding=3;globalEntries[3].visibility=WGPUShaderStage_Fragment;globalEntries[3].buffer.type=WGPUBufferBindingType_Storage;globalEntries[3].buffer.minBindingSize=censusBytes;globalDesc.entryCount=4;')
        renderer=renderer[:start]+part+renderer[end:];start=renderer.index('void WebGpuRenderer::flushVolumeComposite(');part=renderer[start:]
        part=part.replace('WGPUBindGroupEntry globalEntries[3] = {};','WGPUBindGroupEntry globalEntries[4] = {};').replace('globalBgDesc.entryCount = 3;','censusReset(_device,_queue);globalEntries[3].binding=3;globalEntries[3].buffer=censusBuffer;globalEntries[3].size=censusBytes;globalBgDesc.entryCount=4;',1)
        renderer=renderer[:start]+part
    src=out/'cf4-renderer.cpp';src.write_text(renderer)
    db=json.loads((b/'compile_commands.json').read_text())
    c=next(x for x in db if x['file'].endswith('/WebGpuRenderer.cpp') and 'earthcall_webgpu.dir' in x['command'])
    args=shlex.split(c['command']);args[args.index('-c')+1]=str(src);args[args.index('-o')+1]=str(out/'cf4-renderer.o')
    subprocess.run(args,cwd=b,check=True)
    # The probe itself is compiled freshly from the corrected authored-source fixture.
    c=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    probe=(root/'scratch/probes/volume_demand_probe.cpp').read_text()
    probe=probe.replace('    renderer.setLightingEnabled(light.enabled);','    renderer.setLightingEnabled(light.enabled);\n    if(const char* n=std::getenv("EC_REFERENCE_SAMPLES"))renderer.setVolumeSamplesPerChord(std::atoi(n));')
    (out/'probe.cpp').write_text(probe)
    args=shlex.split(c['command']);args[args.index('-c')+1]=str(out/'probe.cpp');args[args.index('-o')+1]=str(out/'probe.o')
    if capture: args.append('-DEC_FRAGMENT_CENSUS')
    subprocess.run(args,cwd=b,check=True)
    args=shlex.split((b/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    for suffix,obj in [('/src/entry.cpp.o',out/'probe.o'),('/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp.o',out/'cf4-renderer.o')]:
        args[next(i for i,x in enumerate(args) if x.endswith(suffix))]=str(obj)
    args[args.index('-o')+1]=str(out/'cf4');subprocess.run(args,cwd=b,check=True)
    (out/'preflight.txt').write_text(__doc__+'\nTwo nodal field sums per panel, two analytic triangular exponentials. Existing source/medium evals and zero proof packets retained. Node proof skips only all-zero quadrature panels; no clairvoyant support search. Fourth order only for sufficiently smooth coefficients: no such global claim for clamps/noise. Panel group >=3 reduces nominal nodes; >=1 enables convergence experiments. Signed stage source/extinction and changed cutoff remain unqualified. No production activation or fidelity claim.\n')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[2])
    p.add_argument('--output',type=Path)
    p.add_argument('--witness-only',action='store_true')
    p.add_argument('--capture',action='store_true')
    a=p.parse_args()
    if a.witness_only: print(json.dumps(witness(),indent=2))
    else:
        assert a.output and a.root.resolve() not in a.output.resolve().parents
        build(a.root.resolve(),a.output.resolve(),a.capture)
