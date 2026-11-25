/* SPDX-License-Identifier: GPL-3.0-only; dedicated sequencer controls. */
#pragma once
static const char *const ds_condition_names[7]={"ALWAYS","EVERY","CHANCE","TRIG","!TRIG","FILL","!FILL"};
static int panel_seq_drum(void){return panel.track==3&&d_seq_is_drum();}
static int panel_seq_note(void){return dc(panel.seq_note,0,panel_seq_drum()?10:3);}
static int panel_seq_packed(int field){return(panel.track<<12)|(panel.seq_step<<4)|field;}
static void panel_seq_edit(int field,int value,int note){d_send(DC_SEQ,ds.pattern,panel_seq_packed(field),value,note);}
static void panel_seq_step(int delta){int length=ds_pattern_at(ds.pattern)->track[panel.track].length;panel.seq_step=(panel.seq_step+delta%length+length)%length;}
static void panel_seq_page(int delta){int length=ds_pattern_at(ds.pattern)->track[panel.track].length,pages=(length+15)/16,page=(panel.seq_step/16+delta%pages+pages)%pages;panel.seq_step=dc(page*16+panel.seq_step%16,0,length-1);}
static void panel_seq_controls(dub_control c[4]){ds_pattern *p=ds_pattern_at(ds.pattern);ds_track *t=&p->track[panel.track];panel.seq_step=dc(panel.seq_step,0,t->length-1);ds_step *s=&t->step[panel.seq_step];int n=panel_seq_note(),page=panel.page[9],drum=panel_seq_drum();
 if(page==0){c[0]=drum?pc(-22,0,0,"PAD",n,0,10):panel.shift?pc(-21,0,0,"NOTE SLOT",n+1,1,4):pc(-20,0,!(s->note[n]&128),"NOTE",s->note[n]&127,0,127);c[1]=pc(-23,0,0,"VELOCITY",s->velocity,1,127);c[2]=pc(-24,0,0,"LENGTH",s->length,1,228);c[3]=pc(-25,0,0,"ON",drum?!!(p->hits[panel.seq_step]&(1u<<n)):!(s->note[n]&128),0,1);}
 else if(page==1){c[0]=pc(-23,0,0,"STEPS",t->length,1,128);c[1]=pc(-26,0,0,"GRID",t->grid,3,4);if(panel.track==3)c[2]=pc(-27,0,0,"TYPE",p->drums,0,1);c[3]=pc(-25,0,0,"MUTE",t->mute,0,1);}
 else if(page==2){c[0]=pc(-28,0,0,"PATTERN",ds.pattern,0,15);c[1]=pc(-23,0,0,"REPEATS",p->repeats,1,16);c[2]=pc(-29,0,0,"NEXT",p->next,0,16);c[3]=pc(DC_GLOBAL,G_TEMPO,0,"INT BPM",dg[G_TEMPO],30,240);}
 else{c[0]=pc(-30,0,0,"COND",s->condition,0,6);if(s->condition==DS_CHANCE)c[1]=pc(-23,0,0,"CHANCE %",s->value,0,100);else if(s->condition==DS_EVERY){c[1]=pc(-23,0,0,"EVERY",(s->value&15)+1,1,16);c[2]=pc(-23,0,0,"PHASE",(s->value>>4)+1,1,(s->value&15)+1);}}
}
static void panel_seq_turn(int encoder,int delta){ds_pattern *p=ds_pattern_at(ds.pattern);ds_track *t=&p->track[panel.track];ds_step *s=&t->step[dc(panel.seq_step,0,t->length-1)];int n=panel_seq_note(),page=panel.page[9];if(encoder==0){panel.track=(panel.track+delta%4+4)%4;panel.seq_step=dc(panel.seq_step,0,p->track[panel.track].length-1);return;}if(encoder==1){if(panel.alt)panel_seq_page(delta);else panel_seq_step(delta);return;}if(ds.saving>=0||encoder<2||encoder>5)return;
 if(page==0){if(encoder==2){if(panel_seq_drum()||panel.shift)panel.seq_note=dc(n+delta,0,panel_seq_drum()?10:3);else panel_seq_edit(DS_NOTE,(s->note[n]&127)+delta,n);}else if(encoder==3)panel_seq_edit(DS_VEL,s->velocity+delta,0);else if(encoder==4){int length=s->length;if(panel.alt)length=delta>0?100+(length>100?length-100:1):(length>100?100:length);else if(length<=100)length=dc(length+delta,1,100);else length=100+dc(length-100+delta,1,128);panel_seq_edit(DS_LENGTH,length,0);}else panel_seq_edit(DS_ON,delta>0,n);}
 else if(page==1){if(encoder==2)panel_seq_edit(DS_TRACK_LENGTH,t->length+delta,0);else if(encoder==3)panel_seq_edit(DS_GRID,t->grid+delta,0);else if(encoder==4&&panel.track==3)panel_seq_edit(DS_DRUM,delta>0,0);else if(encoder==5)panel_seq_edit(DS_MUTE,delta>0,0);}
 else if(page==2){if(encoder==2)d_send(DC_SEQ_CTL,DS_PATTERN,dc(ds.pattern+delta,0,15),0,0);else if(encoder==3)panel_seq_edit(DS_REPEAT,p->repeats+delta,0);else if(encoder==4)panel_seq_edit(DS_NEXT,p->next+delta,0);else d_send(DC_GLOBAL,0,G_TEMPO,dc(dg[G_TEMPO]+delta,30,240),0);}
 else{if(encoder==2)panel_seq_edit(DS_COND,s->condition+delta,0);else if(encoder==3)panel_seq_edit(DS_VALUE,(s->condition==DS_EVERY?(s->value&15)+1:s->value)+delta,0);else if(encoder==4)panel_seq_edit(DS_PHASE,(s->value>>4)+1+delta,0);}
}
