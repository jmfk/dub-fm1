/* Main-loop-only profile transport. Replies contain frozen/completed blocks. */
#pragma once
static int dub_prof_action=-1,dub_prof_waiting,dub_prof_value,dub_prof_sending;
static uint8_t dub_prof_id,dub_prof_last_id,dub_prof_last_value;
static int dub_prof_last_action=-1;
static uint32_t dub_prof_before;
static int dub_profile_receive(const uint8_t *p,uint32_t n){int cmd=d_profile_request(p,n);if(cmd<0)return 0;
 if(dub_prof_action<0){dub_prof_action=cmd;dub_prof_id=p[7];dub_prof_value=cmd==7?p[8]:0;dub_prof_waiting=0;}
 return 1;
}
static void dub_profile_service(void){int action=dub_prof_action;if(action<0||dub_prof_sending)return;
 if(action==7&&!dub_prof_waiting){
  if(dub_prof_last_action==action&&dub_prof_last_id==dub_prof_id&&dub_prof_last_value==dub_prof_value)dub_prof_waiting=2;
  else{dub_prof_before=d_prof_revision;d_prof_pending=dub_prof_value;dub_prof_waiting=1;return;}
 }
 if(dub_prof_waiting==1&&d_prof_revision==dub_prof_before)return;
 d_prof_stats snapshot;uint32_t enabled;if(!d_prof_copy(&snapshot,&enabled))return;
 uint8_t reply[D_PROF_REPLY];unsigned n=d_prof_encode(reply,action,dub_prof_id,&snapshot,enabled);if(!n)return;dub_prof_sending=1;int failed=ota_wire_send(reply,n);dub_prof_sending=0;if(failed)return;
 dub_prof_last_action=action;dub_prof_last_id=dub_prof_id;dub_prof_last_value=(uint8_t)dub_prof_value;dub_prof_action=-1;dub_prof_waiting=0;
}
