"""Scratch selective CF4 screen: original-lattice fallback at observed branches.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
No production activation, proof of smoothness or general error admission.
Branch equality is a heuristic: unseen crossings remain possible. Empty panels
require the existing proof to cover their full interval, otherwise they fall back.
"""
import argparse, hashlib, json, math, struct, sys
from pathlib import Path


def selective_transform(transform, mode, capture, trace_slot=None):
    helper=r'''
static void traceTrialBranches(std::string& wgsl){
 // SCRATCH Kernel invocation state; discarded after each fragment. The
 // two-bit slots are observed branches, never a smoothness certificate.
 struct Edit{size_t pos,len;std::string value;};
 std::string helpers="\nvar<private> trialBranches:vec4<u32>;\n";
 for(unsigned member=0;member<4;++member){
  const auto id=std::to_string(member);unsigned bit=0;size_t search=0;
  const std::string prefix="fn volumeShared_"+id+"_G";
  while((search=wgsl.find(prefix,search))!=std::string::npos){
   const auto body=wgsl.find('{',search);size_t end=body+1;int depth=1;
   for(;end<wgsl.size()&&depth;++end){if(wgsl[end]=='{')++depth;else if(wgsl[end]=='}')--depth;}
   if(depth)std::abort();
   std::string block=wgsl.substr(body+1,end-body-2);std::vector<Edit> edits;
   for(size_t at=0;at<block.size();++at){
    const bool ab=block.compare(at,4,"abs(")==0;
    const bool cl=block.compare(at,6,"clamp(")==0;
    if(!ab&&!cl)continue;
    if(at && (std::isalnum(static_cast<unsigned char>(block[at-1]))||block[at-1]=='_'))continue;
    if(bit>=32u)std::abort();
    const auto open=at+(ab?3:5);size_t close=open+1;int parens=1;
    for(;close<block.size()&&parens;++close){if(block[close]=='(')++parens;else if(block[close]==')')--parens;}
    if(parens)std::abort();
    edits.push_back({at,ab?3u:5u,(ab?"trialAbs":"trialClamp")+id});
    edits.push_back({close-1,0,", "+std::to_string(bit)+"u"});bit+=2;
   }
   std::sort(edits.begin(),edits.end(),[](const Edit& a,const Edit& b){return a.pos>b.pos;});
   for(const auto& e:edits)block.replace(e.pos,e.len,e.value);
   wgsl.replace(body+1,end-body-2,block);search=body+1+block.size()+1;
  }
  helpers+="fn trialAbs"+id+"(x:f32,bit:u32)->f32{trialBranches["+id+"u]|=select(1u,2u,x>=0.0)<<bit;return abs(x);}\n";
  helpers+="fn trialClamp"+id+"(x:f32,lo:f32,hi:f32,bit:u32)->f32{trialBranches["+id+"u]|=select(select(2u,3u,x>=hi),1u,x<=lo)<<bit;return clamp(x,lo,hi);}\n";
  std::fprintf(stderr,"SELECTIVE traced member %u: %u scalar branch sites\n",member,bit/2);
 }
 wgsl=helpers+wgsl;
}
static std::string trialWholeZero(const std::string& wgsl){
 std::string code=R"WGSL(
 var zeroProven=true;var zeroExit=segmentEnd;
 let trialP=ro+rd*panelStart;
 )WGSL";
 for(unsigned i=0;i<4;++i){
  const auto n=std::to_string(i);const std::string mark="let cellExit"+n+" = ";
  const auto at=wgsl.find(mark);if(at==std::string::npos)std::abort();
  const auto start=at+mark.size(),end=wgsl.find(';',start);std::string call=wgsl.substr(start,end-start);
  auto sub=[&](const std::string& a,const std::string& b){size_t p=0;while((p=call.find(a,p))!=std::string::npos){call.replace(p,a.size(),b);p+=b.size();}};
  sub("mediumInst"+n,"trialInst");sub("worldP","trialP");sub("sampleT","panelStart");
  code+="{let trialInst=instances["+std::to_string(i+1)+"u];\n";
  code+="if(all(segmentMidP>=trialInst.origin.xyz-trialInst.halfExtent.xyz)&&all(segmentMidP<=trialInst.origin.xyz+trialInst.halfExtent.xyz)){\n";
  code+="let support=raySupport"+n+";if(support.y>=support.x && panelStart<=support.y){\n";
  code+="if(panelStart<support.x){zeroExit=min(zeroExit,support.x);if(support.x<panelEnd){zeroProven=false;}}else{let q="+call+";if(!(q>=panelEnd)){zeroProven=false;}else{zeroExit=min(zeroExit,q);}}\n}}}\n";
 }
 code+="panelZero=zeroProven;panelExit=zeroExit;\n";
 return code;
}
'''
    transform=helper+transform
    # Start from the original accepted midpoint lattice, grouping contiguous
    # old intervals so fallback sample positions can be exactly reconstructed.
    old=' if(wgsl.find("let firstSample = i32(clamp(ceil(")'
    inject=r'''
 const auto wholeZero=trialWholeZero(wgsl);
 MODE_TRACE
 replace("let stepLength = segmentSpan / f32(segmentSamples);",
         "let oldStepLength=segmentSpan/f32(oldSamples); let stepLength=oldStepLength*TRIAL_GROUP;");
'''.replace('MODE_TRACE','traceTrialBranches(wgsl);' if mode=='branches' else '')
    assert transform.count(old)==1
    transform=transform.replace(old,inject+old)
    transform=transform.replace('let panelStart=segmentStart+f32(step)*stepLength;',r'''
            let panelStart=segmentStart+f32(step*i32(TRIAL_GROUP))*oldStepLength;
            let panelCount=min(i32(TRIAL_GROUP),oldSamples-step*i32(TRIAL_GROUP));
            let panelWidth=f32(panelCount)*oldStepLength;
            let panelEnd=panelStart+panelWidth;
            PANEL_COUNT
'''.replace('PANEL_COUNT','trialStats.x+=1u;' if capture else ''))
    transform=transform.replace('var panelProven=true;', '''var panelProven=true;
            var panelRefine=false;var panelZero=false;
            PREVIOUS_BRANCH'''.replace('PREVIOUS_BRANCH','var previousBranches=vec4<u32>(0u);' if mode=='branches' else ''))
    transform=transform.replace('for(var node=0u;node<2u;node+=1u){', 'for(var node=0u;node<2u+select(0u,u32(panelCount),panelRefine);node+=1u){')
    transform=transform.replace('let sampleT=panelStart+fraction*stepLength;',r'''
            var sampleT=panelStart+fraction*panelWidth;
            if(node>=2u){sampleT=segmentStart+(f32(step*i32(TRIAL_GROUP)+i32(node)-2)+0.5)*oldStepLength;}
            RESET_BRANCH
'''.replace('RESET_BRANCH','trialBranches=vec4<u32>(0u);' if mode=='branches' else ''))
    begin=transform.index('            panelProven=panelProven && walkAllProven;')
    end=transform.index(')WGSL");',begin)
    code=r'''
            if(node<2u){
                nodeSigma[node]=totalExtinction;nodeSource[node]=totalSource;
                if(node==0u){SAVE_BRANCH}else{
                    let bothZero=all(nodeSigma==vec2<f32>(0.0));
                    if(bothZero){
)WGSL"+wholeZero+R"WGSL(
                    }
                    panelRefine=BRANCH_TEST || (bothZero && !panelZero);
                }
            }else{
                let oldT=transmittance;
                if(totalExtinction>0.0){
                    transmittance*=exp(-totalExtinction*oldStepLength);
                    let intervalGain=(oldT-transmittance)/totalExtinction;
                    integratedRadiance+=totalSource*intervalGain;
                }else{integratedRadiance+=oldT*totalSource*oldStepLength;}
                if(u.volumeOutputError.y==0.0 && transmittance<0.01){break;}
            }
            }
            if(panelRefine){
                REFINE_COUNT
                if(u.volumeOutputError.y==0.0 && transmittance<0.01){break;}
                continue;
            }
            if(panelZero){
                ZERO_COUNT
                step=max(step,i32(floor((panelExit-segmentStart)/stepLength))-1);
                continue;
            }
            COARSE_COUNT
'''
    code=code.replace('SAVE_BRANCH','previousBranches=trialBranches;' if mode=='branches' else '')
    code=code.replace('BRANCH_TEST','any(previousBranches!=trialBranches)' if mode=='branches' else 'false')
    for k,v in [('REFINE_COUNT','trialStats.z+=1u;'),('ZERO_COUNT','trialStats.w+=1u;'),('COARSE_COUNT','trialStats.y+=1u;')]:
        code=code.replace(k,v if capture else '')
    transform=transform[:begin]+code+transform[end:]
    transform=transform.replace('let x=sigma*stepLength;','let x=sigma*panelWidth;').replace('var gain=stepLength;','var gain=panelWidth;')
    force='any(previousBranches!=trialBranches)' if mode=='branches' else 'false'
    ending=r'''
 if(std::getenv("EC_SELECTIVE_FORCE_FINE"))replace("panelRefine=FORCE_CONDITION || (bothZero && !panelZero);","panelRefine=true;");
 replace("TRIAL_GROUP",std::to_string(group)+".0");
 EXTRA_CAPTURE
 std::fprintf(stderr,"SELECTIVE mode=MODE_NAME, aligned old intervals, heuristic fallback; UNQUALIFIED\n");
'''.replace('MODE_NAME',mode).replace('FORCE_CONDITION',force)
    # Integer uses need integer literals; float width uses explicit f32 below.
    transform=transform.replace('oldStepLength*TRIAL_GROUP','oldStepLength*f32(i32(TRIAL_GROUP))')
    ending=ending.replace('std::to_string(group)+".0"','std::to_string(group)')
    cap=r'''
 replace("var integratedRadiance = vec3<f32>(0.0);","var integratedRadiance = vec3<f32>(0.0);var trialStats=vec4<u32>(0u);var trialDensity=vec4<u32>(0u);");
 for(unsigned i=0;i<4;++i){const auto n=std::to_string(i);replace("density"+n+" = max(","trialDensity["+n+"u]+=1u;density"+n+" = max(");}
 replace("    let alpha = 1.0 - transmittance;",R"WGSL(
 for(var k=0u;k<4u;k+=1u){census[captureSlot+20u+k]=trialStats[k];census[captureSlot+24u+k]=trialDensity[k];}
    let alpha = 1.0 - transmittance;
 )WGSL");
'''
    ending=ending.replace('EXTRA_CAPTURE',cap if capture else '')
    if trace_slot is not None:
        assert capture and mode=='branches'
        # A bounded log in the diagnostic buffer's extra tail. Only one
        # specified fragment writes here. Never enabled in native cost runs.
        trace=r'''
 wgsl=R"WGSL(
 var<private> trialTraceIndex:u32;
 fn trialRecord(kind:u32,pa:f32,pb:f32,tau:f32,tb:f32,ta:f32,sigma:vec2<f32>,ba:vec4<u32>,bb:vec4<u32>){
  if(captureSlot!=TRACE_SLOTu*64u || trialTraceIndex>=16384u){return;}
  let at=147456u+trialTraceIndex*24u;
  census[at]=kind;census[at+1u]=bitcast<u32>(pa);census[at+2u]=bitcast<u32>(pb);census[at+3u]=bitcast<u32>(tau);
  census[at+4u]=bitcast<u32>(tb);census[at+5u]=bitcast<u32>(ta);census[at+6u]=bitcast<u32>(sigma.x);census[at+7u]=bitcast<u32>(sigma.y);
  for(var k=0u;k<4u;k+=1u){census[at+8u+k]=ba[k];census[at+12u+k]=bb[k];}
  trialTraceIndex+=1u;census[147455u]=trialTraceIndex;
 }
 )WGSL"+wgsl;
 replace("var panelRefine=false;var panelZero=false;","var panelRefine=false;var panelZero=false;let traceTbefore=transmittance;var traceTau=0.0;var traceSecondBranch=vec4<u32>(0u);");
 replace("let bothZero=all(nodeSigma==vec2<f32>(0.0));","traceSecondBranch=trialBranches;let bothZero=all(nodeSigma==vec2<f32>(0.0));");
 replace("let oldT=transmittance;","traceTau+=totalExtinction*oldStepLength;let oldT=transmittance;");
 replace("if(panelRefine){", "if(panelRefine){trialRecord(2u,panelStart,panelEnd,traceTau,traceTbefore,transmittance,nodeSigma,previousBranches,traceSecondBranch);");
 replace("if(panelZero){","if(panelZero){trialRecord(3u,panelStart,panelEnd,0.0,traceTbefore,transmittance,nodeSigma,previousBranches,traceSecondBranch);");
 replace("if (u.volumeOutputError.y == 0.0 && transmittance < 0.01) { break; }","trialRecord(1u,panelStart,panelEnd,0.5*(nodeSigma.x+nodeSigma.y)*panelWidth,traceTbefore,transmittance,nodeSigma,previousBranches,traceSecondBranch);if (u.volumeOutputError.y == 0.0 && transmittance < 0.01) { break; }");
'''.replace('TRACE_SLOT',str(trace_slot))
        # The final cutoff hook occurs both inside and after the panel loop;
        # restrict the latter replacement to the first occurrence.
        trace=trace.replace('replace("if (u.volumeOutputError.y == 0.0 && transmittance < 0.01) { break; }",', 'const auto traceAt=wgsl.find("if (u.volumeOutputError.y == 0.0 && transmittance < 0.01) { break; }");if(traceAt==std::string::npos)std::abort();wgsl.replace(traceAt,std::string("if (u.volumeOutputError.y == 0.0 && transmittance < 0.01) { break; }").size(),')
        ending+=trace
    needle=' std::fprintf(stderr,"CF4 ACTIVE'
    assert transform.count(needle)==1
    transform=transform.replace(needle,ending+needle)
    return transform


