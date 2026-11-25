#!/usr/bin/env python3
"""Offline resample of the owner's eleven WAVs; no device access/dependencies."""
import wave,math,struct,json,hashlib
from pathlib import Path
R=Path(__file__).resolve().parent;source=R.parent.parent/'waveforms/drums'
names=['KICK','SNARE','STICK','TOM808','TOM606','CH808','OH808','CH78','COWBELL','SHAKER','CLAVES']
files=sorted(source.glob('*.wav'));assert len(files)==11
samples=[];offset=[0];report=[]
for file,name in zip(files,names):
 with wave.open(str(file)) as w:
  assert w.getnchannels()==1 and w.getsampwidth()==2
  rate=w.getframerate();raw=w.readframes(w.getnframes());data=struct.unpack('<'+'h'*(len(raw)//2),raw)
 count=round(len(data)*22050/rate);cut=.45*min(1,22050/rate)
 for j in range(count):
  position=j*rate/22050;at=math.floor(position);total=weight=0.
  for k in range(at-23,at+25):
   distance=position-k
   if abs(distance)>=24:continue
   sinc=2*cut if abs(distance)<1e-12 else math.sin(2*math.pi*cut*distance)/(math.pi*distance)
   window=.42+.5*math.cos(math.pi*distance/24)+.08*math.cos(2*math.pi*distance/24);v=sinc*window
   weight+=v
   if 0<=k<len(data):total+=data[k]*v
  samples.append(max(-32767,min(32767,round(total/weight))))
 offset.append(len(samples));report.append(dict(name=name,file=file.name,source_rate=rate,source_frames=len(data),output_frames=count,sha256=hashlib.sha256(file.read_bytes()).hexdigest()))
text='/* Owner-supplied drum waveforms, resampled offline to mono22050Hz16-bit. */\n#pragma once\n'
text+='static const char *const d_drum_names[11]={'+','.join(json.dumps(x) for x in names)+'};\n'
text+='static const uint8_t d_drum_keys[11]={36,38,37,45,41,42,46,44,56,70,75};\n'
text+='static const uint32_t d_drum_offsets[12]={'+','.join(map(str,offset))+'};\n'
text+='static const int16_t d_drum_pcm['+str(len(samples))+']={\n'
text+='\n'.join(','.join(map(str,samples[i:i+24]))+',' for i in range(0,len(samples),24))+'\n};\n'
(R/'drum_samples.h').write_text(text);(R/'drums.json').write_text(json.dumps(dict(rate=22050,pcm_bytes=len(samples)*2,samples=report),indent=2)+'\n');print('Embedded11 drums:',len(samples)*2,'PCM bytes')
