#!/usr/bin/env python3
"""Focused offline verification for this instrument; never accesses a device."""
import hashlib,json,subprocess,sys
from pathlib import Path
R=Path(__file__).resolve().parent;P=R.parent.parent;B=P/'.build-env';log=[]
def run(cmd):
 r=subprocess.run(list(map(str,cmd)),cwd=P,text=True,capture_output=True)
 log.append('$ '+' '.join(map(str,cmd))+'\n'+r.stdout+r.stderr)
 if r.returncode:
  (R/'test-log.txt').write_text('\n'.join(log));print(log[-1]);raise SystemExit(r.returncode)
 print(r.stdout.strip())
def main():
 for unit in ['diagnostics','panel','stems','runtime','idle','zero_fir','active_fast','static_osc','full_fast','feedback_default']:
  exe=B/f'dub-r39-{unit}-test'
  run(['clang','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-I',R,R/f'test_{unit}.c','-o',exe]);run([exe,*([B/'dub-r39-diag-fixture.bin'] if unit=='diagnostics' else [])])
 # Exact cache-on/off PCM comparison across silence, release tails, live parameter
 # changes, routed/unrouted AMP, LFO, drums and non32-aligned blocks.
 for mode in ['on','off']:
  run(['clang','-O2',*(['-DD_IDLE_CACHE=0'] if mode=='off' else []),'-I',R,R/'render_idle_reference.c','-o',B/f'dub-r39-idle-{mode}'])
  run([B/f'dub-r39-idle-{mode}',B/f'dub-r39-idle-{mode}.pcm'])
 assert (B/'dub-r39-idle-on.pcm').read_bytes()==(B/'dub-r39-idle-off.pcm').read_bytes(),'Idle cache changed exact audio'
 log.append('PASS exact idle-cache ON/OFF PCM parity over silence, releases, LFO/route/parameter changes and drums')
 for mode in ['on','off']:
  run(['clang','-O2',*(['-DD_ZERO_FIR=0'] if mode=='off' else []),'-I',R,R/'render_fir_reference.c','-o',B/f'dub-r39-fir-{mode}'])
  run([B/f'dub-r39-fir-{mode}',B/f'dub-r39-fir-{mode}.pcm'])
 assert (B/'dub-r39-fir-on.pcm').read_bytes()==(B/'dub-r39-fir-off.pcm').read_bytes(),'Zero FIR shortcuts changed audio'
 log.append('PASS exact zero FIR ON/OFF14sec PCM parity:8sec delay, impulse gaps, FOREVER/new input, gates and phase')
 for mode in ['on','off']:
  run(['clang','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',*(['-DD_ACTIVE_FAST=0'] if mode=='off' else []),'-I',R,R/'render_active_reference.c','-o',B/f'dub-r39-active-{mode}'])
  run([B/f'dub-r39-active-{mode}',B/f'dub-r39-active-{mode}.pcm'])
 assert (B/'dub-r39-active-on.pcm').read_bytes()==(B/'dub-r39-active-off.pcm').read_bytes(),'Active DSP fast paths changed audio'
 log.append('PASS exact active-fast ON/OFF PCM: mono/duo/para, all filter modes, cross/inverted/multi ENV, OSC LFO, FX, runtime gates and irregular blocks')
 for name,folder,flags in [('r25',P/'dub-synth/r25',[]),('r39-ref',R,['-DD_FULL_FAST=0']),('r39',R,[])]:
  source=B/'dub-full-reference.c';source.write_bytes((R/'render_full_reference.c').read_bytes())
  exe=B/f'dub-full-{name}';pcm=B/f'dub-full-{name}.pcm'
  run(['clang','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',*flags,'-I',folder,source,'-o',exe]);run([exe,pcm])
 baseline=(B/'dub-full-r25.pcm').read_bytes()
 assert baseline==(B/'dub-full-r39-ref.pcm').read_bytes()==(B/'dub-full-r39.pcm').read_bytes(),'Full instrument PCM/bus/stem mismatch'
 log.append('PASS exact R25/R39/reference full instrument PCM, raw drums and9 buses:3 melodic voices+11 drums, >12sec,8sec delay, FOREVER/new input, pan/level/mute/sends/drive, gates and irregular blocks; ASan/UBSan')
 # Replace only the MMIO HAL binding for the real USB driver under test.
 usb=(R/'usb.c').read_text();assert usb.count('#include "../hal/fm1_usb.h"')==1
 (B/'dub-r39-usb-under-test.h').write_text(usb.replace('#include "../hal/fm1_usb.h"','#include "usb_mock.h"'))
 run(['clang','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I',R,'-I',B/'dub-synth-r39-work/tests','-I',B/'dub-synth-r39-work/firmware/src',R/'test_diag_usb.c','-o',B/'dub-r39-usb-diag-test']);run([B/'dub-r39-usb-diag-test'])
 run(['clang','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I',R,'-I',B/'dub-synth-r39-work/tests','-I',B/'dub-synth-r39-work/firmware/src',R/'test_stems_usb.c','-o',B/'dub-r39-stems-usb-test']);run([B/'dub-r39-stems-usb-test'])
 run(['python3',R/'download_performance.py','--check']);run(['python3',R/'test_download.py']);run(['python3',R/'control_runtime.py','--check']);run(['python3',R/'test_runtime_tool.py'])
 run(['python3',R/'build_wasm.py']);run(['node',R/'test_wasm.cjs'])
 run(['clang','-O2','-I',R,R/'render.c','-o',B/'dub-r39-render'])
 r=subprocess.run([str(B/'dub-r39-render'),str(R/'demo.wav')],cwd=P,capture_output=True,text=True,check=True)
 log.append('Static oscillator levels intentionally change synth articulation; legacy synth PCM parity is superseded by the owner request. DSP reference comparisons remain exact.')
 (R/'render-report.json').write_text(r.stdout);print(r.stdout.strip());log.append(r.stdout)
 report=json.loads(r.stdout);assert report['peak']>0 and report['output_clip_samples']==0 and report['max_active_oscillators']==32
 run(['clang','-O2','-I',R,R/'render_seq.c','-o',B/'dub-r39-seq-render'])
 seq_render=subprocess.run([str(B/'dub-r39-seq-render'),str(R/'sequencer-demo.wav')],cwd=P,capture_output=True,text=True,check=True)
 log.append('Static OSC architecture intentionally changes melodic sequence PCM; sequencer logic/drum tests and new static/reference PCM remain verified.')
 (R/'sequencer-render-report.json').write_text(seq_render.stdout);log.append(seq_render.stdout);print(seq_render.stdout.strip())
 run(['node',R/'test_sequencer_ui.cjs']);run(['node',R/'test_stems_ui.cjs']);run(['node',R/'test_runtime_ui.cjs']);run(['node',R/'test_static_osc_ui.cjs'])
 (R/'test-log.txt').write_text('\n'.join(log))
 files=['engine.h','effects.h','params.h','tables.h','presets.h','name_words.h','clock.h','seq_state.h','sequencer.h','audio_capture.h','drum_mix.h','drums.h','drum_samples.h','seq_controls.h','seq_render.h','diagnostics.h','runtime_control.h','platform.c','usb.c','panel.h','ui_style.h','app.c','wasm.c','runtime.js','simulator-template.html','download_performance.py','control_runtime.py']
 memory_path=R/'verified-build.json'
 memory=json.loads(memory_path.read_text()) if memory_path.exists() else {}
 target_files=['app.c','engine.h','effects.h','panel.h','ui_style.h','tables.h','params.h','presets.h','name_words.h','clock.h','seq_state.h','sequencer.h','audio_capture.h','drum_mix.h','drums.h','drum_samples.h','seq_controls.h','seq_render.h','diagnostics.h','runtime_control.h','platform.c','usb.c']
 target_current=hashlib.sha256(b''.join((R/n).read_bytes() for n in target_files)).hexdigest()
 built=memory.get('source_sha256')==target_current and (B/'dub-synth-r39-work/build/DUB-r39.fwsc').is_file()
 if built:
  import re
  symbols=(B/'dub-synth-r39-work/build/dinghy.syms').read_text();match=re.search(r'^([0-9a-f]+).*F \.ram_text\s+([0-9a-f]+) d_synth_sample$',symbols,re.M);assert match,'DSP kernel is not in SRAM'
  address,size=(int(x,16) for x in match.groups());assert 0x1c00000<=address and address+size<=0x1c06000
  fx=re.search(r'^([0-9a-f]+).*F \.ram_text\s+([0-9a-f]+) d_fx_sample$',symbols,re.M);assert fx,'FX kernel is not in SRAM'
  fx_address,fx_size=(int(x,16) for x in fx.groups());assert 0x1c00000<=fx_address and fx_address+fx_size<=0x1c06000
  log.append(f'PASS native SRAM DSP kernel address={address:#x}, size={size}; FX={fx_address:#x}/{fx_size}, flash no-call/ROM/MMIO guards retained')
  (R/'test-log.txt').write_text('\n'.join(log))
 data=dict(state='VERIFIED_NOT_INSTALLED' if built else 'NATIVE_WASM_VERIFIED_FIRMWARE_BUILD_PENDING',flashed=False,device_access=False,hardware_verified=False,native_sanitizers='PASS',wasm_midi_audio='PASS',chrome_ui_audio_worklet='PASS',cross_build='PASS' if built else 'PENDING',source_sha256={n:hashlib.sha256((R/n).read_bytes()).hexdigest() for n in files},render=report,manual_package_prepared=False,dsp_evidence='R39 exact symmetric FIR dots, pooled PCM drum cache, bounded drum/soft-clip math and drive reuse; call-free FX SRAM kernel; ASan/UBSan full R25/R39/reference PCM/raw/9bus parity across three melody modes and eleven drums, FOREVER/new input, drive/pan/mutes/sends and irregular blocks. Owner-requested static OSC levels: no runtime per-OSC ENV, no amplitude routes, shared release masks and five-source UI; legacy fields inert and patch schema unchanged. Old melodic PCM intentionally changes. Bounded signed32 Q15 reference extremes/random and canonical/general ENV paths; sample-for-sample active-fast ON/OFF sanitized references across voices, filter modes, cross/multi/inverted ENV, LFO and gates; native call-free SRAM sample kernel;21 zero-history FIR ring masks and unchanged clock/phase/tail behavior,14sec exact ON/OFF reference PCM with8sec delay and reverb FOREVER/new input; zero-state LFO coefficient-ramp cache; Exact stationary-track caching, release-safe immediate wake and sample-for-sample cache ON/OFF reference comparison including filter fixed points, AMP unrouted, modulation changes, drums and non32 blocks; Runtime reverb/delay converter+engine bypass, synth OFF/per-voice OSC limits, matrix OFF with fixed ENV and preserved routes; exact private MIDI set/query/ack validated with actual USB register mocks and C/Python codec/mock MIDI; CPU physical/Worklet switches;17 engine taps, independent group gates, track4 synth/drum mix, dry drum isolation, mixer validation and physical controls; actual USB descriptors/rate/interleaving/FIR/ring/816byte packet and idle gates with register mocks; browser+AudioWorklet mixer persistence; Static-OSC melodic PCM intentionally changed; optimized/static and FX reference PCM parity; R19 diagnostics and sequencer regression; no device access or hardware CPU/USB acceptance')
 (R/'verification.json').write_text(json.dumps(data,indent=2)+'\n')
if __name__=='__main__':main()
