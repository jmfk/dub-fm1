#!/usr/bin/env python3
"""Resumable, bounded MIDI stability benchmark. Firmware flashing is always manual."""
import argparse,itertools,json,os,resource,shutil,sys,time,tempfile,fcntl
from pathlib import Path
from benchmark_midi import Link,FIELDS
from resumable_eval import CheckpointStore,RunIdentity,StopController,atomic_write_json,fingerprint_json,fingerprint_file,load_jsonl,utc_now
R=Path(__file__).resolve().parent
RESTORE=['osc_per_voice','matrix','delays','reverb','samples','osc','mod_index','drums','clock_mode','clock_output']+[f'delay_{b}_{f}' for b in ['a','b'] for f in ['engine','rate','bits','degrade']]
THRESHOLDS=dict(average_max=80,peak_max=95,min_audio_coverage=.90)
def plan():
 units=[]
 for profile in ['bare','matrix','delays','full']:
  for mode in [1,2,3,4]:
   matrix,delays,reverb={'bare':(0,0,0),'matrix':(1,0,0),'delays':(1,1,0),'full':(1,1,1)}[profile]
   settings=dict(osc_per_voice=mode,matrix=matrix,delays=delays,reverb=reverb,samples=4,osc=33,mod_index=0,drums=1,**{f'delay_{bus}_{f}':2 if f=='rate' else 0 for bus in ['a','b'] for f in ['engine','rate','bits','degrade']})
   unit=dict(profile=profile,settings=settings,stimulus='play');unit['work_unit_id']='case-'+fingerprint_json(unit).split(':')[1][:16];units.append(unit)
 return units
def data_identity(s):return {k:s[k] for k in ['channels','sound_hash','sample_generation','sample_crc','usb_mode','usb_groups','tempo']}
def classify(report,seconds):
 s=report['scopes']['isr'];a=s['average_load_percent'];peak=s['peak_load_percent'];coverage=s['budget_window_seconds']/seconds
 if a is None or peak is None or s['blocks']==0:return 'insufficient-data'
 if s['late_blocks'] or a>=100 or any(report['counter_deltas'].values()):return 'unstable'
 if coverage<THRESHOLDS['min_audio_coverage']:return 'insufficient-data'
 return 'stable' if a<=THRESHOLDS['average_max'] and peak<=THRESHOLDS['peak_max'] else 'low-headroom'
def events(seconds,channels,drums=1,notes_per_track=1):
 # One held note per melodic track: even MONO tracks participate, no12-note burst.
 out=[(.1,True,ch-1,n+t*2) for t,ch in enumerate(channels[:3] if drums else channels) for n in [48,55,58,63][:notes_per_track]]
 for at in sorted(set([i+.2 for i in range(int(seconds))]+[seconds-.15])) if drums else []:
  for note in [36,38,42,45]:out.extend([(at,True,channels[3]-1,note),(at+.08,False,channels[3]-1,note)])
 return sorted(out,key=lambda e:e[0])
def participation(before,after,notes_per_track=1):
 expected=([0,1,2] if after['osc'] else [])+([3] if after['samples'] and after['drums'] else [])
 counts=after['track_osc'];frames=[(y-x)&0xffffffff for x,y in zip(before['track_sound_frames'],after['track_sound_frames'])]
 target=[min(notes_per_track,1<<after['voice_modes'][p])*min(8>>after['voice_modes'][p],[0,1,2,4,8][after['osc_per_voice']]) if after['osc'] else 0 for p in range(3)]
 assert counts[:3]==target,('Intended melodic voices missing',counts,target)
 assert all(frames[p]>256 for p in expected),('Intended track was silent',expected,counts,frames)
 common=(after['common_sound_frames']-before['common_sound_frames'])&0xffffffff
 if len(expected)==4:assert common>256,'No measured blocks with all four tracks contributing'
 return dict(core_state=after.get('core_state',0),core_jobs=(after.get('core_jobs',0)-before.get('core_jobs',0))&0xffffffff,core_wait_ms=((after.get('core_wait_ticks',0)-before.get('core_wait_ticks',0))&0xffffffff)/24000,core_worker_ms=((after.get('core_worker_ticks',0)-before.get('core_worker_ticks',0))&0xffffffff)/24000,core_timeouts=(after.get('core_timeouts',0)-before.get('core_timeouts',0))&0xffffffff,prelimit_clip_frames=(after.get('prelimit_clip_frames',0)-before.get('prelimit_clip_frames',0))&0xffffffff,limiter_gain_q15=after.get('limiter_gain_q15',32768),expected_tracks=[p+1 for p in expected],track_osc=counts,nonzero_block_frames=frames,all_four_block_frames=common,verified=True)
