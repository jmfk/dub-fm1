import importlib.util,json,tempfile
from pathlib import Path
R=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('perf',R/'download_performance.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
fixture=(R.parent.parent/'.build-env/dub-r39-diag-fixture.bin').read_bytes();report=m.decode(fixture,0)
assert report['scopes']['app']['average_load_percent']==50 and report['scopes']['isr']['average_load_percent']==60
for data in [fixture[:-1],b'\xf0\x22'+fixture[2:],fixture[:6]+b'\x41'+fixture[7:]]:
 try:m.decode(data,0)
 except ValueError:pass
 else:raise AssertionError('Malformed reply accepted')
class Message:
 def __init__(self,type,data):self.type=type;self.data=tuple(data)
class Mido:Message=Message
class Input:
 def __init__(self):self.done=False
 def iter_pending(self):
  if self.done:return []
  self.done=True;return[Message('sysex',[1,2,3]),Message('sysex',fixture[1:-1])]
class Output:
 def __init__(self):self.sent=[]
 def send(self,message):self.sent.append(message)
out=Output();assert m.capture(Mido,Input(),out,0)['firmware']=='DUB R39';assert bytes(out.sent[0].data)==m.request(0)[1:-1]
print('PASS C firmware fixture -> Python decoded mean/peak report, malformed replies rejected, snapshot captured over mock MIDI ports; no real MIDI access')
