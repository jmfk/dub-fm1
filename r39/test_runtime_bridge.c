#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "panel.h"
static uint32_t fm1_ms;static uint8_t reply[256];static unsigned reply_n;static int wire_busy;
static int ota_wire_send(const uint8_t *p,uint32_t n){if(wire_busy)return 1;memcpy(reply,p,n);reply_n=n;return 0;}
#include "runtime_bridge.h"
static int32_t out[512];
static void run(void){dub_runtime_service();d_render(out,32);dub_runtime_service();}
int main(void){d_init();panel_init();uint8_t set[]={240,125,68,85,66,1,5,10,9,1,247},get[]={240,125,68,85,66,1,4,11,247};
 assert(dub_runtime_receive(set,sizeof set));dub_runtime_service();assert(!reply_n&&!d_bench_active);d_render(out,32);assert(d_bench_active);dub_runtime_service();assert(reply_n==137&&reply[7]==10&&reply[18]);
 reply_n=0;assert(dub_runtime_receive(get,sizeof get));wire_busy=1;run();assert(!reply_n&&dub_rt_action==4);wire_busy=0;dub_runtime_service();assert(reply_n==137&&reply[7]==11);
 fm1_ms=15001;reply_n=0;run();assert(!d_bench_active&&reply[7]==0&&d_sample_limit==4&&d_osc_limit==8);
 assert(dub_runtime_receive(get,sizeof get));dub_runtime_service();d_qr=d_qw;fm1_ms+=2001;dub_runtime_service();assert(dub_rt_action<0&&!dub_rt_waiting);assert(dub_runtime_receive(get,sizeof get));run();assert(reply[7]==11);
 puts("PASS real runtime bridge defers ACK until audio apply, backpressure/request IDs, lease auto-restore and dropped-queue timeout/retry");}
