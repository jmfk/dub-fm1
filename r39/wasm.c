/* SPDX-License-Identifier: GPL-3.0-only; exact same engine and panel as firmware. */
#include "engine.h"
#include "panel.h"
static int32_t buffer[512];static uint32_t pixels[240*240];
static void browser_rect(int x,int y,int w,int h,uint32_t rgb){int xx=dc(x+w,0,240),yy=dc(y+h,0,240);x=dc(x,0,240);y=dc(y,0,240);uint32_t color=0xff000000u|((rgb&255u)<<16)|(rgb&0xff00u)|((rgb>>16)&255u);for(int j=y;j<yy;j++)for(int i=x;i<xx;i++)pixels[j*240+i]=color;}
void init(void){d_init();panel_init();}
void render(void){d_render(buffer,256);}
int32_t *output(void){return buffer;}
uint32_t *draw(void){panel_render(browser_rect,0);return pixels;}
int command(int op,int ch,int a,int b,int v,int source){if(op>=16){panel_control(op,a,b,v);return 1;}if(op==1)return d_send(DC_MIDI,a,b,v,source);return 0;}
const char *label(int index){const char *n[12];panel_labels(n);return index>=0&&index<12?n[index]:"";}
const char *knoblabel(int k){dub_control c[4];panel_controls(c);return k>=0&&k<4?c[k].name:"";}
int get(int kind,int ch,int p){
 if(kind==0&&ch>=0&&ch<4&&p>=0&&p<P_COUNT)return dp[ch].p[p];
 if(kind==1&&p>=0&&p<G_COUNT)return dg[p];if(kind==2)return panel.osc;
 if(kind==3)return ds.playing;if(kind==4)return d_frame;if(kind==5)return d_active;if(kind==6)return d_clips;
 if(kind==10)return panel.track;if(kind==11)return panel.group;if(kind==12)return panel.page[panel.group];if(kind==13)return panel.page[panel.group];if(kind==14)return panel.shift;if(kind==15)return panel.alt;
 if(kind==75&&p>=0&&p<4)return d_cpu[p];
 if(kind==78)return p?d_mod_span:d_mod_index;
 if(kind==80&&ch>=0&&ch<2&&p>=0&&p<4)return d_delay_cfg[ch][p];if(kind==81&&ch>=0&&ch<2)return d_delay_max_ms(ch);
 if(kind==79)return p==0?d_osc_limit:p==1?d_sample_limit:p==2?(int)d_osc_active_count():(int)d_sample_active_count();
 if(kind==74)return d_usb_groups;
 if(kind==71)return d_usb_mode;if(kind==72)return panel.drum_pad;if(kind==73&&ch>=0&&ch<11&&p>=0&&p<6)return ((uint8_t *)&d_pad[ch])[p];
 if(kind==45){int n=0;for(int i=0;i<11;i++)n+=!!d_drum_voice[i].active;return n;}
 if(kind==46)return ds.pattern;if(kind==47)return ds.playing;if(kind==48)return ds.fill;if(kind==49)return ds.trig;if(kind==50)return panel.seq_step;if(kind==51)return panel_seq_note();if(kind==55)return ds_pattern_at(ds.pattern)->next;if(kind==56)return ds_pattern_at(ds.pattern)->repeats;if(kind==57)return d_seq_is_drum();if(kind==66)return ds.tick;if(kind==68)return ds.error;if(kind==69)return ds.saving;if(kind==70)return ds.revision;
 if(ch>=0&&ch<4){ds_track *t=&ds_pattern_at(ds.pattern)->track[ch];if(kind==52)return t->length;if(kind==53)return t->grid;if(kind==58)return ds.play[ch];if(kind==60&&p>=0&&p<512)return t->step[p/4].note[p%4];if(p>=0&&p<128){ds_step *step=&t->step[p];if(kind==61)return step->velocity;if(kind==62)return step->length;if(kind==63)return step->condition;if(kind==64)return step->value;}}
 if(kind==65&&p>=0&&p<128)return ds_pattern_at(ds.pattern)->hits[p];
 if(kind==44){int total=0;for(int j=0;j<8;j++)for(int k=0;k<RV_N;k++)total+=da(d_rv_buf[j][k]);return total;}
 if(kind==42&&p>=0&&p<V_COUNT)return dv[p];
 if(kind==43)return d_rv_frozen;
 if(kind==41&&ch>=0&&ch<4)return dp[ch].voice_mode;
 if(kind==40&&ch>=0&&ch<2&&p>=0&&p<D_COUNT)return dd[ch][p];
 if(kind==35)return d_clock_mode;if(kind==36)return d_clock_output;if(kind==37)return d_clock_tempo();if(kind==38)return d_clock_generation;if(kind==39)return d_clock_external();
 if(kind==32)return panel.matrix_source;if(kind==34)return panel_key_source(p);if(kind==76){for(int i=0;i<11;i++)if(d_mod_source_keys[i]==p)return i;return -1;}
 if(kind==29&&ch>=0&&ch<2&&p>=0&&p<D_COUNT)return d_fxdd[ch][p];if(kind==30&&p>=0&&p<V_COUNT)return d_fxdv[p];if(kind==31&&p>=0&&p<G_COUNT)return d_fxg[p];
 if(kind==27&&ch>=0&&ch<4&&p>=0&&p<13)return d_route_target(&dp[ch],p);if(kind==28&&ch>=0&&ch<4&&p>=0&&p<13)return d_route_amount(&dp[ch],p);
 if(kind==22)return preset_current;if(kind==23)return preset_selected;if(kind==24)return panel.preset_mode;if(kind==25)return preset_error;if(kind==26)return preset_preview;
 if(kind==20)return panel.octave;
 if(kind==21&&ch>=0&&ch<4&&p>=0&&p<12)return dp[ch].p[d_lfo_ids[p/4][p%4]];
 if(kind==18&&p>=0&&p<12)return panel_active(p);
 if(kind==19&&p>=0&&p<12)return panel_enabled(p);
 if(kind==16&&ch>=0&&ch<4)return dp[ch].filter_triggers;if(kind==17)return d_overflows;
 return 0;
}
void *memset(void *p,int c,unsigned long n){unsigned char *d=p;while(n--)*d++=(unsigned char)c;return p;}
void *memcpy(void *d,const void *s,unsigned long n){unsigned char *a=d;const unsigned char *b=s;while(n--)*a++=*b++;return d;}

