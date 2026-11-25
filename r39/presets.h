/* SPDX-License-Identifier: GPL-3.0-only; bounded patch configs, no DSP state. */
#pragma once
#include "name_words.h"
#define PRESET_SLOTS 8
#define PRESET_TAG 0x32505544u
#define PRESET_LEGACY_TAG 0x31505544u
struct preset_part {int16_t p[P_COUNT];uint8_t env_target[10];int16_t env_amount[10];d_osc_config cfg[8];uint8_t fe[4],pe[4];};
typedef struct {uint32_t tag;char name[41];uint8_t pad[3];struct preset_part part[4];int16_t global[G_COUNT],delay[2][D_COUNT],reverb[V_COUNT];} preset_patch;
struct preset_legacy_part {int16_t p[P_COUNT];d_osc_config cfg[8];uint8_t fe[4],pe[4];};
typedef struct {uint32_t tag;char name[41];uint8_t pad[3];struct preset_legacy_part part[4];int16_t global[G_COUNT],delay[2][D_COUNT],reverb[V_COUNT];} preset_legacy_patch;
static preset_legacy_patch preset_legacy_work;
static void preset_migrate(preset_patch *p,const preset_legacy_patch *old){memset(p,0,sizeof *p);p->tag=old->tag==PRESET_LEGACY_TAG?PRESET_TAG:0;memcpy(p->name,old->name,41);for(int t=0;t<4;t++){struct preset_part *d=&p->part[t];const struct preset_legacy_part *s=&old->part[t];memcpy(d->p,s->p,sizeof s->p);memcpy(d->cfg,s->cfg,sizeof s->cfg);memcpy(d->fe,s->fe,4);memcpy(d->pe,s->pe,4);for(int e=0;e<10;e++){d->env_target[e]=e<8?20:e==8?0:2;d->env_amount[e]=e==8?dc((s->p[P_ENV_AMT]-64)*2,-127,127):127;}}memcpy(p->global,old->global,sizeof p->global);memcpy(p->delay,old->delay,sizeof p->delay);memcpy(p->reverb,old->reverb,sizeof p->reverb);}
_Static_assert(sizeof(preset_patch)<=4084,"Patch must fit one blob sector");
static preset_patch preset_bank[PRESET_SLOTS],preset_backup,preset_work;
static char preset_name[41],preset_draft[41];
static int preset_current=-1,preset_selected,preset_cursor,preset_preview,preset_listen,preset_error;
static volatile int preset_busy,preset_io=-1,preset_io_result;
static uint32_t preset_revision,preset_rng=0x67319a21u;
enum {PS_PREVIEW,PS_RESTORE,PS_LOAD,PS_SAVE,PS_INIT};
static int preset_len(const char *s){int n=0;while(n<40&&s[n])n++;return n;}
static int preset_nonempty(const char *s){for(int i=0;i<40&&s[i];i++)if(s[i]!=' ')return 1;return 0;}
static void preset_copy_name(char *d,const char *s){int n=0;for(;n<40&&s[n];n++)d[n]=s[n];d[n]=0;}
static uint32_t preset_random(void){preset_rng^=preset_rng<<13;preset_rng^=preset_rng>>17;preset_rng^=preset_rng<<5;return preset_rng;}
static void preset_word(int index,char out[7]){int n=0;for(int j=0;j<6;j++){int code=0;for(int b=0;b<5;b++){int pos=index*30+j*5+b;code=code*2+((preset_words[pos/8]>>(7-pos%8))&1);}if(!code)break;out[n++]='A'+code-1;}out[n]=0;}
/* Names are ASCII by construction; compare uppercase words with collapsed spaces. */
static int preset_key_char(const char *s,int *at){while(s[*at]==' ')(*at)++;int c=s[*at];if(!c)return 0;(*at)++;if(c>='a'&&c<='z')c-=32;return c;}
static int preset_same(const char *a,const char *b){int i=0,j=0;for(;;){int x=preset_key_char(a,&i),y=preset_key_char(b,&j);if(x!=y)return 0;if(!x)return 1;/* preserve word boundaries */if((a[i]==' ')!=(b[j]==' ')){int ii=i,jj=j;while(a[ii]==' ')ii++;while(b[jj]==' ')jj++;if(a[ii]&&b[jj])return 0;}}}
static int preset_taken(const char *name,int except){for(int i=0;i<PRESET_SLOTS;i++)if(i!=except&&preset_bank[i].tag==PRESET_TAG&&preset_same(name,preset_bank[i].name))return 1;return 0;}
static void preset_new_name(char out[41]){char a[7],b[7],previous[41];preset_copy_name(previous,preset_name);int start=preset_random()%640;for(int k=0;k<409600;k++){int x=(start+k/640)%640,y=(preset_random()+k)%640;if(x==y)continue;preset_word(x,a);preset_word(y,b);int n=0;for(int j=0;a[j];j++)out[n++]=a[j];out[n++]=' ';for(int j=0;b[j];j++)out[n++]=b[j];out[n]=0;if(!preset_taken(out,-1)&&!preset_same(out,previous))return;}preset_copy_name(out,"NEW PATCH");}
static int preset_valid(const preset_patch *p){
 for(int t=0;t<4;t++)if(((p->pad[0]>>(t*2))&3)==3)return 0;
 if(p->tag!=PRESET_TAG||!preset_nonempty(p->name)||p->name[40])return 0;
 for(int i=0;i<40&&p->name[i];i++){int c=p->name[i];if(c<32||c>126)return 0;}
 for(int t=0;t<4;t++){const struct preset_part *s=&p->part[t];for(int j=0;j<P_COUNT;j++)if(s->p[j]<(j==P_CHANNEL?1:0)||s->p[j]>p_max[j])return 0;
  for(int o=0;o<8;o++){const d_osc_config *c=&s->cfg[o];if(c->wave>3||c->level>127||c->tune< -24||c->tune>24||c->cents< -50||c->cents>50)return 0;for(int j=0;j<4;j++)if(c->env[j]>127)return 0;}
  for(int e=0;e<10;e++)if(s->env_target[e]>=D_TARGETS||s->env_amount[e]< -127||s->env_amount[e]>127)return 0;
  for(int j=0;j<4;j++)if(s->fe[j]>127||s->pe[j]>127)return 0;
 }
 for(int j=0;j<G_COUNT;j++)if(p->global[j]<(j==G_TEMPO?30:0)||p->global[j]>g_max[j])return 0;
 for(int t=0;t<2;t++)for(int j=0;j<D_COUNT;j++)if(p->delay[t][j]<(j==D_TIME_MS?1:0)||p->delay[t][j]>d_max[j])return 0;
 for(int j=0;j<V_COUNT;j++)if(p->reverb[j]<0||p->reverb[j]>v_max[j])return 0;return 1;
}
static void preset_capture(preset_patch *p,const char *name){memset(p,0,sizeof *p);p->tag=PRESET_TAG;preset_copy_name(p->name,name);for(int t=0;t<4;t++){p->pad[0]|=(uint8_t)(((dp[t].voice_mode+1)%3)<<(t*2));memcpy(p->part[t].p,dp[t].p,sizeof dp[t].p);memcpy(p->part[t].env_target,dp[t].env_target,sizeof dp[t].env_target);memcpy(p->part[t].env_amount,dp[t].env_amount,sizeof dp[t].env_amount);memcpy(p->part[t].cfg,dp[t].cfg,sizeof dp[t].cfg);memcpy(p->part[t].fe,dp[t].fe,4);memcpy(p->part[t].pe,dp[t].pe,4);}memcpy(p->global,dg,sizeof dg);memcpy(p->delay,dd,sizeof dd);memcpy(p->reverb,dv,sizeof dv);}
static void preset_apply(const preset_patch *p){for(int t=0;t<4;t++){d_panic_part(&dp[t]);dp[t].voice_mode=(uint8_t)((((p->pad[0]>>(t*2))&3)+2)%3);memcpy(dp[t].p,p->part[t].p,sizeof dp[t].p);memcpy(dp[t].env_target,p->part[t].env_target,sizeof dp[t].env_target);memcpy(dp[t].env_amount,p->part[t].env_amount,sizeof dp[t].env_amount);memcpy(dp[t].cfg,p->part[t].cfg,sizeof dp[t].cfg);memcpy(dp[t].fe,p->part[t].fe,4);memcpy(dp[t].pe,p->part[t].pe,4);}memcpy(dg,p->global,sizeof dg);memcpy(dd,p->delay,sizeof dd);memcpy(dv,p->reverb,sizeof dv);d_fx_init();}
static void preset_defaults(preset_patch *p){memset(p,0,sizeof *p);p->tag=PRESET_TAG;for(int t=0;t<4;t++){struct preset_part *s=&p->part[t];memcpy(s->p,p_default,sizeof s->p);s->p[P_CHANNEL]=t+1;for(int e=0;e<10;e++){s->env_target[e]=e<8?20:e==8?0:2;s->env_amount[e]=e<8?0:e==8?48:127;}for(int i=0;i<8;i++)s->cfg[i]=(d_osc_config){0,100,{1,34,90,32},0,(int8_t)((i&1)?4:-4)};uint8_t f[4]={1,45,0,30},a[4]={1,25,127,42};memcpy(s->fe,f,4);memcpy(s->pe,a,4);}memcpy(p->global,g_default,sizeof dg);memcpy(p->delay[0],d_default,sizeof dd[0]);memcpy(p->delay[1],d_default,sizeof dd[1]);p->delay[1][D_MODE]=0;p->delay[1][D_TIME_MS]=536;p->delay[1][D_FEEDBACK]=45;p->delay[1][D_TO_REV]=15;memcpy(p->reverb,v_default,sizeof dv);}
static void preset_audition(void){static const int notes[4]={48,55,58,63};for(int i=0;i<4;i++)d_note_on(0,notes[i],90,3);}
static void d_patch_command(int action,int slot){
 preset_error=0;preset_io_result=0;
 if(action==PS_RESTORE){if(preset_preview){preset_apply(&preset_backup);preset_preview=0;}preset_listen=0;}
 else if(action==PS_PREVIEW){if(slot<0||slot>=PRESET_SLOTS||!preset_valid(&preset_bank[slot])){if(preset_preview){preset_apply(&preset_backup);preset_preview=0;}preset_listen=0;preset_error=1;goto done;}if(!preset_preview){preset_capture(&preset_backup,preset_name);preset_preview=1;}preset_apply(&preset_bank[slot]);preset_audition();}
 else if(action==PS_LOAD){if(slot<0||slot>=PRESET_SLOTS||!preset_valid(&preset_bank[slot])){preset_error=1;goto done;}preset_apply(&preset_bank[slot]);preset_copy_name(preset_name,preset_bank[slot].name);preset_current=slot;preset_preview=preset_listen=0;}
 else if(action==PS_SAVE){if(slot<0||slot>=PRESET_SLOTS||!preset_nonempty(preset_draft)||preset_taken(preset_draft,slot)||preset_io>=0){preset_error=2;goto done;}preset_capture(&preset_work,preset_draft);preset_io=slot;preset_io_result=0;}
 else if(action==PS_INIT){if(!preset_nonempty(preset_draft)||preset_taken(preset_draft,-1)){preset_error=2;goto done;}preset_defaults(&preset_work);preset_copy_name(preset_work.name,preset_draft);preset_apply(&preset_work);preset_copy_name(preset_name,preset_draft);preset_current=-1;preset_preview=preset_listen=0;}
 done:preset_busy=0;
}
static void preset_request(int action,int slot){if(preset_busy||preset_io>=0)return;preset_busy=1;if(!d_send(DC_PATCH,action,slot,0,0))preset_busy=0;}
/* Persistence adapters call this after writing the staged snapshot. */
static void preset_saved(int ok){if(preset_io<0)return;int slot=preset_io;if(ok){memcpy(&preset_bank[slot],&preset_work,sizeof preset_work);preset_copy_name(preset_name,preset_work.name);preset_current=slot;preset_revision++;preset_io_result=1;}else{preset_error=3;preset_io_result=-1;}preset_io=-1;}
static void preset_start(void){memset(preset_bank,0,sizeof preset_bank);preset_current=-1;preset_selected=preset_cursor=preset_preview=preset_listen=preset_error=preset_busy=0;preset_io=-1;preset_revision=0;preset_name[0]=0;preset_new_name(preset_name);}
