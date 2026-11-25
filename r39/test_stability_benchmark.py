import json,os,subprocess,sys,tempfile,unittest
from pathlib import Path
from stability_benchmark import plan,classify,events
from benchmark_midi import request,decode
from validate_run_state import validate
R=Path(__file__).resolve().parent
class Checks(unittest.TestCase):
 def test_plan_and_classification(self):
  p=plan();self.assertEqual(len(p),16);self.assertEqual(len({x['work_unit_id'] for x in p}),16);self.assertEqual([x['stimulus'] for x in p[:3]],['play','play','play'])
  s=dict(blocks=100,average_load_percent=50,peak_load_percent=70,late_blocks=0,budget_window_seconds=5);r=dict(scopes={'isr':s},counter_deltas={'dsp_clip_samples':0});self.assertEqual(classify(r,5),'stable');s['late_blocks']=1;self.assertEqual(classify(r,5),'unstable');s['late_blocks']=0;s['budget_window_seconds']=1;self.assertEqual(classify(r,5),'insufficient-data');s['average_load_percent']=130;self.assertEqual(classify(r,5),'unstable')
  self.assertTrue(all(0<=x[0]<=5 for x in events(5,[1,2,3,4])))
 def test_runtime_wire(self):
  p=R.parent.parent/'.build-env/dub-r39-runtime-reply.bin';s=decode(p.read_bytes(),4,77);self.assertEqual((s['samples'],s['osc'],s['active_osc']),(2,4,2))
  for cmd,seq in [(5,77),(4,78)]:
   with self.assertRaises(ValueError):decode(p.read_bytes(),cmd,seq)
  bad=bytearray(p.read_bytes());bad[-2]^=1
  with self.assertRaises(ValueError):decode(bad,4,77)
  self.assertEqual(len(request(5,1,4,12)),11)
  for field,v in [(4,13),(5,34),(8,0),(20,0)]:
   with self.assertRaises(ValueError):request(5,1,field,v)
 def test_crash_resume_mismatch_and_equivalence(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);run=root/'resume';normal=root/'normal';code='''import os,signal,sys
sys.path.insert(0,sys.argv[1])
import stability_benchmark as b
old=b.CheckpointStore.record_result
def record(self,*a,**k):
 result=old(self,*a,**k)
 if self.completed_count==1:os._exit(77)
 return result
b.CheckpointStore.record_result=record
sys.argv=['benchmark','--simulate','--run-dir',sys.argv[2]]
b.main()
'''
   q=subprocess.run([sys.executable,'-c',code,str(R),str(run)],capture_output=True,text=True);self.assertEqual(q.returncode,77,q.stderr)
   first=(run/'results.jsonl').read_bytes();self.assertEqual(len(first.splitlines()),1)
   with (run/'results.jsonl').open('ab') as f:f.write(b'{"torn":')
   def execute(path,*extra):return subprocess.run([sys.executable,str(R/'stability_benchmark.py'),'--simulate','--run-dir',str(path),*extra],capture_output=True,text=True)
   q=execute(run);self.assertEqual(q.returncode,0,q.stderr);self.assertTrue((run/'results.jsonl').read_bytes().startswith(first));validate(run,require_complete=True)
   rows=[json.loads(x) for x in (run/'results.jsonl').read_text().splitlines()];self.assertEqual(len(rows),16);self.assertEqual(len({x['work_unit_id'] for x in rows}),16)
   saved=(run/'results.jsonl').read_bytes();self.assertEqual(execute(run).returncode,0);self.assertEqual((run/'results.jsonl').read_bytes(),saved)
   self.assertNotEqual(execute(run,'--seconds','6').returncode,0);self.assertEqual((run/'results.jsonl').read_bytes(),saved)
   q=execute(normal);self.assertEqual(q.returncode,0,q.stderr);other=[json.loads(x) for x in (normal/'results.jsonl').read_text().splitlines()]
   compare=lambda data:[(x['work_unit_id'],x['payload']['unit'],x['payload']['classification'],x['payload']['report']['scopes']) for x in data]
   self.assertEqual(compare(rows),compare(other))
 def test_sigterm_resume(self):
  with tempfile.TemporaryDirectory() as d:
   run=Path(d)/'sigterm';code='''import os,signal,sys
sys.path.insert(0,sys.argv[1])
import stability_benchmark as b
old=b.CheckpointStore.record_result
def record(self,*a,**k):
 result=old(self,*a,**k)
 if self.completed_count==1:os.kill(os.getpid(),signal.SIGTERM)
 return result
b.CheckpointStore.record_result=record
sys.argv=['benchmark','--simulate','--run-dir',sys.argv[2],'--canary']
b.main()
'''
   q=subprocess.run([sys.executable,'-c',code,str(R),str(run)],capture_output=True,text=True);self.assertNotEqual(q.returncode,0);self.assertEqual(json.loads((run/'state.json').read_text())['status'],'interrupted')
   q=subprocess.run([sys.executable,str(R/'stability_benchmark.py'),'--simulate','--canary','--run-dir',str(run)],capture_output=True,text=True);self.assertEqual(q.returncode,0,q.stderr);validate(run,require_complete=True)
if __name__=='__main__':unittest.main()
