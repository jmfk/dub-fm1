#!/usr/bin/env python3
"""SMP1 reference client for DUB samples. Never uses updater/firmware commands.
Hardware actions require an explicit subcommand and exact input/output port names.
WAV input must already be mono22050Hz signed16 PCM; use DUB Librarian to convert.
"""
import argparse,json,struct,time,wave,zlib
from pathlib import Path
PREFIX=b'\x7dSMP\x01';MAX=81888;PCM_MAX=81376
ERRORS=['OK','BUSY: stop transport, notes and USB recording','BAD_DATA','BAD_CRC','OFFSET','FLASH','TOKEN','UNSUPPORTED','STALE: read the bank again']
def pack(data):
 out=bytearray()
 for at in range(0,len(data),7):
  group=data[at:at+7];out.append(sum((b>>7)<<i for i,b in enumerate(group)));out.extend(b&127 for b in group)
 return bytes(out)
def unpack(data):
 out=bytearray();at=0
 while at<len(data):
  h=data[at];at+=1;group=data[at:at+7]
  if not group or h>127 or h>>len(group) or any(b>127 for b in group):raise ValueError('Invalid7bit encoding')
  out.extend(b|((h>>i&1)<<7) for i,b in enumerate(group));at+=len(group)
 return bytes(out)
def encode(cmd,seq,body):return b'\xf0'+PREFIX+bytes([cmd,seq])+pack(body+struct.pack('<I',zlib.crc32(bytes([cmd,seq])+body)))+b'\xf7'
def decode(frame,cmd,seq):
 if len(frame)<15 or frame[:6]!=b'\xf0'+PREFIX or frame[6:8]!=bytes([cmd|64,seq]) or frame[-1]!=247:raise ValueError('Invalid response')
 raw=unpack(frame[8:-1]);body=raw[:-4]
 if len(raw)<5 or zlib.crc32(bytes([cmd|64,seq])+body)!=struct.unpack('<I',raw[-4:])[0]:raise ValueError('Response CRC')
 if body[0]:raise RuntimeError(ERRORS[body[0]] if body[0]<len(ERRORS) else 'Unknown device status')
 return body[1:]
def validate_bank(data):
 if not 512<=len(data)<=MAX or data[:4]!=b'FSB1' or struct.unpack_from('<III',data,4)!=(1,11,len(data)):raise ValueError('Invalid FSB1 header')
 used=0
 for i in range(11):
  record=data[16+i*40:56+i*40];offset,n,rate,flags=struct.unpack_from('<IIII',record,24)
  if not n:
   if any(record):raise ValueError('Nonempty absent record')
   continue
  name=record[:24];end=name.find(0)
  if n<2 or rate!=22050 or flags>1 or offset!=used or not 1<=end<=23 or any(b<32 or b>126 for b in name[:end]) or any(name[end:]) or n>(len(data)-512-used)//2:raise ValueError('Invalid FSB1 record')
  used+=n*2
 if used+512!=len(data) or any(data[456:512]):raise ValueError('Invalid FSB1 size/padding')
 return data