def measure(link,unit,seconds,stop):
 for key,v in unit['settings'].items():link.set(key,v)
 link.set('panic',1)
 before=link.get();assert before['session'] and not before['playing']
 for k,v in unit['settings'].items():assert before[k]==v,(k,before[k],v)
 if getattr(link,'simulated',False):link.mock_play=unit['stimulus']=='play';link.seconds=seconds
 base=link.perf(True);assert base['firmware']=='DUB R39' and not base['usb']['flowing']
 held=set();start=time.monotonic()
 try:
  for at,on,ch,n in events(seconds,before['channels'],before['drums']) if unit['stimulus']=='play' else []:
   if stop.requested:raise InterruptedError('Stop requested')
   delay=start+at-time.monotonic()
   if delay>0 and not getattr(link,'simulated',False):time.sleep(delay)
   link.note(ch,n,on)
   if on:held.add((ch,n))
   else:held.discard((ch,n))
  if stop.requested:raise InterruptedError('Stop requested')
  delay=start+seconds-time.monotonic()
  if delay>0 and not getattr(link,'simulated',False):time.sleep(delay)
  report=link.perf()
  assert not report['usb']['flowing'],'USB recording started during window'
  if unit['stimulus']=='play' and before['drums']:link.note(before['channels'][3]-1,45,True);held.add((before['channels'][3]-1,45))
  after=link.get()
  proof=participation(before,after) if unit['stimulus']=='play' else None
 finally:
  for ch,n in held:link.note(ch,n,False)
  link.set('panic',1)
 assert base['window_generation']==report['window_generation'] and report['uptime_ms']>=base['uptime_ms'],'Counter reset/device reboot inside window'
 assert before['pattern']==after['pattern'] and after['session'] and not after['playing'],'Session/transport changed during window'
 assert data_identity(before)==data_identity(after),'Sound/sample/routing changed during window'
 for k,v in unit['settings'].items():assert after[k]==v,'Runtime changed during window: '+k
 if not before['osc']:assert after['active_osc']==0,'OSC OFF was not silent'
 if not before['samples']:assert after['active_samples']==0,'SAMPLES OFF was not silent'
 if unit['stimulus']=='play' and before['osc']:assert after['active_osc']>0,'No synth voices were observed; check routing'
 if unit['stimulus']=='play' and before['samples'] and before['drums']:assert after['active_samples']>0,'No sample voice in post-window probe; check routing'
 assert before['clock_mode']==after['clock_mode']==0 and before['clock_output']==after['clock_output']==0,'Benchmark clock changed'
 import download_performance as perf
 report['counter_deltas']={k:(report['counters'][k]-base['counters'][k])&0xffffffff for k in perf.COUNTERS}
 return dict(participation=proof,unit=unit,before=before,after=after,report=report,classification=classify(report,seconds),wall_seconds=time.monotonic()-start,simulated=getattr(link,'simulated',False))
