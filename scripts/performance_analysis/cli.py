"""Standalone command. No imports from Earthcall, no GPU/device/save access."""
import argparse
from datetime import datetime
import json
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import zipfile
from zoneinfo import ZoneInfo
from . import VERSION
from .analysis import (CONTEXT, InputError, fingerprint, frame_records, load_json,
                       summarize_demand, summarize_frames, summarize_model, summarize_wgsl, summarize_scaling)
from .timeline import summarize_trace
from .output import write_report


def envelope(inputs=(), context=None):
    return {'schema_version':1,'analyzer_version':VERSION,
            'created_at':datetime.now(ZoneInfo('America/Los_Angeles')).isoformat(),
            'signature':'Codex / GPT-6.1 Sol / 01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca',
            'inputs':[fingerprint(x) for x in inputs], 'context':context or {}, 'gaps':[]}


def verify_unchanged(report):
    for original in report['inputs']:
        if fingerprint(original['path']) != original:
            raise InputError('input changed during analysis: '+original['path'])


def artifact_summary(data):
    if not isinstance(data,dict) or not isinstance(data.get('images'),list) or not all(isinstance(x,dict) for x in data['images']):
        raise InputError('artifact expects an existing compiler-profile report with images')
    return {'evidence':'historical native fidelity/compilation artifact; not timing',
            'scope':data.get('scope'),'images':data['images'],
            'module_records':len(data.get('modules',[])),
            'summary':data.get('summary'),
            'performance_run':data.get('performance_run'),
            'production_admitted_skips':data.get('production_admitted_skips')}


def analyze(args):
    paths=[getattr(args,k) for k in ('frames','trace','demand','shader','model','artifact','context') if getattr(args,k)]
    if not any(getattr(args,k) for k in ('frames','trace','demand','shader','model','artifact')):
        raise InputError('provide at least one evidence input')
    report=envelope(paths,load_json(args.context) if args.context else None)
    if args.frames:
        filters={}
        for pair in args.frame_filter:
            if '=' not in pair: raise InputError('frame filter syntax: key=value')
            key,value=pair.split('=',1)
            if key not in CONTEXT: raise InputError('unknown frame context filter: '+key)
            filters[key]=value
        report['frames']=summarize_frames(frame_records(args.frames),filters)
        if not report['frames']['groups']:report['gaps'].append('No usable frame samples remain after filters/exclusions.')
        if not report['frames']['gpu_samples']: report['gaps'].append('GPU execution time is unavailable; CPU waits do not substitute for GPU timestamps.')
        if args.pixel_scaling:report['scaling']=summarize_scaling(report['frames'],args.pixel_scaling)
    else: report['gaps'].append('No frame samples supplied: no frame-time or movement-FPS verdict.')
    if args.pixel_scaling and not args.frames:raise InputError('pixel scaling requires frame samples')
    if args.trace: report['trace']=summarize_trace(load_json(args.trace))
    else:report['gaps'].append('No timeline trace supplied: queue pressure, scheduling and presentation causes are unresolved.')
    if args.demand: report['demand']=summarize_demand(load_json(args.demand))
    if args.shader: report['shader']=summarize_wgsl(args.shader.read_text(),args.entry)
    if args.model: report['model']=summarize_model(load_json(args.model))
    if args.artifact:
        report['artifact']=artifact_summary(load_json(args.artifact))
        report['gaps'].append('Imported native artifact is dated evidence; its compiler records and pixels are not fresh runtime costs.')
    report['gaps'].append('No automatic bottleneck cause is inferred from static syntax, diagnostic counts or missing measurements.')
    verify_unchanged(report); write_report(args.output,report)
    print((args.output/'summary.txt').read_text(),end='')
    print('Report:',args.output.resolve()/'report.html')