def analyse(root, capture_file, original_control=False):
    rows=json.loads((root/'docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json').read_text())['rows']
    inputs=json.loads((root/'docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json').read_text())
    assert len(rows)==len(inputs['rays'])==2304, 'expected the frozen 2304-ray reference'
    assert hashlib.sha256((root/inputs['fixture']).read_bytes()).hexdigest()==inputs['fixture_sha256'], 'stale reference fixture'
    raw=capture_file.read_bytes();assert len(raw)>=len(rows)*256
    t=[];rgb=[];control=[];counts=[0]*8;answers=[]
    for i,(ref,ray) in enumerate(zip(rows,inputs['rays'])):
        u=struct.unpack_from('<64I',raw,256*i);f=struct.unpack_from('<64f',raw,256*i)
        assert u[0]==1 and list(u[1:3])==ref[:2]==ray[:2]
        assert list(f[4:7])==inputs['origin'] and list(f[8:11])==ray[2:5] and [f[7],f[11]]==ray[5:7]
        value=list(f[12:16]);assert all(math.isfinite(x) for x in value)
        answers.append(value);t.append(abs(value[0]-ref[2]));rgb.extend(abs(value[k]-ref[k+2]) for k in (1,2,3))
        control.extend(abs(a-b) for a,b in zip(value,ray[7:11]))
        counts=[a+b for a,b in zip(counts,u[20:28])]
    def stats(v):
        v=sorted(v);return {'mean_abs':sum(v)/len(v),'rms':math.sqrt(sum(x*x for x in v)/len(v)),'p99_abs':v[int(.99*len(v))],'max_abs':v[-1]}
    result={'scope':'instrumented actual native outputs vs Opus independent converged rows; one view/clock only','rays':len(rows),'T':stats(t),'RGB':stats(rgb),'work_columns':['panels','coarse','fallback','whole_zero','density0','density1','density2','density3'],'work_counts':counts,'min_T':min(x[0] for x in answers),'negative_RGB':sum(x<0 for row in answers for x in row[1:]),'row1823':{'pixel':rows[1823][:2],'answer':answers[1823],'reference':rows[1823][2:6]},'max_difference_from_original_current':max(control)}
    if original_control:
        assert max(control)<=1e-6, result
        result['original_control_passed']=True
    return result


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,required=True)
    p.add_argument('--output',type=Path)
    p.add_argument('--mode',choices=['aligned','branches'],default='branches')
    p.add_argument('--capture',action='store_true')
    p.add_argument('--trace-slot',type=int)
    p.add_argument('--analyse',type=Path,help='Read a capture and compare every ray with the independent reference; no GPU/build')
    p.add_argument('--original-control',action='store_true',help='Require the forced-original-lattice capture to match current within 1e-6')
    a=p.parse_args()
    if a.analyse:
        report=json.dumps(analyse(a.root.resolve(),a.analyse,a.original_control),indent=2)+'\n'
        if a.output:a.output.write_text(report)
        print(report)
    else:
        assert a.output and (a.trace_slot is None or 0<=a.trace_slot<2304)
        sys.path.insert(0,str(a.root/'scratch/probes'))
        import ray_transport_cf4_trial as parent
        parent.build(a.root.resolve(),a.output.resolve(),a.capture,lambda x:selective_transform(x,a.mode,a.capture,a.trace_slot),4 if a.trace_slot is not None else 1)
