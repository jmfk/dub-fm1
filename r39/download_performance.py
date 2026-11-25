#!/usr/bin/env python3
"""Owner-run read-only USB MIDI diagnostics. --check never imports/enumerates MIDI."""
import argparse,datetime,json,time
from pathlib import Path
PREFIX=bytes([0x7d,0x44,0x55,0x42,1])
SCOPE=['blocks','frames','ticks_low','ticks_high','budget_low','budget_high','peak_ticks','peak_budget','late_blocks']
FIELDS=['firmware_release','timer_hz','sample_rate','uptime_ms','window_generation']+['app_'+x for x in SCOPE]+['isr_'+x for x in SCOPE]+['dsp_clip_samples','command_queue_overflows','fx_safety_clamps','clock_queue_drops','usb_audio_underruns','usb_audio_overruns','usb_audio_missed_packets','usb_max_sof_gap_polls','usb_resets','usb_audio_stream_selected','usb_audio_flowing','usb_audio_host_rate']
COUNTERS=['dsp_clip_samples','command_queue_overflows','fx_safety_clamps','clock_queue_drops','usb_audio_underruns','usb_audio_overruns','usb_audio_missed_packets','usb_resets']
def request(command):
 if command not in (0,1):raise ValueError('Only snapshot0 or statistics-reset1 is supported')
 return bytes([0xf0])+PREFIX+bytes([command,0xf7])
def decode(wire,command):
 wire=bytes(wire)
 if len(wire)!=8+len(FIELDS)*5+2 or wire[:6]!=bytes([0xf0])+PREFIX or wire[-1]!=0xf7 or wire[6]!=(0x40|command) or wire[7]!=len(FIELDS):raise ValueError('Unexpected DUB performance reply identity/version/size')
 if any(x>127 for x in wire[1:-1]):raise ValueError('Invalid7-bit payload')
 check=0
 for x in wire[1:-2]:check^=x
 if check!=wire[-2]:raise ValueError('Performance checksum mismatch')
 values=[]
 for i in range(len(FIELDS)):
  word=wire[8+i*5:13+i*5]
  if word[4]>15:raise ValueError('Overflowing32-bit word')
  values.append(sum(x<<(j*7) for j,x in enumerate(word)))
 raw=dict(zip(FIELDS,values))
 if raw['firmware_release']<19 or raw['timer_hz']==0 or raw['sample_rate']!=44100:raise ValueError('Unsupported DUB performance configuration')
 scopes={}
 for scope in ['app','isr']:
  v={key:raw[scope+'_'+key] for key in SCOPE};ticks=v['ticks_low']+(v['ticks_high']<<32);budget=v['budget_low']+(v['budget_high']<<32)
  scopes[scope]=dict(blocks=v['blocks'],frames_counter=v['frames'],elapsed_ticks=ticks,budget_ticks=budget,average_load_percent=100*ticks/budget if budget else None,peak_load_percent=100*v['peak_ticks']/v['peak_budget'] if v['peak_budget'] else None,peak_ticks=v['peak_ticks'],peak_budget=v['peak_budget'],late_blocks=v['late_blocks'],budget_window_seconds=budget/raw['timer_hz'])
 return dict(schema='dub-performance/v1',firmware='DUB R'+str(raw['firmware_release']),window_generation=raw['window_generation'],uptime_ms=raw['uptime_ms'],timer_hz=raw['timer_hz'],sample_rate=raw['sample_rate'],scopes=scopes,counters={k:raw[k] for k in COUNTERS},usb=dict(stream_selected=bool(raw['usb_audio_stream_selected']),flowing=bool(raw['usb_audio_flowing']),host_rate=raw['usb_audio_host_rate'],max_sof_gap_polls_since_boot=raw['usb_max_sof_gap_polls']),scope_note='Wall-time audio budget utilization, including nested interrupt preemption; not total whole-chip CPU. App excludes platform scaling/tap; ISR includes them. Other counters are cumulative since boot unless a timed baseline is supplied.')
