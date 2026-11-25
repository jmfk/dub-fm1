#!/usr/bin/env python3
"""Standalone WASM from same fixed-point engine and screen."""
import os,subprocess,base64,json,shutil
from pathlib import Path
R=Path(__file__).resolve().parent;P=R.parent.parent
sdk=P/'.build-env/dub-wasi-sdk'
clang=sdk/'bin/clang'
if not clang.exists():raise SystemExit('Missing .build-env/dub-wasi-sdk/bin/clang')
exports=['mix_data','mix_size','mix_pending','mix_import','mix_ack','init','render','output','draw','command','label','knoblabel','get','preset_data','preset_input','preset_size','preset_record_size','preset_pending','preset_staged','preset_ack','preset_import','clock_next','clock_stamp','seq_data','seq_input','seq_size','seq_pending','seq_ack','seq_import']
cmd=[str(clang),'--no-default-config','--target=wasm32','-nostdinc','-I'+str(R/'freestanding'),'-O2','-fno-builtin','-nostdlib','-Wl,--no-entry','-Wl,--export-memory','-Wl,--initial-memory=1048576','-Wl,--max-memory=1048576',*[f'-Wl,--export={x}' for x in exports],str(R/'wasm.c'),'-o',str(R/'engine.wasm')]
subprocess.run(cmd,check=True)
runtime=(R/'runtime.js').read_text().replace('__MODEL__',json.dumps({**json.loads((R/'parameters.json').read_text()),'drums':[x['name'] for x in json.loads((R/'drums.json').read_text())['samples']]})).replace('__WASM__',base64.b64encode((R/'engine.wasm').read_bytes()).decode())
(R/'simulator.html').write_text((R/'simulator-template.html').read_text().replace('<script src="runtime.js"></script>','<script>'+runtime+'</script>'))
print('Built engine.wasm and self-contained simulator.html')
