"""Prepare or run a bounded Metal trace of the §6ey Release Engine fixture.
Run only with Zach ready to keep the native window foreground. Trace overhead
is diagnostic; it never replaces clean paired economics or accuracy admission.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-29.
"""
import argparse
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import plistlib
import shlex
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
BUILD = Path('/private/tmp/earthcall-6ey-release-build')
sys.path.insert(0, str(ROOT / 'scratch/probes'))
import ray_transport_6ey_engine as E


def need(value, why):
    if not value:
        raise ValueError(why)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n')


def replace(text, old, new):
    need(text.count(old) == 1, 'unique derivation site')
    return text.replace(old, new)


TRACE = r'''
  need(mode=="trace","trace-only diagnostic");
  Json warm=Json::array();for(bool on:{false,true})for(int i=0;i<64;++i)warm.push_back(frame(i,on));fence(k);result["warmup"]=warm;
  WGPULimits limits={};need(wgpuDeviceGetLimits(k->device,&limits)==WGPUStatus_Success,"native device limits");
  result["device_binding_limit"]=limits.maxStorageBufferBindingSize;result["strip_rows"]=k->stripRows;
  result["process_id"]=getpid();result["scope"]="instrumented queued Engine ticks, not clean timing or physical scanout";
  write(output.parent_path()/"trace-ready.json",result);
  auto traceWait=Clock::now();
  while(!fs::exists(output.parent_path()/"trace-go")){
   need(elapsed(traceWait)<45000,"trace attach timeout");glfwPollEvents();
   need(glfwGetWindowAttrib(e.window(),GLFW_FOCUSED)==GLFW_TRUE,"focus lost waiting for trace attach");
   std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  Json samples=Json::array();
  for(int order=0;order<4;++order){
   bool on=order==1||order==2;fence(k);Json activation=Json::array();for(int i=0;i<2;++i)activation.push_back(frame(0,on));fence(k);
   auto sid=os_signpost_id_generate(ecTraceLog);os_signpost_interval_begin(ecTraceLog,sid,"EC6EZSample","order=%d routed=%d frames=64",order,int(on));
   Json frames=Json::array();auto start=Clock::now();
   for(int i=0;i<64;++i){auto row=frame(i,on);need(row["volume_compiles"].get<uint32_t>()==0,"cold composition in trace sample");frames.push_back(row);}
   fence(k);auto duration=elapsed(start);os_signpost_interval_end(ecTraceLog,sid,"EC6EZSample","order=%d routed=%d",order,int(on));
   samples.push_back(Json{{"order",order},{"on",on},{"activation",activation},{"frames",frames},{"instrumented_complete_ms",duration},{"signpost_id",sid}});
   result["samples"]=samples;write(output.parent_path()/"progress.json",result);
  }
  result["final_publication"]=publication(k);
'''


