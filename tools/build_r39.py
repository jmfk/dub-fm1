#!/usr/bin/env python3
"""Standalone offline build of the imported R39. Never opens devices or installs."""
import argparse,hashlib,importlib.util,json,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--check',action='store_true');p.add_argument('--toolchain',type=Path,default=ROOT/'.build-env/jieli');p.add_argument('--sdk',type=Path,default=ROOT/'.build-env/ac79-sdk');p.add_argument('--docker-context',default='orbstack');p.add_argument('--image',default='dub-fm1-build:local');a=p.parse_args()
 spec=importlib.util.spec_from_file_location('r39_build',ROOT/'r39/build.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 files=module.FILES;identity=hashlib.sha256(b''.join((ROOT/'r39'/n).read_bytes() for n in files)).hexdigest();record=json.loads((ROOT/'r39/verified-build.json').read_text());assert identity==record['source_sha256'],'R39 source differs from imported verified build'
 assert (ROOT/'platform/firmware/app.ld').is_file() and (ROOT/'platform/dub_build.py').is_file()
 if a.check:print('PASS: R39 source identity and standalone platform present; no devices opened');return
 toolchain=a.toolchain.resolve();sdk=a.sdk.resolve();assert (toolchain/'toolchain/pi32v2/bin/clang').is_file(),'Expected TOOLCHAIN_DIR/toolchain/pi32v2/bin/clang';assert (sdk/'cpu/wl82/tools/uboot.boot').is_file(),'Expected SDK_DIR/cpu/wl82/tools/uboot.boot'
 stage=ROOT/'.build-env/r39-build';stage.mkdir(parents=True,exist_ok=True)
 for name in ['firmware','tools','tests']:shutil.copytree(ROOT/'platform'/name,stage/name,dirs_exist_ok=True)
 for n in files:
  target=stage/'firmware'/n if n.startswith('hal/') else stage/'firmware/src'/n;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/'r39'/n,target)
 shutil.copy2(ROOT/'r39/app.c',stage/'firmware/src/dinghy.c');shutil.copy2(ROOT/'platform/dub_build.py',stage/'dub_build.py')
 cmd=['docker','--context',a.docker_context,'run','--rm','--platform','linux/amd64','--network','none','--mount',f'type=bind,source={stage},target=/work','--mount',f'type=bind,source={toolchain},target=/opt/jieli,readonly','--mount',f'type=bind,source={sdk},target=/opt/ac79-sdk,readonly','-e','JIELI_TOOLCHAIN=/opt/jieli/toolchain','-e','AC79_SDK=/opt/ac79-sdk','-w','/work',a.image,'python3','dub_build.py'];subprocess.run(cmd,check=True)
 assert hashlib.sha256((stage/'build/DUB-r39.fwsc').read_bytes()).hexdigest()==record['firmware_sha256'],'Built package differs from verified R39'
 print('PASS: built firmware matches imported R39 byte-for-byte; no devices opened')
if __name__=='__main__':main()
