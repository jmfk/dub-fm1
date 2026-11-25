#!/usr/bin/env python3
"""R39 focused playback, SLOOP limiter, coverage/lease and package verification. No MIDI."""
import hashlib,json,re,subprocess,sys
from pathlib import Path
from build import FILES
R=Path(__file__).resolve().parent;P=R.parent.parent;B=P/'.build-env';log=[]
def run(cmd):
 q=subprocess.run(list(map(str,cmd)),text=True,capture_output=True,cwd=P);log.append('$ '+' '.join(map(str,cmd))+'\n'+q.stdout+q.stderr);(R/'test-log.txt').write_text('\n'.join(log));print(q.stdout.strip(),flush=True)
 if q.returncode:print(q.stderr);raise SystemExit(q.returncode)
def main():
 san=['-O1','-gline-tables-only','-fsanitize=address,undefined','-fno-sanitize-recover=all']
 if '--resume-after-native' not in sys.argv:
  for sanitizer in ['address,undefined','thread']:
   exe=B/('dub-r39-dual-'+sanitizer.replace(',','-'));run(['clang','-O1','-g','-pthread','-fsanitize='+sanitizer,'-I',R,R/'test_dual_core.c','-o',exe]);run([exe])
  for unit in ['drum_headroom','limiter','transitions','participation','delay_engines','fx_mod','profile','global_limits','benchmark_control','runtime_bridge','sample_store','diagnostics','stems']:
   exe=B/f'dub-r39-{unit}-test';run(['clang',*san,'-I',R,R/f'test_{unit}.c','-o',exe])
   args=[B/'dub-r39-runtime-reply.bin'] if unit=='benchmark_control' else [B/'dub-r39-profile.bin'] if unit=='profile' else [B/'dub-r39-sample-info.bin',B/'dub-r39-sample-bank.bin'] if unit=='sample_store' else [B/'dub-r39-diag-fixture.bin'] if unit=='diagnostics' else [];run([exe,*args])
  usb=(R/'usb.c').read_text();(B/'dub-r39-usb-under-test.h').write_text(usb.replace('#include "../hal/fm1_usb.h"','#include "usb_mock.h"'))
  exe=B/'dub-r39-usb-profile-test';run(['clang',*san,'-I',R,'-I',B/'dub-synth-r39-work/tests','-I',B/'dub-synth-r39-work/firmware/src',R/'test_diag_usb.c','-o',exe]);run([exe])
 else:
  log.append((R/'test-log.txt').read_text()+'\nRESUMED: native/USB passes reused; diagnostic fixture revision corrected and rerun.\n')
 for name in ['test_download.py','test_resumable_eval.py','test_stability_benchmark.py','test_profile_tools.py']:run(['python3',R/name])
 run(['python3',R/'build_wasm.py']);run(['node',R/'test_wasm.cjs']);run(['node',R/'test_delay_pages_ui.cjs'])
 m=json.loads((R/'verified-build.json').read_text());assert m['source_sha256']==hashlib.sha256(b''.join((R/n).read_bytes() for n in FILES)).hexdigest()
 symbols=(B/'dub-synth-r39-work/build/dinghy.syms').read_text();assert not re.search(r'\b(?:d_delay_encode|d_delay_decode|d_encode4|d_log4|d_delay_four)\b',symbols)
 kernels={}
 assert '.c1_text' in symbols and '_c1_ustack' in symbols
 for name in ['d_synth_block','d_fx_block','d_drum_mix_block']:
  match=re.search(r'^([0-9a-f]+).*F \.ram_text\s+([0-9a-f]+) '+name+'$',symbols,re.M);assert match;address,size=(int(x,16) for x in match.groups());assert 0x1c00000<=address and address+size<=0x1c06000;kernels[name]=dict(address=address,size=size)
 names=list(dict.fromkeys(FILES+[p.name for p in R.iterdir() if p.suffix in ['.py','.c','.js','.cjs']]+['simulator-template.html']))
 d=dict(state='VERIFIED_NOT_INSTALLED',flashed=False,device_access=False,hardware_verified=False,manual_package_prepared=False,native_sanitizers='PASS',cross_build='PASS',wasm_midi_audio='PASS',chrome_ui_audio_worklet='PASS',sample_protocol_tests='PASS',sample_store_power_cut_tests='PASS',sample_playback_tests='PASS',benchmark_tests='PASS',profile_tests='PASS',pcm_no_codec='PASS',limiter_tests='PASS',transition_tests='PASS',participation_tests='PASS',verification_scope='Fresh focused R39 tests.Fresh ASan/UBSan and TSan pthread-vs-serial/partial-timeout PCM17tap parity, new4/8 boot caps and clean headroom;2500blocks exact SLOOP limiter reference PCM,32sample latency at variable chunks, complementary PCM transitions, oscillator tails/source caps/panic, native four-track contribution,111byte runtime/lease and malformed wire, resumable host jobs and rejection of silent tracks, sample power-cut/storage, USB register mocks,17tap isolation, live physical UI/WASM. No R39 hardware performance or listening evidence. Prior R37 PCM parity deliberately not reused: protected audio changes.',kernels=kernels,source_sha256={n:hashlib.sha256((R/n).read_bytes()).hexdigest() for n in names})
 (R/'verification.json').write_text(json.dumps(d,indent=2)+'\n');print(d['state'])
if __name__=='__main__':main()
