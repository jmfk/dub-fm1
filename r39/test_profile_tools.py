import json,os,signal,subprocess,sys,tempfile,unittest
from pathlib import Path
import profile_midi as wire
import profile_benchmark as b
from validate_run_state import validate
R=Path(__file__).resolve().parent
class Checks(unittest.TestCase):
 def test_c_wire(self):
  data=(R.parent.parent/'.build-env/dub-r39-profile.bin').read_bytes();q=wire.decode(data,6,77)
  self.assertEqual(q['blocks'],2);self.assertEqual(q['late_blocks'],1);self.assertEqual(q['stage_ticks']['commands'],210000);self.assertEqual(q['events']['sample_cache_fill'],2);self.assertGreater(q['slowest_blocks'][0]['load_percent'],100);self.assertEqual(q['cache_read_ticks'],10);self.assertEqual(q['late_cache_read_ticks'],5)
  for bad in [data[:-1],data[:9]+bytes([data[9]^1])+data[10:]]:
   with self.assertRaises(ValueError):wire.decode(bad,6,77)
  with self.assertRaises(ValueError):wire.decode(data,7,77)
  with self.assertRaises(ValueError):wire.decode(data,6,78)
  self.assertEqual(len(wire.request(6,1)),9);self.assertEqual(len(wire.request(7,1,1)),10)
  for args in [(5,1,0),(7,128,0),(7,1,2)]:
   with self.assertRaises(ValueError):wire.request(*args)
 def test_plan(self):
  p=b.plan();self.assertEqual(len(p),14);self.assertEqual(len({u['work_unit_id'] for u in p}),14)
  self.assertEqual(p[0]['settings'],p[1]['settings']);self.assertEqual([u['profiling'] for u in p[:2]],[0,1]);self.assertTrue(all(u['settings']['mod_index']==0 for u in p));self.assertEqual((p[0]['settings']['osc'],p[0]['settings']['samples']),(33,4))
 def test_reject_silent_tracks(self):
  before={'track_sound_frames':[0]*4,'common_sound_frames':0}
  after={'osc':33,'samples':4,'drums':1,'track_osc':[1,1,1,0],'voice_modes':[2]*4,'osc_per_voice':1,'track_sound_frames':[1000]*4,'common_sound_frames':1000}
  self.assertTrue(b.b.participation(before,after)['verified'])
  after['track_osc']=[0,0,3,0]
  with self.assertRaises(AssertionError):b.b.participation(before,after)
  after['track_osc']=[1,1,1,0];after['track_sound_frames'][1]=0
  with self.assertRaises(AssertionError):b.b.participation(before,after)
 def test_resume_and_signal(self):
  for mode in ['hard','signal']:
   with tempfile.TemporaryDirectory() as d:
    run=Path(d)/'resume';normal=Path(d)/'normal'
    code='''import sys,os,signal
sys.path.insert(0,sys.argv[1])
import profile_benchmark as b
old=b.CheckpointStore.record_result
def record(self,*args,**kwargs):
 result=old(self,*args,**kwargs)
 if self.completed_count==1:
  if sys.argv[-1]=='signal':os.kill(os.getpid(),signal.SIGTERM)
  else:os._exit(77)
 return result
b.CheckpointStore.record_result=record
p=b.argparse.Namespace(run_dir=sys.argv[2],input='Felucca',output='Felucca',seconds=5,canary=False,canary_run=None,simulate=True)
b.run(p)
'''
    q=subprocess.run([sys.executable,'-c',code,str(R),str(run),mode],capture_output=True,text=True);self.assertNotEqual(q.returncode,0,q.stderr)
    rows=(run/'results.jsonl').read_bytes();self.assertEqual(len(rows.splitlines()),1)
    if mode=='signal':self.assertEqual(json.loads((run/'state.json').read_text())['status'],'interrupted')
    with (run/'results.jsonl').open('ab') as f:f.write(b'{"torn":')
    def execute(path,*extra):return subprocess.run([sys.executable,str(R/'profile_benchmark.py'),'--simulate','--run-dir',str(path),*extra],capture_output=True,text=True)
    q=execute(run);self.assertEqual(q.returncode,0,q.stderr);validate(run,require_complete=True);saved=(run/'results.jsonl').read_bytes();self.assertTrue(saved.startswith(rows));self.assertEqual(len(saved.splitlines()),14)
    self.assertEqual(execute(run).returncode,0);self.assertEqual((run/'results.jsonl').read_bytes(),saved);self.assertNotEqual(execute(run,'--seconds','6').returncode,0)
    self.assertEqual(execute(normal).returncode,0)
    a=json.loads((run/'final.json').read_text());z=json.loads((normal/'final.json').read_text());self.assertTrue(a['settings_restored'] and a['profiler_disabled'])
    norm=lambda f:[(r['work_unit_id'],r['payload']['unit'],r['payload']['profile'],r['payload']['classification']) for r in f['results']]
    self.assertEqual(norm(a),norm(z))
if __name__=='__main__':unittest.main()