class MockLink:
 simulated=True
 def __init__(self,*unused):
  self.s=dict(osc_per_voice=4,matrix=1,delays=1,reverb=0,samples=4,osc=8,mod_index=0,drums=0,session=0,playing=0,pattern=0,channels=[1,2,3,4],active_osc=0,active_samples=0,usb_mode=0,usb_groups=7,sound_hash=123,sample_generation=0,sample_crc=0,revision=0,audio_frame=0,tempo=120,clock_mode=2,clock_output=0);self.s.update({f'delay_{b}_{f}':2 if f=='rate' else 0 for b in ['a','b'] for f in ['engine','rate','bits','degrade']});self.backup=None;self.held=set();self.gen=0;self.mock_play=False;self.seconds=5
 def get(self):
  self.s['active_osc']=min(8*len({ch for ch,n in self.held if ch<3 or not self.s['drums']}),self.s['osc'] if self.s['osc']<=32 else 32) if self.s['osc'] else 0
  self.s['active_samples']=min(11,self.s['samples']) if self.s['samples'] and self.s['drums'] and any(ch==3 for ch,n in self.held) else 0
  self.s.update(track_osc=[min([0,1,2,4,8][self.s['osc_per_voice']],2)*min(4,len({n for c,n in self.held if c==t})) if self.s['osc'] else 0 for t in range(3)]+[0],voice_modes=[2]*4,track_sound_frames=[220000 if self.mock_play and (self.s['osc'] if t<3 else self.s['samples']) else 0 for t in range(4)],common_sound_frames=10000 if self.mock_play and self.s['osc'] and self.s['samples'] else 0)
  self.s['active_osc']=sum(self.s['track_osc'])
  return dict(self.s)
 def set(self,k,v):
  if k=='session':
   if v and not self.s['session']:self.backup={k:self.s[k] for k in RESTORE};self.s.update(session=1,clock_mode=0,clock_output=0)
   elif not v and self.s['session']:self.s.update(self.backup);self.s['session']=0;self.held.clear()
  elif k=='panic':self.held.clear();self.mock_play=False
  else:self.s[k]=v
  self.s['revision']+=1;return self.get()
 def note(self,ch,n,on,velocity=90):
  self.mock_play=True
  if on:self.held.add((ch,n))
  else:self.held.discard((ch,n))
 def perf(self,reset=False):
  if reset:self.gen+=1
  osc=min(24 if self.s['drums'] else 32,self.s['osc']);smp=min(11,self.s['samples']) if self.s['drums'] else 0
  avg=10+5*self.s['matrix']+20*self.s['delays']+25*self.s['reverb']+(osc*2+smp if self.mock_play else 0);peak=avg*1.1
  scope=dict(blocks=860,frames_counter=220160,average_load_percent=avg,peak_load_percent=peak,late_blocks=10 if avg>=100 else 0,budget_window_seconds=self.seconds)
  import download_performance as p
  return dict(firmware='DUB R39',window_generation=self.gen,uptime_ms=10000+self.gen*10000+(0 if reset else 5000),scopes={'app':dict(scope),'isr':dict(scope)},counters={k:0 for k in p.COUNTERS},usb={'flowing':False})
 def close(self):pass
def host_profile():
 rss=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss*(1 if sys.platform=='darwin' else 1024);ram=os.sysconf('SC_PHYS_PAGES')*os.sysconf('SC_PAGE_SIZE');assert rss<=ram*.7,'Host memory headroom gate failed'
 usage=resource.getrusage(resource.RUSAGE_SELF)
 return dict(peak_rss_bytes=rss,physical_ram_bytes=ram,memory_gate_fraction=.7,process_swaps=usage.ru_nswap,major_page_faults=usage.ru_majflt,oom_evidence='Process alive/completed; no system-wide OOM audit')
def code_identity():return fingerprint_json({n:fingerprint_file(R/n) for n in ['stability_benchmark.py','benchmark_midi.py','download_performance.py','resumable_eval.py','verified-build.json']})
def run(args,link=None,stop=None):
 lock_id=fingerprint_json([args.input,args.output,args.simulate]).split(':')[1][:20]
 with (Path(tempfile.gettempdir())/('dub-benchmark-'+lock_id+'.lock')).open('a') as lock:
  try:fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
  except BlockingIOError:raise RuntimeError('Another benchmark owns these MIDI ports')
  return _run(args,link,stop)