static preset_patch preset_import_buffer[8];
void *preset_data(void){return preset_bank;}
void *preset_input(void){return preset_import_buffer;}
int preset_size(void){return sizeof preset_bank;}
int preset_record_size(void){return sizeof(preset_patch);}
int preset_pending(void){return preset_io;}
void *preset_staged(void){return &preset_work;}
void preset_ack(int ok){preset_saved(ok);}
int preset_import(int length){if(length&&length!=sizeof preset_import_buffer){if(length!=8*sizeof(preset_legacy_patch))return 0;/* Validate all before replacing the live bank. Expand backwards in-place. */
 for(int i=7;i>=0;i--){memcpy(&preset_legacy_work,(uint8_t *)preset_import_buffer+i*sizeof(preset_legacy_patch),sizeof preset_legacy_work);if(preset_legacy_work.tag&&preset_legacy_work.tag!=PRESET_LEGACY_TAG)return 0;preset_migrate(&preset_import_buffer[i],&preset_legacy_work);}}for(int i=0;i<8;i++)if(preset_import_buffer[i].tag){if(!preset_valid(&preset_import_buffer[i]))return 0;for(int j=0;j<i;j++)if(preset_import_buffer[j].tag&&preset_same(preset_import_buffer[i].name,preset_import_buffer[j].name))return 0;}memcpy(preset_bank,preset_import_buffer,sizeof preset_bank);if(preset_taken(preset_name,-1))preset_new_name(preset_name);return 1;}

int clock_next(void){return d_clock_take();}
uint32_t clock_stamp(void){return d_clock_stamp;}

void *seq_data(void){return &ds_work;}
void *seq_input(void){return &ds_work;}
int seq_size(void){return sizeof ds_work;}
int seq_pending(void){return ds_io;}
void seq_ack(int ok){ds_ack=ok?1:-1;d_seq_service();}
int seq_import(int bank){return ds_import(bank);}

void *mix_data(void){return &d_mix_work;}
int mix_size(void){return sizeof d_mix_work;}
int mix_pending(void){return d_mix_io;}
int mix_import(void){return d_mix_import();}
void mix_ack(int ok){d_mix_result=ok?1:-1;d_mix_io=0;}