def change_pad(data,pad,sample=None,name='SAMPLE',loop=False):
 validate_bank(data);records=[];pcm=bytearray()
 for i in range(11):
  old=data[16+i*40:56+i*40];offset,n,rate,flags=struct.unpack_from('<IIII',old,24)
  if i==pad:
   if sample is None:records.append(bytes(40));continue
   if len(sample)<4 or len(sample)%2:raise ValueError('PCM must contain >=2 signed16 samples')
   encoded=name.encode('ascii','replace')[:23];record=encoded+bytes(24-len(encoded))+struct.pack('<IIII',len(pcm),len(sample)//2,22050,int(loop));payload=sample
  elif n:record=old[:24]+struct.pack('<IIII',len(pcm),n,rate,flags);payload=data[512+offset:512+offset+n*2]
  else:records.append(bytes(40));continue
  records.append(record);pcm.extend(payload)
 if len(pcm)>PCM_MAX:raise ValueError(f'Sample bank exceeds {PCM_MAX} PCM bytes')
 catalog=b'FSB1'+struct.pack('<III',1,11,512+len(pcm))+b''.join(records);return validate_bank(catalog+bytes(512-len(catalog))+pcm)
def wave_bytes(pcm,loop=False):
 fmt=struct.pack('<HHIIHH',1,1,22050,44100,2,16);chunks=b'fmt '+struct.pack('<I',16)+fmt+b'data'+struct.pack('<I',len(pcm))+pcm
 if loop:
  smpl=struct.pack('<15I',0,0,45351,60,0,0,0,1,0,0,0,0,len(pcm)//2-1,0,0);chunks+=b'smpl'+struct.pack('<I',len(smpl))+smpl
 return b'RIFF'+struct.pack('<I',len(chunks)+4)+b'WAVE'+chunks
class Link:
 def __init__(self,inp,out):
  import mido
  self.mido=mido;self.inp=mido.open_input(inp);self.out=mido.open_output(out);self.seq=0
 def close(self):self.inp.close();self.out.close()
 def request(self,cmd,body=b'',timeout=3):
  self.seq=(self.seq+1)&127;wire=encode(cmd,self.seq,body)
  for _ in range(3):
   self.out.send(self.mido.Message('sysex',data=wire[1:-1]));deadline=time.monotonic()+timeout
   while time.monotonic()<deadline:
    for msg in self.inp.iter_pending():
     if msg.type=='sysex':
      raw=bytes(msg.data)
      if len(raw)>=7 and raw[:5]==PREFIX and raw[5:7]==bytes([cmd|64,self.seq]):return decode(b'\xf0'+raw+b'\xf7',cmd,self.seq)
    time.sleep(.003)
  raise RuntimeError('No SMP1 reply. Requires DUB R29 or later sample firmware and correct USB-MIDI ports.')
 def info(self):
  reply=self.request(1)
  if len(reply)!=36:raise ValueError('INFO size')
  values=struct.unpack('<9I',reply);keys=['version','rate','pads','blob_capacity','pcm_capacity','used_pcm','generation','blob_bytes','flags'];info=dict(zip(keys,values))
  if values[:5]!=(1,22050,11,MAX,PCM_MAX) or not 512<=info['blob_bytes']<=MAX or info['used_pcm']>PCM_MAX:raise ValueError('Unsupported INFO')
  info['free_pcm']=PCM_MAX-info['used_pcm'];return info
 def bank(self,info):
  data=bytearray()
  while len(data)<info['blob_bytes']:
   count=min(96,info['blob_bytes']-len(data));reply=self.request(3,struct.pack('<IIB',info['generation'],len(data),count))
   if len(reply)!=4+count or struct.unpack_from('<I',reply)[0]!=len(data):raise ValueError('READ offset')
   data.extend(reply[4:])
  return validate_bank(bytes(data))
 def upload(self,data,info):
  validate_bank(data);reply=self.request(4,struct.pack('<III',info['generation'],len(data),zlib.crc32(data)),timeout=15)
  if len(reply)!=8 or struct.unpack_from('<I',reply,4)[0]!=0:raise ValueError('BEGIN response')
  token=struct.unpack_from('<I',reply)[0]
  try:
   for at in range(0,len(data),96):
    group=data[at:at+96];reply=self.request(5,struct.pack('<II',token,at)+group)
    if len(reply)!=4 or struct.unpack('<I',reply)[0]!=at+len(group):raise ValueError('DATA offset')
   result=self.request(6,struct.pack('<I',token),timeout=5)
   if result!=struct.pack('<II',(info['generation']+1)&0xffffffff,len(data)):raise ValueError('COMMIT response')
  except BaseException:
   try:self.request(7,struct.pack('<I',token))
   except Exception:pass
   raise
 def pad(self,pad,info):
  meta=self.request(2,bytes([pad]))
  if len(meta)!=42 or meta[0]!=pad:raise ValueError('LIST response')
  n=struct.unpack_from('<I',meta,30)[0];data=bytearray()
  if not 2<=n<=500000:raise ValueError('Invalid sample length')
  while len(data)<n*2:
   count=min(96,n*2-len(data));reply=self.request(8,bytes([pad])+struct.pack('<IIB',info['generation'],len(data),count))
   if len(reply)!=5+count or reply[0]!=pad or struct.unpack_from('<I',reply,1)[0]!=len(data):raise ValueError('READ PAD offset')
   data.extend(reply[5:])
  return bytes(data),meta

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--input',required=True);parser.add_argument('--output',required=True);sub=parser.add_subparsers(dest='command',required=True)
 sub.add_parser('info');sub.add_parser('list')
 for name in ['export-bank','import-bank']:
  p=sub.add_parser(name);p.add_argument('file',type=Path)
 for name in ['export-pad','set-pad','reset-pad']:
  p=sub.add_parser(name);p.add_argument('pad',type=int,choices=range(1,12))
  if name!='reset-pad':p.add_argument('file',type=Path)
  if name=='set-pad':p.add_argument('--name',default='SAMPLE');p.add_argument('--loop',action='store_true')
 args=parser.parse_args();link=Link(args.input,args.output)
 try:
  info=link.info()
  if args.command=='info':print(json.dumps(info,indent=2));return
  if args.command=='list':
   for pad in range(11):
    meta=link.request(2,bytes([pad]))
    if len(meta)!=42 or meta[0]!=pad or meta[1]>1:raise ValueError('Invalid LIST response')
    print(pad+1,meta[2:26].split(b'\0')[0].decode('ascii'),struct.unpack_from('<I',meta,30)[0], 'LOOP' if struct.unpack_from('<I',meta,38)[0] else 'ONE-SHOT','CUSTOM' if meta[1] else 'FACTORY')
  elif args.command=='export-bank':
   with args.file.open('xb') as f:f.write(link.bank(info))
  elif args.command=='import-bank':
   if not 512<=args.file.stat().st_size<=MAX:raise ValueError('Invalid bank length')
   link.upload(validate_bank(args.file.read_bytes()),info)
  elif args.command=='export-pad':
   if args.file.exists():raise ValueError('Output already exists')
   data,meta=link.pad(args.pad-1,info)
   with args.file.open('xb') as f:f.write(wave_bytes(data,bool(struct.unpack_from('<I',meta,38)[0])))
  else:
   data=link.bank(info)
   if args.command=='reset-pad':data=change_pad(data,args.pad-1)
   else:
    with wave.open(str(args.file)) as w:
     if (w.getnchannels(),w.getsampwidth(),w.getframerate(),w.getcomptype())!=(1,2,22050,'NONE'):raise ValueError('Use mono22050Hz16bit PCM WAV; convert with DUB Librarian')
     if not 2<=w.getnframes()<=PCM_MAX//2:raise ValueError("Sample does not fit the PCM bank")
     pcm=w.readframes(w.getnframes())
    data=change_pad(data,args.pad-1,pcm,args.name,args.loop)
   link.upload(data,info)
 finally:link.close()
if __name__=='__main__':main()
