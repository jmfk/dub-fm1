#!/usr/bin/env python3
"""Offline-only build; never opens MIDI or runs an installer."""
import hashlib,json,re,shutil,subprocess,sys,zipfile
from pathlib import Path
R=Path(__file__).resolve().parent;P=R.parent.parent;S=P/'.build-env/dub-synth-r39-work'
FILES=['hal/fm1_irq.h','hal/fm1_cpu1.h','hal/fm1_cpu1.S','hal/fm1_sys.h','hal/fm1_guard.h','hal/dub_cpu1.h','drum_parallel.h','limiter.h','drum_block.h','profile.h','profile_bridge.h','block_render.h','runtime_bridge.h','benchmark_control.h','sample_config.h','sample_format.h','sample_bank.h','sample_transfer.h','store.h','app.c','engine.h','effects.h','panel.h','ui_style.h','tables.h','params.h','presets.h','name_words.h','clock.h','seq_state.h','sequencer.h','audio_capture.h','drum_mix.h','drums.h','drum_samples.h','seq_controls.h','seq_render.h','diagnostics.h','runtime_control.h','platform.c','usb.c']
def stage():
 assert not (P/'manual-flash/dub-synth-r39').exists(),'Handed-off R39 is immutable; use R39'
 S.mkdir(exist_ok=True)
 # Frozen, tested platform. Never mutate its source checkout or previous package.
 with zipfile.ZipFile(P/'analog-test/releases/analog-test-r5/staged-source.zip') as z:
  for n in z.namelist():
   if n.startswith(('firmware/','tools/','tests/')) or n in ['build.sh','LICENSE','LICENSING.md']:
    d=S/n;d.parent.mkdir(parents=True,exist_ok=True);d.write_bytes(z.read(n))
 assert 'APP_SYSEX(p,n)' in (S/'firmware/src/ota.c').read_text(),'Missing main-loop application SysEx dispatch'
 # Dinghy's MIDI+capture platform, without synth-lab host playback, BLE or diagnostics.
 for n in ['uac_fir.h','store.h']:
  shutil.copy2(P/'dinghy/firmware/src'/n,S/'firmware/src'/n)
 for n in FILES:
  dst=S/'firmware'/n if n.startswith('hal/') else S/'firmware/src'/n
  shutil.copy2(R/n,dst)
 shutil.copy2(R/'app.c',S/'firmware/src/dinghy.c');shutil.copy2(R/'host-demo.txt',S/'tests/dub-demo.txt')
 q=S/'firmware/src/gfx.c';t=q.read_text();t=t.replace('static uint16_t cv_px[CV_MAX] __attribute__((section(".pool")));', '#if defined(DINGHY_HOST) && defined(__APPLE__)\nstatic uint16_t cv_px[CV_MAX] __attribute__((section("__DATA,__cvpool")));\n#else\nstatic uint16_t cv_px[CV_MAX] __attribute__((section(".pool")));\n#endif');q.write_text(t)
 q=S/'firmware/app.ld';t=q.read_text().replace('RAM    (rwx)', 'C1MEM  (rw)  : ORIGIN = 0x01C06000, LENGTH = 0x1F00\n    RAM    (rwx)')
 t=t.replace('.ram_text : { _rt_start = .; KEEP(*(.ram_text*)) . = ALIGN(4); _rt_end = .; }', '.ram_text : { _rt_start = .; KEEP(*(.ram_text*)) . = ALIGN(4); _rt_kernel_end = .; }')
 t=t.replace('    _rt_load = LOADADDR(.ram_text);','    _rt_load = LOADADDR(.ram_text);\n    .c1_text : { KEEP(*(.c1_text*)) . = ALIGN(4); _rt_end = .; } > RAMTEXT AT > XIP\n    _c1_load = LOADADDR(.c1_text);\n    .c1_bss (NOLOAD) : { _c1_bss_start = .; *(.bss.c1stack) *(.c1_bss*) . = ALIGN(64); _c1_bss_end = .; } > C1MEM')
 t=t.replace('*(.noinit*)','*(.noinit) *(.noinit.c1bar)')
 q.write_text(t)
 q=S/'tools/build.py';t=re.sub(r'SCREEN_NAME = "[^"]*"','SCREEN_NAME = "DUB R39"',q.read_text(),count=1);t=t.replace('(\"cc\", \"-c\", FW / \"hal\" / \"fm1_isr.S\", \"-o\", OUT / \"fm1_isr.o\"),', '(\"cc\", \"-c\", FW / \"hal\" / \"fm1_isr.S\", \"-o\", OUT / \"fm1_isr.o\"), (\"cc\", \"-c\", FW / \"hal\" / \"fm1_cpu1.S\", \"-o\", OUT / \"fm1_cpu1.o\"),')
 t=t.replace('OUT / \"fm1_isr.o\", OUT / \"dinghy.o\"', 'OUT / \"fm1_isr.o\", OUT / \"fm1_cpu1.o\", OUT / \"dinghy.o\"')
 t=t.replace('(\"text.bin\", \"data.bin\", \"ramtext.bin\")','(\"text.bin\", \"data.bin\", \"ramtext.bin\", \"c1text.bin\")')
 t=t.replace('(\"common/bin/objdump\", \"-t\", elf),','(\"common/bin/objcopy\", \"-O\", \"binary\", \"-j\", \".c1_text\", elf, OUT / \"c1text.bin\"), (\"common/bin/objdump\", \"-t\", elf),')
 t=t.replace('(\"ramtext.bin\", \"_rt_load\"), (\"data.bin\", \"_data_load\")','(\"ramtext.bin\", \"_rt_load\"), (\"c1text.bin\", \"_c1_load\"), (\"data.bin\", \"_data_load\")')
 q.write_text(t)
 (S/'dub_build.py').write_text('''import sys,json,re
from pathlib import Path
sys.path.insert(0,'tools')
import build as b
b.PRODUCT='FM-1_990';b.VERSION='dub-r39'
b.OUT.mkdir(exist_ok=True)
ota=b.build_loader();img,syms,dis,rt=b.build_app()
errors,notes=b.check(img,syms,dis,rt);errors+=b.mmio_check()
for n in notes: print('ok',n)
assert not errors,errors
def sv(name):return int(re.search(r'^([0-9a-f]+).*\\s'+name+r'$',syms,re.M).group(1),16)
assert sv('_c1_bss_start')>=0x1c06000 and sv('_c1_bss_end')<=0x1c07f00
assert sv('_c1_load')-sv('_rt_load')==sv('_rt_kernel_end')-sv('_rt_start')
assert sv('_rt_end')<=0x1c06000
print('ok core1 stacks/scratch',sv('_c1_bss_end')-sv('_c1_bss_start'),'B; copy/PC/region bounds checked')
(b.OUT/'DUB-r39.bin').write_bytes(img)
(b.OUT/'DUB-r39.fwsc').write_bytes(b.fm1pkg_make.ufw(b.fm1pkg_make.flash_image(img,b.fm1pkg_make.KEY),ota,b.PRODUCT))
(b.OUT/'dinghy.syms').write_text(syms)
''')
 return hashlib.sha256(b''.join((R/f).read_bytes() for f in FILES)).hexdigest()
