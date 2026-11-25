#!/usr/bin/env python3
"""Bounded, resumable 4OSC/4PCM peak investigation; installation is owner-manual."""
import argparse,fcntl,json,os,shutil,sys,tempfile,time
from pathlib import Path
from profile_midi import ProfileLink,STAGES,EVENTS
import stability_benchmark as b
from resumable_eval import CheckpointStore,RunIdentity,StopController,atomic_write_json,fingerprint_json,fingerprint_file,load_jsonl
R=Path(__file__).resolve().parent

def plan():
 units=[]
 for name,profile,samples,osc,enabled in [('3TRK+4PCM-delay-OFF','delays',4,4,0),('3TRK+4PCM-delay-ON','delays',4,4,1),('3TRK+4PCM-bare','bare',4,4,1),('3TRK+4PCM-matrix','matrix',4,4,1),('OSC-only-delay','delays',0,4,1),('PCM-only-delay','delays',4,0,1),('3TRK+4PCM-delay-repeat','delays',4,4,1),('PCM-only-bare','bare',4,0,1)]:
  template=next(u for u in b.plan() if u['profile']==profile and u['settings']['drums']==1)
  settings={**template['settings'],'samples':samples,'osc':33 if osc else 0,'osc_per_voice':1,'mod_index':0}
  unit=dict(name=name,profile=profile,settings=settings,profiling=enabled,stimulus='play')
  unit['work_unit_id']='profile-'+fingerprint_json(unit).split(':')[1][:16];units.append(unit)
 for label,mode,cfg,reverb in [('twoOSC-per-track',2,[0,2,0,0],0),('fourOSC-per-track',1,[0,2,0,0],0),('eightOSC-per-track',2,[0,2,0,0],0),('lofi-32s-PCM16',1,[1,5,0,0],0),('filtered-1s',1,[0,0,0,0],0),('reverb',1,[0,2,0,0],1)]:
  template=units[1];unit={**template,'notes_per_track':4 if label in ['fourOSC-per-track','eightOSC-per-track'] else 1,'name':'3TRK+4PCM-'+label,'settings':{**template['settings'],'osc_per_voice':mode,'reverb':reverb,**{f'delay_{bus}_{f}':cfg[j] for bus in ['a','b'] for j,f in enumerate(['engine','rate','bits','degrade'])}}};unit.pop('work_unit_id',None);unit['work_unit_id']='profile-'+fingerprint_json(unit).split(':')[1][:16];units.append(unit)
 return units

def code_identity():return fingerprint_json({n:fingerprint_file(R/n) for n in ['profile_benchmark.py','profile_midi.py','stability_benchmark.py','benchmark_midi.py','download_performance.py','resumable_eval.py','verified-build.json']})

def measure(link,unit,seconds,stop):
 for key,value in unit['settings'].items():link.set(key,value)
 link.set('panic',1);before=link.get();assert before['session'] and not before['playing']
 for key,value in unit['settings'].items():assert before[key]==value,(key,before[key],value)
 link.profile(7,unit['profiling']);base=link.perf(True);assert base['firmware']=='DUB R39' and not base['usb']['flowing']
 if getattr(link,'simulated',False):link.mock_play=True;link.seconds=seconds
 start=time.monotonic();held=set()
 try:
  for at,on,ch,n in b.events(seconds,before['channels'],1,unit.get('notes_per_track',1)):
   if stop.requested:raise InterruptedError('Stop requested')
   delay=start+at-time.monotonic()
   if delay>0 and not getattr(link,'simulated',False):time.sleep(delay)
   link.note(ch,n,on)
   if on:held.add((ch,n))
   else:held.discard((ch,n))
  delay=start+seconds-time.monotonic()
  if delay>0 and not getattr(link,'simulated',False):time.sleep(delay)
  if stop.requested:raise InterruptedError('Stop requested')
  report=link.perf();profile=link.profile(7,0) if unit['profiling'] else None
  after=link.get()
  proof=b.participation(before,after,unit.get('notes_per_track',1))
 finally:
  link.profile(7,0)
  for ch,n in held:link.note(ch,n,False)
  link.set('panic',1)
 assert before['pattern']==after['pattern'] and after['session'] and not after['playing']
 assert b.data_identity(before)==b.data_identity(after),'Sound/sample/routing changed'
 for key,value in unit['settings'].items():assert after[key]==value,'Runtime changed: '+key
 assert before['clock_mode']==after['clock_mode']==0 and before['clock_output']==after['clock_output']==0
 assert base['window_generation']==report['window_generation'] and report['uptime_ms']>=base['uptime_ms'],'Device reset inside window'
 assert not report['usb']['flowing']
 if unit['profiling']:
  assert profile['blocks']>0 and profile['frames']>=seconds*44100*.8,'Insufficient stage profile coverage'
  if before['samples']:assert profile['events']['drum_on'] and profile['events']['sample_cache_fill'],'No actual PCM read/trigger evidence'
  if before['osc']:assert profile['events']['synth_note_on'] and after['active_osc'],'No synth evidence'
 import download_performance as perf
 report['counter_deltas']={k:(report['counters'][k]-base['counters'][k])&0xffffffff for k in perf.COUNTERS}
 return dict(participation=proof,unit=unit,before=before,after=after,report=report,profile=profile,classification=b.classify(report,seconds),wall_seconds=time.monotonic()-start,simulated=getattr(link,'simulated',False),note='OFF calibration has no per-stage/source-event telemetry. ON uses actual trigger/cache-fill evidence, not a short-lived post-window sample probe.')

