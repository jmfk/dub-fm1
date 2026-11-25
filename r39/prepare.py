#!/usr/bin/env python3
"""Seal only a verified FM-1 cross-build. Does not install or open MIDI."""
import hashlib,json,os,shutil,subprocess,zipfile
from pathlib import Path
from build import FILES
R=Path(__file__).resolve().parent;P=R.parent.parent;S=P/'.build-env/dub-synth-r39-work';DEST=P/'manual-flash/dub-synth-r39';TMP=P/'manual-flash/.prepare-dub-synth-r39'
def main():
 assert not DEST.exists(),'Handed-off R39 is immutable; create R39'
 memory=json.loads((R/'verified-build.json').read_text());checks=json.loads((R/'verification.json').read_text())
 assert checks['state']=='VERIFIED_NOT_INSTALLED','R39 is build-only; targeted tests are pending. No installation package prepared.'
 assert all(checks.get(k)=='PASS' for k in ['sample_protocol_tests','sample_store_power_cut_tests','sample_playback_tests']),'R39 sample protocol/storage/playback tests are pending; no installation package prepared.'
 assert checks.get('profile_tests')=='PASS','R39 stage/transport/host recovery checks missing'
 assert checks.get('benchmark_tests')=='PASS','R39 runtime/recovery checks missing'
 assert checks['native_sanitizers']=='PASS' and checks['chrome_ui_audio_worklet']=='PASS' and checks['cross_build']=='PASS'
 for n,digest in checks['source_sha256'].items():assert hashlib.sha256((R/n).read_bytes()).hexdigest()==digest,n+' changed after verification'
 for n in FILES:assert (R/n).read_bytes()==(S/'firmware'/n if n.startswith('hal/') else S/'firmware/src'/n).read_bytes(),n+' changed after cross-build'
 assert (S/'build/DUB-r39.fwsc').is_file() and memory['device_access'] is False
 assert memory['source_sha256']==hashlib.sha256(b''.join((R/n).read_bytes() for n in FILES)).hexdigest()
 assert memory['firmware_sha256']==hashlib.sha256((S/'build/DUB-r39.fwsc').read_bytes()).hexdigest()
 assert not TMP.exists(),'Incomplete preparation exists; inspect it first'
 TMP.mkdir()
 for n in ['DELAY-ENGINES.md','PROFILING.md','profile_benchmark.py','profile_midi.py','BENCHMARK.md','benchmark_midi.py','stability_benchmark.py','resumable_eval.py','validate_run_state.py','SAMPLE-PROTOCOL.md','VALIDATION-PLAN.md','sample_tool.py','README.md','control_runtime.py','download_performance.py','WORK-NOTE.md','MEMORY.md','verification.json','verified-build.json','test-log.txt','build-log.txt','simulator.html','drums.json','drum-format-verification.json']:
  shutil.copy2(R/n,TMP/n)
 shutil.copy2(S/'build/DUB-r39.fwsc',TMP/'DUB-r39.fwsc')
 for n in ['fm1_install.py','manual_install.py']:shutil.copy2(P/'manual-flash/analog-test-r5'/n,TMP/n)
 q=TMP/'manual_install.py';s=q.read_text();a=s.index("        print('ANALOG R5:");b=s.index("        print('Installationen ersätter",a)
 s=s[:a]+'''        print('DUB R39: fyra MIDI-delar, åtta oscillatorer per del, mixer och gemensamma FX.')
        print('MIDI-kanal1–4. Åtta presetslots; osparade ändringar finns bara i RAM.')
        print('Valbar stereo44.1/48 eller17kanaler22.05k/16. MAIN/TRACKS/DRUMS separat.')
        print('R39: FX > DLY A/B: tryck samma knapp igen för sida2 ENGINE/RATE/BITS/DEGRADE.')
        print('Alla delaycodecs borttagna: direkt PCM16. FILTERED1/2/4s, LOFI upp till32s.')
        print('Bitreduction är en valbar effekt; ingen komprimerad lagring.')
        print('Trummixern kör aktiv-pad-block i SRAM med återanvända parametrar.')
        print('Globala sessionsval. Omstart: PCM16/4s max. Byte tömmer den delayens svans.')
        print('R39: kärna2 mixar trummor parallellt med syntarna. Standard4trummor/8OSC.')
        print('Trummix: mer marginal före DRIVE; trumspårets DRIVE är0 vid start.')
        print('2C i footern betyder lyckat kärnstarttest; MIDI rapporterar jobb/väntan/timeouts.')
        print('CPU > MODRATE: K1 intervall. CPU > VOICES: K1 samples, K2 OSC.')
        print('Delay1 feedback är40 som standard; gamla presets behåller sina värden.')
        print('REVERB är OFF vid start. SET > SHIFT+ARP öppnar CPU-brytarna.')
        print('Trummixer ingår. Ingen USB-ljuduppspelning eller BLE.')
        print('FM-1 CPU och ljud återstår att verifiera efter din manuella installation.')
'''+s[b:];q.write_text(s)
 launcher='''#!/bin/sh
DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
PY="$DIR/../../.build-env/midi-venv/bin/python"
if [ ! -x "$PY" ]; then echo 'Pythonmiljön saknas; inget startades.'; exit 1; fi
if [ "${1:-}" = '--check' ]; then exec "$PY" "$DIR/manual_install.py" --check; fi
if [ "$#" -ne 0 ]; then echo 'Okänt argument'; exit 1; fi
"$PY" "$DIR/manual_install.py"
result=$?
printf '\\nTryck Retur för att stänga.\\n'
read -r reply
exit "$result"
'''
 (TMP/'Installera DUB R39.command').write_text(launcher);(TMP/'Installera DUB R39.command').chmod(0o755)
 (TMP/'Benchmark DUB R39.command').write_text('#!/bin/sh\nDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1\nPY="$DIR/../../.build-env/midi-venv/bin/python"\nNAME=${1:-owner}\ncase "$NAME" in *[!A-Za-z0-9_-]*|\'\') echo \'Run-namn får bara innehålla bokstäver, siffror, _ eller -.\'; exit 1;; esac\nRUNS="$DIR/../../dub-synth/hardware-reports/benchmarks"\nif [ ! -x "$PY" ]; then echo \'Pythonmiljön saknas.\'; exit 1; fi\n"$PY" "$DIR/stability_benchmark.py" --canary --run-dir "$RUNS/r39-$NAME-canary" || exit $?\n"$PY" "$DIR/validate_run_state.py" "$RUNS/r39-$NAME-canary" --require-complete || exit $?\n"$PY" "$DIR/stability_benchmark.py" --run-dir "$RUNS/r39-$NAME-full" --canary-run "$RUNS/r39-$NAME-canary"\nresult=$?\nprintf \'\\nBenchmark slutade med kod %s. Resultat: %s\\nTryck Retur för att stänga.\\n\' "$result" "$RUNS/r39-$NAME-full"\nread -r reply\nexit "$result"\n');(TMP/'Benchmark DUB R39.command').chmod(0o755)
 (TMP/'Profilera DUB R39.command').write_text('#!/bin/sh\nDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1\nPY="$DIR/../../.build-env/midi-venv/bin/python"\nNAME=${1:-owner}\ncase "$NAME" in *[!A-Za-z0-9_-]*|\'\') echo \'Ogiltigt run-namn\'; exit 1;; esac\nRUNS="$DIR/../../dub-synth/hardware-reports/benchmarks"\nif [ ! -x "$PY" ]; then echo \'Pythonmiljön saknas.\'; exit 1; fi\n"$PY" "$DIR/profile_benchmark.py" --canary --run-dir "$RUNS/r39-profile-$NAME-canary" || exit $?\n"$PY" "$DIR/validate_run_state.py" "$RUNS/r39-profile-$NAME-canary" --require-complete || exit $?\n"$PY" "$DIR/profile_benchmark.py" --run-dir "$RUNS/r39-profile-$NAME-full" --canary-run "$RUNS/r39-profile-$NAME-canary"\nresult=$?\nprintf \'\\nResultat: %s\\nTryck Retur för att stänga.\\n\' "$RUNS/r39-profile-$NAME-full"\nread -r reply\nexit "$result"\n');(TMP/'Profilera DUB R39.command').chmod(0o755)
 for n in ['LICENSE','LICENSING.md']:shutil.copy2(P/'dinghy'/n,TMP/n)
 with zipfile.ZipFile(TMP/'source.zip','w',zipfile.ZIP_DEFLATED) as z:
  for base in ['firmware','tools','tests']:
   for f in sorted((S/base).rglob('*')):
    if f.is_file() and '__pycache__' not in f.parts and f.name!='.DS_Store' and f.suffix not in ['.pyc','.o']:z.write(f,f.relative_to(S))
  z.write(S/'dub_build.py','dub_build.py')
  z.write(S/'build.sh','build.sh')
  z.write(P/'BUILD-SETUP.md','BUILD-SETUP.md')
  for n in ['LICENSE','LICENSING.md']:z.write(P/'dinghy'/n,n)
  for f in R.iterdir():
   if f.is_file() and f.suffix in ['.py','.c','.h','.md','.js','.cjs','.json']:z.write(f,'application/'+f.name)
  for f in (R/'freestanding').glob('*.h'):z.write(f,'application/freestanding/'+f.name)
 checks.update(state='PREPARED_NOT_INSTALLED',manual_package_prepared=True)
 (TMP/'verification.json').write_text(json.dumps(checks,indent=2)+'\n')
 hashes={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in TMP.iterdir() if f.is_file()}
 manifest=dict(version='DUB Synth R39',package='DUB-r39.fwsc',description='DUB R39: DLY A/B repeat press toggles page1 sound controls and page2 ENGINE/RATE/BITS/DEGRADE. Core1 drum mixer parallel to melodic tracks, hardened SLOOP/X0X mailbox and partial-job rollback. Default4PCM/8OSC, cleaner drum headroom and drum DRIVE0. Limiter/PCM16 retained. Hardware CPU/listening pending owner manual flash.',sha256=hashes)
 (TMP/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 subprocess.run([str(P/'.build-env/midi-venv/bin/python'),str(TMP/'manual_install.py'),'--check'],check=True)
 os.rename(TMP,DEST);print('PREPARED ONLY, NOT INSTALLED:',DEST)
if __name__=='__main__':main()