def main():
 try:
  probe=subprocess.run(['docker','--context','orbstack','version','--format','{{.Server.Version}}'],capture_output=True,text=True,timeout=5)
  if probe.returncode or not probe.stdout.strip():raise RuntimeError('Docker API unavailable')
 except (subprocess.TimeoutExpired,RuntimeError) as error:
  (R/'build-log.txt').write_text('BLOCKED: OrbStack Docker API did not respond.\nNo firmware compiler ran, no installer/device access. Native/WASM evidence is in test-log.txt.\n')
  raise SystemExit('FM-1 cross-build blocked: OrbStack Docker API unavailable; no device access.')
 identity=stage()
 script='set -eu\npython3 dub_build.py\n'
 cmd=['docker','--context','orbstack','run','--rm','--platform','linux/amd64','--network','none','--mount',f'type=bind,source={S},target=/work','--mount',f'type=bind,source={P/".build-env/jieli"},target=/opt/jieli,readonly','--mount',f'type=bind,source={P/".build-env/ac79-sdk"},target=/opt/ac79-sdk,readonly','-e','JIELI_TOOLCHAIN=/opt/jieli/toolchain','-e','AC79_SDK=/opt/ac79-sdk','-w','/work','m-vave-dinghy-build:local','sh','-c',script]
 with (R/'build-log.txt').open('w') as f:
  try: result=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,timeout=180)
  except subprocess.TimeoutExpired:raise SystemExit('Docker build timed out; no device access. Check OrbStack.')
 log=(R/'build-log.txt').read_text();print(log[-4500:]);result.check_returncode()
 m=re.search(r'image (\d+) B of (\d+); RAM .data\+\.bss (\d+) B of (\d+); pool (\d+) B of (\d+)',log);assert m
 data=dict(zip(['image_bytes','image_capacity','ram_bytes','ram_capacity','pool_bytes','pool_capacity'],map(int,m.groups())))
 assert identity==hashlib.sha256(b''.join((R/f).read_bytes() for f in FILES)).hexdigest(),'Source changed during build'
 data.update(sample_protocol='SMP1',custom_sample_flash_base=0xB7000,custom_sample_flash_end=0xDF000,custom_sample_bank_bytes=81920,custom_sample_pcm_capacity=81376,custom_sample_format='mono22050Hz signed16 PCM; one-shot/held loop',ram_text_bytes=len((S/"build/ramtext.bin").read_bytes()),ram_text_capacity=24576,cpu1_code_bytes=len((S/'build/c1text.bin').read_bytes()),cpu1_default='ON_AFTER_SELFTEST',default_osc_limit=8,default_sample_limit=4,firmware_sha256=hashlib.sha256((S/'build/DUB-r39.fwsc').read_bytes()).hexdigest(),source_sha256=identity,device_access=False,hardware_verified=False,sample_rate=44100,fx_rate=22050,sequencer_tracks=4,sequencer_patterns=16,max_steps_per_track=128,drum_parts=11,drum_pcm_bytes=90182,sequence_bank_bytes=8756,parts=4,oscillators_per_part=8,filters=4,usb_midi=True,trs_midi=True,usb_capture=True,usb_multitrack_channels=17,usb_multitrack_rate=22050,usb_multitrack_bits=16,usb_groups=["MAIN","TRACKS","DRUMS"],usb_default="STEREO",usb_playback=False,ble=False,midi_performance_report=True,midi_stage_profiler=True,profiler_default='OFF',profiler_stages=6,profiler_slowest_blocks=4,profiler_fields=116,cpu_scopes=['app_render_wall_time','audio_isr_wall_time'],patches='Eight DUP2 payload slots with DUP1 migration; explicit save only; hardware persistence unverified',cpu_scope='Separate app-render and audio-ISR wall-time; not total whole-chip CPU',package_identity='FM-1_990 local experimental, not globally unique')
 (R/'verified-build.json').write_text(json.dumps(data,indent=2)+'\n')
if __name__=='__main__':main()