def compare_reports(baseline,candidate):
    # A run/build may intentionally differ. Every other context must be complete
    # and identical, with the same sample counts/camera/clock inputs provided in
    # the required comparison_contract. Nothing claims paired ABBA from two files.
    a=baseline.get('context',{}).get('comparison_contract')
    b=candidate.get('context',{}).get('comparison_contract')
    required=('backend','zone','width','height','route','clock_policy','sampling','surface_policy','device','configuration')
    if not isinstance(a,dict) or not isinstance(b,dict) or any(k not in a or k not in b for k in required):
        raise InputError('A/B requires complete context.comparison_contract; see task documentation')
    if a!=b: raise InputError('A/B comparison contracts differ; no pooled comparison')
    def group_map(report):
        result={}
        for g in report.get('frames',{}).get('groups',[]):
            for field in ('backend','zone','width','height','route','clock_policy'):
                if str(g['context'].get(field)) != str(a[field]):
                    raise InputError('frame context contradicts comparison contract: '+field)
            key=tuple(g['context'][k] for k in CONTEXT if k not in ('run','build'))
            if any(x=='unknown' for x in key):raise InputError('unknown frame context prevents A/B comparison')
            if key in result:raise InputError('multiple runs per comparison context; compare explicit run pairs')
            result[key]=g
        return result
    left=group_map(baseline);right=group_map(candidate)
    if not left or left.keys()!=right.keys():raise InputError('A/B frame contexts do not match')
    rows=[]
    for key,g in left.items():
        other=right[key]
        for metric,old in g['timings'].items():
            if metric not in other['timings']:continue
            new=other['timings'][metric]
            def delta(field):return None if old[field]==0 else (new[field]/old[field]-1)*100
            rows.append({'context':dict(zip((k for k in CONTEXT if k not in ('run','build')),key)),
                         'metric':metric,'baseline':old,'candidate':new,
                         'p50_change_percent':delta('p50'),'p95_change_percent':delta('p95')})
    if not rows:raise InputError('A/B has no common measured timing metrics')
    return {'rows':rows,'warnings':['Exploratory unpaired comparison only; repeated ABBA runs, fidelity gates, thermal/load control and Person acceptance are not established.']}


def demo(args):
    args.output.mkdir(parents=True,exist_ok=False)
    inputs=args.output/'inputs';inputs.mkdir()
    context={'label':'SYNTHETIC DEMONSTRATION — not an Earthcall measurement',
             'comparison_contract':dict(backend='synthetic',zone='demo',width=2560,height=1440,route='fixed',clock_policy='fixed',sampling=96,surface_policy='offscreen',device='synthetic',configuration='demo')}
    records=[]
    for i in range(24):
        records.append(dict(run='synthetic',phase='warm',zone='demo',view='fixed',width=2560,height=1440,
                            backend='synthetic',build='demo',route='fixed',clock_policy='fixed',frame_id=i,
                            total_ms=160+i%3,render3d_ms=155+i%3,wait_surface_ms=130+i%3,wait_submit_ms=0,
                            volumeCpuPrepareMs=.5,volumeProgramCompiles=0,volumeLowerings=0,
                            volumeParameterBytesUploaded=0,volumeProofBytesUploaded=0,
                            gpuVolumePassMs=0,gpuVolumePassTimingValid=False,gpuTimingSampleFrame=i))
    (inputs/'frames.json').write_text(json.dumps(records,indent=2)+'\n')
    (inputs/'context.json').write_text(json.dumps(context,indent=2)+'\n')
    (inputs/'trace.json').write_text(json.dumps({'traceEvents':[
        dict(ph='X',pid=1,tid=1,name='synthetic frame',cat='CPU wall clock',ts=0,dur=160000),
        dict(ph='X',pid=1,tid=1,name='synthetic surface wait',cat='wait',ts=10000,dur=130000),
        dict(ph='X',pid=1,tid=2,name='synthetic GPU-tagged span',cat='GPU illustrative',ts=0,dur=145000)]})+'\n')
    (inputs/'model.json').write_text(json.dumps({'dimensions':{
        'covered_pixels':{'value':2560*1440,'meaning':'assumed full framebuffer coverage','source':'synthetic upper-bound assumption'},
        'samples':{'value':96,'meaning':'assumed samples per pixel','source':'synthetic assumption'},
        'media':{'value':4,'meaning':'assumed active media per sample','source':'synthetic assumption'}},
        'terms':[{'name':'illustrative density demands','constant':1,'factors':{'covered_pixels':1,'samples':1,'media':1},
                  'unit':'expression visits','cadence':'frame','assumptions':['All media demanded at all samples; no proof skips or early exits.']}]} ,indent=2)+'\n')
    analyze(argparse.Namespace(frames=inputs/'frames.json',context=inputs/'context.json',trace=inputs/'trace.json',
                               model=inputs/'model.json',demand=None,shader=None,artifact=None,entry=None,
                               frame_filter=[],pixel_scaling=None,output=args.output/'analysis'))


