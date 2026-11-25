#!/usr/bin/env python3
"""Generate fixed-point DSP tables offline, never access devices."""
import math,json
from pathlib import Path
R=Path(__file__).resolve().parent
P=[('MODE',0,3),('CUT',72,127),('RES',32,120),('ENV AMT',88,127),('LFO RATE',32,8191),('LFO AMT',12,127),('LFO SHAPE',0,2),('KEYTRACK',0,127),('LEVEL',90,127),('PAN',64,127),('DLY A',34,127),('DLY B',0,127),('REVERB',30,127),('DRIVE',8,127),('MUTE',0,1),('CHANNEL',1,16)]
P += [('LFO TARGET',0,50),('LFO2 RATE',48,8191),('LFO2 AMT',0,127),('LFO2 SHAPE',1,2),('LFO2 TARGET',0,50),('LFO3 RATE',64,8191),('LFO3 AMT',0,127),('LFO3 SHAPE',2,2),('LFO3 TARGET',0,50)]
# ENV AMT is bipolar around 64.
G=[('TEMPO',112,240),('MASTER',96,127),('DLY A RET',80,127),('DLY B RET',70,127),('REV RET',80,127)]
D=[('MODE',1,1),('TIME MS',402,32000),('SYNC',0,16),('FEEDBACK',40,94),('TONE',70,127),('DRIVE',12,127),('MOD',14,127),('ROUTE',2,2),('WIDTH',100,127),('TO REV',32,127)]
V=[('DECAY',88,100),('DAMPING',85,127),('DIFFUSION',90,127),('MOD',22,127),('LOW CUT',32,127),('PREDELAY',18,80)]
def array(name,vals,ctype):return f'static const {ctype} {name}[{len(vals)}]={{'+','.join(map(str,vals))+'};\n'
def main():
 t='/* SPDX-License-Identifier: GPL-3.0-only; generated offline. */\n#pragma once\n#include <stdint.h>\n'
 t+=array('d_inc',[round(440*2**((n-69)/12)*2**32/44100) for n in range(128)],'uint32_t')
 t+=array('d_cents',[round(2**((i-50)/1200)*65536) for i in range(101)],'uint32_t')
 # Nominal scalar feedback RT60, mean effective tank-line length; not a measured acoustic tail.
 t+=array('d_reverb_time_ms',[100 if i==0 else round(-3*math.log(10)*((sum([1493,1601,1747,1867,1999,2137,2281,2411])/8-2)/22050)/math.log(i/100)*1000) for i in range(100)],'uint16_t')
 t+=array('d_times',[max(1,round(1+3999*(i/127)**2.6)) for i in range(128)],'uint16_t')
 t+=array('d_lfo_inc',[round(.03*2**(i/16)*2**32/44100) for i in range(128)],'uint32_t')
 t+=array('d_lfo_sync_base',[round((2**32)*d*256/(44100*60*n)) for n,d in [(8,1),(4,1),(2,1),(3,2),(4,3),(1,1),(3,4),(2,3),(1,2),(3,8),(1,3),(1,4),(1,6),(1,8),(1,12),(1,16)]],'uint32_t')
 t+=array('d_lfo_millihz',[round(1000*.03*2**(i/16)) for i in range(128)],'uint16_t')
 t+=array('d_hz',[round(35*(14000/35)**(i/127)) for i in range(128)],'uint16_t')
 t+='static const int16_t d_waves[4][8][257]={\n'
 for wave in range(4):
  t+='{'
  for harmonics in [128,64,32,16,8,4,2,1]:
   vals=[]
   for i in range(256):
    p=2*math.pi*i/256
    if wave==0:x=sum(math.sin(k*p)/k for k in range(1,harmonics+1))*2/math.pi
    elif wave==1:x=sum(math.sin(k*p)/k for k in range(1,harmonics+1,2))*4/math.pi
    elif wave==2:x=sum((-1)**((k-1)//2)*math.sin(k*p)/(k*k) for k in range(1,harmonics+1,2))*8/math.pi**2
    else:x=math.sin(p)
    vals.append(round(max(-1,min(1,x*.78))*32767))
   t+='{'+','.join(map(str,vals+[vals[0]]))+'},\n'
  t+='},\n'
 t+='};\nstatic const int32_t d_fc[128][16][4]={\n'
 for i in range(128):
  g=math.tan(math.pi*(35*(14000/35)**(i/127))/44100);t+='{' 
  for j in range(16):
   k=2-1.88*j/15;a1=1/(1+g*(g+k));a2=g*a1;a3=g*a2
   t+='{'+','.join(str(round(v*2**24)) for v in [a1,a2,a3,k])+'},'
  t+='},\n'
 t+='};\n'
 # Windowed-sinc halfband: DC=1, odd taps (except centre) exactly zero.
 taps=[]
 for i in range(15):
  x=i-7;s=.5 if x==0 else math.sin(math.pi*.5*x)/(math.pi*x)
  taps.append(s*(.54-.46*math.cos(2*math.pi*i/14)))
 norm=sum(taps);iv=[round(v/norm*32768) for v in taps];iv[7]+=32768-sum(iv)
 t+=array('d_fir',iv,'int16_t')
 t+=array('d_pitch_lfo_ratio',[round(65536*2**(c/1200)) for c in range(-600,601)],'uint32_t')
 (R/'tables.h').write_text(t)
 h='/* SPDX-License-Identifier: GPL-3.0-only; generated parameter contract. */\n#pragma once\n'
 for prefix,ls in [('P',P),('G',G),('D',D),('V',V)]:
  names=[x[0].replace(' ','_') for x in ls]
  h+='enum {'+','.join(prefix+'_'+n for n in names)+','+prefix+'_COUNT};\n'
  h+=array(prefix.lower()+'_default',[x[1] for x in ls],'int16_t')
  h+=array(prefix.lower()+'_max',[x[2] for x in ls],'int16_t')
  h+='static const char *const '+prefix.lower()+'_name[]={'+','.join(json.dumps(x[0]) for x in ls)+'};\n'
 (R/'params.h').write_text(h)
 (R/'parameters.json').write_text(json.dumps(dict(params=P,globals=G,delay=D,reverb=V),indent=2)+'\n')
if __name__=='__main__':main()
