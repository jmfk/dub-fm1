/* Main-loop runtime transport bridge. Host tests provide clock and wire sink. */
#pragma once
static volatile int dub_rt_action=-1,dub_rt_field,dub_rt_value;static int dub_rt_waiting;static uint32_t dub_rt_before,dub_rt_last_ms,dub_rt_queued_ms;static uint8_t dub_rt_id;
static void dub_runtime_service(void){
 int action=dub_rt_action;
 if(action<0&&d_runtime_lease_due(fm1_ms,dub_rt_last_ms)){dub_rt_field=9;dub_rt_value=0;dub_rt_id=0;dub_rt_action=action=5;}
 if(action<0)return;
 if(dub_rt_waiting&&(uint32_t)(fm1_ms-dub_rt_queued_ms)>2000u){dub_rt_action=-1;dub_rt_waiting=0;return;}
 if(action==2){if(!dub_rt_waiting){dub_rt_before=d_cpu_revision;if(!d_send(DC_CPU,dub_rt_field,0,dub_rt_value,0))return;dub_rt_queued_ms=fm1_ms;dub_rt_waiting=1;return;}if(d_cpu_revision==dub_rt_before)return;}
 if(action==4||action==5){if(!dub_rt_waiting){dub_rt_before=d_rt_revision;if(!d_send(DC_RUNTIME,action==4?-1:dub_rt_field,0,dub_rt_value,0))return;dub_rt_queued_ms=fm1_ms;dub_rt_waiting=1;return;}if(d_rt_revision==dub_rt_before)return;}
 uint8_t reply[11+D_RT_FIELDS];unsigned n=action>=4?d_runtime_extended_reply(reply,action,dub_rt_id):d_runtime_reply(reply,action);if(!n)return;if(ota_wire_send(reply,n))return;dub_rt_action=-1;dub_rt_waiting=0;
}
static int dub_runtime_receive(const uint8_t *p,uint32_t n){int runtime=d_runtime_request(p,n);if(runtime<0)return 0;if(dub_rt_action<0){if(runtime==2){dub_rt_field=p[7];dub_rt_value=p[8];}if(runtime>=4){dub_rt_id=p[7];dub_rt_last_ms=fm1_ms;if(runtime==5){dub_rt_field=p[8];dub_rt_value=p[9];}}__asm__ volatile("":::"memory");dub_rt_action=runtime;}return 1;}
