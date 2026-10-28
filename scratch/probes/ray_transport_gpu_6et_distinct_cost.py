"""Section6et: distinct captured-ray parity/pilot gate and complete resident cost.
Based on Sparkly's clean harness, Sonnet Surveyor's layout and Firmament counsel.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. No full-image reference, production connection or FPS admission.
"""
import argparse
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import zipfile

import ray_transport_gpu_6em_corrective_takeover as E
import sonnet_surveyor_harness_patch as P
import sonnet_surveyor_capture as V
from ray_transport_6eo_reference_pilot import ROOT,ZONE,read,write,digest,need,git

HOST=r'''        if(count!=230400u||numTiles!=14400u||scaleK!=1u)throw std::runtime_error("6et distinct capture extent");
        auto poisonAll=[&](){
            auto headers=gpuRays;
            for(auto& r:headers){r.subpanelOffset=r.subpanelCount=r.overlapOffset=r.overlapCount=0xcdcdcdcdu;}
            std::vector<uint32_t> scratch(subpanelBytes/4,0xcdcdcdcdu);
            wgpuQueueWriteBuffer(gpu.queue,rBuf,0,headers.data(),rayBytes);
            wgpuQueueWriteBuffer(gpu.queue,sBuf,0,scratch.data(),subpanelBytes);
            scratch.assign(overlapBytes/4,0xcdcdcdcdu);
            wgpuQueueWriteBuffer(gpu.queue,oBuf,0,scratch.data(),overlapBytes);
            std::vector<GpuResult> output(executionRays);
            for(auto& row:output)for(float& f:row.rgb_T)f=NAN;
            wgpuQueueWriteBuffer(gpu.queue,outBuf,0,output.data(),outBytes);
        };
        std::vector<GpuResult> baseline;J preflight=J::array();
        for(int mode:{0,1,2}){
            poisonAll();execute(mode,1,false);auto rows=readResults();
            for(size_t i=0;i<count;++i)for(float f:rows[i].rgb_T)
                if(!std::isfinite(f))throw std::runtime_error("6et nonfinite/unpublished mode="+std::to_string(mode)+" ray="+std::to_string(i));
            uint64_t different=0;
            if(mode==1)baseline=rows;
            if(mode==2)for(size_t i=0;i<count;++i)different+=std::memcmp(&rows[i],&baseline[i],sizeof(GpuResult))!=0;
            if(different)throw std::runtime_error("6et full distinct parity failure: "+std::to_string(different));
            const auto path=outputPath+".mode"+std::to_string(mode)+".bin";
            std::ofstream(path,std::ios::binary).write(reinterpret_cast<const char*>(rows.data()),outBytes);
            uint32_t refused=0;
            if(mode!=0){
                auto hb=readBackBuffer(rBuf,rayBytes);auto headers=reinterpret_cast<const GpuRayHeader*>(hb.data());
                for(size_t i=0;i<count;++i)refused+=bool(headers[i].overlapCount&0x80000000u);
                std::ofstream(outputPath+".headers"+std::to_string(mode)+".bin",std::ios::binary).write(reinterpret_cast<const char*>(hb.data()),hb.size());
            }
            preflight.push_back({{"mode",mode},{"rays",count},{"finite",true},{"different_from_bulk",different},{"refused",refused}});
            std::cerr<<"6ET PREFLIGHT mode="<<mode<<" rays="<<count<<" different="<<different<<" refused="<<refused<<std::endl;
        }
        J result={{"rays",count},{"tiles",numTiles},{"scale",scaleK},{"preflight",preflight},{"wait_style",waitStyle},
            {"scope","complete resident captured-ray chain; setup/uploads/time preparation/Screen/reference outside timing"},
            {"preflight_full_schedule_poison",true},{"obsolete_stream_allocations",false},
            {"bytes",{{"rays",rayBytes},{"subpanels",subpanelBytes},{"overlaps",overlapBytes},{"outputs",outBytes}}}};
        if(std::string(std::getenv("EC_ET_PHASE")?std::getenv("EC_ET_PHASE"):"preflight")=="timing"){
            for(int k=0;k<4;++k)for(int mode:{0,1,2})execute(mode,1,false);
            J observations=J::array();
            for(int block=0;block<8;++block){
                const std::vector<int> order=(block%4==1||block%4==2)?std::vector<int>{2,1,0,0,1,2}:std::vector<int>{0,1,2,2,1,0};
                for(int position=0;position<int(order.size());++position){
                    const int mode=order[position];const auto t=execute(mode,1,false);
                    observations.push_back({{"block",block},{"position",position},{"mode",mode},{"encode_ms",t[0]},{"completion_ms",t[1]},{"total_ms",t[2]},{"polls",lastPolls}});
                }
            }
            result["observations"]=observations;
        }
        std::ofstream(outputPath)<<result.dump(2)<<std::endl;return 0;
    }
}
'''