class MockProfile(b.MockLink):
 def profile(self,command=6,enabled=0):
  if command==7:self.profile_enabled=enabled
  total={s:1000*(i+1) for i,s in enumerate(STAGES)}
  return dict(enabled=bool(getattr(self,'profile_enabled',0)),blocks=864,frames=221184,late_blocks=0,stage_ticks=total,late_stage_ticks={s:0 for s in STAGES},events=dict(zip(EVENTS,[100,12 if self.s['osc'] else 0,55,55 if self.s['samples'] else 0,100 if self.s['samples'] else 0,6912])),slowest_blocks=[dict(load_percent=70,stage_ticks=total,stage_ms={s:t/24000 for s,t in total.items()},events=dict(zip(EVENTS,[0,0,0,0,4,8])))])

def summarize(rows):
 lines=['# R39 targeted stage profile','','Real hardware unless explicitly marked simulated. Timings include nested IRQ/preemption and profiler overhead. OFF/ON cases calibrate added instrumentation; no automatic overhead subtraction. Profile and performance snapshots may differ by a few final blocks.','','| Case | ISR mean | ISR peak | Late blocks | Profile late blocks |','|---|---:|---:|---:|---:|']
 for row in rows:
  p=row['payload'];s=p['report']['scopes']['isr'];profile=p['profile'];lines.append(f"| {p['unit']['name']} | {s['average_load_percent']:.2f}% | {s['peak_load_percent']:.2f}% | {s['late_blocks']}/{s['blocks']} | {profile['late_blocks'] if profile else 'OFF'} |")
 lines+=['','## Verified track participation','','| Case | OSC per track | Nonzero frames per track | All4 frames | Pre-limiter clip frames | Limiter gain Q15 |','|---|---|---|---:|---:|---:|']
 for row in rows:
  p=row['payload'];v=p['participation'];lines.append('| '+p['unit']['name']+' | '+str(v['track_osc'])+' | '+str(v['nonzero_block_frames'])+' | '+str(v['all_four_block_frames'])+' | '+str(v['prelimit_clip_frames'])+' | '+str(v['limiter_gain_q15'])+' |')
 lines+=['','## Core1 hardware evidence','','| Case | State(3=ON) | Jobs | Worker ms | Wait ms | Timeouts |','|---|---:|---:|---:|---:|---:|']
 for row in rows:
  p=row['payload'];v=p['participation'];lines.append('| '+p['unit']['name']+' | '+str(v['core_state'])+' | '+str(v['core_jobs'])+' | '+str(v['core_worker_ms'])+' | '+str(v['core_wait_ms'])+' | '+str(v['core_timeouts'])+' |')
 for row in rows:
  p=row['payload'];q=p['profile']
  if not q:continue
  lines+=['', '## '+p['unit']['name'],'', '| Slow block | Load | Commands ms | Control ms | Synth ms | Drums ms | FX ms | Output ms | Cache read ms (inside drums) | Cache fills | MIDI commands |','|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|']
  for i,record in enumerate(q['slowest_blocks']):lines.append('| '+str(i+1)+' | '+f"{record['load_percent']:.2f}% | "+' | '.join(f"{record['stage_ms'][s]:.3f}" for s in STAGES)+f" | {record.get('cache_read_ms',0):.3f} | {record['events']['sample_cache_fill']} | {record['events']['commands']} |")
 return '\n'.join(lines)+'\n'

