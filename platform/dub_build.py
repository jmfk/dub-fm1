import sys,json,re
from pathlib import Path
sys.path.insert(0,'tools')
import build as b
b.PRODUCT='FM-1_990';b.VERSION='dub-r39'
b.OUT.mkdir(exist_ok=True)
ota=b.build_loader();img,syms,dis,rt=b.build_app()
errors,notes=b.check(img,syms,dis,rt);errors+=b.mmio_check()
for n in notes: print('ok',n)
assert not errors,errors
def sv(name):return int(re.search(r'^([0-9a-f]+).*\s'+name+r'$',syms,re.M).group(1),16)
assert sv('_c1_bss_start')>=0x1c06000 and sv('_c1_bss_end')<=0x1c07f00
assert sv('_c1_load')-sv('_rt_load')==sv('_rt_kernel_end')-sv('_rt_start')
assert sv('_rt_end')<=0x1c06000
print('ok core1 stacks/scratch',sv('_c1_bss_end')-sv('_c1_bss_start'),'B; copy/PC/region bounds checked')
(b.OUT/'DUB-r39.bin').write_bytes(img)
(b.OUT/'DUB-r39.fwsc').write_bytes(b.fm1pkg_make.ufw(b.fm1pkg_make.flash_image(img,b.fm1pkg_make.KEY),ota,b.PRODUCT))
(b.OUT/'dinghy.syms').write_text(syms)