def _run(args,link=None,stop=None):
 host_profile()
 units=plan();selected=units[:3] if args.canary else units
 root=Path(args.run_dir).resolve();assert root.name,'A concrete persistent run directory is required'
 if not args.simulate:assert root.is_relative_to((R.parent.parent/'dub-synth/hardware-reports/benchmarks').resolve()),'Hardware results must stay under the persistent project benchmark directory'
 parent=root.parent;parent.mkdir(parents=True,exist_ok=True);assert shutil.disk_usage(parent).free>16*1024*1024,'Insufficient disk for checkpoints'
 sentinel=parent/('.benchmark-write-'+str(os.getpid())+'.json');atomic_write_json(sentinel,{'ok':True});assert json.loads(sentinel.read_text())=={'ok':True};sentinel.unlink()
 stop=stop or StopController();stop.install();link=link or (MockLink if args.simulate else Link)(args.input,args.output)
 store=None;session=False
 try:
  initial=link.get()
  if initial['session']:link.set('session',0);initial=link.get()
  assert not initial['playing'] and len(set(initial['channels']))==4,'Stop transport; MIDI channels must be distinct'
  assert link.perf()['firmware']=='DUB R39','Manually flash R39 before running'
  assert not link.perf()['usb']['flowing'],'Stop USB audio recording before running'
  shared=dict(plan=units,seconds=args.seconds,thresholds=THRESHOLDS,sound=data_identity(initial),code=code_identity(),simulated=args.simulate,ports=[args.input,args.output])
  family=fingerprint_json(shared)
  if not args.canary and not args.simulate:
   assert args.canary_run,'A separate successful hardware canary is required'
   c=Path(args.canary_run).resolve();assert c!=root
   from validate_run_state import validate
   validate(c,require_complete=True)
   proof=json.loads((c/'final.json').read_text());assert proof['canary'] and proof['family_fingerprint']==family and not proof['failures'] and not proof['simulated'],'Canary input/code mismatch or transport failures'
   assert all(x['payload']['classification']!='insufficient-data' for x in proof['results']),'Canary did not produce adequate timing evidence'
   assert proof['host']['peak_rss_bytes']<=proof['host']['physical_ram_bytes']*.7,'Canary memory gate failed'
  identity=RunIdentity(root.name,fingerprint_json(dict(plan=units,stimulus='four-melody-or-three-plus-eleven-drums-v2')),fingerprint_json(dict(shared=shared,canary=args.canary,unit_limit=len(selected))),code_identity(),len(selected))
  store=CheckpointStore(root,identity)
  if (root/'COMPLETE').exists():print('Already complete:',root,flush=True);return
  session=True;link.set('session',1);started=time.monotonic()
  for unit in selected:
   uid=unit['work_unit_id']
   if store.is_accounted(uid):continue
   if stop.requested:raise InterruptedError('Requested stop')
   try:payload=measure(link,unit,args.seconds,stop)
   except InterruptedError:raise
   except Exception as e:
    store.record_failure(uid,dict(type=type(e).__name__,message=str(e)));raise
   payload['host']=host_profile();store.record_result(uid,payload)
   done=store.completed_count+store.failed_count;elapsed=time.monotonic()-started
   print(f'{done}/{len(selected)} durable: {unit["profile"]} S{unit["settings"]["samples"]}/O{unit["settings"]["osc"]} {unit["stimulus"]}: {payload["classification"]}, ISR {payload["report"]["scopes"]["isr"]["average_load_percent"]:.1f}%, ETA <= {(len(selected)-done)*(args.seconds+2):.0f}s; {root}',flush=True)
  restored=link.set('session',0);session=False
  for k in RESTORE:assert restored[k]==initial[k],'Restore readback mismatch: '+k
  records=load_jsonl(root/'results.jsonl',repair_torn_tail=False);failures=load_jsonl(root/'failures.jsonl',repair_torn_tail=False)
  stable=[x['payload'] for x in records if x['payload']['classification']=='stable'];stable.sort(key=lambda x:(x['unit']['profile'],x['unit']['settings']['samples'],x['unit']['settings']['osc']))
  write_summary(root,records,failures,args.simulate)
  store.finish(dict(family_fingerprint=family,canary=args.canary,simulated=args.simulate,stable=stable,stable_frontier=frontier(stable),results=records,failures=failures,settings_restored=True,thresholds=THRESHOLDS,host=host_profile(),elapsed_this_invocation=time.monotonic()-started,scope='Sampled audio deadline/headroom benchmark, not proof for every patch or audible verification.'),allow_failures=True)
  print('COMPLETE:',root,'stable',len(stable),'/',len(records),flush=True)
 except BaseException as e:
  if store and not (root/'COMPLETE').exists():store.mark_interrupted(type(e).__name__+': '+str(e))
  raise
 finally:
  if session:
   try:link.set('session',0)
   except Exception:print('Restore reply unavailable; firmware lease restores after 15s without runtime heartbeats.',file=sys.stderr)
  link.close()