def package(output):
    source=Path(__file__).resolve().parent
    if not source.is_dir():raise InputError('package from the source checkout, not from inside a zipapp')
    with output.open('xb') as stream:
        stream.write(b'#!/usr/bin/env python3\n')
        with zipfile.ZipFile(stream,'w',compression=zipfile.ZIP_DEFLATED) as archive:
            archive.writestr('__main__.py','from performance_analysis.cli import main\nmain()\n')
            for file in sorted(source.glob('*.py')):archive.write(file,'performance_analysis/'+file.name)
    output.chmod(0o755)
    print('Portable executable:',output.resolve(),'· Python 3.9+ required; standard library only')


def doctor():
    tools={x:shutil.which(x) for x in ('python3','xctrace','trace_processor','sample')}
    launcher=tools['xctrace'];note=None
    if shutil.which('xcrun'):
        r=subprocess.run(['xcrun','--find','xctrace'],text=True,capture_output=True,timeout=10)
        tools['xctrace']=r.stdout.strip() if r.returncode==0 else None
        if r.returncode:note=r.stderr.strip()
    print(json.dumps({'python':sys.version,'platform':platform.platform(),'tools':tools,
                      'xctrace_launcher':launcher,'selected_toolchain_note':note,
                      'scope':'availability only; no profiler, network install, GPU or app launched',
                      'native_perfetto_note':'Export Chrome trace JSON or use an existing trace_processor separately; raw .pftrace/.trace bundles are not parsed by this executable.'},indent=2))


def main(argv=None):
    parser=argparse.ArgumentParser(description='Separate offline analysis of timing, demand and complexity. Never changes Earthcall or saves.')
    parser.add_argument('--version',action='version',version=VERSION)
    sub=parser.add_subparsers(dest='command',required=True)
    p=sub.add_parser('analyze',help='read inputs and write a fresh offline report directory')
    for name in ('frames','trace','demand','shader','model','artifact','context'):
        p.add_argument('--'+name,type=Path)
    p.add_argument('--entry',action='append',help='WGSL reachability root; repeat as needed')
    p.add_argument('--frame-filter',action='append',default=[],help='explicit frame context key=value; does not filter traces')
    p.add_argument('--pixel-scaling',metavar='CPU_METRIC',help='fit observed median duration versus pixels across matched labeled contexts; not a Big-O proof')
    p.add_argument('--output',type=Path,required=True)
    p=sub.add_parser('compare',help='exploratory comparison of reports with matching explicit contracts')
    p.add_argument('baseline',type=Path);p.add_argument('candidate',type=Path);p.add_argument('--output',type=Path,required=True)
    p=sub.add_parser('demo',help='generate a clearly labeled synthetic demonstration')
    p.add_argument('--output',type=Path,required=True)
    p=sub.add_parser('package',help='build a portable executable zipapp; no app/CMake dependency')
    p.add_argument('--output',type=Path,required=True)
    sub.add_parser('doctor',help='inspect installed capture tools without running them')
    args=parser.parse_args(argv)
    try:
        if args.command=='analyze':analyze(args)
        elif args.command=='demo':demo(args)
        elif args.command=='doctor':doctor()
        elif args.command=='package':package(args.output)
        else:
            report=envelope([args.baseline,args.candidate])
            report['comparison']=compare_reports(load_json(args.baseline),load_json(args.candidate))
            verify_unchanged(report);write_report(args.output,report)
            print('Comparison:',args.output.resolve()/'report.html')
    except (InputError,OSError,ValueError,KeyError,TypeError,subprocess.SubprocessError) as error:
        parser.exit(2,'Analysis refused: '+str(error)+'\n')