def generate():
    original=E.clean_source(ROOT);s=P.patch(original)
    # Pure host changes. No fake reference file is required or consumed.
    for a,b in [('    J rays, ref;','    J rays;'),('    { std::ifstream rref(std::getenv("EC_POSE_REFERENCE")); rref >> ref; }\n',''),('    const auto& Rows = ref.at("rows");\n','')]:
        need(s.count(a)==1,'reference ingestion anchor');s=s.replace(a,b)
    a=s.index('    // Warmup\n');b=s.index('    auto cuBuf=',a);s=s[:a]+s[b:]
    a=s.index('    for(uint32_t capacity:{65536u}){');b=s.index('        // mode 0 current marcher',a)
    s=s[:a]+'    { // No obsolete tile allocations/bind groups. Actual modes0/1/2 use whole buffers.\n'+s[b:]
    a=s.index('        J preflightRows=');s=s[:a]+HOST
    needle='dispatch(cpUnfused, bgUnfused, count);';a=s.index('else if(mode==2){');b=s.index('else if(mode==3){',a);part=s[a:b]
    need(part.count(needle)==1,'routed consumer hook')
    part=part.replace(needle,'if(!std::getenv("EC_ET_OMIT_CONSUMER"))'+needle)
    s=s[:a]+part+s[b:]
    shader=lambda text:re.findall(r'R"WGSL\((.*?)\)WGSL"',text,re.S)
    need(shader(s)==shader(original),'GPU literal arithmetic drift')
    need('tileSingles' not in s and 'tileOverlaps' not in s and 'std::vector<Tile>' not in s,'dead stream allocations remain')
    return s



def missing_s1_source():
    source=generate();a=source.index('else if(mode==2){');b=source.index('else if(mode==3){',a);part=source[a:b]
    entry='wgpuComputePassEncoderSetPipeline(p, cpS1Unfused);'
    location=part.index(entry);needle='wgpuComputePassEncoderDispatchWorkgroupsIndirect(p, argsBuf, 0);'
    dispatch=part.index(needle,location);part=part[:dispatch]+'/* negative: S1 producer omitted; poisoned headers/schedule remain */'+part[dispatch+len(needle):]
    return source[:a]+part+source[b:]


def prepare(work):
    need(not work.exists(),'fresh work required');work.mkdir()
    manifest=Path('/private/tmp/earthcall-6en-distinct-s4/case/manifest.json')
    meta,rays,_=V.verify_case(ROOT,manifest,digest(ZONE));layout=read(meta['layout'])
    need(len(rays['rays'])==230400 and len(layout['tile_offsets'])==14401,'full distinct capture')
    parent=ROOT/'docs/audits/GPU_RAY_TRANSPORT_6EN_MODE2_OWNERSHIP_2026-10-28.json';rec=read(parent)
    need(digest(parent.with_suffix('.zip'))==rec['archive_sha256'],'capture evidence identity')
    with zipfile.ZipFile(parent.with_suffix('.zip')) as z:
        data=z.read('capture/metadata.json');need(hashlib.sha256(data).hexdigest()==rec['members_sha256']['capture/metadata.json'],'capture metadata identity')
        capture_meta=json.loads(data);need(digest(manifest.parent.parent/'census.bin')==capture_meta['ray_header']['census_sha256'],'raw native census identity')
        need(meta['rays_sha256']==capture_meta['manifest']['rays_sha256'] and meta['layout_sha256']==capture_meta['manifest']['layout_sha256'],'retained capture/layout identity')
    (work/'probe.cpp').write_text(generate());(work/'rays.json').write_bytes(Path(meta['rays']).read_bytes());(work/'layout.json').write_bytes(Path(meta['layout']).read_bytes())
    write(work/'input.json',dict(parent_record_sha256=digest(parent),parent_archive_sha256=rec['archive_sha256'],rays_sha256=digest(work/'rays.json'),layout_sha256=digest(work/'layout.json'),zone_sha256=digest(ZONE),engine_src_tree=git(ROOT,'rev-parse','HEAD:src'),clock=7.25))
    print('PREPARED',work)