def run(args,link=None,stop=None):
 lock=fingerprint_json([args.input,args.output,args.simulate]).split(':')[1][:20]
 with (Path(tempfile.gettempdir())/('dub-benchmark-'+lock+'.lock')).open('a') as guard:
  fcntl.flock(guard,fcntl.LOCK_EX|fcntl.LOCK_NB)
  root=Path(args.run_dir).resolve();parent=root.parent;parent.mkdir(parents=True,exist_ok=True)
  if not args.simulate:assert root.is_relative_to((R.parent.parent/'dub-synth/hardware-reports/benchmarks').resolve())
  assert shutil.disk_usage(parent).free>16*1024*1024
  sentinel=parent/('.profile-write-'+str(os.getpid())+'.json');atomic_write_json(sentinel,{'ok':True});assert json.loads(sentinel.read_text())=={'ok':True};sentinel.unlink()
  host=b.host_profile();stop=stop or StopController();stop.install();link=link or (MockProfile if args.simulate else ProfileLink)(args.input,args.output);session=False;store=None
  try:
   initial=link.get()
   if initial['session']:link.set('session',0);initial=link.get()
   assert not initial['playing'] and len(set(initial['channels']))==4
   assert link.perf()['firmware']=='DUB R39' and not link.perf()['usb']['flowing']
   units=plan();chosen=units[:2] if args.canary else units
   family=fingerprint_json(dict(plan=units,sound=b.data_identity(initial),code=code_identity(),seconds=args.seconds,simulated=args.simulate,ports=[args.input,args.output]))
   if not args.canary and not args.simulate:
    from validate_run_state import validate
    c=Path(args.canary_run).resolve();assert c!=root;validate(c,require_complete=True)
    proof=json.loads((c/'final.json').read_text());assert proof['canary'] and proof['family']==family and not proof['failures'] and not proof['simulated'];assert proof['host']['peak_rss_bytes']<=proof['host']['physical_ram_bytes']*.7;assert all(r['payload']['classification']!='insufficient-data' for r in proof['results'])
   identity=RunIdentity(root.name,fingerprint_json(dict(plan=units,stimulus='R39 one held note per melodic track plus four drum pads,5s')),fingerprint_json(dict(family=family,canary=args.canary,count=len(chosen))),code_identity(),len(chosen))
   store=CheckpointStore(root,identity)
   if (root/'COMPLETE').exists():print('Already complete:',root);return
   link.set('session',1);session=True;start=time.monotonic()
   for unit in chosen:
    uid=unit['work_unit_id']
    if store.is_accounted(uid):continue
    if stop.requested:raise InterruptedError('Stop requested')
    try:payload=measure(link,unit,args.seconds,stop)
    except InterruptedError:raise
    except Exception as e:store.record_failure(uid,dict(type=type(e).__name__,message=str(e)));raise
    payload['host']=b.host_profile();store.record_result(uid,payload);done=store.completed_count+store.failed_count
    print(f"{done}/{len(chosen)} durable {unit['name']}: {payload['classification']}; ETA <= {(len(chosen)-done)*(args.seconds+3):.0f}s; {root}",flush=True)
   link.profile(7,0);restored=link.set('session',0);session=False
   for key in b.RESTORE:assert restored[key]==initial[key],'Restore mismatch: '+key
   rows=load_jsonl(root/'results.jsonl',repair_torn_tail=False);failures=load_jsonl(root/'failures.jsonl',repair_torn_tail=False)
   (root/'summary.md').write_text(('SIMULATED — not hardware evidence\n\n' if args.simulate else '')+summarize(rows))
   store.finish(dict(canary=args.canary,family=family,simulated=args.simulate,host=b.host_profile(),results=rows,failures=failures,settings_restored=True,profiler_disabled=True,elapsed=time.monotonic()-start),allow_failures=True);print('COMPLETE:',root)
  except BaseException as e:
   if store and not (root/'COMPLETE').exists():store.mark_interrupted(type(e).__name__+': '+str(e))
   raise
  finally:
   if session:
    try:link.profile(7,0);link.set('session',0)
    except Exception:print('No restore ACK; existing15s runtime lease disables profiler/restores settings.',file=sys.stderr)
   link.close()

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--run-dir',required=True);p.add_argument('--input',default='Felucca');p.add_argument('--output',default='Felucca');p.add_argument('--seconds',type=float,default=5);p.add_argument('--canary',action='store_true');p.add_argument('--canary-run');p.add_argument('--simulate',action='store_true');a=p.parse_args();assert a.seconds==5,'Keep versioned R39 5s workload';run(a)
if __name__=='__main__':main()
