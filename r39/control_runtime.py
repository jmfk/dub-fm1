#!/usr/bin/env python3
"""R39 runtime CPU switches; changes RAM only, never flashes or resets counters."""
import argparse,json,time
PREFIX=bytes([0x7d,0x44,0x55,0x42,1])
OSC=['OFF','1','2','4','8']
def request(field=None,value=None):
 if field is None:return b'\xf0'+PREFIX+bytes([3,0xf7])
 if field not in range(4) or value not in range(5 if field==0 else 2):raise ValueError('Invalid runtime setting')
 return b'\xf0'+PREFIX+bytes([2,field,value,0xf7])
def decode(wire,command):
 w=bytes(wire)
 if len(w)!=14 or w[:6]!=b'\xf0'+PREFIX or w[6]!=0x40|command or w[7]!=4 or w[-1]!=0xf7:raise ValueError('Invalid runtime reply identity/size')
 check=0
 for b in w[1:12]:check^=b
 if check!=w[12] or w[8]>4 or any(x>1 for x in w[9:12]):raise ValueError('Invalid runtime reply state/checksum')
 return dict(osc_per_voice=OSC[w[8]],matrix=bool(w[9]),delays=bool(w[10]),reverb=bool(w[11]))
def capture(mido,inp,out,field=None,value=None,timeout=8):
 command=3 if field is None else 2
 out.send(mido.Message('sysex',data=request(field,value)[1:-1]));end=time.monotonic()+timeout
 while time.monotonic()<end:
  for msg in inp.iter_pending():
   if msg.type=='sysex' and bytes(msg.data[:5])==PREFIX and len(msg.data)>5 and msg.data[5]==0x40|command:return decode(b'\xf0'+bytes(msg.data)+b'\xf7',command)
  time.sleep(.005)
 raise RuntimeError('No runtime control reply. Manually install R39 or later; check exact MIDI ports.')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--check',action='store_true');p.add_argument('--list',action='store_true');p.add_argument('--query',action='store_true');p.add_argument('--input');p.add_argument('--output');p.add_argument('--osc',type=str.upper,choices=OSC)
 for name in ['matrix','delays','reverb']:p.add_argument('--'+name,choices=['on','off'])
 a=p.parse_args()
 if a.check:
  assert request()==b'\xf0'+PREFIX+b'\x03\xf7'
  assert len(request(3,0))==10
  print('PASS offline runtime request codec; no MIDI opened');return
 import mido
 if a.list:print(json.dumps({'inputs':mido.get_input_names(),'outputs':mido.get_output_names()},indent=2));return
 if not a.input or not a.output:p.error('Specify --input and --output from --list')
 if a.input not in mido.get_input_names() or a.output not in mido.get_output_names():p.error('Exact MIDI ports unavailable; no request sent')
 edits=[(i,OSC.index(v) if i==0 else int(v=='on')) for i,v in enumerate([a.osc,a.matrix,a.delays,a.reverb]) if v is not None]
 with mido.open_input(a.input) as inp,mido.open_output(a.output) as out:
  state=capture(mido,inp,out) # Verify R39 responds before applying a setting.
  for field,value in edits:state=capture(mido,inp,out,field,value)
 print(json.dumps(state,indent=2));print('Acknowledged RAM settings; no flash or performance-counter reset.')
if __name__=='__main__':
 try:main()
 except (ValueError,RuntimeError,OSError,ImportError) as e:raise SystemExit(str(e))
