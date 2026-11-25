#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static int32_t pcm[512];
static void tick(void){d_render(pcm,256);}
int main(void){d_init();panel_init();d_part *t=&dp[0];
 assert(d_route_target(t,3)==4&&d_route_target(t,11)==0&&d_route_target(t,12)==2);
 for(int l=0;l<3;l++)t->p[d_lfo_ids[l][1]]=0;
 d_route_set(t,0,0,4);d_route_set(t,0,1,127);t->p[P_LFO_SHAPE]=1;t->lfo[0]=0;d_prepare_part(t);int quiet=t->osc_lfo_amp[0];t->lfo[0]=0x80000000u;d_prepare_part(t);assert(quiet<10&&t->osc_lfo_amp[0]>32000&&t->osc_lfo_amp[1]==32768);
 d_note_on(0,60,100,0);d_route_set(t,0,0,12);t->lfo[0]=0x80000000u;d_prepare_part(t);uint32_t high=t->osc[0].inc,other=t->osc[1].inc;t->lfo[0]=0;d_prepare_part(t);assert(t->osc[0].inc<high&&t->osc[1].inc==other);
 d_route_set(t,0,0,20);t->osc[0].env.v=D_Q;d_route_set(t,3,0,0);d_prepare_part(t);assert(t->modulation[DL_FILTER]==127);
 d_route_set(t,3,0,20);d_prepare_part(t);assert(!t->modulation[DL_FILTER]);
 d_route_set(t,12,0,20);d_route_set(t,3,0,2);d_route_set(t,3,1,-127);d_prepare_part(t);assert(d_amp_envelopes(t,8)==0);t->osc[0].env.v=D_Q/2;assert(d_amp_envelopes(t,8)==16384);
 d_route_set(t,3,0,20);d_prepare_part(t);assert(d_amp_envelopes(t,0)==32768);assert(d_osc_running(t,0));d_note_off(0,60,0);assert(!d_osc_running(t,0));
 d_init();panel_init();t=&dp[0];panel_nav(7,1);panel_turn(2,3);assert(panel.matrix_source==3);panel_turn(3,1);tick();assert(t->env_target[0]==5);panel_turn(4,-127);tick();assert(t->env_amount[0]==0);
 preset_copy_name(preset_draft,"ROUTE SAVE");preset_request(PS_SAVE,0);tick();preset_saved(1);t->env_target[0]=20;preset_request(PS_LOAD,0);tick();assert(t->env_target[0]==5&&t->env_amount[0]==0);
 for(int g=0;g<6;g++)if(g==0||g==1||g==5){panel_nav(g,0);panel_button(9,1,1);assert(panel.group==7&&panel.page[7]==0);panel.shift=1;panel_button(9,1,2);assert(panel.group==7&&panel.page[7]==1);panel.shift=0;panel.alt=1;panel_button(4,1,3);panel.alt=0;assert(panel.group==g);}
 preset_patch p;preset_capture(&p,"INVALID");p.part[0].env_target[0]=255;assert(!preset_valid(&p));
 t->env_target[8]=4;panel_nav(0,1);dub_control c[4];panel_controls(c);assert(c[3].op==-1);int before=t->env_amount[8];panel_turn(5,1);tick();assert(t->env_amount[8]==before);
 // Every permitted destination renders with four active parts, bounded output.
 for(int dest=0;dest<D_TARGETS;dest++){d_init();for(int tr=0;tr<4;tr++){d_route_set(&dp[tr],0,0,dest);d_route_set(&dp[tr],0,1,127);d_route_set(&dp[tr],11,0,dest);for(int n=0;n<4;n++)d_note_on(tr,48+n*4,100,0);}for(int i=0;i<32;i++){tick();for(int n=0;n<512;n++)assert(pcm[n]>=-32767&&pcm[n]<=32767);}}
 puts("PASS real oscillator amp/pitch LFO routing, ENV filter redirect/off/inversion, disconnected amp/note-off, physical matrix edits and shortcuts, patch roundtrip/validation, all51 destinations bounded across four parts");
}
