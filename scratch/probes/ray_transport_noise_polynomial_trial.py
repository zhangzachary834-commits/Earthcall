"""Scratch affine-ray, fixed-cell Perlin value recipe and native economic screen.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
Human origin: Zach's next-pass commission after Astra's section 6bq review.
Generic real polynomial derivation; no production mathematics or activation.
Screen promotion belongs in OntoMath, device lowering in Screen. Numerical
execution changes association and rounded coordinates; it is NOT a bit cache.
"""
import argparse, json, math, random, sys
from pathlib import Path


class Recipe:
    """Small expression DAG for the generic cell-local value identity only."""
    def __init__(self):
        self.nodes=[]; self.memo={}
    def op(self,op,a,b):
        if op=='+':
            if a==0:return b
            if b==0:return a
        if op=='-':
            if b==0:return a
            if a==b:return 0
        if op=='*':
            if a==0 or b==0:return 0
            if a==1:return b
            if b==1:return a
        if isinstance(a,(int,float)) and isinstance(b,(int,float)):
            return {'+':lambda:a+b,'-':lambda:a-b,'*':lambda:a*b}[op]()
        key=(op,a,b)
        if key not in self.memo:
            name='r'+str(len(self.nodes));self.memo[key]=name;self.nodes.append((name,op,a,b))
        return self.memo[key]
    def add(self,a,b):return self.op('+',a,b)
    def sub(self,a,b):return self.op('-',a,b)
    def mul(self,a,b):return self.op('*',a,b)
    def fade(self,q,v):
        A,S,M=self.add,self.sub,self.mul
        q2=M(q,q);q3=M(q2,q);qm=S(q,1);v2=M(v,v);v3=M(v2,v);v4=M(v3,v);v5=M(v4,v)
        return [M(q3,A(M(q,S(M(q,6),15)),10)),M(M(M(30,q2),M(qm,qm)),v),M(M(M(M(30,q),qm),S(M(2,q),1)),v2),M(M(10,A(S(M(6,q2),M(6,q)),1)),v3),M(M(15,S(M(2,q),1)),v4),M(6,v5)]
    def mix(self,a,b,t):
        A,S,M=self.add,self.sub,self.mul
        d=[S(y,x) for x,y in zip(a,b)];c=[0]*(len(a)+len(t)-1)
        for i,x in enumerate(d):
            for j,y in enumerate(t):c[i+j]=A(c[i+j],M(x,y))
        for i,x in enumerate(a):c[i]=A(c[i],x)
        return c
    def build(self):
        f=[self.fade('q.'+k,'v.'+k) for k in 'xyz']
        # Original intrinsic mixes z, then y, then x. Gradients are constants
        # within a fixed cell; each corner dot along the local ray is affine.
        corners={n:['a'+n,'b'+n] for n in ['000','100','010','110','001','101','011','111']}
        z=[self.mix(corners[n+'0'],corners[n+'1'],f[2]) for n in ['00','10','01','11']]
        y=[self.mix(z[0],z[2],f[1]),self.mix(z[1],z[3],f[1])]
        self.result=[self.mul(2.2,x) for x in self.mix(y[0],y[1],f[0])]
        return self
    @staticmethod
    def text(a):return str(float(a)) if isinstance(a,(int,float)) else a
    def wgsl(self):
        t=self.text
        lines=[f'let {n}={t(a)} {op} {t(b)};' for n,op,a,b in self.nodes]
        lines += [f'polyCoefficients[{i}u]={t(x)};' for i,x in enumerate(self.result)]
        return '\n'.join(lines)
    def evaluate(self,inputs):
        values=dict(inputs)
        def v(a):return a if isinstance(a,(int,float)) else values[a]
        for n,op,a,b in self.nodes:
            a,b=v(a),v(b);values[n]={'+':lambda:a+b,'-':lambda:a-b,'*':lambda:a*b}[op]()
        return [v(x) for x in self.result]


