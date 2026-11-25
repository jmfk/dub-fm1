#!/usr/bin/env python3
"""R39 runtime MIDI control. No updater or persistent sample/music writes."""
import argparse,time
PREFIX=bytes([0x7d,0x44,0x55,0x42,1])
FIELDS={'osc_per_voice':0,'matrix':1,'delays':2,'reverb':3,'samples':4,'osc':5,'mod_index':6,'drums':7,'panic':8,'session':9,'clock_mode':10,'clock_output':11,**{f'delay_{b}_{f}':12+i*4+j for i,b in enumerate(['a','b']) for j,f in enumerate(['engine','rate','bits','degrade'])}}
def request(command,seq,field=0,value=0):
 if command not in (4,5) or not 0<=seq<128:raise ValueError('Invalid runtime command/id')
 if command==5 and (field not in range(20) or value not in range([5,2,2,2,13,34,4,2,2,2,3,2,2,6,4,128,2,6,4,128][field]) or field==8 and value!=1):raise ValueError('Invalid runtime value')
 return b'\xf0'+PREFIX+bytes([command,seq])+ (bytes([field,value]) if command==5 else b'')+b'\xf7'
def decode(wire,command,seq):
 w=bytes(wire)
 if len(w)!=137 or w[:6]!=b'\xf0'+PREFIX or w[6:9]!=bytes([command|64,seq,126]) or w[-1]!=247 or any(b>127 for b in w[1:-1]):raise ValueError('Invalid R39 runtime response')
 check=0
 for b in w[1:-2]:check^=b
 if check!=w[-2]:raise ValueError('Runtime checksum mismatch')
 p=w[9:-2]
 def u32(at):
  if p[at+4]>15:raise ValueError('Invalid uint32')
  return sum(p[at+i]<<(7*i) for i in range(5))
 if p[0]:raise RuntimeError('Device busy; stop transport/preview/storage before configuration' if p[0]==1 else 'Device rejected configuration')
 if p[1]>4 or any(v>1 for v in p[2:5]) or p[5]>12 or p[6]>33 or p[7]>3 or any(v>1 for v in p[8:11]) or p[11]>15 or any(not 1<=v<=16 for v in p[12:16]) or p[16]>32 or p[17]>11 or p[18]>1 or p[19]>7 or p[47]>2 or p[48]>1 or not 30<=p[45]+128*p[46]<=240 or any(p[49+b*4+f]>(5 if f==1 else 3 if f==2 else 127 if f==3 else 1) for b in range(2) for f in range(4)):raise ValueError('Invalid runtime field range')
 if u32(95)>32768 or any(x>8 for x in p[57:61]) or any(x>2 for x in p[86:90]):raise ValueError('Invalid participation fields')
 return dict(core_state=p[100],core_jobs=u32(101),core_wait_ticks=u32(106),core_worker_ticks=u32(111),core_worker_peak_ticks=u32(116),core_timeouts=u32(121),prelimit_clip_frames=u32(90),limiter_gain_q15=u32(95),track_osc=list(p[57:61]),track_sound_frames=[u32(61+i*5) for i in range(4)],common_sound_frames=u32(81),voice_modes=list(p[86:90]),osc_per_voice=p[1],matrix=p[2],delays=p[3],reverb=p[4],samples=p[5],osc=p[6],mod_index=p[7],drums=p[8],session=p[9],playing=p[10],pattern=p[11],channels=list(p[12:16]),active_osc=p[16],active_samples=p[17],usb_mode=p[18],usb_groups=p[19],sound_hash=u32(20),sample_generation=u32(25),sample_crc=u32(30),revision=u32(35),audio_frame=u32(40),tempo=p[45]+128*p[46],clock_mode=p[47],clock_output=p[48],**{f'delay_{b}_{f}':p[49+i*4+j] for i,b in enumerate(['a','b']) for j,f in enumerate(['engine','rate','bits','degrade'])})
class Link:
 def __init__(self,inp,out):
  import mido
  self.mido=mido;self.inp=mido.open_input(inp)
  try:self.out=mido.open_output(out)
  except BaseException:self.inp.close();raise
  self.seq=0
 def close(self):self.inp.close();self.out.close()
 def call(self,command,field=0,value=0):
  self.seq=(self.seq%127)+1;seq=self.seq;wire=request(command,seq,field,value)
  for attempt in range(3):
   self.out.send(self.mido.Message('sysex',data=wire[1:-1]));end=time.monotonic()+2
   while time.monotonic()<end:
    for msg in self.inp.iter_pending():
     if msg.type=='sysex' and bytes(msg.data[:7])==PREFIX+bytes([command|64,seq]):return decode(b'\xf0'+bytes(msg.data)+b'\xf7',command,seq)
    time.sleep(.002)
  raise RuntimeError('No R39 runtime reply: verify owner manual R39 flash and exact MIDI ports')
 def get(self):return self.call(4)
 def set(self,key,value):
  state=self.call(5,FIELDS[key],int(value))
  if key!='panic' and state[key]!=int(value):raise RuntimeError('Runtime readback did not match '+key)
  return state
 def note(self,ch,n,on,velocity=90):self.out.send(self.mido.Message('note_on' if on else 'note_off',channel=ch,note=n,velocity=velocity if on else 0))
 def perf(self,reset=False):
  import download_performance as p
  return p.capture(self.mido,self.inp,self.out,1 if reset else 0)
def value(text,maximum):return 0 if text.upper()=='OFF' else maximum+1 if text.upper()=='INF' else int(text)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--input',required=True);p.add_argument('--output',required=True);p.add_argument('--samples');p.add_argument('--osc');p.add_argument('--mod',type=int,choices=[32,16,8,4]);p.add_argument('--panic',action='store_true');a=p.parse_args()
 link=Link(a.input,a.output)
 try:
  s=link.get()
  for key,v in [('samples',value(a.samples,11) if a.samples else None),('osc',value(a.osc,32) if a.osc else None),('mod_index',[32,16,8,4].index(a.mod) if a.mod else None),('panic',1 if a.panic else None)]:
   if v is not None:s=link.set(key,v)
  import json;print(json.dumps(s,indent=2))
 finally:link.close()
if __name__=='__main__':main()
