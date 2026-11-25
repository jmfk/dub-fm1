import importlib.util
from pathlib import Path
R=Path(__file__).resolve().parent;s=importlib.util.spec_from_file_location('runtime',R/'control_runtime.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
reply=(R.parent.parent/'.build-env/dub-r39-runtime-fixture.bin').read_bytes();state=m.decode(reply,2);assert state['osc_per_voice']=='1' and not state['matrix'] and not state['delays'] and not state['reverb']
class Message:
 def __init__(self,kind,data):self.type=kind;self.data=tuple(data)
class Mido:Message=Message
class Input:
 def iter_pending(self):return [Message('sysex',reply[1:-1])]
class Output:
 def __init__(self):self.sent=[]
 def send(self,msg):self.sent.append(msg)
out=Output();assert m.capture(Mido,Input(),out,3,0)==state;assert bytes(out.sent[0].data)==m.request(3,0)[1:-1]
for pos in [0,6,7,8,12,13]:
 bad=bytearray(reply);bad[pos]^=1
 try:m.decode(bad,2)
 except ValueError:pass
 else:raise AssertionError(pos)
print('PASS C runtime reply -> Python, checksum/state validation and mock MIDI set/ack; no device access')