def frontier(stable):
 groups={}
 for p in stable:
  u=p['unit'];c=u['settings']
  if u['stimulus']!='play':continue
  key=(u['profile'],c['drums'],c['mod_index']);sample=min(c['samples'],11) if c['drums'] else 0;osc=min(c['osc'],24 if c['drums'] else 32)
  groups.setdefault(key,[]).append((sample,osc,p))
 out=[]
 for group,rows in groups.items():
  for sample,osc,p in rows:
   if any(s>=sample and o>=osc and (s>sample or o>osc) for s,o,_ in rows):continue
   out.append(dict(profile=group[0],drums=group[1],mod_samples=[32,16,8,4][group[2]],samples=p['unit']['settings']['samples'],osc=p['unit']['settings']['osc'],effective_samples=sample,effective_osc=osc,silent=not (sample or osc),work_unit_id=p['unit']['work_unit_id'],isr=p['report']['scopes']['isr']))
 return out
def caplabel(v,inf):return 'OFF' if not v else 'INF' if v==inf else str(v)
def write_summary(root,records,failures,simulated):
 from resumable_eval import atomic_write
 lines=['# DUB stability benchmark', '', '**SIMULATED — no device measurements.**' if simulated else 'Measured FM-1 audio budget; stability applies only to this sound/configuration and short workload.', '', '| Profile | Stimulus/type | SAMPLES | OSC | Mod samples | ISR avg % | Peak % | Late blocks | Audio coverage | Result |','|---|---|---:|---:|---:|---:|---:|---:|---:|---|']
 for row in records:
  p=row['payload'];u=p['unit'];c=u['settings'];s=p['report']['scopes']['isr'];caps=caplabel
  lines.append(f"| {u['profile']} | {u['stimulus']}/{'DRUM' if c['drums'] else '4SYN'} | {caps(c['samples'],12)} | {caps(c['osc'],33)} | {[32,16,8,4][c['mod_index']]} | {s['average_load_percent']:.2f} | {s['peak_load_percent']:.2f} | {s['late_blocks']}/{s['blocks']} | {s['budget_window_seconds']:.2f}s | {p['classification']} |")
 lines+=['','## Stable tested frontier','']
 for row in frontier([r['payload'] for r in records if r['payload']['classification']=='stable']):
  lines.append(f"- {row['profile']}, {'DRUM' if row['drums'] else '4SYN'}, mod {row['mod_samples']}: SAMPLES {caplabel(row['samples'],12)}, OSC {caplabel(row['osc'],33)}"+(' (silent baseline)' if row['silent'] else ''))
 lines+=['',f'Configuration/transport errors: {len(failures)}. These are not DSP stability results.','Stable: ISR average <=80%, peak <=95%, zero late blocks and monitored error deltas, >=90% measured audio-frame coverage.','The final short sample probe and readback occur after the timed CPU snapshot. Settings are restored after the run.']
 atomic_write(root/'summary.md',('\n'.join(lines)+'\n').encode())
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--input',default='Felucca');p.add_argument('--output',default='Felucca');p.add_argument('--run-dir',required=True);p.add_argument('--seconds',type=float,default=5.0);p.add_argument('--canary',action='store_true');p.add_argument('--canary-run');p.add_argument('--simulate',action='store_true');a=p.parse_args();assert 3<=a.seconds<=8,'Bounded windows must fit inside the 15s lease';run(a)
if __name__=='__main__':main()