def capture(mido,port_in,port_out,command,timeout=4):
 port_out.send(mido.Message('sysex',data=request(command)[1:-1]));deadline=time.monotonic()+timeout
 while time.monotonic()<deadline:
  for msg in port_in.iter_pending():
   if msg.type=='sysex' and len(msg.data)>5 and bytes(msg.data[:5])==PREFIX and msg.data[5]==(0x40|command):return decode(bytes([0xf0])+bytes(msg.data)+bytes([0xf7]),command)
  time.sleep(.005)
 raise RuntimeError('No DUB performance reply. Manually install R39 or later and check the exact USB MIDI ports.')
def offline_check():
 assert len(FIELDS)==35 and len(request(0))==8 and request(1)[1]!=0x22
 values=[0]*35;values[0:5]=[19,24000000,44100,1000,1];values[5:14]=[100,12800,500000,1,1000000,1,40000,70000,2];values[14:23]=[100,12800,600000,1,1000000,1,60000,70000,3]
 data=bytearray([0xf0])+PREFIX+bytes([0x40,35])
 for value in values:
  data.extend((value>>(7*j))&127 for j in range(5))
 check=0
 for x in data[1:]:check^=x
 data.extend([check,0xf7]);r=decode(data,0);assert r['scopes']['app']['elapsed_ticks']==2**32+500000 and r['scopes']['app']['late_blocks']==2
 broken=bytearray(data);broken[10]^=1
 try:decode(broken,0)
 except ValueError:pass
 else:raise AssertionError('Malformed report accepted')
 print('PASS offline protocol/64-bit statistics/checksum; no MIDI imported, enumerated or opened.')
def main(argv=None):
 parser=argparse.ArgumentParser(description=__doc__);actions=parser.add_mutually_exclusive_group(required=True);actions.add_argument('--check',action='store_true');actions.add_argument('--list',action='store_true');actions.add_argument('--reset',action='store_true');actions.add_argument('--download',type=Path);parser.add_argument('--input',dest='input_name');parser.add_argument('--output',dest='output_name');parser.add_argument('--seconds',type=float,default=0)
 args=parser.parse_args(argv)
 if args.check:offline_check();return
 if args.seconds and (not args.download or not 1<=args.seconds<=3600):parser.error('--seconds requires --download and1..3600 seconds')
 if args.download and args.download.exists():parser.error('Output already exists; choose a new filename')
 import mido
 if args.list:
  print('INPUTS:',*mido.get_input_names(),sep='\n  ');print('OUTPUTS:',*mido.get_output_names(),sep='\n  ');return
 if not args.input_name or not args.output_name:parser.error('Specify exact --input and --output names from --list')
 if args.input_name not in mido.get_input_names() or args.output_name not in mido.get_output_names():parser.error('Specified MIDI port is unavailable; no request sent')
 with mido.open_input(args.input_name) as inp,mido.open_output(args.output_name) as out:
  if args.reset:
   result=capture(mido,inp,out,1);print('Performance counters reset; generation',result['window_generation']);return
  baseline=None
  if args.seconds:
   baseline=capture(mido,inp,out,1);print(f'Measuring for {args.seconds:g} seconds. Play the intended workload now.');time.sleep(args.seconds)
  result=capture(mido,inp,out,0)
 if baseline:
  if result['window_generation']!=baseline['window_generation'] or result['uptime_ms']<baseline['uptime_ms']:raise RuntimeError('Device restarted or statistics were reset during capture; repeat the measurement')
  result['baseline']=baseline['counters'];result['counter_deltas']={k:(result['counters'][k]-baseline['counters'][k])&0xffffffff for k in COUNTERS};result['requested_window_seconds']=args.seconds
 result['captured_at_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();result['ports']={'input':args.input_name,'output':args.output_name}
 with args.download.open('x') as f:json.dump(result,f,indent=2);f.write('\n')
 print('Saved',args.download)
 for label,scope in [('App render','app'),('Audio ISR', 'isr')]:
  s=result['scopes'][scope];average=s['average_load_percent'];peak=s['peak_load_percent'];print(label,':', 'no blocks measured' if average is None else f'average {average:.2f}%, peak {peak:.2f}%, late blocks {s["late_blocks"]}')
if __name__=='__main__':
 try:main()
 except (RuntimeError,ValueError,OSError,ImportError) as error:raise SystemExit(str(error))
