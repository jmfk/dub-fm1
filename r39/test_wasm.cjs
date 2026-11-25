const fs=require('fs'),assert=require('assert');
(async()=>{const e=(await WebAssembly.instantiate(fs.readFileSync(__dirname+'/engine.wasm'),{})).instance.exports;
function block(n=1){while(n--)e.render();}function midi(st,n,v,s=0){e.command(1,0,st,n,v,s);block();}
for(let ch=0;ch<4;ch++){e.init();midi(0x90|ch,48+ch*3,100);let energy=0;for(let n=0;n<40;n++){block();for(const x of new Int32Array(e.memory.buffer,e.output(),512))energy+=Math.abs(x);}assert(energy>10000);assert.equal(e.get(5,0,0),2);midi(0x80|ch,48+ch*3,0);block(900);assert.equal(e.get(5,0,0),0);}
e.init();for(let i=0;i<4;i++)midi(0x90,60+i*3,100);assert.equal(e.get(5,0,0),8);assert.equal(e.get(16,0,0),1);midi(0xb0,80,127);assert.equal(e.get(16,0,0),2);
assert(e.draw()>0);e.command(22,0,5,0,0,0);block();assert.equal(e.get(11,0,0),5);
for(let l=0;l<3;l++){e.command(16,0,6+l,1,100,0);e.command(17,0,3,1,0,0);e.command(17,0,5,l,0,0);block();assert.equal(e.get(13,0,0),l);assert.equal(e.get(21,0,l*4+1),l===0?13:1);assert.equal(e.get(21,0,l*4+3),l);}
const bankBefore=new Uint8Array(e.memory.buffer,e.preset_data(),e.preset_size()).slice();new Uint8Array(e.memory.buffer,e.preset_input(),e.preset_size()).fill(255);assert.equal(e.preset_import(),0);assert.deepEqual(new Uint8Array(e.memory.buffer,e.preset_data(),e.preset_size()),bankBefore);console.log('PASS same WASM motor: four MIDI parts, sound/releases, legato/CC80, physical panel and pixels');
})().catch(e=>{console.error(e);process.exit(1)});
