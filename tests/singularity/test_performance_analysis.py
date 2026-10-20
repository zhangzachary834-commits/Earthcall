"""Offline analyzer correctness and packaged CLI witnesses. No app/GPU/save writes.
Codex / GPT-6.1 Sol / 01a122d7-0a5e-7c00-ac5b-fcd4f7c332ca / 2026-10-14 PDT.
"""
import copy
import csv
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
from performance_analysis.analysis import (InputError, frame_records, stats, summarize_frames,
                                            summarize_model, summarize_wgsl, summarize_demand,summarize_scaling)
from performance_analysis.cli import compare_reports, envelope, verify_unchanged
from performance_analysis.output import render_html
from performance_analysis.timeline import summarize_trace


def frame(**updates):
    return dict(dict(run='run',phase='warm',zone='zone',view='front',width=800,height=600,
                     backend='Metal',build='head',route='fixed',clock_policy='fixed',frame_id=1,
                     total_ms=50,render3d_ms=40,wait_surface_ms=20,wait_submit_ms=5),**updates)


class FrameTests(unittest.TestCase):
    def test_nested_timers_and_percentiles(self):
        r=summarize_frames([frame(frame_id=i,total_ms=n) for i,n in enumerate((10,20,30,40))])
        g=r['groups'][0]
        self.assertEqual(g['timings']['recording_remainder_ms']['p50'],15)
        self.assertEqual(g['timings']['total_ms']['p95'],38.5)
        self.assertFalse(g['observed_p95_under_33_3ms'])

    def test_exclusion_and_explicit_phases(self):
        r=summarize_frames([frame(),frame(frame_id=2,phase='startup'),frame(frame_id=3,occluded=True),
                            frame(frame_id=4,surface_status='lost'),frame(frame_id=5,zone='other')],{'zone':'zone'})
        self.assertEqual(len(r['groups']),2)
        self.assertEqual(r['excluded'],{'occluded':1,'surface_unavailable':1,'context_filter':1})

    def test_delayed_gpu_owner_and_deduplication(self):
        a=frame(view='first')
        b=frame(frame_id=2,view='second',gpuVolumePassMs=30,gpuVolumePassTimingValid=True,gpuTimingSampleFrame=1)
        c=frame(frame_id=3,view='third',gpuVolumePassMs=30,gpuVolumePassTimingValid=True,gpuTimingSampleFrame=1)
        r=summarize_frames([a,b,c]);g=r['gpu_samples']['gpuVolumePassMs']
        self.assertEqual(len(g),1);self.assertEqual(g[0]['context']['view'],'first')
        self.assertNotIn('gpuVolumePassMs',r['groups'][1]['timings'])

    def test_unavailable_gpu_is_not_zero_cost(self):
        r=summarize_frames([frame(gpuVolumePassMs=0,gpuVolumePassTimingValid=True,gpuTimingSampleFrame=1)])
        self.assertEqual(r['gpu_samples'],{});self.assertEqual(r['gpu_unavailable']['gpuVolumePassMs'],1)
        r=summarize_frames([frame(gpuVolumePassMs=22,gpuVolumePassTimingValid=False)])
        self.assertEqual(r['gpu_samples'],{})
        r=summarize_frames([frame(gpuVolumePassMs=22,gpuVolumePassTimingValid=True,gpuMainPassTimingSupported=False)])
        self.assertEqual(r['gpu_samples'],{})

    def test_unknown_gpu_owner_is_preserved(self):
        r=summarize_frames([frame(gpuVolumePassMs=22,gpuVolumePassTimingValid=True,gpuTimingSampleFrame=0)])
        self.assertIsNone(r['gpu_samples']['gpuVolumePassMs'][0]['context'])

    def test_refuse_invalid_and_conflicting_samples(self):
        for row in [frame(total_ms=float('nan')),frame(width=-1),frame(volumeLowerings=.5),
                    frame(wait_surface_ms=99),frame(occluded='perhaps')]:
            with self.subTest(row=row),self.assertRaises(InputError):summarize_frames([row])
        with self.assertRaises(InputError):summarize_frames([frame(),frame()])
        with self.assertRaises(InputError):summarize_frames([
            frame(gpuVolumePassMs=2,gpuVolumePassTimingValid=True,gpuTimingSampleFrame=0),
            frame(frame_id=2,gpuVolumePassMs=3,gpuVolumePassTimingValid=True,gpuTimingSampleFrame=0)])

    def test_csv_and_jsonl(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'frames.csv'
            with p.open('w',newline='') as f:
                writer=csv.DictWriter(f,fieldnames=frame().keys());writer.writeheader();writer.writerow(frame())
            self.assertEqual(summarize_frames(frame_records(p))['groups'][0]['frames'],1)
            p=Path(tmp)/'frames.jsonl';p.write_text(json.dumps(frame())+'\n')
            self.assertEqual(len(frame_records(p)),1)


class TraceTests(unittest.TestCase):
    def x(self,name,start,duration,tid=1):return dict(ph='X',pid=1,tid=tid,name=name,ts=start,dur=duration)

    def test_nested_self_time_and_display_unit(self):
        r=summarize_trace({'displayTimeUnit':'ms','traceEvents':[self.x('outer',0,10000),self.x('child',1000,3000)]})
        d={x['name']:x for x in r['ranked_spans']}
        self.assertEqual(d['outer']['exclusive_total_ms'],7)
        self.assertEqual(d['child']['inclusive_total_ms'],3)
        self.assertEqual(r['tracks'][0]['covered_ms'],10)

    def test_parallel_tracks_not_pooled(self):
        r=summarize_trace([self.x('CPU',0,10000),self.x('GPU tagged',0,10000,2)])
        self.assertEqual([x['covered_ms'] for x in r['tracks']],[10,10])
        self.assertNotIn('total_frame_ms',r)

    def test_crossing_spans_decline_exclusive(self):
        r=summarize_trace([self.x('a',0,10000),self.x('b',5000,10000)])
        self.assertFalse(r['tracks'][0]['exclusive_valid'])
        self.assertEqual(r['tracks'][0]['covered_ms'],15)
        self.assertTrue(all(x['exclusive_total_ms'] is None for x in r['ranked_spans']))

    def test_begin_end_and_negative_counter(self):
        r=summarize_trace([dict(ph='B',pid=1,tid=1,name='a',ts=1000),dict(ph='E',pid=1,tid=1,ts=3000),
                           dict(ph='C',name='delta',ts=0,args={'n':-1})])
        self.assertEqual(r['ranked_spans'][0]['inclusive_total_ms'],2)
        self.assertEqual(r['counter_samples'][0]['value'],-1)

    def test_trace_refusals_and_ignored_flows(self):
        for data in [[dict(ph='B',ts=0)],[dict(ph='E',ts=0)],[self.x('a',0,-1)],[dict(ph='X',ts=0)]]:
            with self.assertRaises(InputError):summarize_trace(data)
        self.assertEqual(summarize_trace([dict(ph='s',ts=0)])['ignored_phases'],{'s':1})


class StructureTests(unittest.TestCase):
    def test_observed_scaling_is_stratified_not_big_o(self):
        frames=summarize_frames([frame(frame_id=1,width=100,height=100,total_ms=2),frame(frame_id=2,width=200,height=200,total_ms=8)])
        scaling=summarize_scaling(frames,'total_ms')
        self.assertAlmostEqual(scaling['fits'][0]['observed_log_log_slope'],1)
        self.assertIn('not asymptotic',scaling['evidence'])
        with self.assertRaises(InputError):summarize_scaling(summarize_frames([frame()]),'total_ms')
        with self.assertRaises(InputError):summarize_scaling(summarize_frames([frame(frame_id=1,width=100),frame(frame_id=2,width=200,build='other')]),'total_ms')
    def test_reachable_calls_and_nested_comments(self):
        text='// fn fake() { sin(x); }\n/* outer /* cos(x) */ end */\nfn leaf(x:f32)->f32 {return sin(x);}\nfn main() { if (true) { leaf(1.);leaf(2.); }}\nfn dead(){cos(1.);}'
        r=summarize_wgsl(text,['main'])
        self.assertEqual(r['reachable_functions'],['leaf','main'])
        self.assertEqual(r['reachable_call_sites'],{'leaf':2,'sin':1})
        with self.assertRaises(InputError):summarize_wgsl(text,['absent'])
        with self.assertRaises(InputError):summarize_wgsl('/* unclosed')

    def test_explicit_model_counts_not_ms(self):
        m={'dimensions':{'pixels':{'value':100,'meaning':'coverage','source':'assumed'},'samples':{'value':10,'meaning':'per-pixel samples','source':'assumed'}},
           'terms':[{'name':'eval','unit':'visits','cadence':'frame','assumptions':['No exits'],'factors':{'pixels':1,'samples':1}}]}
        r=summarize_model(m);self.assertEqual(r['terms'][0]['modeled_count'],1000)
        self.assertNotIn('predicted_ms',r)
        m['terms'][0]['factors']['unknown']=1
        with self.assertRaises(InputError):summarize_model(m)

    def test_instrumented_demand_never_promoted(self):
        r=summarize_demand([{'view':'inside','pixels_identical':False,'globals':{'sample_visits':5,'all_proven_jumps':2,'transport_updates':3},
                             'media':[{'proof_query':5,'proven_empty':2,'density_demand':3,'shared_shape_demand':3,'noise_demand':2,'positive_density':1}]}])
        self.assertIn('instrumented',r['evidence'])
        with self.assertRaises(InputError):summarize_demand([{'globals':{'sample_visits':8,'all_proven_jumps':2,'transport_updates':3}}])

    def test_html_escapes_input(self):
        r=envelope(context={'x':'</script><img src=x onerror=alert(1)>'})
        html=render_html(r)
        self.assertNotIn('<img src=x',html);self.assertIn('&lt;img',html)

    def test_input_mutation_is_detected(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'input.json';p.write_text('{}');r=envelope([p]);p.write_text('{"x":1}')
            with self.assertRaises(InputError):verify_unchanged(r)


class CliTests(unittest.TestCase):
    def test_packaged_cli_end_to_end_and_no_clobber(self):
        launcher=ROOT/'scripts/earthcall-analyze'
        with tempfile.TemporaryDirectory() as tmp:
            out=Path(tmp);exe=out/'analyze.pyz'
            subprocess.run([sys.executable,str(launcher),'package','--output',str(exe)],check=True,capture_output=True)
            r=subprocess.run([sys.executable,str(exe),'demo','--output',str(out/'demo')],check=True,capture_output=True,text=True)
            self.assertIn('p95 162.000',r.stdout)
            report=out/'demo/analysis/report.json';data=json.loads(report.read_text())
            self.assertIn('SYNTHETIC',data['context']['label'])
            self.assertEqual(data['frames']['groups'][0]['counters']['volumeLowerings']['total'],0)
            self.assertTrue((report.parent/'report.html').is_file())
            before=hashlib.sha256(report.read_bytes()).hexdigest()
            r=subprocess.run([sys.executable,str(exe),'analyze','--frames',str(out/'demo/inputs/frames.json'),'--output',str(report.parent)],capture_output=True)
            self.assertEqual(r.returncode,2)
            self.assertEqual(hashlib.sha256(report.read_bytes()).hexdigest(),before)

    def test_compare_contracts_and_unknown_context(self):
        contract=dict(backend='Metal',zone='zone',width=800,height=600,route='fixed',clock_policy='fixed',sampling=96,surface_policy='offscreen',device='M5',configuration='same')
        a={'context':{'comparison_contract':contract},'frames':summarize_frames([frame()])}
        b=copy.deepcopy(a);b['frames']['groups'][0]['timings']['total_ms']['p50']=40
        self.assertAlmostEqual(next(x for x in compare_reports(a,b)['rows'] if x['metric']=='total_ms')['p50_change_percent'],-20)
        b['context']['comparison_contract']['width']=1024
        with self.assertRaises(InputError):compare_reports(a,b)
        with self.assertRaises(InputError):compare_reports({'frames':a['frames']},a)
        b=copy.deepcopy(a);b['frames']['groups'][0]['context']['width']=999
        with self.assertRaises(InputError):compare_reports(b,b)


if __name__=='__main__':unittest.main()