def fingerprint(work):
    return dict(binary_sha256=digest(work/'build/probe'),cpp_sha256=digest(work/'probe.cpp'),build_receipt_sha256=digest(work/'build/source.json'),
        rays_sha256=digest(work/'rays.json'),layout_sha256=digest(work/'layout.json'),zone_sha256=digest(ZONE),engine_src_tree=git(ROOT,'rev-parse','HEAD:src'))


def run(work,label,phase,style,negative=False):
    need(label.replace('-','').isalnum(),'simple run label');out=work/(label+'.json');log=work/(label+'.log');need(not out.exists() and not log.exists(),'run overwrite')
    if phase=='timing':
        import ray_transport_6et_analysis as A
        need(A.qualify(work,'preflight')['pilot_both_candidate_modes_pass'],'timing before parity/reference pilot gate')
        prior=read(work/'preflight-receipt.json');need(prior['after']==fingerprint(work) and prior['exit_code']==0,'preflight source identity before timing')
        for name,sha in prior['outputs_sha256'].items():need(digest(work/name)==sha,'preflight output identity before timing')
    before=fingerprint(work);env={k:v for k,v in os.environ.items() if not k.startswith('EC_') and k!='METAL_CAPTURE_ENABLED'}
    env.update(EC_POSE_RAYS=str(work/'rays.json'),EC_SURVEYOR_LAYOUT=str(work/'layout.json'),EC_POSE_CLOCK='7.25',EC_GPU_SCALE='1',EC_MATCH_REPEAT='1',EC_GPU_BUDGET='1024',EC_GPU_SUB_CAP='64',EC_GPU_OVER_CAP='16',EC_GPU_MARGIN='.001',EC_GPU_WAIT_STYLE=str(style),EC_ET_PHASE=phase)
    if negative:env['EC_ET_OMIT_CONSUMER']='1'
    start=dt.datetime.now(dt.timezone.utc).isoformat();t=time.monotonic()
    with log.open('w') as f:
        try:code=subprocess.run([str(work/'build/probe'),str(out)],cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=180).returncode
        except subprocess.TimeoutExpired:code='timeout'
    after=fingerprint(work);receipt=dict(label=label,phase=phase,wait_style=style,started_utc=start,elapsed_seconds=time.monotonic()-t,exit_code=code,before=before,after=after,log_sha256=digest(log),negative=negative)
    receipt['outputs_sha256']={p.name:digest(p) for p in work.glob(label+'.json*')};write(work/(label+'-receipt.json'),receipt)
    need(before==after,'run identity drift');text=log.read_text();need(not any(s in text.lower() for s in ['validation error','uncaptured error','device lost']),'GPU environment/validation failure')
    if negative:need(code not in [0,'timeout'] and '6et nonfinite/unpublished mode=2' in text,'negative failure boundary')
    else:need(code==0,'native run failed '+label)
    print('PASS',label,code)


def main():
    p=argparse.ArgumentParser(description=__doc__);q=p.add_subparsers(dest='command',required=True)
    s=q.add_parser('prepare');s.add_argument('--work',type=Path,required=True)
    s=q.add_parser('run');s.add_argument('--work',type=Path,required=True);s.add_argument('--label',required=True);s.add_argument('--phase',choices=['preflight','timing'],default='preflight');s.add_argument('--style',type=int,choices=[0,1],default=1);s.add_argument('--negative',action='store_true')
    a=p.parse_args()
    if a.command=='prepare':prepare(a.work)
    else:run(a.work,a.label,a.phase,a.style,a.negative)


if __name__=='__main__':main()
