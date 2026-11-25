/* Audio-owner-only runtime snapshots and temporary benchmark configuration. */
#pragma once
static uint8_t d_bench_saved[18],d_bench_pattern;
static uint32_t d_bench_hash_more(uint32_t h,const void *data,unsigned n){const uint8_t *p=data;while(n--)h=(h^*p++)*16777619u;return h;}
static uint32_t d_bench_sound_hash(void){uint32_t h=2166136261u;
 for(int p=0;p<4;p++){d_part *t=&dp[p];h=d_bench_hash_more(h,t->p,sizeof t->p);h=d_bench_hash_more(h,t->cfg,sizeof t->cfg);h=d_bench_hash_more(h,t->fe,4);h=d_bench_hash_more(h,t->pe,4);h=d_bench_hash_more(h,t->env_target,sizeof t->env_target);h=d_bench_hash_more(h,t->env_amount,sizeof t->env_amount);h=d_bench_hash_more(h,&t->voice_mode,sizeof t->voice_mode);}
 h=d_bench_hash_more(h,dg,sizeof dg);h=d_bench_hash_more(h,dd,sizeof dd);h=d_bench_hash_more(h,dv,sizeof dv);return d_bench_hash_more(h,d_pad,sizeof d_pad);
}
static void d_rt_put32(unsigned at,uint32_t value){for(unsigned i=0;i<5;i++){d_rt_snapshot[at+i]=(uint8_t)(value&127u);value>>=7;}}
static void d_bench_snapshot(int status){memset(d_rt_snapshot,0,sizeof d_rt_snapshot);d_rt_snapshot[0]=(uint8_t)status;
 for(int i=0;i<4;i++)d_rt_snapshot[1+i]=d_cpu[i];d_rt_snapshot[5]=d_sample_limit;d_rt_snapshot[6]=d_osc_limit;d_rt_snapshot[7]=d_mod_index;d_rt_snapshot[8]=!!d_seq_is_drum();d_rt_snapshot[9]=d_bench_active;d_rt_snapshot[10]=ds.playing;d_rt_snapshot[11]=ds.pattern;
 for(int i=0;i<4;i++)d_rt_snapshot[12+i]=(uint8_t)dp[i].p[P_CHANNEL];d_rt_snapshot[16]=(uint8_t)d_osc_active_count();d_rt_snapshot[17]=(uint8_t)d_sample_active_count();d_rt_snapshot[18]=d_usb_mode;d_rt_snapshot[19]=d_usb_groups;
 d_rt_put32(20,d_bench_sound_hash());d_rt_put32(25,d_sample_generation);
 /* Only reads the immutable active payload; query overhead is outside measured windows. */
 d_rt_put32(30,d_sample_payload?d_sample_crc(d_sample_payload,d_sample_payload_bytes):0);
 d_rt_put32(35,d_rt_revision);d_rt_put32(40,d_frame);d_rt_snapshot[45]=(uint8_t)(dg[G_TEMPO]&127);d_rt_snapshot[46]=(uint8_t)(dg[G_TEMPO]>>7);d_rt_snapshot[47]=d_clock_mode;d_rt_snapshot[48]=d_clock_output;for(int b=0;b<2;b++)for(int f=0;f<4;f++)d_rt_snapshot[49+b*4+f]=d_delay_cfg[b][f];
 for(int p=0;p<4;p++){unsigned active=0;for(int i=0;i<8;i++)active+=d_budget_osc_active(p,i);d_rt_snapshot[57+p]=(uint8_t)active;d_rt_put32(61+p*5,d_sound_frames[p]);d_rt_snapshot[86+p]=dp[p].voice_mode;}
 d_rt_put32(81,d_common_frames);d_rt_put32(90,d_lim.preclip_frames);d_rt_put32(95,(uint32_t)d_lim.gain);d_rt_snapshot[100]=(uint8_t)d_core_state;d_rt_put32(101,d_core_jobs);d_rt_put32(106,d_core_wait_ticks);d_rt_put32(111,d_core_job_ticks);d_rt_put32(116,d_core_peak_ticks);d_rt_put32(121,d_core_timeouts);
}
static void d_bench_end(void){d_prof_disable();if(!d_bench_active)return;d_full_panic();ds_pattern_at(d_bench_pattern)->drums=d_bench_saved[7];for(int i=0;i<4;i++)d_cpu_set(i,d_bench_saved[i]);d_sample_limit_set(d_bench_saved[4]);d_osc_limit_set(d_bench_saved[5]);d_mod_index=d_bench_saved[6];d_clock_set(0,d_bench_saved[8]);d_clock_set(1,d_bench_saved[9]);for(int b=0;b<2;b++)for(int f=0;f<4;f++)d_delay_config_set(b,f,d_bench_saved[10+b*4+f]);d_bench_active=0;}
static void d_bench_command(int field,int value){int status=0;
 if(field>=0&&!d_runtime_field_valid((unsigned)field,(unsigned)value))status=2;
 else if(field==9){
  if(!value)d_bench_end();
  else if(!d_bench_active){
   if(ds.playing||ds.saving>=0||d_mix_io||d_sample_pause||preset_preview||preset_busy||preset_io>=0)status=1;
   else{for(int i=0;i<4;i++)d_bench_saved[i]=d_cpu[i];d_bench_saved[4]=d_sample_limit;d_bench_saved[5]=d_osc_limit;d_bench_saved[6]=d_mod_index;d_bench_saved[7]=(uint8_t)d_seq_is_drum();d_bench_saved[8]=(uint8_t)d_clock_mode;d_bench_saved[9]=(uint8_t)d_clock_output;for(int b=0;b<2;b++)for(int f=0;f<4;f++)d_bench_saved[10+b*4+f]=d_delay_cfg[b][f];d_bench_pattern=ds.pattern;d_full_panic();d_clock_set(0,D_CLK_INT);d_clock_set(1,0);d_bench_active=1;}
  }
 }else if(field>=0&&field<=7&&(ds.playing||d_sample_pause))status=1;
 else if(field>=0&&field<=3)d_cpu_set(field,value);
 else if(field==4)d_sample_limit_set(value);
 else if(field==5)d_osc_limit_set(value);
 else if(field==6)d_mod_index=(uint8_t)value;
 else if(field==7){if(ds_pattern_at(ds.pattern)->drums!=value){d_full_panic();ds_pattern_at(ds.pattern)->drums=(uint8_t)value;}}
 else if(field==8){d_full_panic();if(d_bench_active){for(int p=0;p<4;p++){memset(dp[p].lfo,0,sizeof dp[p].lfo);for(int i=0;i<8;i++)dp[p].osc[i].phase=0;}d_mod_left=0;}}
 else if(field>=12&&field<=19)d_delay_config_set((field-12)/4,(field-12)%4,value);
 else if(field==10)d_clock_set(0,value);else if(field==11)d_clock_set(1,value);
 d_rt_revision++;d_bench_snapshot(status);
}
