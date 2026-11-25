/* SPDX-License-Identifier: GPL-3.0-only; own menus in KEEL visual system. */
#pragma once
#include "ui_style.h"
#define UC_BG 0x080e19u
#define UC_TILE 0x142035u
#define UC_TEXT 0xedf4ffu
#define UC_MUTED 0x95a8c2u
#define UC_DIM 0x26364eu
#define UC_CYAN 0x42d9f5u
#define UC_AMBER 0xffc66bu
static const uint32_t dub_accents[4]={0xffc66b,0x58e3b0,0x42d9f5,0xb8a4ff};
#define D_GROUPS 14
static const char *const dub_groups[D_GROUPS]={"SYN","ENV","FX","MIX","SET","LFO","PRESETS","MOD","VOICE","SEQ","USB","DRUM MIX","CPU","DELAY"};
static const char *const dub_pages[D_GROUPS][4]={{"OSC","FILT",""},{"FENV","POST","",""},{"DLY A","DLY B","VERB","RETS"},{"CHANNELS","","",""},{"MIDI","VOICES","SOUND",""},{"LFO1","LFO2","LFO3",""},{"LOAD","SAVE","INIT"},{"LIST","MATRIX","",""},{"SETUP","","",""},{"STEP","TRACK","PAT","COND"},{"AUDIO","","",""},{"PADS","","",""},{"ISOLATE","MODRATE","VOICES",""},{"DLY A","DLY B","",""}};
static struct {int delay_page[2],drum_pad,seq_step,seq_note,seq_transport_held,seq_fill_held,seq_trig_held,seq_alt_held,voice_back_group,voice_back_page,mod_back_group,mod_back_page,matrix_bank,matrix_source,preset_back_group,preset_back_page,preset_mode,confirm,track,group,back_group,back_page,page[D_GROUPS],osc,shift,alt,octave,playing;int key_track[27],key_note[27];uint8_t key_held[27],shortcut_held[27];uint32_t down[12];} panel;
static volatile uint32_t panel_cpu,panel_cpu_peak,panel_late;
static int d_page_count(int group){return group==13?2:group==12?3:group>=10?1:group==9?4:group==3||group==8?1:group==0||group==1||group==7?2:group==2?4:3;}
static int panel_mod_family(void){return panel.group==0||panel.group==1||panel.group==5||panel.group==7||panel.group==8;}
static const uint8_t d_mod_source_keys[13]={0,2,4,6,7,9,11,12,14,16,18,19,21};
static int panel_key_source(int key){for(int i=0;i<5;i++)if(d_mod_source_keys[i]==key)return d_mod_sources[i];return -1;}
static const char *const d_matrix_banks[5]={"SYNTH","DLY A","DLY B","VERB","RETURNS"};
static void panel_target_bank(int target){if(target<20)panel.matrix_bank=0;else if(target>=21)panel.matrix_bank=target<31?1:target<41?2:target<47?3:4;}
typedef struct {int op,index,source;const char *name;int value,min,max;} dub_control;
static dub_control pc(int op,int idx,int src,const char *name,int value,int min,int max){return(dub_control){op,idx,src,name,value,min,max};}
static volatile d_part *const d_observe=dp;
static void preset_cursor_move(int d){int n=preset_len(preset_draft)+1;preset_cursor=(preset_cursor+d%n+n)%n;}
static void preset_backspace(void){int n=preset_len(preset_draft);if(preset_cursor>0){for(int i=preset_cursor-1;i<n;i++)preset_draft[i]=preset_draft[i+1];preset_cursor--;}}
static void preset_space(void){int n=preset_len(preset_draft);if(n>=40)return;for(int i=n;i>=preset_cursor;i--)preset_draft[i+1]=preset_draft[i];preset_draft[preset_cursor++]=' ';}
static void preset_next_word(void){int n=preset_len(preset_draft),i=preset_cursor;while(i<n&&preset_draft[i]!=' ')i++;while(i<n&&preset_draft[i]==' ')i++;while(i<n&&preset_draft[i]!=' ')i++;if(i>=n&&preset_cursor>=n){i=0;while(i<n&&preset_draft[i]!=' ')i++;}preset_cursor=i;}
static void preset_replace_word(void){int n=preset_len(preset_draft),a=preset_cursor,b=preset_cursor;if(a&&(!preset_draft[a]||preset_draft[a]==' '))a--;while(a>0&&preset_draft[a-1]!=' ')a--;b=a;while(b<n&&preset_draft[b]!=' ')b++;char candidate[41],word[7];int start=preset_random()%640;for(int j=0;j<640;j++){preset_word((start+j)%640,word);int w=0;while(word[w])w++;if(n-(b-a)+w>40)continue;int pos=0;for(int i=0;i<a;i++)candidate[pos++]=preset_draft[i];for(int i=0;i<w;i++)candidate[pos++]=word[i];for(int i=b;i<n;i++)candidate[pos++]=preset_draft[i];candidate[pos]=0;if(!preset_same(candidate,preset_draft)&&!preset_taken(candidate,-1)){preset_copy_name(preset_draft,candidate);preset_cursor=a+w;return;}}preset_error=2;}
static void preset_select_slot(int delta){preset_selected=(preset_selected+delta%8+8)%8;if(preset_listen)preset_request(PS_PREVIEW,preset_selected);}
static void panel_preset_page(int page){panel.preset_mode=page+1;panel.confirm=0;panel.page[6]=page;preset_error=0;preset_io_result=0;if(preset_preview)preset_request(PS_RESTORE,0);if(page){preset_copy_name(preset_draft,preset_name);if(page==2)preset_new_name(preset_draft);preset_cursor=0;}}
#include "seq_controls.h"
static void panel_controls(dub_control c[4]){
 volatile d_part *t=&d_observe[panel.track];int page=panel.page[panel.group];
 for(int k=0;k<4;k++)c[k]=pc(-1,0,0,"",0,0,0);
 if(d_sample_pause==2)return;
 if((panel.group==4&&page==1)||(panel.group==12&&page==2)){c[0]=pc(DC_SAMPLE_LIMIT,0,0,"SAMPLES",d_sample_limit,0,12);c[1]=pc(DC_OSC_LIMIT,0,0,"OSC",d_osc_limit,0,33);return;}
 if(panel.group==13||(panel.group==2&&page<2&&panel.delay_page[page])){int bus=page;
  c[0]=pc(DC_DELAY_CFG,0,bus,"ENGINE",d_delay_cfg[bus][0],0,1);
  c[1]=pc(DC_DELAY_CFG,1,bus,"RATE",d_delay_shift(bus),0,d_delay_cfg[bus][0]?5:2);
  c[2]=pc(DC_DELAY_CFG,2,bus,"BITS",d_delay_cfg[bus][2],0,3);if(d_delay_cfg[bus][0])c[3]=pc(DC_DELAY_CFG,3,bus,"DEGRADE",d_delay_cfg[bus][3],0,127);return;
 }
 if(panel.group==12){if(page==1){c[0]=pc(DC_MOD_RATE,0,0,"INTERVAL",d_mod_index,0,3);return;}static const char *const names[4]={"OSC/V","MATRIX","DELAYS","REVERB"};for(int k=0;k<4;k++)c[k]=pc(DC_CPU,k,0,names[k],d_cpu[k],0,k?1:4);return;}
 if(panel.group==10){c[0]=pc(DC_USB_MODE,0,0,"USB MODE",d_usb_mode,0,1);for(int j=0;j<3;j++)c[j+1]=pc(DC_USB_GROUP,j,0,j==0?"MAIN":j==1?"TRACKS":"DRUMS",!!(d_usb_groups&(1u<<j)),0,1);return;}
 if(panel.group==11){d_pad_mix *m=&d_pad[panel.drum_pad];if(panel.shift){c[0]=pc(DC_DRUM_MIX,1,0,"PAN",m->pan,0,127);c[1]=pc(DC_DRUM_MIX,5,0,"MUTE",m->mute,0,1);}else{c[0]=pc(DC_DRUM_MIX,0,0,"VOLUME",m->level,0,127);for(int j=0;j<3;j++)c[j+1]=pc(DC_DRUM_MIX,2+j,0,j==0?"SEND 1":j==1?"SEND 2":"SEND 3",m->send[j],0,128);}return;}
 if(panel.group==9){panel_seq_controls(c);return;}
 if(panel.group==8){c[0]=pc(DC_VOICE,0,0,"MODE",t->voice_mode,0,2);return;}
 if(panel.group==6){if(panel.preset_mode==2)c[0]=pc(-1,0,0,"SLOT",preset_selected+1,1,8);return;}
 if(panel.group==7){int src=panel.matrix_source;c[0]=pc(-2,0,0,"SOURCE",d_source_index(src),0,4);c[1]=pc(DC_ROUTE,src,0,"TARGET",d_route_target(t,src),0,D_TARGETS-1);c[2]=pc(DC_ROUTE,src,1,"AMOUNT",d_route_amount(t,src),src<3?0:-127,127);if(page==1)c[3]=pc(-4,0,0,"VIEW",panel.matrix_bank,0,4);return;}
 if(panel.group==0){
  if(page==0){volatile d_osc_config *o=&t->cfg[panel.osc];c[0]=pc(DC_OSC,panel.osc,0,"WAVE",o->wave,0,3);c[1]=pc(DC_OSC,panel.osc,1,"TUNE",o->tune,-24,24);c[2]=pc(DC_OSC,panel.osc,2,"CENTS",o->cents,-50,50);c[3]=pc(DC_OSC,panel.osc,3,"LEVEL",o->level,0,127);}
  else{int ids[4]={P_MODE,P_CUT,P_RES,panel.shift?P_KEYTRACK:P_ENV_AMT};
   for(int k=0;k<4;k++)c[k]=pc(DC_PARAM,ids[k],0,p_name[ids[k]],t->p[ids[k]],0,p_max[ids[k]]);if(!panel.shift)c[3]=t->env_target[8]==0?pc(DC_ROUTE,11,1,"ENV AMT",t->env_amount[8],-127,127):pc(-1,0,0,"",0,0,0);}

 }else if(panel.group==1){
  int src=page==0?8:9;volatile uint8_t *e=src==8?t->fe:t->pe;static const char *const names[4]={"ATTACK","DECAY","SUSTAIN","RELEASE"};
  for(int k=0;k<4;k++)c[k]=pc(DC_ENV,k,src,names[k],e[k],0,127);
 }else if(panel.group==2){
  if(page<2){int ids[4];if(panel.shift){ids[0]=D_MODE;ids[1]=D_TONE;ids[2]=D_DRIVE;ids[3]=D_MOD;}else if(panel.alt){ids[0]=D_WIDTH;ids[1]=D_TO_REV;ids[2]=D_ROUTE;ids[3]=D_SYNC;}else{ids[0]=D_TIME_MS;ids[1]=D_FEEDBACK;ids[2]=D_SYNC;ids[3]=D_ROUTE;}
   for(int k=0;k<4;k++)c[k]=pc(DC_DELAY,ids[k],page,d_name[ids[k]],ids[k]==D_TIME_MS?dc(dd[page][ids[k]],1,d_delay_max_ms(page)):dd[page][ids[k]],ids[k]==D_TIME_MS?1:0,ids[k]==D_TIME_MS?d_delay_max_ms(page):d_max[ids[k]]);
   int code=dd[page][D_SYNC],sync=d_delay_sync_active(code);
   if(!panel.shift&&!panel.alt){if(sync)c[0]=pc(-8,D_SYNC,page,"TIME BT",sync,1,8);c[2]=pc(-7,D_SYNC,page,"TIMING",!!sync,0,1);}
   else if(!panel.shift&&panel.alt)c[3]=pc(-8,D_SYNC,page,"BEAT",d_delay_sync_choice(code),1,8);
  }else if(page==2){int ids[4]={V_DECAY,V_DAMPING,V_DIFFUSION,V_MOD};if(panel.shift){ids[0]=V_LOW_CUT;ids[1]=V_PREDELAY;ids[2]=-1;ids[3]=-1;}
   for(int k=0;k<4;k++)if(ids[k]>=0)c[k]=pc(DC_REVERB,ids[k],0,ids[k]==V_DECAY?"TIME":v_name[ids[k]],dv[ids[k]],0,v_max[ids[k]]);}
  if(page==3){int ids[4]={G_DLY_A_RET,G_DLY_B_RET,G_REV_RET,G_MASTER};for(int k=0;k<4;k++)c[k]=pc(DC_GLOBAL,ids[k],0,g_name[ids[k]],dg[ids[k]],0,g_max[ids[k]]);}
 }else if(panel.group==3){
  int ids[4]={P_LEVEL,P_DLY_A,P_DLY_B,P_REVERB};static const char *const labels[4]={"VOLUME","SEND 1","SEND 2","SEND 3"};
  for(int k=0;k<4;k++)c[k]=pc(DC_PARAM,ids[k],0,labels[k],t->p[ids[k]],0,127);
 }else if(panel.group==5){
  static const char *const labels[4]={"RATE","AMOUNT","SHAPE","TARGET"};
  for(int k=0;k<4;k++){int id=d_lfo_ids[page][k];c[k]=pc(DC_PARAM,id,0,labels[k],k==3?d_route_target(t,page):t->p[id],0,p_max[id]);}
  int code=t->p[d_lfo_ids[page][0]],sync=!!(code&D_LFO_SYNC);c[0]=panel.shift?pc(-6,d_lfo_ids[page][0],0,"MODE",sync,0,1):pc(-5,d_lfo_ids[page][0],sync,"RATE",sync?(code>>7)&15:code&127,0,sync?15:127);
 }else if(page==2){int ids[3]={P_PAN,P_DRIVE,P_MUTE};for(int k=0;k<3;k++)c[k]=pc(DC_PARAM,ids[k],0,p_name[ids[k]],t->p[ids[k]],0,p_max[ids[k]]);
 }else if(page==0){c[0]=pc(DC_PARAM,P_CHANNEL,0,"CHANNEL",t->p[P_CHANNEL],1,16);c[1]=pc(DC_GLOBAL,G_TEMPO,0,"INT BPM",dg[G_TEMPO],30,240);c[2]=pc(DC_CLOCK,0,0,"CLK IN",d_clock_mode,0,2);c[3]=pc(DC_CLOCK,1,0,"CLK OUT",d_clock_output,0,1);}
}
/* A single main-menu layer resolves labels, LEDs and physical dispatch. */
static int panel_main_group(int i){
 if(i==0)return panel.shift?3:2;
 if(i==1)return 0;
 if(i==2)return 1;
 if(i==3)return 5;
 if(i==4)return panel.shift?6:4;
 return -1;
}
static int panel_main_page(int i){(void)i;return -1;}
static int panel_can_back(void){return panel.group>=10||panel.group==8||panel.group==3||panel.group==7||(panel.group==6&&!preset_busy&&preset_io<0);}
static void panel_panic(void){memset(panel.key_held,0,sizeof panel.key_held);memset(panel.shortcut_held,0,sizeof panel.shortcut_held);panel.playing=0;d_send(DC_PANIC,0,0,0,0);}
static int panel_enabled(int i){
 if(d_sample_pause==2)return i==11;
 if(i<0||i>=12)return 0;
 if(i==11)return 1;
 if(panel.group==9){if(i==8)return ds.saving<0&&(panel.shift||!ds.playing);if(i==10)return ds.saving<0;return 1;}
 if(panel.group>=10&&i>=6&&i<=9)return (panel.group==11&&i==7)||(panel.group==12&&i<=8)||(panel.group==13&&i<=7);
 if(panel.group==4&&panel.shift&&i==6)return 0;
 if(panel.group==4&&i==9)return 1;
 if(i==6&&panel.shift&&panel_mod_family())return 1;
 if(i==9&&panel_mod_family())return 1;
 if(panel.group==7&&i>=6&&i<=8)return 1;
 if(panel.group==6&&panel.preset_mode){if(i>=6&&i<=9)return panel.preset_mode==1?i==9:1;return 1;}
 if(i>=6&&i<=9)return panel.group==3||i-6<d_page_count(panel.group);
 return 1;
}
static int panel_active(int i){
 if(panel.group==9){if(i<4)return panel.page[9]==i;if(i==5)return panel.shift;if(i==6)return ds.fill;if(i==7)return ds.trig;if(i==8)return ds.saving>=0;if(i==9)return panel.alt;if(i==10)return ds.playing;if(i==11)return !!panel.down[11];return 0;}
 if(i<5){int g=panel_main_group(i),p=panel_main_page(i);return panel.group==g&&(p<0||panel.page[g]==p);}
 if(i==5)return panel.shift;
 if(i==11)return !!panel.down[11];
 if(panel.group==7&&i>=6&&i<=8){int target=d_route_target(&d_observe[panel.track],panel.matrix_source);if(panel.shift)return target==47+i-6;return i==6?target>=21&&target<31:i==7?target>=31&&target<41:target>=41&&target<47;}
 if(i==9&&panel.group==7)return 1;
 if(i==6&&panel.shift&&panel_mod_family())return panel.group==8;
 if(i>=6&&i<=9)return panel.group==3?i-6==panel.track:panel_enabled(i)&&i-6==panel.page[panel.group];
 if(i==10)return panel.alt;
 return 0;
}
static void panel_labels(const char *names[12]){
 if(d_sample_pause==2){for(int i=0;i<12;i++)names[i]="";names[11]="CANCEL";return;}
 if(panel.group==9){static const char *const seq[12]={"STEP","TRACK","PAT","COND","EXIT","SHIFT","FILL","TRIG","SAVE","ALT","PLAY","PANIC"};for(int i=0;i<12;i++)names[i]=seq[i];names[8]=panel.shift?"CLEAR":"SAVE";names[10]=ds.playing?"STOP":"PLAY";return;}
 names[0]=panel.shift?"MIX":"FX";names[1]="SYN";names[2]="ENV";names[3]="LFO";names[4]=panel.shift?"PRESET":"SET";names[5]="SHIFT";
 static const char *const tracks[4]={"TRK1","TRK2","TRK3","TRK4"};
 for(int i=0;i<4;i++)names[6+i]=panel.group==3?tracks[i]:i<d_page_count(panel.group)?dub_pages[panel.group][i]:"";
 if(panel.shift&&panel_mod_family())names[6]="VOICE";
 if(panel_mod_family())names[9]=panel.shift?"MATRIX":"MOD";
 if(panel.group==8)names[6]="VOICE";
 if(panel.group==7){names[6]=panel.shift?"RET A":"DLY A";names[7]=panel.shift?"RET B":"DLY B";names[8]=panel.shift?"RET RV":"VERB";}
 if(panel.shift&&panel_mod_family())names[6]="VOICE";
 if(panel.group==6&&panel.preset_mode){names[3]="NO";names[4]=panel.preset_mode==1?"LOAD":panel.preset_mode==3?"NEW":panel.confirm?"YES":"SAVE";if(panel.preset_mode==1){names[6]=names[7]=names[8]="";names[9]=preset_listen?"STOP":"LISTEN";}else{names[6]="RANDOM";names[7]="BKSP";names[8]=panel.shift?"WORD>":"SPACE";names[9]="WORD>";}}
 if(panel.alt&&!panel.shift&&panel_can_back())names[4]="BACK";
 if(panel.group==4){names[9]="SEQ";if(panel.shift){names[6]="";names[7]="USB";names[8]="CPU";}}
 if(panel.group==3&&panel.shift)names[9]="DRUM";
 if(panel.group>=10){for(int i=6;i<=9;i++)names[i]="";if(panel.group==11)names[7]="SAVE";if(panel.group==12){names[6]="ISOLATE";names[7]="MODRATE";names[8]="VOICES";names[9]="";}if(panel.group==13){names[6]="DLY A";names[7]="DLY B";}}
 names[10]=panel.shift?(ds.playing?"STOP":"PLAY"):"ALT";names[11]=d_bench_active?"STOP":"PANIC";
}
static void panel_nav(int group,int page){
 if(group<0||group>=D_GROUPS)return;
 if(group==9||panel.group==9)panel.alt=0;
 if(group==8&&panel.group!=8){panel.voice_back_group=panel.group;panel.voice_back_page=panel.page[panel.group];}
 if(group==7&&panel.group!=7){panel.mod_back_group=panel.group;panel.mod_back_page=panel.page[panel.group];panel_target_bank(d_route_target(&d_observe[panel.track],panel.matrix_source));}
 if(panel.group==6&&group!=6&&preset_preview)preset_request(PS_RESTORE,0);
 if(group==6&&panel.group!=6){panel.preset_back_group=panel.group;panel.preset_back_page=panel.page[panel.group];panel.preset_mode=0;}
 if(group==3&&panel.group!=3){panel.back_group=panel.group;panel.back_page=panel.page[panel.group];}
 panel.group=group;panel.page[group]=dc(page,0,d_page_count(group)-1);
}
static void panel_back(void){if(!panel_can_back())return;if(panel.group==12||panel.group==13){panel_nav(4,1);}else if(panel.group==10){panel_nav(4,1);}else if(panel.group==11){panel_nav(3,0);}else if(panel.group==6){if(preset_preview)preset_request(PS_RESTORE,0);if(panel.confirm)panel.confirm=0;else if(panel.preset_mode)panel.preset_mode=0;else panel_nav(panel.preset_back_group,panel.preset_back_page);}else if(panel.group==8)panel_nav(panel.voice_back_group,panel.voice_back_page);else if(panel.group==7)panel_nav(panel.mod_back_group,panel.mod_back_page);else panel_nav(panel.back_group,panel.back_page);}
static void panel_cycle(int group){
 if(group==6){if(preset_preview)preset_request(PS_RESTORE,0);panel_nav(6,0);panel.preset_mode=0;return;}
 int page=panel.group==group?(panel.page[group]+1)%d_page_count(group):panel.page[group];
 panel_nav(group,page);
}
static void panel_audition(int on){
 static const int notes[4]={48,55,58,63}; /* Cm9 voicing */
 for(int k=0;k<4;k++)d_send(DC_NOTE,0,notes[k],on?90:0,3);
 panel.playing=on;
}
static void panel_button(int i,int down,uint32_t now){
 if(i<0||i>=12)return;
 if(d_sample_pause==2){if(!down)panel.down[i]=0;if(!down&&i==5)panel.shift=0;if(!down&&i==10)panel.alt=0;if(down&&i==11)d_sample_abort_requested=1;return;}
 if(i==11){panel.down[i]=down?(now?now:1):0;if(down)panel_panic();return;}
 panel.down[i]=down?(now?now:1):0;
 if(i==5){panel.shift=down;return;}
 if(i==6&&!down&&panel.seq_fill_held){panel.seq_fill_held=0;d_send(DC_SEQ_CTL,DS_SET_FILL,0,0,0);return;}
 if(i==7&&!down&&panel.seq_trig_held){panel.seq_trig_held=0;d_send(DC_SEQ_CTL,DS_SET_TRIG,0,0,0);return;}
 if(i==9&&!down&&panel.seq_alt_held){panel.seq_alt_held=0;panel.alt=0;return;}
 if(i==10){if(!down&&panel.seq_transport_held){panel.seq_transport_held=0;return;}if(panel.group==9){if(down&&ds.saving<0)d_send(DC_SEQ_CTL,DS_TOGGLE,0,0,0);return;}if(down&&panel.shift){panel.seq_transport_held=1;d_send(DC_SEQ_CTL,DS_TOGGLE,0,0,0);return;}if(!down&&panel.seq_transport_held){panel.seq_transport_held=0;return;}panel.alt=down;return;}
 if(i==9&&panel.group==9){panel.alt=down;panel.seq_alt_held=down;return;}
 if(!down||!panel_enabled(i))return;
 if(panel.group==9){if(i<4)panel.page[9]=i;else if(i==4)panel_nav(4,panel.page[4]);else if(i==6){panel.seq_fill_held=1;d_send(DC_SEQ_CTL,DS_SET_FILL,1,0,0);}else if(i==7){panel.seq_trig_held=1;d_send(DC_SEQ_CTL,DS_SET_TRIG,1,0,0);}else if(i==8)d_send(DC_SEQ_CTL,panel.shift?DS_CLEAR:DS_SAVE,0,panel.seq_step,panel.track);else if(i==9)panel.alt=1;return;}
 if(i==9&&!down){panel.alt=0;return;}
 if(panel.group==4&&panel.shift&&i==6)return;
 
 if(panel.group==4&&panel.shift&&i==8){panel_nav(12,0);return;}
 if(panel.group==4&&panel.shift&&i==7){panel_nav(10,0);return;}
 if(panel.group==3&&panel.shift&&i==9){panel.track=3;panel_nav(11,0);return;}
 if(panel.group==11&&i==7){d_send(DC_DRUM_SAVE,0,0,0,0);return;}
 if(panel.group==4&&i==9){panel_nav(9,0);return;}
 if(i==4&&panel.alt&&!panel.shift&&panel_can_back()){panel_back();return;}
 if(panel.group==6){
  if(preset_busy||preset_io>=0)return;
  if(panel.preset_mode&&i==3){if(preset_preview)preset_request(PS_RESTORE,0);if(panel.confirm)panel.confirm=0;else if(panel.preset_mode)panel.preset_mode=0;else panel_nav(panel.preset_back_group,panel.preset_back_page);return;}
  if(!panel.preset_mode&&i>=6&&i<=8){panel_preset_page(i-6);return;}
  if(panel.preset_mode&&i==4){if(panel.preset_mode==1){preset_request(PS_LOAD,preset_selected);panel.preset_mode=0;}else if(panel.preset_mode==3){preset_request(PS_INIT,0);panel.preset_mode=0;}else if(preset_bank[preset_selected].tag==PRESET_TAG&&!panel.confirm)panel.confirm=1;else{preset_request(PS_SAVE,preset_selected);panel.confirm=0;}return;}
  if(panel.preset_mode==1&&i==9){if(preset_listen)preset_request(PS_RESTORE,0);else{preset_listen=1;preset_request(PS_PREVIEW,preset_selected);}return;}
  if(panel.preset_mode>=2&&i>=6&&i<=9){if(i==6){preset_new_name(preset_draft);preset_cursor=0;}else if(i==7)preset_backspace();else if(i==9||panel.shift)preset_next_word();else preset_space();return;}
 }
 if(i==6&&panel.shift&&panel_mod_family()){panel_nav(8,0);return;}
 if(panel.group==7&&i>=6&&i<=8){int target=panel.shift?47+i-6:i==6?22:i==7?32:41;d_send(DC_ROUTE,panel.track,panel.matrix_source,target,0);panel_target_bank(target);return;}
 if(i==9&&panel_mod_family()){panel_nav(7,panel.shift?1:0);return;}

 if(i<5){int g=panel_main_group(i),page=panel_main_page(i);if(page>=0)panel_nav(g,page);else panel_cycle(g);}
 else if(i>=6&&i<=9){if(panel.group==3)panel.track=i-6;else if(panel.group==2&&i<8){int bus=i-6;if(panel.page[2]==bus)panel.delay_page[bus]^=1;panel_nav(2,bus);}else panel_nav(panel.group,i-6);}

}
static void panel_turn(int encoder,int delta){
 if(!delta||d_sample_pause==2)return;delta=dc(delta,-127,127);
 if(panel.group==13||(panel.group==2&&panel.page[2]<2&&panel.delay_page[panel.page[2]])){if(encoder>=2&&encoder<=5){dub_control c[4];panel_controls(c);dub_control a=c[encoder-2];if(a.op>=0)d_send(DC_DELAY_CFG|256,panel.page[panel.group],a.index,delta,0);return;}}
 if(panel.group==12){if(panel.page[12]==2){if(encoder==2)d_send(DC_SAMPLE_LIMIT|256,0,0,delta,0);if(encoder==3)d_send(DC_OSC_LIMIT|256,0,0,delta,0);}else if(panel.page[12]==1){if(encoder==2)d_send(DC_MOD_RATE|256,0,0,delta,0);}else if(encoder>=2&&encoder<=5)d_send(DC_CPU|256,encoder-2,0,delta,0);return;}
 if(panel.group==10){if(encoder==2)d_send(DC_USB_MODE,0,0,delta>0,0);else if(encoder>=3&&encoder<=5)d_send(DC_USB_GROUP,0,encoder-3,delta>0,0);return;}
 if(panel.group==11){if(encoder==0){panel.track=(panel.track+delta%4+4)%4;if(panel.track!=3)panel_nav(3,0);return;}if(encoder==1){panel.drum_pad=(panel.drum_pad+delta%11+11)%11;return;}if(encoder>=2&&encoder<=5){dub_control c[4];panel_controls(c);dub_control a=c[encoder-2];if(a.op>=0)d_send(a.op|256,panel.drum_pad,a.index,delta,0);}return;}
 if(panel.group==9){panel_seq_turn(encoder,delta);return;}
 if(panel.group==6){if(preset_busy||preset_io>=0||panel.confirm)return;if(panel.preset_mode>=2){if(encoder==0)preset_cursor_move(delta);else if(encoder==2&&panel.preset_mode==2)preset_select_slot(delta);else if(encoder==4)preset_replace_word();else if(encoder==5){static const char chars[]=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.";int n=preset_len(preset_draft),at=0;while(chars[at]&&chars[at]!=preset_draft[preset_cursor])at++;if(!chars[at])at=0;int count=sizeof(chars)-1;char ch=chars[(at+delta%count+count)%count];if(preset_cursor<40){preset_draft[preset_cursor]=ch;if(preset_cursor==n)preset_draft[n+1]=0;}}}else if(encoder==0)preset_select_slot(delta);return;}
 if(panel.group==7&&encoder==2){panel.matrix_source=d_mod_sources[(d_source_index(panel.matrix_source)+delta%5+5)%5];panel_target_bank(d_route_target(&d_observe[panel.track],panel.matrix_source));return;}
 if(panel.group==7&&encoder==3){dub_control c[4];panel_controls(c);int target=d_target_step(c[1].value,delta);d_send(DC_ROUTE,panel.track,panel.matrix_source,target,0);panel_target_bank(target);return;}
 if(panel.group==7&&panel.page[7]==1&&encoder==5){panel.matrix_bank=(panel.matrix_bank+delta%5+5)%5;return;}
 if(panel.group==5&&panel.page[5]<3&&encoder==5){int src=panel.page[5];d_send(DC_ROUTE,panel.track,src,d_target_step(d_route_target(&d_observe[panel.track],src),delta),0);return;}
 if(panel.group==5&&panel.page[5]<3&&encoder==2){int id=d_lfo_ids[panel.page[5]][0],code=d_observe[panel.track].p[id];if(panel.shift)code=d_lfo_rate_mode(code,delta>0);else if(code&D_LFO_SYNC)code=(code&~(15<<7))|(dc(((code>>7)&15)+delta,0,15)<<7);else code=(code&~127)|dc((code&127)+delta,0,127);d_send(DC_PARAM,panel.track,id,code,0);return;}
 if(panel.group==2&&panel.page[2]<2&&!panel.shift){int bus=panel.page[2],code=dd[bus][D_SYNC],sync=d_delay_sync_active(code);if(!panel.alt&&encoder==4){d_send(DC_DELAY,bus,D_SYNC,d_delay_time_mode(code,delta>0),0);return;}if((!panel.alt&&encoder==2&&sync)||(panel.alt&&encoder==5)){d_send(DC_DELAY,bus,D_SYNC,dc(d_delay_sync_choice(code)+delta,1,8),0);return;}}
 if(encoder==0&&panel.alt){panel.osc=(panel.osc+delta%8+8)%8;return;}
 if(encoder==0){panel.track=(panel.track+delta%4+4)%4;return;}
 if(encoder==6)return; /* PRESETS reserved: no patch library in this version */
 if(encoder==1){if(panel.group==0&&panel.page[0]==0)panel.osc=(panel.osc+delta%8+8)%8;return;}
 if(encoder<2||encoder>5)return;dub_control c[4];panel_controls(c);dub_control a=c[encoder-2];if(a.op<0)return;
 int step=a.op==DC_DELAY&&a.index==D_TIME_MS?5:1;int val=delta*step;
 if(a.op==DC_OSC)d_send(a.op|256,panel.track,a.index,val,a.source);
 else if(a.op==DC_ENV||a.op==DC_ROUTE)d_send(a.op|256,panel.track,a.index,val,a.source);
 else d_send(a.op|256,a.op==DC_DELAY?a.source:panel.track,a.index,val,0);
}
static void panel_key(int key,int down){
 if(d_sample_pause==2){if(key>=0&&key<27&&!down){panel.key_held[key]=0;panel.shortcut_held[key]=0;}return;}
 if(key<0||key>=27)return;
 if(!down&&panel.shortcut_held[key]){panel.shortcut_held[key]=0;return;}
 if(down&&panel.shortcut_held[key])return;
 if(down&&panel.group==7&&panel.shift&&!panel.key_held[key]){panel.shortcut_held[key]=1;int source=panel_key_source(key);if(source>=0){panel.matrix_source=source;panel_target_bank(d_route_target(&d_observe[panel.track],source));}return;}
 if(down&&panel.group==9){if(panel.key_held[key]||ds.saving>=0)return;int note=dc(53+key+panel.octave*12,0,127);if(panel_seq_drum()){int pad=-1;for(int i=0;i<11;i++)if(d_mod_source_keys[i]==key)pad=i;if(pad<0)return;panel.seq_note=pad;note=d_drum_keys[pad];}panel_seq_edit(DS_KEY,note,0);panel.key_held[key]=1;panel.key_track[key]=panel.track;panel.key_note[key]=note;d_send(DC_NOTE,panel.track,note,100,2);return;}
 if(down){if(panel.key_held[key])return;panel.key_held[key]=1;panel.key_track[key]=panel.track;panel.key_note[key]=dc(53+key+panel.octave*12,0,127);d_send(DC_NOTE,panel.track,panel.key_note[key],100,2);}
 else if(panel.key_held[key]){d_send(DC_NOTE,panel.key_track[key],panel.key_note[key],0,2);panel.key_held[key]=0;}
}
static void panel_control(int op,int a,int b,int value){
 if(op==16)panel_button(a,b,(uint32_t)value);else if(op==17)panel_turn(a,b);else if(op==18)panel_key(a,b);else if(op==19){if(panel.group==11){panel.drum_pad=(panel.drum_pad+a%11+11)%11;}else if(panel.group==10){}else if(panel.group==9){if(panel.shift)panel.seq_note=dc(panel_seq_note()+a,0,panel_seq_drum()?10:3);else if(panel.alt)panel_seq_page(a);else panel_seq_step(a);}else if(panel.group==7&&(panel.alt||panel.shift)){panel_turn(panel.alt?3:2,a);}else if(panel.group==6){if(!preset_busy&&preset_io<0&&!panel.confirm){if(panel.preset_mode>=2)preset_cursor_move(a);else preset_select_slot(a);}}else if(panel.alt)panel.osc=(panel.osc+a%8+8)%8;else if(panel.shift)panel.track=(panel.track+a%4+4)%4;else panel.octave=dc(panel.octave+a,-4,4);}
 else if(op==20){d_send(DC_SEQ_CTL,DS_SET_FILL,0,0,0);d_send(DC_SEQ_CTL,DS_SET_TRIG,0,0,0);panel.seq_fill_held=panel.seq_trig_held=panel.seq_transport_held=panel.seq_alt_held=0;for(int k=0;k<27;k++)panel_key(k,0);panel.shift=panel.alt=0;memset(panel.down,0,sizeof panel.down);}
 else if(op==21)panel_panic();
 else if(op==22)panel_nav(a,b);
 else if(op==24)d_send(DC_SEQ_CTL,a,b,0,0);
}
static void panel_init(void){preset_start();memset(&panel,0,sizeof panel);panel_cpu=panel_cpu_peak=panel_late=0;}
typedef void (*dub_rect)(int,int,int,int,uint32_t);
static dub_rect pr;
static void ptext(int x,int y,const char *s,uint32_t color,int scale,int max){
 for(int n=0;s[n]&&n<max;n++){uint8_t ch=s[n];if(ch<32||ch>126)ch='?';for(int i=0;i<5;i++)for(int j=0;j<7;j++)if(panel_font[ch-32][i]&(1u<<j))pr(x+(n*6+i)*scale,y+j*scale,scale,scale,color);}
}
static char *pn(char *out,int val){if(val<0){*out++='-';val=-val;}char b[12];int n=0;do{b[n++]='0'+val%10;val/=10;}while(val);while(n)*out++=b[--n];*out=0;return out;}
static char *ps(char *out,const char *s){while(*s)*out++=*s++;*out=0;return out;}
static void pvalue(dub_control c,char out[24]){
 const char *word=0;int n=c.value;
 if(c.op==-20){static const char *const notes[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};pn(ps(out,notes[n%12]),n/12-1);return;}
 if(c.op==-22){ps(out,d_drum_name(n));return;}
 if(c.op==-24){pn(out,n<=100?n:n-100);int len=0;while(out[len])len++;ps(out+len,n<=100?"%":"X");return;}
 if(c.op==-25){ps(out,n?"ON":"OFF");return;}
 if(c.op==-26){ps(out,n==3?"1/8T":"1/16");return;}
 if(c.op==-27){ps(out,n?"DRUM":"MEL");return;}
 if(c.op==-28||c.op==-29){if(n==16)ps(out,"STOP");else{out[0]=n<8?'A':'B';out[1]=(char)('1'+n%8);out[2]=0;}return;}
 if(c.op==-30){ps(out,ds_condition_names[n]);return;}
 if(c.op==DC_OSC_LIMIT||c.op==DC_SAMPLE_LIMIT){if(!n)ps(out,"OFF");else if(n==(c.op==DC_OSC_LIMIT?33:12))ps(out,"INF");else pn(out,n);return;}
 if(c.op==DC_MOD_RATE){pn(out,d_mod_intervals[dc(n,0,3)]);return;}
 if(c.op==DC_DELAY_CFG){if(c.index==0)ps(out,n?"LOFI":"FILT");else if(c.index==1){static const char *const rate[6]={"22K","11K","5K5","2K8","1K4","0K7"};ps(out,rate[n]);}else if(c.index==2)pn(out,16-n*4);else if(!n)ps(out,"OFF");else pn(out,n);return;}
 if(c.op==DC_CPU){if(c.index)ps(out,n?"ON":"OFF");else if(!n)ps(out,"OFF");else pn(out,d_cpu_oscs[n]);return;}
 if(c.op==DC_USB_GROUP){ps(out,n?"ON":"OFF");return;}
 if(c.op==DC_USB_MODE){ps(out,n?"17CH":"STEREO");return;}
 if(c.op==DC_DRUM_MIX&&c.index>=2&&c.index<=4){if(!n)ps(out,"TRK");else pn(out,n-1);return;}
 if(c.op==DC_DRUM_MIX&&c.index==5){ps(out,n?"ON":"OFF");return;}
 if(c.op==DC_VOICE){static const char *const modes[3]={"MONO","DUO","PARA"};ps(out,modes[dc(n,0,2)]);return;}
 if(c.op==-7)word=n?"SYNC":"MS";
 if(c.op==-8){char *at=ps(out,d_delay_sync_names[n]);if(d_delay_sync_ms(n,d_clock_tempo())>D_DELAY_MAX_MS)ps(at,"*");return;}
 if(c.op==-6)word=n?"SYNC":"FREE";
 if(c.op==-5){if(c.source)word=d_lfo_sync_names[n];else{int hz=d_lfo_millihz[n&127];char *at=pn(out,hz/1000);*at++='.';*at++=(char)('0'+hz/100%10);*at++=(char)('0'+hz/10%10);ps(at,"HZ");return;}}
 if(c.op==DC_REVERB&&c.index==V_DECAY){if(n==100)ps(out,"FOREVER");else{int tenth=(d_reverb_time_ms[n]+50)/100;char *at=pn(out,tenth/10);*at++='.';*at++=(char)('0'+tenth%10);ps(at,"S");}return;}
 if(c.op==DC_CLOCK){static const char *const modes[]={"INT","EXT","AUTO"};word=c.index?(n?"ON":"OFF"):modes[n];}
 if(c.op==-4)word=d_matrix_banks[n];
 if(c.op==-2)word=d_source_names[d_mod_sources[n]];
 if(c.op==DC_ROUTE&&!c.source)word=d_target_names[n];
 if(c.op==DC_OSC&&c.source==0){static const char *const w[]={"SAW","SQUARE","TRI","SINE"};word=w[n];}
 if(c.op==DC_PARAM){if(c.index==P_MODE){static const char *const w[]={"LP12","BP","HP12","NOTCH"};word=w[n];}else if(c.index==P_CUT){pn(out,d_hz[n]);return;}else if(c.index==P_ENV_AMT)n=(n-64)*2;else if(c.index==P_MUTE)word=n?"ON":"OFF";else if(c.index==P_LFO_SHAPE){static const char *const w[]={"SINE","TRI","SAW"};word=w[n];}}
 if(c.op==DC_DELAY){if(c.index==D_MODE)word=n?"DUB":"CLEAN";else if(c.index==D_ROUTE){static const char *const w[]={"MONO","STEREO","P-PONG"};word=w[n];}else if(c.index==D_SYNC){word=d_delay_sync_names[d_delay_sync_active(n)];}}
 if(c.op==DC_PARAM){
  for(int i=0;i<3;i++){
   if(c.index==d_lfo_ids[i][0]){if(n&D_LFO_SYNC){ps(out,d_lfo_sync_names[(n>>7)&15]);return;}int hz=d_lfo_millihz[n&127];char *at=pn(out,hz/1000);*at++='.';*at++=(char)('0'+hz/100%10);*at++=(char)('0'+hz/10%10);ps(at,"HZ");return;}
   if(c.index==d_lfo_ids[i][2]){static const char *const shapes[]={"SINE","TRI","SAW"};word=shapes[n];}
   if(c.index==d_lfo_ids[i][3]){word=d_target_names[n];}
  }
 }
 if(c.op==DC_ENV&&c.index!=2)n=d_times[n];
 if(word)ps(out,word);else pn(out,n);
}
#include "seq_render.h"
static void panel_presets_render(void){
 char b[64];int mode=panel.preset_mode;const char *name=mode>=2?preset_draft:mode==1&&preset_bank[preset_selected].tag==PRESET_TAG?preset_bank[preset_selected].name:preset_name;
 ptext(6,38,mode>=2?"PATCH NAME / SELECT CURSOR":mode==1?"BROWSE / TEMPORARY AUDITION":"CURRENT PATCH",UC_MUTED,1,37);
 for(int i=0;i<40;i++){int x=7+(i%20)*11,y=53+(i/20)*13;char c[2]={name[i],0};if(!name[i]){if(mode>=2&&i==preset_cursor){pr(x-1,y-2,8,11,UC_AMBER);}break;}int chosen=mode>=2&&i==preset_cursor;if(chosen)pr(x-1,y-2,8,11,UC_AMBER);ptext(x,y,c,chosen?UC_BG:UC_TEXT,1,1);}
 if(mode>=2){if(preset_cursor==40)pr(228,66,2,10,UC_AMBER);if(mode==3)ps(b,"INIT WORKING PATCH");else pn(ps(b,"SAVE SLOT "),preset_selected+1);ptext(7,84,b,UC_CYAN,1,36);ptext(7,106,"K1 SLOT   K3 RANDOM WORD",UC_MUTED,1,36);ptext(7,120,"SELECT CURSOR / K4 CHARACTER",UC_MUTED,1,36);ptext(7,139,"MAX 40 / GENERATED MAX 13",UC_MUTED,1,36);if(panel.confirm)ptext(7,159,"REPLACE THIS SLOT? NO / YES",UC_AMBER,1,36);}
 else{for(int i=0;i<8;i++){int y=84+i*10;if(i==preset_selected)pr(5,y-1,230,9,UC_TILE);pn(b,i+1);ptext(8,y,b,i==preset_selected?UC_AMBER:UC_MUTED,1,1);ptext(23,y,preset_bank[i].tag==PRESET_TAG?preset_bank[i].name:"-- EMPTY --",i==preset_selected?UC_CYAN:UC_MUTED,1,34);}}
 const char *status=preset_error==3?"SAVE FAILED / RAM SLOT RETAINED":preset_error==2?"NAME EMPTY OR ALREADY USED":preset_error==1?"EMPTY OR INVALID SLOT":preset_io>=0?"SAVING...":preset_busy?"APPLYING...":preset_listen?"LISTENING / LOAD COMMITS":preset_io_result==1?"SAVED":"ALT+EDIT BACK / REC PANIC";
 ptext(6,176,status,preset_error?UC_AMBER:UC_MUTED,1,38);
}
static int panel_edit_source(void){if(panel.group==1)return panel.page[1]==0?11:12;if(panel.group==5&&panel.page[5]<3)return panel.page[5];return -1;}
static int panel_edit_target(void){int src=panel_edit_source();if(src>=0)return d_route_amount(&d_observe[panel.track],src)?d_route_target(&d_observe[panel.track],src):20;if(panel.group==0)return panel.page[0]==1?0:12+panel.osc;return 20;}
static void panel_mod_list_render(void){volatile d_part *t=&d_observe[panel.track];char b[12];
 ptext(5,87,"SOURCE",UC_MUTED,1,6);ptext(57,87,"DESTINATION",UC_MUTED,1,12);ptext(208,87,"AMT",UC_MUTED,1,3);
 for(int row=0;row<5;row++){int src=d_mod_sources[row],y=102+row*15,dest=d_route_target(t,src),amount=d_route_amount(t,src),selected=src==panel.matrix_source;if(selected)pr(4,y-1,232,12,UC_TILE);uint32_t color=selected?UC_AMBER:amount&&dest!=20?UC_TEXT:UC_MUTED;ptext(5,y,d_source_names[src],color,1,6);ptext(57,y,d_target_full_name(dest),color,1,23);pn(b,amount);ptext(208,y,b,color,1,4);}
}
static void panel_matrix_render(void){
 volatile d_part *t=&d_observe[panel.track];char b[12];int selected=panel.matrix_source,dest=d_route_target(t,selected),bank=panel.matrix_bank;
 static const int starts[5]={0,21,31,41,47},counts[5]={12,10,10,6,4};int count=counts[bank],start=starts[bank],width=180/count;
 static const char *const dl[10]={"MD","MS","SY","FB","TN","DR","MO","RT","WD","RV"},*const vl[6]={"DC","DM","DF","MO","LC","PD"},*const rl[4]={"A","B","REV","MSTR"};
 for(int d=0;d<count;d++){int x=43+d*width;if(!bank){ptext(x+1,84,d<4?(d==0?"F":d==1?"P":d==2?"A":"N"):"P",UC_CYAN,1,1);if(d>=4){pn(b,d-3);ptext(x+1,91,b,UC_MUTED,1,1);}}else ptext(x+2,89,bank<3?dl[d]:bank==3?vl[d]:rl[d],UC_CYAN,1,4);}
 for(int row=0;row<5;row++){int src=d_mod_sources[row],y=102+row*15;if(src==selected)pr(4,y,232,12,UC_TILE);ptext(5,y+2,d_source_names[src],src==selected?UC_AMBER:UC_MUTED,1,6);int target=d_route_target(t,src),amount=d_route_amount(t,src);
  for(int d=0;d<count;d++){int actual=bank?start+d:d<4?d:d+8,x=43+d*width;pr(x+3,y+5,1,1,UC_DIM);if(target==actual&&amount)pr(x+2,y+4,4,3,amount<0?UC_AMBER:UC_CYAN);if(src==selected&&dest==actual){pr(x,y,width-1,1,UC_AMBER);pr(x,y+11,width-1,1,UC_AMBER);pr(x,y,1,12,UC_AMBER);pr(x+width-2,y,1,12,UC_AMBER);}}
 }
}
static void panel_render(dub_rect rect,int device){
 pr=rect;char buf[64],*s;pr(0,0,240,240,UC_BG);if(panel.group==9){buf[0]=ds.pattern<8?'A':'B';buf[1]=(char)('1'+ds.pattern%8);buf[2]=0;s=ps(buf+2,ds.playing?" RUN S":" STOP S");pn(s,ds.play[panel.track]+1);ptext(5,5,buf,UC_TEXT,1,16);}else ptext(5,5,preset_name,UC_TEXT,1,13);
 pn(ps(buf,panel.octave<0?"OCT":"OCT+"),panel.octave);ptext(103,5,buf,UC_MUTED,1,8);
 pr(167,2,35,12,dub_accents[panel.track]);pn(ps(buf,"TRK"),panel.track+1);ptext(171,5,buf,UC_BG,1,4);pn(ps(buf,"CH"),d_observe[panel.track].p[P_CHANNEL]);ptext(210,5,buf,UC_MUTED,1,5);pr(4,17,232,1,UC_DIM);
 int page=panel.page[panel.group];s=ps(buf,dub_groups[panel.group]);s=ps(s," / ");s=ps(s,dub_pages[panel.group][page]);if(panel.group==0&&page==0){s=ps(s,"  OSC");pn(s,panel.osc+1);}else if(panel.group==0||panel.group==1||panel.group==5)ps(s," TRACK");if(panel.group==7&&(panel.shift||panel.alt))ps(buf,panel.alt?"MOD / ALT OCT DESTINATION":"MOD / SHIFT OCT SOURCE");ptext(5,23,buf,UC_CYAN,1,32);
 s=pn(buf,panel.group==2&&page<2?panel.delay_page[page]+1:page+1);s=ps(s,"/");pn(s,panel.group==2&&page<2?2:d_page_count(panel.group));ptext(215,23,buf,UC_MUTED,1,4);
 if(panel.group==6)panel_presets_render();else{
 dub_control c[4];panel_controls(c);for(int k=0;k<4;k++){
  int x=4+k*59;if(c[k].op==-1)continue;pr(x,36,55,47,UC_TILE);pr(x,36,2,47,dub_accents[k]);ptext(x+5,41,c[k].name,UC_MUTED,1,8);pvalue(c[k],buf);int len=0;while(buf[len])len++;ptext(x+5,54,buf,UC_TEXT,len<=4?2:1,8);
  int bar=(c[k].value-c[k].min)*45/(c[k].max>c[k].min?c[k].max-c[k].min:1);pr(x+5,76,45,2,UC_DIM);pr(x+5,76,bar,2,dub_accents[k]);
 }
 if(panel.group==13||(panel.group==2&&page<2&&panel.delay_page[page])){
  ptext(6,98,"PCM16 / GLOBAL / SESSION",UC_MUTED,1,38);ps(pn(ps(buf,"MAX TIME "),d_delay_max_ms(page)/1000)," SECONDS");ptext(6,117,buf,UC_CYAN,2,19);
  ptext(6,145,d_delay_cfg[page][0]?"LOWER RATE = LONGER DELAY":"FILTERED: 1 / 2 / 4 SECONDS",UC_MUTED,1,38);
  ptext(6,160,"DEGRADE PROCESSES EACH REPEAT",UC_MUTED,1,38);ptext(6,174,"CHANGE CLEARS THIS DELAY TAIL",UC_AMBER,1,38);
 }else if((panel.group==4&&page==1)||(panel.group==12&&page==2)){
  ptext(6,96,"GLOBAL SOURCE LIMITS",UC_MUTED,1,38);
  pn(ps(buf,"SAMPLES "),(int)d_sample_active_count());ptext(6,113,buf,UC_CYAN,1,38);
  pn(ps(buf,"OSC "),(int)d_osc_active_count());ptext(6,128,buf,UC_CYAN,1,38);
  ptext(6,146,"RELEASED / OLDEST REPLACED",UC_MUTED,1,38);
  ptext(6,161,"OFF = SILENT / INF = NO CAP",UC_MUTED,1,38);
  ptext(6,174,"GLOBAL / SESSION ONLY",UC_AMBER,1,38);
 }else if(panel.group==12&&panel.page[12]==1){
  static const char *const rates[4]={"~1378 HZ","~2756 HZ","~5513 HZ","11025 HZ"};
  ptext(6,96,"SAMPLES BETWEEN MOD UPDATES",UC_MUTED,1,38);
  ptext(6,116,rates[d_mod_index],UC_CYAN,2,24);
  ptext(6,144,"LOWER INTERVAL = MORE CPU",UC_AMBER,1,38);
  ptext(6,160,"DEFAULT 32 / SESSION ONLY",UC_MUTED,1,38);
 }
 else if(panel.group==12&&panel.page[12]==0){ptext(6,96,"GLOBAL / SESSION ONLY",UC_AMBER,1,38);ptext(6,112,"OSC OFF: SKIP SYNTH DSP",UC_MUTED,1,38);ptext(6,124,"OSC/V: LIMIT PER VOICE GROUP",UC_MUTED,1,38);ptext(6,136,"MATRIX OFF: FIXED ENV ONLY",UC_MUTED,1,38);ptext(6,148,"FX OFF SKIPS ENGINE + FILTERS",UC_MUTED,1,38);ptext(6,160,"FX SWITCH CLEARS TAILS",UC_MUTED,1,38);ptext(6,173,"COMPARE FRESH CPU WINDOWS",UC_CYAN,1,38);}
 else if(panel.group==10){ptext(6,96,d_usb_mode?"17 CH / 22050 HZ / 16 BIT":"2 CH / 44100 OR 48000 HZ / 16 BIT",UC_TEXT,1,38);ptext(6,112,"1-2 MAIN MIX / 3-6 TRACKS DRY",UC_MUTED,1,38);ptext(6,124,"6: SYN4 OR DRUM MIX / 7-17 DRUMS",UC_MUTED,1,38);ptext(6,145,"MODE CHANGE RECONNECTS USB",UC_AMBER,1,38);ptext(6,159,"STOP RECORDING BEFORE CHANGING",UC_MUTED,1,38);ptext(6,173,"SYNTH / MONITOR STAYS 44100 HZ",UC_MUTED,1,38);}
 else if(panel.group==11){for(int i=0;i<11;i++){int x=6+i*21;int h=d_pad[i].level*40/127;pr(x,106,14,40,UC_TILE);pr(x,146-h,14,h,d_pad[i].mute?UC_DIM:UC_CYAN);pn(buf,i+1);ptext(x,93,buf,i==panel.drum_pad?UC_AMBER:UC_MUTED,1,2);if(d_drum_voice[i].active)pr(x,149,14,3,UC_TEXT);if(i==panel.drum_pad){pr(x-1,103,16,1,UC_AMBER);pr(x-1,153,16,1,UC_AMBER);}}ptext(6,160,d_drum_name(panel.drum_pad),UC_AMBER,1,18);ptext(6,173,d_mix_io?"SAVING":d_mix_result<0?"SAVE FAILED":d_mix_result>0?"SAVED":"ALGO/OCT PAD / SHIFT PAN + MUTE",UC_MUTED,1,38);}
 else if(panel.group==9){panel_seq_render();}else if(panel.group==7){if(page==1)panel_matrix_render();else panel_mod_list_render();}else if(panel.group==0||panel.group==1||panel.group==5||panel.group==8){
 /* Static per-note oscillator LEVEL -> shared filter -> shared track AMP. */
 volatile d_part *t=&d_observe[panel.track];int column=0,edit_src=panel_edit_source(),edit_dest=panel_edit_target(),selected_slot=panel.osc/(8/d_voice_count(t));
 ptext(6,87,panel.group==8?(t->voice_mode==0?"1 VOICE / 8 OSC":t->voice_mode==1?"2 VOICES / 4 OSC":"4 VOICES / 2 OSC"):"OSC",UC_MUTED,1,26);pn(ps(buf,"EDIT "),panel.osc+1);ptext(191,87,buf,UC_AMBER,1,7);
 for(int slot=0;slot<d_voice_count(t);slot++){
  int start=column;
  for(int i=0;i<8;i++)if(i/(8/d_voice_count(t))==slot){
   int x=8+column*28;uint32_t color=slot<4?dub_accents[slot]:UC_MUTED;
   pr(x+3,98,18,11,color);
   int on=d_osc_running((d_part *)t,i)&&t->cfg[i].level;
   if(!on)pr(x+4,99,16,9,UC_BG);
   pn(buf,i+1);ptext(x+9,100,buf,on?UC_BG:UC_TEXT,1,1);
   pr(x+16,109,1,10,UC_MUTED); /* sound to individual amp */
   int lfo_pitch=0;for(int l=0;l<3;l++)if(t->p[d_lfo_ids[l][1]]&&d_route_target(t,l)==12+i)lfo_pitch=1;
   if(lfo_pitch){ptext(x,121,"L",UC_CYAN,1,1);pr(x,105,1,27,UC_CYAN);pr(x,105,4,1,UC_CYAN);}
   int editing=panel.group==0&&page==0&&i==panel.osc;pr(x+12,119,10,11,editing?UC_AMBER:UC_TILE);ptext(x+14,121,"L",editing?UC_BG:UC_TEXT,1,1);pn(buf,t->cfg[i].level);ptext(x+3,132,buf,UC_MUTED,1,3);
   pr(x+16,130,1,16,UC_MUTED);
   if(i==panel.osc){pr(x+3,112,18,2,UC_AMBER);pr(x+3,98,18,1,UC_AMBER);}
   if(edit_dest==12+i||edit_dest==1)pr(x+3,109,18,1,UC_CYAN);
   column++;
  }
  if(slot<4&&column>start){int x=6+start*28,w=(column-start)*28-2;
   uint32_t frame=slot==selected_slot?UC_AMBER:UC_DIM;pr(x,95,w,1,frame);pr(x,140,w,1,frame);
   pr(x,95,1,46,frame);pr(x+w-1,95,1,46,frame);
  }
 }
 pr(24,146,197,1,UC_MUTED);pr(119,146,1,5,UC_MUTED);
 for(int layer=0;layer<2;layer++){
  int y=151+layer*22;int target=layer?2:0;
  int dest=layer?2:0;int editing=edit_dest==dest;pr(76,y,87,14,editing?UC_CYAN:UC_TILE);ptext(80,y+4,layer?"TRACK AMP":"TRACK FILTER",editing?UC_BG:UC_TEXT,1,13);
  char *at=buf;int count=0;uint32_t env_color=UC_MUTED;for(int e=8;e<10;e++)if(t->env_target[e]==dest&&t->env_amount[e]){if(e+3==edit_src)env_color=UC_CYAN;if(count<2){if(count)*at++=' ';if(e<8)at=pn(ps(at,"E"),e+1);else at=ps(at,e==8?"FE":"AE");}count++;}if(!count)ps(buf,"ENV --");else if(count>2)ps(at,"+");ptext(7,y+4,buf,env_color,1,8);
  if(count){pr(56,y+7,20,1,env_color);pr(73,y+5,1,5,env_color);}
  for(int l=0;l<3;l++){
   int connected=t->p[d_lfo_ids[l][3]]==target&&t->p[d_lfo_ids[l][1]]>0;
   int x=179+l*20;pn(ps(buf,"L"),l+1);ptext(x,y+4,buf,connected?UC_CYAN:UC_DIM,1,2);
   if(connected){pr(169,y+1,16+l*20,1,UC_CYAN);pr(184+l*20,y+1,1,2,UC_CYAN);pr(169,y+1,1,7,UC_CYAN);pr(163,y+7,6,1,UC_CYAN);}
  }
  if(!layer)pr(119,y+14,1,8,UC_MUTED);
 }
 }else{
  for(int track=0;track<4;track++){
   int x=5+track*59;volatile d_part *t=&d_observe[track];uint32_t color=dub_accents[track];
   pn(ps(buf,"TRK"),track+1);ptext(x+9,92,buf,track==panel.track?color:UC_MUTED,1,4);
   pr(x+8,106,14,43,UC_TILE);int height=t->p[P_LEVEL]*43/127;
   pr(x+8,149-height,14,height,color);int peak=dc((int)t->peak*43/20000,0,43);pr(x+25,149-peak,3,peak,UC_TEXT);
   int ids[3]={P_DLY_A,P_DLY_B,P_REVERB};for(int k=0;k<3;k++){int h=t->p[ids[k]]*43/127;pr(x+32+k*6,106,4,43,UC_TILE);pr(x+32+k*6,149-h,4,h,color);}
   if(track==panel.track)pr(x+4,153,49,2,color);
  }
 }
 if((panel.group==2&&page<2&&panel.delay_page[page])||(panel.group==4&&page==1)||panel.group>=10||panel.group==0||panel.group==1||panel.group==5||panel.group==7||panel.group==8||panel.group==9){}
 else if(panel.group==2&&page<2&&d_delay_sync_active(dd[page][D_SYNC])&&!panel.shift&&!panel.alt){int ms=d_delay_sync_ms(d_delay_sync_active(dd[page][D_SYNC]),d_clock_tempo());if(ms>d_delay_max_ms(page)){ps(pn(ps(buf,"SYNC CLAMPED / MAX "),d_delay_max_ms(page)/1000),"S");}else pn(ps(buf,"SYNC TIME MS: "),ms);ptext(6,159,buf,ms>d_delay_max_ms(page)?UC_AMBER:UC_MUTED,1,38);ptext(6,173,"K1 BEAT / K3 MS OR SYNC",UC_MUTED,1,38);}
 else if(panel.group==2&&page<2&&!panel.shift&&!panel.alt){char *end=ps(buf,d_delay_cfg[page][0]?"LOFI / ":"FILTERED / ");ps(pn(ps(end,"MAX "),d_delay_max_ms(page)/1000),"S");ptext(6,159,buf,UC_MUTED,1,38);ptext(6,173,"PRESS DLY AGAIN: ENGINE",UC_MUTED,1,38);}
 else if(panel.group==2&&page==2&&!panel.shift){int held=d_fxdv[V_DECAY]==100;ptext(6,159,held?"FOREVER / REVERB INPUT ON":"NOMINAL TIME / DAMPING SHAPES TAIL",held?UC_AMBER:UC_MUTED,1,38);ptext(6,173,held?"LOWER TIME FOR DECAY / REC PANIC":"FOREVER: SUSTAIN + NEW SOUND",UC_MUTED,1,38);}
 else if(panel.group==4&&page==0){const char *source=d_clock_mode==D_CLK_INT?"CLOCK INT":d_clock_external()?(d_clock_source==0?"CLOCK EXT / USB":"CLOCK EXT / TRS"):d_clock_mode==D_CLK_EXT?"CLOCK EXT / WAIT":"CLOCK AUTO / INTERNAL";ptext(6,159,source,UC_CYAN,1,36);ptext(6,173,d_clock_output?"CLOCK OUT ON / USB 24 PPQN":"CLOCK OUT OFF",UC_MUTED,1,36);}
 
 else{
  const char *scope=panel.group==3?"MIX: VOL / SEND1 / SEND2 / SEND3":panel.group==5?"TRACK LFO / INDEPENDENT PHASE":panel.group==2||(panel.group==3&&page==2)?"GLOBAL FX / FOUR PART SENDS":"PART SOUND / SHARED FILTER";
  const char *hint=panel.group==3?"HOME/SAVE/ARP/SEQ: TRK / REC PANIC":panel.shift?(panel.group==2?"SHIFT: FX TONE / DRIVE / MOD":panel.group==0&&page==1?"SHIFT: K4 KEY TRACKING":"SHIFT MAIN MENU: FX -> MIX"):panel.alt?(panel.group==2?"ALT: WIDTH / SEND / ROUTING":"ALT+SELECT/OCT: OSC"):"ALT+SELECT/OCT: OSC / SELECT: TRK";
  ptext(6,159,scope,UC_MUTED,1,38);ptext(6,173,hint,panel.shift?UC_AMBER:UC_MUTED,1,38);
 }
 }
 if(d_bench_active){pr(4,170,232,14,UC_BG);ptext(6,174,"BENCHMARK / REC: STOP",UC_AMBER,1,38);}
 if(d_sample_pause==2){pr(4,35,232,149,UC_BG);ptext(8,67,"SAMPLE TRANSFER",UC_CYAN,2,20);ptext(8,99,"AUDIO PAUSED",UC_MUTED,1,30);ps(pn(ps(buf,"PROGRESS "),(int)d_sample_progress),"%");ptext(8,123,buf,UC_TEXT,1,30);ptext(8,155,"REC = CANCEL",UC_AMBER,1,30);}
 const char *names[12];panel_labels(names);for(int i=0;i<12;i++){
  int x=4+(i%6)*39,y=i<6?190:204;int active=panel_active(i);
  uint32_t color=i==5||i==10?UC_AMBER:!panel_enabled(i)?0x526077:UC_CYAN;
  pr(x,y,37,11,active?color:UC_TILE);ptext(x+2,y+2,names[i],active?UC_BG:color,1,6);
 }
 s=pn(buf,d_clock_tempo());s=ps(s," BPM  ");s=pn(s,d_active);s=ps(s,"/32 OSC ");if(ds.playing){s=ps(s,"SEQ ");*s++=ds.pattern<8?'A':'B';*s++=(char)('1'+ds.pattern%8);*s=0;}else s=ps(s,"STOP");ps(s,d_core_state==3?" /2C":" /1C");ptext(5,220,buf,UC_TEXT,1,38);
 if(device){s=ps(buf,"CPU ");s=pn(s,panel_cpu);s=ps(s,"% PK ");s=pn(s,panel_cpu_peak);s=ps(s,"% LATE ");pn(s,panel_late);}else ps(buf,"HOST / FM-1 CPU NOT MEASURED");
 ptext(5,232,buf,UC_MUTED,1,34);
 ptext(217,232,"R39",UC_CYAN,1,3);
}
