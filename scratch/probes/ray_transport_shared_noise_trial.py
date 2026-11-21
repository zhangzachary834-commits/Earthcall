"""Scratch single-slot shared Perlin reuse, with actual native hit census.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
New mechanism versus old per-medium caches: one pure-intrinsic cell slot shared
by every caller, keyed on actual f32 floor(P), never on a medium/domain identity.
Final noise values are never reused across different fractional positions.
Kernel-only invocation state; no persisted resource, authored state or policy.
"""
import argparse,sys
from pathlib import Path

def shader_transform(unused,mode,capture):
    code=r'''
static std::string cf4Shader(std::string wgsl){
 if(wgsl.find("fn volumeShared_3_G0(")==std::string::npos)return wgsl;
 if(std::getenv("EC_SHARED_NOISE_BASELINE"))return wgsl;
 auto replace=[&](const std::string& a,const std::string& b){size_t p=0;unsigned n=0;while((p=wgsl.find(a,p))!=std::string::npos){wgsl.replace(p,a.size(),b);p+=b.size();++n;}if(!n){std::fprintf(stderr,"noise missing hook: %s\n",a.c_str());std::abort();}};
 const auto at=wgsl.find("fn cnoise3(P: vec3<f32>) -> f32 {");if(at==std::string::npos)std::abort();
 const auto end=wgsl.find("\n}",at);if(end==std::string::npos)std::abort();
 auto body=wgsl.substr(at,end+2-at);std::string decl;
 DECLARATIONS
 BODY_TRANSFORM
 wgsl.replace(at,end+2-at,body);wgsl=decl+wgsl;
 CAPTURE
 std::fprintf(stderr,"SHARED NOISE mode=MODE, one invocation slot for all callers\n");
 if(const char* dump=std::getenv("EC_SHARED_NOISE_WGSL")){if(FILE* f=std::fopen(dump,"w")){std::fwrite(wgsl.data(),1,wgsl.size(),f);std::fclose(f);}}
 return wgsl;
}
'''
    # All bypassed points retain the original intrinsic. Exact bit keys include
    # signed zero. Adjacent integers and permutation arithmetic stay original.
    declarations='decl="\\nvar<private> noiseKey:vec3<u32>;var<private> noiseValid:bool;\\n";'
    if mode=='full':declarations+='decl+="var<private> noiseGradients:array<vec3<f32>,8>;\\n";'
    if mode=='xy':declarations+='decl+="var<private> noiseXY:vec4<f32>;\\n";'
    if capture:declarations+='decl+="var<private> noiseStats:vec4<u32>;\\n";'
    transform=r'''
 const std::string pi="    let Pi0 = floor(P);";
 auto sub=[&](std::string& text,const std::string& a,const std::string& b){const auto p=text.find(a);if(p==std::string::npos)std::abort();text.replace(p,a.size(),b);};
 sub(body,pi,pi+R"WGSL(
    let reusable=all(abs(Pi0)<vec3<f32>(1048576.0));
    let key=bitcast<vec3<u32>>(Pi0);
    let sameCell=reusable&&noiseValid&&all(key==noiseKey);
    let sameXY=reusable&&noiseValid&&all(key.xy==noiseKey.xy);
    COUNTERS
 )WGSL");
'''
    counters='noiseStats.x+=1u;if(sameCell){noiseStats.y+=1u;}if(sameXY){noiseStats.z+=1u;}if(!reusable){noiseStats.w+=1u;}' if capture else ''
    transform=transform.replace('COUNTERS',counters)
    if mode=='hits':
        transform+='sub(body,"    let Pi1 = Pi0 + vec3<f32>(1.0);","    if(reusable){noiseKey=key;noiseValid=true;}\\n    let Pi1 = Pi0 + vec3<f32>(1.0);");\n'
    elif mode=='xy':
        transform+=r'''
 sub(body,"    let ixy = permute4(permute4(ix) + iy);",R"WGSL(
    var ixy:vec4<f32>;
    if(sameXY){ixy=noiseXY;}else{
        ixy=permute4(permute4(ix)+iy);
        if(reusable){noiseXY=ixy;noiseKey=key;noiseValid=true;}
    }
 )WGSL");
'''
    else:
        transform+=r'''
 const auto begin=body.find("    let Pi1 = Pi0 + vec3<f32>(1.0);");
 const auto dot=body.find("    let n000 = dot(g000, Pf0);");
 if(begin==std::string::npos||dot==std::string::npos)std::abort();
 auto miss=body.substr(begin,dot-begin);
 sub(miss,"    let Pf0 = fract(P);\n    let Pf1 = Pf0 - vec3<f32>(1.0);","");
 const std::vector<std::string> names={"g000","g100","g010","g110","g001","g101","g011","g111"};
 std::string locals,hit,save;
 for(size_t i=0;i<names.size();++i){const auto& n=names[i];sub(miss,"var "+n+" =",n+" =");locals+="var "+n+":vec3<f32>;\n";hit+=n+"=noiseGradients["+std::to_string(i)+"u];\n";save+="noiseGradients["+std::to_string(i)+"u]="+n+";\n";}
 body.replace(begin,dot-begin,locals+"if(sameCell){\n"+hit+"}else{\n"+miss+"if(reusable){\n"+save+"noiseKey=key;noiseValid=true;\n}\n}\nlet Pf0=fract(P);let Pf1=Pf0-vec3<f32>(1.0);\n");
'''
    capture_code=r'''
 replace("    let alpha = 1.0 - transmittance;",R"WGSL(
    for(var k=0u;k<4u;k+=1u){census[captureSlot+20u+k]=noiseStats[k];}
    let alpha = 1.0 - transmittance;
)WGSL");
'''
    return code.replace('DECLARATIONS',declarations).replace('BODY_TRANSFORM',transform).replace('CAPTURE',capture_code if capture else '').replace('MODE',mode)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);p.add_argument('--mode',choices=['hits','full','xy'],default='full');p.add_argument('--capture',action='store_true')
    a=p.parse_args();assert a.mode!='hits' or a.capture
    sys.path.insert(0,str(a.root/'scratch/probes'));import ray_transport_cf4_trial as parent
    parent.build(a.root.resolve(),a.output.resolve(),a.capture,lambda x:shader_transform(x,a.mode,a.capture))
    # Parent supplies scratch compile/link and optional capture scaffolding only.
    # Remove its unrelated quadrature witness to avoid implying transport changed.
    (a.output/'rule-witness.json').unlink(missing_ok=True)
    (a.output/'preflight.txt').write_text(__doc__+'\nMode: '+a.mode+'; capture: '+str(a.capture)+'\nOriginal lattice/transport/noise result arithmetic retained. Full mode reuses normalized gradients; xy reuses the XY permutation. Native parity and cost are separate admission gates. No production activation.\n')