def witness(recipe):
    # Independent direct corner interpolation versus derived polynomial. This
    # is a double-arithmetic algebra witness, not a native numerical contract.
    rng=random.Random(6516);worst=0
    def fade(x):return x*x*x*(x*(6*x-15)+10)
    def mix(a,b,t):return a+(b-a)*t
    for trial in range(512):
        q=[rng.uniform(.02,.98) for _ in range(3)];v=[rng.uniform(-1,1) for _ in range(3)]
        if trial<8:v=[0 if (trial>>k)&1 else .6 for k in range(3)]
        gradients={n:[rng.uniform(-1,1) for _ in range(3)] for n in ['000','100','010','110','001','101','011','111']}
        inputs={**{'q.'+k:q[i] for i,k in enumerate('xyz')},**{'v.'+k:v[i] for i,k in enumerate('xyz')}}
        for n,g in gradients.items():
            inputs['a'+n]=sum(g[i]*(q[i]-int(n[i])) for i in range(3));inputs['b'+n]=sum(g[i]*v[i] for i in range(3))
        coefficients=recipe.evaluate(inputs)
        end=min([(1-q[i])/v[i] if v[i]>0 else -q[i]/v[i] for i in range(3) if v[i]!=0]+[1.])
        for k in range(17):
            u=end*k/16;p=[q[i]+v[i]*u for i in range(3)]
            dots={n:sum(g[i]*(p[i]-int(n[i])) for i in range(3)) for n,g in gradients.items()}
            z=[mix(dots[n+'0'],dots[n+'1'],fade(p[2])) for n in ['00','10','01','11']]
            direct=2.2*mix(mix(z[0],z[2],fade(p[1])),mix(z[1],z[3],fade(p[1])),fade(p[0]))
            value=coefficients[-1]
            for c in coefficients[-2::-1]:value=value*u+c
            worst=max(worst,abs(value-direct))
    assert worst<1e-10,worst
    counts={op:sum(n[1]==op for n in recipe.nodes) for op in ['+','-','*']}
    return dict(cases=512*17,max_double_error=worst,degree=16,coefficients=17,coefficient_DAG_scalar_operations=counts,extra_corner_dots=16,horner_stages=16,scope='real-polynomial algebra witness, not f32 admission; gradients/hash/domain costs additional')


