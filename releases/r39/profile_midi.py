#!/usr/bin/env python3
"""Opt-in R39 stage telemetry. No updater or persistent writes."""
import time
from benchmark_midi import Link,PREFIX
STAGES=['commands','control','synth','drums','fx','output']
EVENTS=['commands','synth_note_on','note_off','drum_on','sample_cache_fill','control_updates']
FIELDS=116

def request(command,seq,enabled=0):
 if command not in (6,7) or not 0<=seq<=127 or enabled not in (0,1):raise ValueError('Invalid profile request')
 return b'\xf0'+PREFIX+bytes([command,seq])+ (bytes([enabled]) if command==7 else b'')+b'\xf7'

def decode(wire,command,seq):
 w=bytes(wire)
 if len(w)!=11+FIELDS*5 or w[:6]!=b'\xf0'+PREFIX or w[6:9]!=bytes([command|64,seq,FIELDS]) or w[-1]!=247 or any(x>127 for x in w[1:-1]):raise ValueError('Invalid R39 profile identity/size')
 check=0
 for x in w[1:-2]:check^=x
 if check!=w[-2]:raise ValueError('Profile checksum mismatch')
 values=[]
 for i in range(FIELDS):
  word=w[9+i*5:14+i*5]
  if word[4]>15:raise ValueError('Overflowing profile word')
  values.append(sum(x<<(7*j) for j,x in enumerate(word)))
 v=values
 if v[0]!=39 or v[1]!=24000000 or v[2]>1 or v[6]>v[4]:raise ValueError('Invalid profile configuration')
 total={s:v[8+2*i]+(v[9+2*i]<<32) for i,s in enumerate(STAGES)}
 late={s:v[20+2*i]+(v[21+2*i]<<32) for i,s in enumerate(STAGES)}
 if any(late[s]>total[s] for s in STAGES):raise ValueError('Impossible late-stage totals')
 top=[]
 for k in range(4):
  b=v[44+k*17:61+k*17]
  if not b[1]:
   if any(b):raise ValueError('Invalid empty profile record')
   continue
  if not b[2] or sum(b[4:10])!=b[0] or b[16]>b[7]:raise ValueError('Profile stage sum/budget mismatch')
  top.append(dict(ticks=b[0],frames=b[1],budget_ticks=b[2],frame=b[3],seconds_from_window=((b[3]-v[7])&0xffffffff)/44100,load_percent=b[0]*100/b[2],stage_ticks=dict(zip(STAGES,b[4:10])),stage_ms={s:t/24000 for s,t in zip(STAGES,b[4:10])},events=dict(zip(EVENTS,b[10:16])),cache_read_ticks=b[16],cache_read_ms=b[16]/24000))
 if any(top[i]['load_percent']<top[i+1]['load_percent'] for i in range(len(top)-1)):raise ValueError('Unsorted top profile blocks')
 return dict(schema='dub-stage-profile/v1',firmware='DUB R39',timer_hz=v[1],enabled=bool(v[2]),generation=v[3],blocks=v[4],frames=v[5],late_blocks=v[6],start_frame=v[7],stage_ticks=total,late_stage_ticks=late,cache_read_ticks=v[112]+(v[113]<<32),late_cache_read_ticks=v[114]+(v[115]<<32),stage_max_ticks=dict(zip(STAGES,v[32:38])),events=dict(zip(EVENTS,v[38:44])),slowest_blocks=top,scope='Stage wall time includes nested IRQ preemption and timing overhead. Statistics bookkeeping is outside stage timers but inside app/ISR totals. Four slowest app blocks; IRQ-only/platform peaks may differ.')

class ProfileLink(Link):
 def profile(self,command=6,enabled=0):
  self.seq=self.seq%127+1;seq=self.seq;wire=request(command,seq,enabled)
  for attempt in range(3):
   self.out.send(self.mido.Message('sysex',data=wire[1:-1]));end=time.monotonic()+2
   while time.monotonic()<end:
    for msg in self.inp.iter_pending():
     if msg.type=='sysex' and bytes(msg.data[:7])==PREFIX+bytes([command|64,seq]):
      result=decode(b'\xf0'+bytes(msg.data)+b'\xf7',command,seq)
      if command==7 and result['enabled']!=bool(enabled):raise RuntimeError('Profiler enable/freeze ACK mismatch')
      return result
    time.sleep(.002)
  raise RuntimeError('No R39 profile reply; verify manual R39 flash and MIDI ports')
