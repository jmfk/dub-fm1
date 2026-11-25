#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static int32_t pcm[512];static void tick(void){d_render(pcm,256);}static void tap(int i){panel_button(i,1,1);panel_button(i,0,2);tick();}
static void save(int slot,const char *name){preset_selected=slot;preset_copy_name(preset_draft,name);preset_request(PS_SAVE,slot);tick();assert(preset_io==slot);preset_saved(1);assert(preset_current==slot);}
int main(void){
 d_init();panel_init();assert(preset_len(preset_name)<=13&&preset_len(preset_name)>2);
 for(int i=0;i<640;i++){char w[7];preset_word(i,w);int n=preset_len(w);assert(n>0&&n<=6);for(int k=0;k<n;k++)assert(w[k]>='A'&&w[k]<='Z');}
 panel_button(5,1,1);tap(4);panel_button(5,0,2);assert(panel.group==6);tap(7);assert(panel.preset_mode==2);assert(preset_len(preset_draft));
 int original=dp[0].p[P_CUT];save(0,"FIRST PATCH");assert(preset_valid(&preset_bank[0]));
 dp[0].p[P_CUT]=40;dp[1].cfg[3].wave=3;dd[1][D_FEEDBACK]=77;save(1,"SECOND PATCH");dp[0].p[P_CUT]=99;
 preset_listen=1;preset_request(PS_PREVIEW,0);tick();assert(preset_preview&&dp[0].p[P_CUT]==original&&d_gates(&dp[0])==4);assert(preset_same(preset_name,"SECOND PATCH"));
 preset_request(PS_PREVIEW,1);tick();assert(dp[0].p[P_CUT]==40&&dp[1].cfg[3].wave==3&&dd[1][D_FEEDBACK]==77);
 preset_request(PS_RESTORE,0);tick();assert(!preset_preview&&dp[0].p[P_CUT]==99&&!d_gates(&dp[0]));
 preset_listen=1;preset_request(PS_PREVIEW,0);tick();panel_nav(0,0);tick();assert(!preset_preview&&dp[0].p[P_CUT]==99);
 preset_request(PS_LOAD,0);tick();assert(dp[0].p[P_CUT]==original&&preset_current==0&&preset_same(preset_name,"FIRST PATCH"));
 preset_patch before=preset_bank[0];preset_copy_name(preset_draft,"FAILED WRITE");preset_request(PS_SAVE,0);tick();preset_saved(0);assert(preset_error==3);assert(!memcmp(&before,&preset_bank[0],sizeof before));
 preset_copy_name(preset_draft,"   ");preset_request(PS_SAVE,2);tick();assert(preset_error==2&&preset_io<0);
 preset_copy_name(preset_draft," first   patch ");preset_request(PS_SAVE,2);tick();assert(preset_error==2&&preset_io<0);
 preset_copy_name(preset_draft,"CUSTOM SPACE NAME");preset_cursor=8;preset_replace_word();assert(preset_draft[0]=='C'&&preset_draft[1]=='U');assert(preset_len(preset_draft)<=40);
 preset_copy_name(preset_draft,"ONE TWO");preset_cursor=3;preset_space();assert(preset_cursor==4);preset_backspace();assert(preset_same(preset_draft,"ONE TWO"));preset_cursor=7;preset_next_word();assert(preset_cursor==3);
 preset_copy_name(preset_draft,"INIT NAME");preset_request(PS_INIT,0);tick();assert(preset_current==-1&&dp[0].p[P_CUT]==p_default[P_CUT]);assert(!memcmp(&before,&preset_bank[0],sizeof before));
 before.part[0].p[P_CHANNEL]=0;assert(!preset_valid(&before));
 panel_nav(6,0);panel.preset_mode=0;tap(7);preset_selected=0;tap(4);assert(panel.confirm);tap(3);assert(!panel.confirm&&panel.preset_mode==2);panel.alt=1;tap(4);panel.alt=0;assert(!panel.preset_mode);
 puts("PASS names/640 words, physical PRESETS editor, coherent four-track/FX snapshots, non-destructive multi-patch preview/restore/load, failed save retention, uniqueness, word/cursor edits, INIT isolation, overwrite confirmation and validation");
}