def transform(recipe,mode,capture,compare_noise=False):
    # The host only recognizes this frozen affine call shape. A production
    # emitter would ask OntoMath for affine binding and a reusable value recipe.
    cpp=r'''
static std::string cf4Shader(std::string wgsl){
 if(wgsl.find("fn volumeShared_3_G0(")==std::string::npos)return wgsl;
 if(std::getenv("EC_POLY_BASELINE"))return wgsl;
 auto sub=[](std::string& t,const std::string& a,const std::string& b){auto p=t.find(a);if(p==std::string::npos){std::fprintf(stderr,"poly missing hook: %s\n",a.c_str());std::abort();}t.replace(p,a.size(),b);};
 const auto at=wgsl.find("fn cnoise3(P: vec3<f32>) -> f32 {");const auto end=wgsl.find("\n}",at);if(at==std::string::npos||end==std::string::npos)std::abort();
 const auto dot=wgsl.find("    let n000 = dot(g000, Pf0);",at);if(dot==std::string::npos||dot>end)std::abort();
 std::string originalGradient=wgsl.substr(at+std::string("fn cnoise3(P: vec3<f32>) -> f32 {").size(),dot-at-std::string("fn cnoise3(P: vec3<f32>) -> f32 {").size());
 std::string helper=R"WGSL(
// Kernel-only invocation state. Ray identity is the current invocation; call
// identity binds immutable origin/frequency parameters, so (call,cell) is the
// full affine-binding key here. No state survives a ray, frame or device.
var<private> polyRayDir:vec3<f32>;
var<private> polySampleT:f32;
var<private> polyKey:vec4<u32>;
var<private> polyValid:bool;
var<private> polyAnchorT:f32;
var<private> polyRate:f32;
var<private> polyQ:vec3<f32>;
var<private> polyV:vec3<f32>;
DECLARATIONS
fn polyPrepare(P:vec3<f32>, v:vec3<f32>){
GRADIENTS
let q=Pf0;
CORNER_DOTS
COEFFICIENTS
}
fn polyNoise(P:vec3<f32>, velocity:vec3<f32>, binding:u32)->f32{
 let cell=floor(P);
 let rate=max(max(abs(velocity.x),abs(velocity.y)),abs(velocity.z));
 let eligible=all(abs(cell)<vec3<f32>(1048576.0))&&rate>0.0&&rate<1e20;
 let key=vec4<u32>(bitcast<vec3<u32>>(cell),binding);
 let same=polyValid&&all(key==polyKey);
 let u=(polySampleT-polyAnchorT)*polyRate;
 let model=polyQ+polyV*u;
 // Numerical candidate: both actual and reconstructed f32 positions must
 // remain in the prepared cell. This is not a real-arithmetic error proof.
 let hit=eligible&&same&&u>=0.0&&all(model>=vec3<f32>(0.0))&&all(model<vec3<f32>(1.0));
 COUNTERS
 if(!eligible){return cnoise3(P);}
 if(!hit){
   ON_MISS
   polyKey=key;polyValid=true;polyAnchorT=polySampleT;polyRate=rate;
   polyQ=fract(P);polyV=velocity/rate;
   PREPARE
 }
 EVALUATE
}
)WGSL";
 sub(helper,"GRADIENTS",originalGradient);
 sub(helper,"DECLARATIONS",R"WGSL(DECLARATIONS_TEXT)WGSL");
 sub(helper,"CORNER_DOTS",R"WGSL(DOTS_TEXT)WGSL");
 sub(helper,"COEFFICIENTS",R"WGSL(COEFFICIENTS_TEXT)WGSL");
 sub(helper,"COUNTERS",R"WGSL(COUNTERS_TEXT)WGSL");
 sub(helper,"ON_MISS",R"WGSL(MISS_TEXT)WGSL");
 sub(helper,"PREPARE",R"WGSL(PREPARE_TEXT)WGSL");
 sub(helper,"EVALUATE",R"WGSL(EVALUATE_TEXT)WGSL");
 wgsl=helper+wgsl;
 sub(wgsl,"let rd = normalize(farWorld - ro);","let rd = normalize(farWorld - ro);polyRayDir=rd;");
 sub(wgsl,"            let worldP = ro + rd * sampleT;","            polySampleT=sampleT;\n            let worldP = ro + rd * sampleT;");
 const std::string call="cnoise3(((P.v[instances[g_instIdx].paramOffset + 35]) * (p)))";
 for(unsigned member=0;member<4;++member){auto begin=wgsl.find("fn volumeShared_"+std::to_string(member)+"_G0(");auto finish=wgsl.find("\n}",begin);auto p=wgsl.find(call,begin);if(begin==std::string::npos||p==std::string::npos||p>finish){std::fprintf(stderr,"poly unsupported call binding member %u: %s\n",member,begin==std::string::npos?"missing shared function":wgsl.substr(begin,finish+2-begin).c_str());std::abort();}wgsl.replace(p,call.size(),"polyNoise(((P.v[instances[g_instIdx].paramOffset + 35]) * (p)),P.v[instances[g_instIdx].paramOffset + 35]*polyRayDir,"+std::to_string(member)+"u)");}
 CAPTURE
 std::fprintf(stderr,"POLY mode=MODE; same original sample lattice/transport; unqualified numerical lowering\n");
 if(const char* p=std::getenv("EC_POLY_WGSL")){if(FILE* f=std::fopen(p,"w")){std::fwrite(wgsl.data(),1,wgsl.size(),f);std::fclose(f);}}
 return wgsl;
}
'''
    names=['000','100','010','110','001','101','011','111']
    dots=[]
    for n in names:
        vector='vec3<f32>('+','.join('Pf1.'+'xyz'[i] if n[i]=='1' else 'Pf0.'+'xyz'[i] for i in range(3))+')'
        dots += [f'let a{n}=dot(g{n},{vector});',f'let b{n}=dot(g{n},v);']
    declarations='var<private> polyCoefficients:array<f32,17>;'
    counters=miss=''
    if capture:
        declarations+='\nvar<private> polyStats:vec4<u32>;var<private> polyHistogram:array<u32,32>;var<private> polyRun:u32;var<private> polyReset:u32;'
        if compare_noise:declarations+='\nvar<private> polyMaxNoiseError:f32;'
        counters='polyStats.x+=1u;if(hit){polyStats.y+=1u;polyRun+=1u;}if(!eligible){polyStats.w+=1u;}if(eligible&&same&&!hit){polyReset+=1u;}'
        miss='if(polyRun>0u){polyHistogram[min(polyRun,31u)]+=1u;}polyRun=1u;polyStats.z+=1u;'
    if mode=='hits':
        declarations=declarations.replace('var<private> polyCoefficients:array<f32,17>;','')
        coefficients='';dots=[];evaluate='return cnoise3(P);';prepare=''
    else:
        coefficients=recipe.wgsl();prepare='polyPrepare(P,polyV);'
        evaluate='let localU=(polySampleT-polyAnchorT)*polyRate;\nvar value=polyCoefficients[16u];\n'
        evaluate+='\n'.join(f'value=value*localU+polyCoefficients[{k}u];' for k in range(15,-1,-1))
        if compare_noise:evaluate+='\npolyMaxNoiseError=max(polyMaxNoiseError,abs(value-cnoise3(P)));'
        evaluate+='\nreturn value;'
    for a,b in {'DECLARATIONS_TEXT':declarations,'DOTS_TEXT':'\n'.join(dots),'COEFFICIENTS_TEXT':coefficients,'COUNTERS_TEXT':counters,'MISS_TEXT':miss,'PREPARE_TEXT':prepare,'EVALUATE_TEXT':evaluate,'MODE':mode}.items():cpp=cpp.replace(a,b)
    capturecode=r'''
sub(wgsl,"    let alpha = 1.0 - transmittance;",R"WGSL(
 for(var k=0u;k<4u;k+=1u){census[captureSlot+20u+k]=polyStats[k];}
 if(polyRun>0u){polyHistogram[min(polyRun,31u)]+=1u;}
 for(var k=0u;k<32u;k+=1u){census[captureSlot+24u+k]=polyHistogram[k];}
 census[captureSlot+56u]=polyReset;
    let alpha = 1.0 - transmittance;
)WGSL");
'''
    if compare_noise:capturecode=capturecode.replace(' census[captureSlot+56u]=polyReset;',' census[captureSlot+56u]=polyReset;\n census[captureSlot+57u]=bitcast<u32>(polyMaxNoiseError);')
    return cpp.replace('CAPTURE',capturecode if capture else '')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path);p.add_argument('--mode',choices=['hits','polynomial'],default='hits');p.add_argument('--capture',action='store_true');p.add_argument('--compare-noise',action='store_true');p.add_argument('--witness-only',action='store_true');a=p.parse_args()
    recipe=Recipe().build();w=witness(recipe)
    if a.witness_only:print(json.dumps(w,indent=2));sys.exit()
    assert a.output and (a.mode!='hits' or a.capture)
    assert not a.compare_noise or (a.capture and a.mode=='polynomial')
    sys.path.insert(0,str(a.root/'scratch/probes'));import ray_transport_cf4_trial as parent
    parent.build(a.root.resolve(),a.output.resolve(),a.capture,lambda _:transform(recipe,a.mode,a.capture,a.compare_noise))
    (a.output/'rule-witness.json').unlink(missing_ok=True)
    (a.output/'polynomial-witness.json').write_text(json.dumps(w,indent=2)+'\n')
    (a.output/'preflight.txt').write_text(__doc__+'\nMode '+a.mode+'; capture '+str(a.capture)+'\nNo CF4 transform. All polynomial preparation/domain/fallback work included. Original evaluator is the control.\n')
