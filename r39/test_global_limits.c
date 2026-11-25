/* Staged regression cases. NOT RUN during owner-requested build-only work. */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static int32_t out[512];
static int has(int p,int note){for(int i=0;i<8;i++)if(d_budget_osc_active(p,i)&&dp[p].slot[(int)dp[p].osc[i].slot].note==note)return 1;return 0;}
int main(void){
 d_init();assert(d_osc_limit==8&&d_sample_limit==4);ds_pattern_at(0)->drums=1;
 d_osc_limit_set(4);d_sample_limit_set(2);
 for(int p=0;p<3;p++)for(int n=0;n<4;n++){d_note_on(p,48+n*3,90,0);assert(d_osc_active_count()<=4);}
 for(int i=0;i<11;i++){d_drum_on(d_drum_keys[i],90,0);assert(d_sample_active_count()<=2&&d_osc_active_count()==4);}
 d_sample_limit_set(1);assert(d_sample_active_count()==1&&d_osc_active_count()==4);
 d_osc_limit_set(1);assert(d_osc_active_count()==1&&d_sample_active_count()==1);
 d_drum_on(36,90,0);d_drum_on(36,90,0);assert(d_sample_active_count()==1&&d_osc_active_count()==1);
 // A mono group under global3 must allocate only3 actual OSCs.
 d_init();dp[0].voice_mode=0;d_osc_limit_set(3);d_note_on(0,48,90,0);assert(d_osc_active_count()==3);
 d_note_on(1,55,90,0);assert(d_osc_active_count()==3&&has(1,55));
 // Released OSCs are stolen before held groups; budget remains exact.
 d_init();d_osc_limit_set(4);d_note_on(0,48,90,0);d_note_on(1,52,90,0);d_note_off(0,48,0);d_note_on(2,55,90,0);
 assert(!has(0,48)&&has(1,52)&&has(2,55)&&d_osc_active_count()==4);
 // Old note-off must not release a new repetition after stealing.
 d_init();d_osc_limit_set(1);d_note_on(0,48,90,0);d_note_on(1,55,90,0);d_note_on(0,48,90,0);
 assert(dp[0].held[0][48]==2);d_note_off(0,48,0);assert(has(0,48)&&d_slot_gates(&dp[0]));d_note_off(0,48,0);assert(!d_slot_gates(&dp[0]));
 // Late stolen-note off does not erase another chord's release mask.
 d_init();d_osc_limit_set(4);d_note_on(0,48,90,0);d_note_on(0,52,90,0);d_note_on(0,55,90,0);assert(!has(0,48));
 d_note_off(0,52,0);d_note_off(0,55,0);unsigned tail=d_release[0].tail;assert(tail);d_frame++;d_note_off(0,48,0);assert(d_release[0].tail==tail);
 // Sampling released-first behavior is independent of synth allocation.
 d_init();ds_pattern_at(0)->drums=1;d_sample_limit_set(2);d_drum_on(36,90,4);d_drum_on(38,90,0);d_drum_off(36,4);d_drum_on(37,90,0);
 assert(!d_drum_voice[0].active&&d_drum_voice[1].active&&d_drum_voice[2].active);
 // Physical VOICES page: K1 samples and K2 OSC; other knobs unused.
 panel_init();panel_nav(12,0);assert(panel_enabled(8));panel_button(8,1,1);panel_button(8,0,2);assert(panel.page[12]==2);
 panel_turn(2,-127);panel_turn(3,-127);d_render(out,32);panel_turn(2,3);panel_turn(3,4);d_render(out,32);assert(d_sample_limit==3&&d_osc_limit==4);
 d_full_panic();assert(!d_osc_active_count()&&!d_sample_active_count());d_init();assert(d_osc_limit==8&&d_sample_limit==4);
 // Direct Settings > VOICES replaces INFO; both selectors address the global caps.
 panel_init();panel_button(4,1,3);panel_button(4,0,4);assert(panel.group==4);
 const char *labels[12];panel_labels(labels);assert(!strcmp(labels[7],"VOICES"));
 panel_button(7,1,5);panel_button(7,0,6);assert(panel.group==4&&panel.page[4]==1);
 dub_control c[4];panel_controls(c);assert(c[0].op==DC_SAMPLE_LIMIT&&c[1].op==DC_OSC_LIMIT&&c[2].op<0&&c[3].op<0);
 panel.track=3;panel_turn(2,-127);panel_turn(3,-127);d_render(out,32);panel_turn(2,3);panel_turn(3,8);d_render(out,32);assert(d_sample_limit==3&&d_osc_limit==8);
 panel_turn(4,90);panel_turn(5,90);d_render(out,32);assert(d_sample_limit==3&&d_osc_limit==8);
 panel_turn(2,-127);panel_turn(3,127);d_render(out,32);assert(!d_sample_limit&&d_osc_limit==33);
 panel_button(4,1,7);panel_button(4,0,8);assert(panel.group==4&&panel.page[4]==2);
 panel_button(4,1,9);panel_button(4,0,10);assert(panel.page[4]==0);
 panel_button(4,1,11);panel_button(4,0,12);assert(panel.page[4]==1);
 // OFF stops the running source, blocks new notes, clears isolated PCM, and never resumes old held notes.
 d_init();ds_pattern_at(0)->drums=1;d_note_on(0,60,100,0);d_drum_on(36,100,0);d_render(out,32);
 assert(d_osc_active_count()>0&&d_sample_active_count()==1);d_osc_limit_set(0);assert(!d_osc_active_count()&&d_sample_active_count()==1);
 unsigned phase=dp[0].osc[0].phase;d_note_on(0,64,100,0);d_render(out,32);assert(!d_osc_active_count()&&dp[0].osc[0].phase==phase);
 d_osc_limit_set(33);assert(!d_osc_active_count());d_note_off(0,60,0);d_note_on(0,67,100,0);assert(d_osc_active_count()>0);
 d_sample_limit_set(0);assert(!d_sample_active_count()&&d_osc_active_count()>0);for(int i=0;i<11;i++)assert(!d_drum_raw[i]);for(int i=0;i<9;i++)assert(!d_drum_bus[i]);
 for(int src=0;src<5;src++)d_drum_on(36,100,src);d_render(out,32);assert(!d_sample_active_count());
 d_sample_limit_set(12);assert(!d_sample_active_count());for(int i=0;i<11;i++)d_drum_on(d_drum_keys[i],100,0);assert(d_sample_active_count()==11);
 d_init();d_osc_limit_set(33);for(int p=0;p<4;p++){dp[p].voice_mode=0;d_note_on(p,48+p,100,0);}assert(d_osc_active_count()==32);
 d_osc_limit_set(0);d_sample_limit_set(0);for(int p=0;p<4;p++)d_note_on(p,60,100,0);d_render(out,32);assert(!d_osc_active_count()&&!d_sample_active_count());
 for(int i=0;i<64;i++)assert(out[i]==0);d_osc_limit_set(33);d_sample_limit_set(12);assert(!d_osc_active_count()&&!d_sample_active_count());
 panel_init();panel_nav(4,1);panel_controls(c);char text[12];pvalue(c[0],text);assert(!strcmp(text,"INF"));pvalue(c[1],text);assert(!strcmp(text,"INF"));
 panel_turn(2,-127);panel_turn(3,-127);d_render(out,32);panel_controls(c);pvalue(c[0],text);assert(!strcmp(text,"OFF"));pvalue(c[1],text);assert(!strcmp(text,"OFF"));
 d_init();ds_pattern_at(0)->drums=1;static const int16_t loop_pcm[4]={1000,-1000,2000,-2000};d_sample_views[0]=(d_sample_view){loop_pcm,4,1};d_drum_cache_init();d_drum_on(36,100,0);d_render(out,64);assert(d_sample_active_count()==1);d_sample_limit_set(0);d_render(out,32);assert(!d_sample_active_count());d_sample_limit_set(12);d_render(out,32);assert(!d_sample_active_count());d_drum_on(36,100,0);assert(d_sample_active_count()==1);d_drum_off(36,0);d_render(out,256);assert(!d_sample_active_count());
 puts("PASS OFF blocks and stops each source independently, no OSC phase advance/PCM raw residue, INF restores capacity without stale-note resurrection, OFF/INF labels");
 puts("PASS Settings > VOICES via physical SAVE and repeated EDIT, global K1/K2 bounds, unused K3/K4");
 puts("PASS independent sample/OSC limits, partial mono groups, released-first/oldest, retrigger and note-off/tail ownership, physical controls/defaults");
}
