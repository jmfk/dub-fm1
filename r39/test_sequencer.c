#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "engine.h"
#include "panel.h"
static int32_t out[512];
static void block(void){d_render(out,256);}static void frames(int n){while(n){int c=n>256?256:n;d_render(out,c);n-=c;}}
static void one(int pattern){ds_pattern *p=ds_pattern_at(pattern);for(int t=0;t<4;t++)p->track[t].length=1;}
static void put(int p,int t,int step,int note,int length){ds_step *s=&ds_pattern_at(p)->track[t].step[step];s->note[0]=(uint8_t)note;s->length=(uint8_t)length;}
static int rectangles;static void rect(int x,int y,int w,int h,uint32_t c){(void)c;assert(x>=0&&y>=0&&x+w<=240&&y+h<=240);rectangles++;}
int main(void){
 d_init();panel_init();assert(sizeof(ds_step)==8&&sizeof(ds_pattern)==4372&&sizeof(ds_bank)==8756);for(int p=0;p<16;p++){assert(ds_pattern_at(p)->next==p);for(int t=0;t<4;t++)assert(ds_pattern_at(p)->track[t].length==16);}
 // INT96PPQN, four notes, shared velocity and captured percentage/x-step lengths.
 one(0);for(int n=0;n<4;n++)ds_pattern_at(0)->track[0].step[0].note[n]=(uint8_t)(60+n*3);dg[G_TEMPO]=120;d_clock_set(0,D_CLK_INT);d_seq_control(DS_PLAY,0,0,0);block();assert(d_active==8&&d_gates(&dp[0])==4&&dp[0].slot[0].velocity==100);frames(22050-256);assert(ds.tick==96&&ds.loop[0]==5); // step0 at start and each sixteenth
 d_seq_stop();assert(!dp[0].held[DS_SOURCE][60]);
 d_init();put(0,0,0,60,50);d_seq_control(DS_PLAY,0,0,0);assert(ds.off[0][60]==12);for(int i=0;i<11;i++)ds_tick();assert(dp[0].held[DS_SOURCE][60]);ds_tick();assert(!dp[0].held[DS_SOURCE][60]);
 d_init();put(0,0,0,60,102);d_seq_control(DS_PLAY,0,0,0);assert(ds.off[0][60]==48);for(int i=0;i<48;i++)ds_tick();assert(!dp[0].held[DS_SOURCE][60]);
 // Repeated long notes replace their own deadline without leaking counts or retriggering shared filter.
 d_init();one(0);put(0,0,0,60,228);d_seq_control(DS_PLAY,0,0,0);for(int i=0;i<1000;i++)ds_tick();assert(dp[0].held[DS_SOURCE][60]==1&&dp[0].filter_triggers==1);d_note_on(0,67,100,0);d_seq_stop();assert(dp[0].held[0][67]&&!dp[0].held[DS_SOURCE][60]);
 // Each track1..128 and triplet/straight periods, independent loops.
 d_init();ds_pattern_at(0)->track[0].length=128;ds_pattern_at(0)->track[1].length=3;ds_pattern_at(0)->track[1].grid=3;d_seq_control(DS_PLAY,0,0,0);for(int i=0;i<96;i++)ds_tick();assert(ds.play[0]==4&&ds.play[1]==0&&ds.loop[1]==1&&ds_span()==3072);for(int i=96;i<3072;i++)ds_tick();assert(ds.play[0]==0&&ds.loop[0]==1);
 // Every/phase, chance endpoints, positive/negative performance conditions.
 ds_step s={{60,188,188,188},100,100,DS_EVERY,0x12};ds.loop[0]=1;assert(ds_condition(&s,0));ds.loop[0]=0;assert(!ds_condition(&s,0));s.condition=DS_CHANCE;s.value=0;for(int i=0;i<100;i++)assert(!ds_condition(&s,0));s.value=100;for(int i=0;i<100;i++)assert(ds_condition(&s,0));s.value=50;int played=0;for(int i=0;i<1000;i++)played+=ds_condition(&s,0);assert(played>350&&played<650);
 for(int condition=DS_TRIG;condition<=DS_NOT_FILL;condition++){s.condition=(uint8_t)condition;for(int on=0;on<2;on++){ds.fill=ds.trig=(uint8_t)on;assert(ds_condition(&s,0)==((condition==DS_TRIG||condition==DS_FILL)?on:!on));}}
 d_init();one(0);put(0,0,0,60,100);ds_pattern_at(0)->track[0].step[0].condition=DS_FILL;ds.fill=1;d_seq_control(DS_PLAY,0,0,0);assert(dp[0].held[DS_SOURCE][60]);
 // Repeat twice then chain; next STOP releases only sequencer-owned notes.
 d_init();one(0);one(1);put(0,0,0,60,228);put(1,0,0,72,100);ds_pattern_at(0)->repeats=2;ds_pattern_at(0)->next=1;ds_pattern_at(1)->next=16;d_note_on(1,65,100,0);d_seq_control(DS_PLAY,0,0,0);for(int i=0;i<47;i++)ds_tick();assert(ds.pattern==0);ds_tick();assert(ds.pattern==1&&!dp[0].held[DS_SOURCE][60]&&dp[0].held[DS_SOURCE][72]);for(int i=0;i<24;i++)ds_tick();assert(!ds.playing&&dp[1].held[0][65]);
 // Atomic keyboard chords, physical-only isolated pages, navigation and performance release across EXIT/blur.
 d_init();panel_init();panel_nav(4,0);const char *labels[12];panel_labels(labels);assert(!strcmp(labels[9],"SEQ")&&panel_enabled(9));panel_button(9,1,1);assert(panel.group==9);panel_labels(labels);assert(!strcmp(labels[0],"STEP")&&!strcmp(labels[4],"EXIT"));for(int key=0;key<4;key++){panel_key(key,1);}block();for(int n=0;n<4;n++)assert(!(ds_pattern_at(0)->track[0].step[0].note[n]&128));for(int key=0;key<4;key++)panel_key(key,0);block();
 panel_button(1,1,1);panel_turn(2,112);block();assert(ds_pattern_at(0)->track[0].length==128);panel.seq_step=0;panel.alt=1;panel_control(19,1,0,0);assert(panel.seq_step==16);panel.alt=0;panel.shift=1;panel_control(19,1,0,0);assert(panel.seq_note==1);panel.shift=0;panel_button(6,1,1);block();assert(ds.fill);panel_button(4,1,1);panel_button(6,0,2);block();assert(!ds.fill);panel_nav(9,0);panel_button(7,1,1);block();assert(ds.trig);panel_control(20,0,0,0);block();assert(!ds.trig&&!panel.alt&&!panel.shift);
 for(int t=0;t<4;t++)for(int page=0;page<4;page++){panel.track=t;panel_nav(9,page);panel_render(rect,0);}assert(rectangles>100);
 // Outside transport is Shift PLAY, while plain PLAY stays ALT.
 panel_nav(0,0);panel_button(10,1,1);assert(panel.alt&&!ds.playing);panel_button(10,0,2);assert(!panel.alt);panel_button(5,1,1);panel_button(10,1,1);block();assert(ds.playing&&!panel.alt);panel_button(5,0,2);panel_button(10,0,2);assert(!panel.alt);panel_button(5,1,1);panel_button(10,1,1);block();assert(!ds.playing);panel_button(10,0,2);panel_button(5,0,2);
 // Drum mode preserves separate music, all11 samples play and mix to track4 effects.
 d_init();put(0,3,0,60,100);d_seq_edit(0,(3<<12)|DS_DRUM,1,0);d_seq_edit(0,(3<<12)|DS_HITS,2047,0);assert(ds_pattern_at(0)->track[3].step[0].note[0]==60);for(int pad=0;pad<11;pad++){d_drum_clear();d_note_on(3,d_drum_keys[pad],100,0);int64_t audio=0;for(int i=0;i<128;i++){block();for(int j=0;j<512;j++)audio+=abs(out[j]);}assert(audio>1000);}d_drum_clear();d_seq_control(DS_PLAY,0,0,0);int active=0;for(int i=0;i<11;i++)active+=d_drum_voice[i].active;assert(active==11);d_seq_edit(0,(3<<12)|DS_DRUM,0,0);assert(ds_pattern_at(0)->hits[0]==2047&&ds_pattern_at(0)->track[3].step[0].note[0]==60);
 // EXT waits for actual F8, interpolates96PPQN, rejects other source, stops free-run on missing clock.
 d_init();put(0,0,0,60,100);d_clock_set(0,D_CLK_EXT);d_seq_control(DS_PLAY,0,0,0);frames(44100);assert(!ds.started&&!d_gates(&dp[0]));d_clock_receive(0);frames(919);d_clock_receive(0);assert(ds.started);uint32_t pulses=ds.pulses;d_clock_receive(1);assert(ds.pulses==pulses);for(int i=0;i<24;i++){frames(919);d_clock_receive(0);}assert(ds.tick==96);uint32_t tick=ds.tick;frames(44100);assert(ds.tick<=tick+3);d_seq_midi_transport(0xfc,0);assert(!ds.playing);d_seq_midi_transport(0xfb,0);assert(ds.playing);
 // All16 patterns validate/persist in8 independent banks; malformed data never applies.
 d_init();ds_pattern_at(15)->track[3].length=128;ds_pattern_at(15)->drums=1;ds_pattern_at(15)->hits[127]=2047;d_seq_control(DS_SAVE,0,0,0);for(int bank=0;bank<8;bank++){assert(ds_io==bank&&ds_valid(&ds_work));ds_bank copy=ds_work;ds_work.checksum^=1;assert(!ds_import(bank));ds_work=copy;ds_ack=1;d_seq_service();}assert(ds_io<0&&ds.saving<0);ds_stage(7);ds_bank copy=ds_work;ds_pattern_at(15)->hits[127]=0;ds.saving=-1;assert(ds_import(7)&&ds_pattern_at(15)->hits[127]==2047);ds_work=copy;ds_work.pattern[1].track[3].grid=2;ds_work.checksum=ds_hash(&ds_work);assert(!ds_import(7));
 d_seq_control(DS_PLAY,0,0,0);d_seq_control(DS_SAVE,0,0,0);assert(ds.saving<0);d_send(DC_PANIC,0,0,0,0);block();assert(!ds.playing&&!ds.fill&&!ds.trig);for(int i=0;i<512;i++)assert(!out[i]);
 puts("PASS4-track96PPQN sequencing,4-note chords, velocity/%/x lengths, ownership/repeated pitch,1–128/triplets, all conditions, repeat/NEXT/STOP, isolated physical pages/navigation/performance blur, transport,11 PCM drums/preserved modes, external clock/transport and validated16-pattern storage/panic");
}