def prepare(work):
    need(not work.exists(), 'fresh trace work directory')
    work.mkdir()
    before = E.R.snapshot()
    base = E.CPP.read_text()
    text = replace(base, '#include <cstring>', '#include <cstring>\n#include <os/signpost.h>\n#include <unistd.h>\nstatic os_log_t ecTraceLog=os_log_create("org.earthcall.transport","6ez");')
    text = replace(text, 'auto begin=Clock::now();e.tick(dt);double tickMs=elapsed(begin);',
                   'auto fid=os_signpost_id_generate(ecTraceLog);os_signpost_interval_begin(ecTraceLog,fid,"EC6EZFrame","index=%d routed=%d",i,int(on));auto begin=Clock::now();e.tick(dt);double tickMs=elapsed(begin);os_signpost_interval_end(ecTraceLog,fid,"EC6EZFrame");')
    start = text.index('  if(mode=="timing"||mode=="session"){')
    end = text.index('  write(output,result);e.shutdown();', start)
    text = text[:start] + TRACE + text[end:]
    # No renderer alteration: only the independent main and signposts change.
    # Original timestamp-free device initialization stays source-bound.
    cpp = work / 'engine-trace.cpp'
    cpp.write_text(text)
    db = json.loads((BUILD / 'compile_commands.json').read_text())
    row = next(v for v in db if v['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in v['command'])
    cache = (BUILD / 'CMakeCache.txt').read_text()
    need('CMAKE_HOME_DIRECTORY:INTERNAL=' + str(ROOT) in cache and 'CMAKE_BUILD_TYPE:STRING=Release' in cache,
         'correct-checkout Release cache')
    old = json.loads(Path('/private/tmp/earthcall-6ey-engine-human/build.json').read_text())
    need(before == old['after'], 'unchanged production sources and fixture')
    objects = {str(p): sha(p) for p in (BUILD / 'CMakeFiles/earthcall_webgpu.dir').rglob('*.o')}
    need(objects == old['object_sha256'], 'same 238 Release object bytes')
    compile_cmd = shlex.split(row['command'])
    compile_cmd[compile_cmd.index('-c') + 1] = str(cpp)
    compile_cmd[compile_cmd.index('-o') + 1] = str(work / 'engine.o')
    compile_cmd += ['-std=c++17']
    link = shlex.split((BUILD / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    link[next(i for i, v in enumerate(link) if v.endswith('/src/entry.cpp.o'))] = str(work / 'engine.o')
    link[link.index('-o') + 1] = str(work / 'engine')
    with (work / 'build.log').open('w') as log:
        subprocess.run(compile_cmd, cwd=BUILD, stdout=log, stderr=subprocess.STDOUT, check=True)
        subprocess.run(link, cwd=BUILD, stdout=log, stderr=subprocess.STDOUT, check=True)
    app = work / 'Earthcall Transport Trace.app'
    mac = app / 'Contents/MacOS'
    mac.mkdir(parents=True)
    (mac / 'engine').symlink_to(work / 'engine')
    (app / 'Contents/Info.plist').write_bytes(plistlib.dumps(dict(CFBundleIdentifier='org.earthcall.transport-trace-6ez',
          CFBundleExecutable='engine', CFBundleName='Earthcall Transport Trace', CFBundlePackageType='APPL', NSHighResolutionCapable=True)))
    session = work / 'session'
    saves = E.stage(session)
    fixture = E.tree_hash(saves)
    old_receipt = json.loads(Path('/private/tmp/earthcall-6ey-engine-human/session/receipt.json').read_text())
    need(fixture == old_receipt['initial_fixture_sha256'], 'same 56-file initial fixture')
    need(E.R.snapshot() == before, 'production source and fixture unchanged through preparation')
    write(work / 'build.json', dict(schema='6ez-prepared-Release-trace-1', before=before,
          after=E.R.snapshot(), base_cpp_sha256=sha(E.CPP), derived_cpp_sha256=sha(cpp),
          binary_sha256=sha(work / 'engine'), compile_argv=compile_cmd, link_argv=link,
          object_sha256=objects, initial_fixture_sha256=fixture, script_sha256=sha(Path(__file__)),
          native_executed=False, profile_executed=False,
          scope='same production Release objects; derived independent main/signposts; no production source edit'))
    print('PASS prepared and linked Release trace; NO native execution:', work, flush=True)


def run(work):
    meta = json.loads((work / 'build.json').read_text())
    need(E.R.snapshot() == meta['after'] and sha(work / 'engine') == meta['binary_sha256'], 'bound source/binary')
    need(sha(Path(__file__)) == meta['script_sha256'] and sha(work / 'engine-trace.cpp') == meta['derived_cpp_sha256'], 'bound executed tools')
    session = work / 'session'
    need(not (session / 'receipt.json').exists(), 'one diagnostic native attempt per prepared directory')
    need(E.tree_hash(session / 'saves') == meta['initial_fixture_sha256'], 'unchanged isolated initial fixture')
    env = {k: v for k, v in os.environ.items() if not k.startswith(('EC_', 'EARTHCALL_'))}
    env['EARTHCALL_DATA'] = str(session / 'saves')
    app = work / 'Earthcall Transport Trace.app/Contents/MacOS/engine'
    argv = [str(app), 'trace', str(session / 'result.json')]
    trace = work / 'metal.trace'
    instrument_argv = ['xcrun', 'xctrace', 'record', '--template', 'Metal System Trace',
                       '--instrument', 'Points of Interest', '--time-limit', '45s',
                       '--output', str(trace)]
    started = time.monotonic()
    started_utc = dt.datetime.now(dt.timezone.utc).isoformat()
    process = profiler = None
    native_code = profiler_code = None
    failure = None
    with (session / 'native.log').open('w') as log, (work / 'xctrace.log').open('w') as trace_log:
        try:
            process = subprocess.Popen(argv, cwd=session, env=env, stdout=log, stderr=subprocess.STDOUT)
            print('Native window launched. Keep Earthcall Transport Trace in foreground until it closes.', flush=True)
            (session / 'foreground-ready').touch()
            while not (session / 'trace-ready.json').exists():
                need(process.poll() is None and time.monotonic() - started < 165, 'native warmup/focus refused or timed out')
                time.sleep(.2)
            instrument_argv += ['--attach', str(process.pid)]
            profiler = subprocess.Popen(instrument_argv, stdout=trace_log, stderr=subprocess.STDOUT)
            attach_start = time.monotonic()
            while 'Recording started' not in (work / 'xctrace.log').read_text():
                need(profiler.poll() is None and process.poll() is None and time.monotonic() - attach_start < 30,
                     'trace did not acknowledge recording; no sample admitted')
                time.sleep(.2)
            (session / 'trace-go').touch()
            native_code = process.wait(timeout=min(60, 230 - (time.monotonic() - started)))
            need(native_code == 0, 'native trace witness failed')
            profiler_code = profiler.wait(timeout=min(60, 230 - (time.monotonic() - started)))
            need(profiler_code == 0 and trace.exists(), 'trace did not save')
        except Exception as error:
            failure = str(error)
        finally:
            for child in [process, profiler]:
                if child is not None and child.poll() is None:
                    child.send_signal(signal.SIGINT)
                    try:
                        child.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        child.kill()
                        child.wait()
            native_code = process.returncode if process else native_code
            profiler_code = profiler.returncode if profiler else profiler_code
            write(session / 'receipt.json', dict(started_utc=started_utc, process_cap_seconds=240,
                  elapsed_seconds=time.monotonic() - started, argv=argv, cwd=str(session),
                  xctrace_argv=instrument_argv, native_exit_code=native_code, profiler_exit_code=profiler_code,
                  failure=failure, build_sha256=sha(work / 'build.json'),
                  before=meta['after'], after=E.R.snapshot(),
                  final_zone_sha256=sha(session / 'saves/zones/Northern Veil/zone.ecform')))
    need(failure is None, failure or 'trace failed')
    subprocess.run(['xcrun', 'xctrace', 'export', '--input', str(trace), '--toc', '--output', str(work / 'toc.xml')], check=True)
    print('PASS diagnostic trace saved; analyze tables before any performance claim:', trace)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('command', choices=['prepare', 'run'])
    p.add_argument('--work', type=Path, required=True)
    a = p.parse_args()
    (prepare if a.command == 'prepare' else run)(a.work.resolve())


if __name__ == '__main__':
    main()
