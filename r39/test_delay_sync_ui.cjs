const path=require('path'),assert=require('assert'),{chromium}=require(path.resolve(__dirname,'../../.build-env/designer-qa/node_modules/playwright'));
(async()=>{const b=await chromium.launch({executablePath:'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',headless:true});const p=await b.newPage({viewport:{width:1280,height:1080}}),errors=[];p.on('pageerror',e=>errors.push(e.message));await p.goto(require('url').pathToFileURL(__dirname+'/simulator.html').href);await p.evaluate(()=>dubSynth.ready);await p.keyboard.press('Digit1');
for(let bus=0;bus<2;bus++){
 await p.keyboard.press('F'+(bus+1));const original=await p.evaluate(i=>dubSynth.engine.get(40,i,1),bus);
 assert.equal(await p.locator('#encodersRight small').first().textContent(),'TIME MS');
 await p.locator('#screen').screenshot({path:path.join(__dirname,'screen-delay-ms.png')});
 await p.evaluate(()=>dubSynth.knob(4,1));assert.equal(await p.evaluate(i=>dubSynth.engine.get(40,i,2),bus),4);assert.equal(await p.locator('#encodersRight small').first().textContent(),'TIME BT');
 await p.evaluate(()=>dubSynth.knob(2,-2));assert.equal(await p.evaluate(i=>dubSynth.engine.get(40,i,2),bus),2);
 await p.locator('#screen').screenshot({path:path.join(__dirname,'screen-delay-sync.png')});
 await p.evaluate(()=>dubSynth.knob(4,-1));assert.equal(await p.evaluate(i=>dubSynth.engine.get(40,i,2),bus),10);assert.equal(await p.evaluate(i=>dubSynth.engine.get(40,i,1),bus),original);
 await p.evaluate(()=>dubSynth.knob(2,1));assert.equal(await p.evaluate(i=>dubSynth.engine.get(40,i,1),bus),original+5);
 await p.evaluate(()=>dubSynth.knob(4,1));assert.equal(await p.evaluate(i=>dubSynth.engine.get(40,i,2),bus),2);assert.equal(await p.evaluate(i=>dubSynth.engine.get(40,i,1),bus),original+5);
}
await p.evaluate(()=>dubSynth.knob(2,6));assert.equal(await p.evaluate(()=>dubSynth.engine.get(40,1,2)),8);await p.locator('#screen').screenshot({path:path.join(__dirname,'screen-delay-bar.png')});
await p.keyboard.press('Digit5');await p.evaluate(()=>dubSynth.knob(3,-127));assert.equal(await p.evaluate(()=>dubSynth.state.internalTempo),30);await p.keyboard.press('Digit1');await p.keyboard.press('F2');await p.locator('#screen').screenshot({path:path.join(__dirname,'screen-delay-8s.png')});
await p.keyboard.down('Shift');await p.keyboard.press('Digit5');await p.keyboard.up('Shift');await p.keyboard.press('F2');await p.keyboard.press('Digit5');await p.reload();await p.evaluate(()=>dubSynth.ready);await p.keyboard.down('Shift');await p.keyboard.press('Digit5');await p.keyboard.up('Shift');await p.keyboard.press('F1');await p.keyboard.press('Digit5');assert.equal(await p.evaluate(()=>dubSynth.engine.get(40,0,2)),2);assert.equal(await p.evaluate(()=>dubSynth.engine.get(40,1,2)),8);
await p.keyboard.press('Digit1');await p.keyboard.press('F1');await p.evaluate(()=>{dubSynth.knob(4,-1);for(let i=0;i<13;i++)dubSynth.knob(2,127);});assert.equal(await p.evaluate(()=>dubSynth.engine.get(40,0,1)),8000);await p.locator('#screen').screenshot({path:path.join(__dirname,'screen-delay-8000ms.png')});
assert.deepEqual(errors,[]);await b.close();console.log('PASS both physical delay pages: K1 MS/beat, K3 MS/SYNC, inactive values retained, actual preset save/reload, 8s bar at30BPM and8000ms screenshots and no browser errors');})().catch(e=>{console.error(e);process.exit(1)});
