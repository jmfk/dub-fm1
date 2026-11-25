/* Register-level USB driver test; no OS USB/MIDI or flash operations. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "diagnostics.h"
#include "runtime_control.h"
#include "profile.h"
#define DUB_RUNTIME_CONTROL 1
#define RING_PUBLISH() __asm__ volatile("":::"memory")
#define FELUCCA_CDC 0
#define FELUCCA_UAC 1
#define FELUCCA_OTA 1
#define HALF_FRAMES 128u
#define FM1_USB_HAL "usb_mock.h"
#define DUB_DIAGNOSTICS 1
static int requests,last_command=-1;static uint32_t now;
static void dub_diag_receive(const uint8_t *p,uint32_t n){last_command=d_profile_request(p,n);if(last_command<0)last_command=d_runtime_request(p,n);if(last_command<0)last_command=d_perf_request(p,n);requests++;}
static uint32_t ota_now_ms(void){return now;}
static void ota_idle(void);
#include "../../.build-env/dub-r39-usb-under-test.h"
static uint8_t profile_bytes[1024];static unsigned profile_n;static int draining;
static void profile_drain(void){while(so_r!=so_w){uint32_t pkt=sx_out_q[so_r++%SXQ],cin=pkt&15,count=cin==4||cin==7?3:cin-4;for(unsigned j=0;j<count;j++)profile_bytes[profile_n++]=(uint8_t)(pkt>>(8*(j+1)));}}
static void ota_idle(void){now++;if(draining)profile_drain();}
static void receive(const uint8_t *p,unsigned n){for(unsigned at=0;at<n;){unsigned count=n-at<3?n-at:3,cin=count==3&&at+3<n?4:4+count;uint32_t packet=cin;for(unsigned j=0;j<count;j++)packet|=(uint32_t)p[at+j]<<(8*(j+1));midi_in_event(packet);at+=count;}}
int main(void){uint8_t request[8]={0xf0,0x7d,0x44,0x55,0x42,1,0,0xf7};receive(request,8);assert(requests==1&&last_command==0&&!usb.uboot_req&&!usb.ota_req&&!sx_ready);request[6]=1;receive(request,8);assert(requests==2&&last_command==1&&!sx_ready);for(int i=0;i<8;i++){if(i==4)sysex_byte(0xf8);sysex_byte(request[i]);}assert(requests==3&&!sx_ready);assert(mi_w==mi_r);midi_in_event(9u|(0x90u<<8)|(60u<<16)|(100u<<24));assert(mi_w-mi_r==1&&((midi_in_q[mi_r%MQ]>>8)&255)==0x90);mi_r=mi_w;uint8_t loader[6]={0xf0,0x22,0x24,0x35,0x7d,0xf7};receive(loader,6);assert(usb.uboot_req&&requests==3);usb.uboot_req=0;
 uint32_t fields[D_PERF_FIELDS]={19,24000000,44100};uint8_t reply[D_PERF_REPLY_SIZE],decoded[D_PERF_REPLY_SIZE];unsigned length=d_perf_encode(reply,0,fields);usb.config=1;assert(!ota_wire_send(reply,length));unsigned at=0;while(so_r!=so_w){uint32_t pkt=sx_out_q[so_r++%SXQ],cin=pkt&15,count=cin==4||cin==7?3:cin-4;for(unsigned j=0;j<count;j++)decoded[at++]=(uint8_t)(pkt>>(8*(j+1)));}assert(at==length&&!memcmp(reply,decoded,length));uint8_t runtime[]={0xf0,0x7d,0x44,0x55,0x42,1,2,3,0,0xf7};receive(runtime,10);assert(last_command==2&&requests==4&&!usb.uboot_req&&!usb.ota_req&&!sx_ready);runtime[7]=4;receive(runtime,10);assert(requests==4&&!usb.uboot_req&&!usb.ota_req);sx_ready=0;uint8_t prof_get[]={240,125,68,85,66,1,6,77,247},prof_set[]={240,125,68,85,66,1,7,78,1,247};receive(prof_get,9);assert(requests==5&&last_command==6&&!usb.uboot_req&&!usb.ota_req&&!sx_ready);receive(prof_set,10);assert(requests==6&&last_command==7&&!sx_ready);
 d_prof_stats profile={0};profile.generation=1;uint8_t prof_wire[D_PROF_REPLY];assert(d_prof_encode(prof_wire,6,77,&profile,0)==sizeof prof_wire);draining=1;assert(!ota_wire_send(prof_wire,sizeof prof_wire));profile_drain();assert(profile_n==sizeof prof_wire&&!memcmp(profile_bytes,prof_wire,sizeof prof_wire));
 puts("PASS profile6/7 intercepted without updater dispatch,591-byte USB-MIDI response across64-packet ring with actual TX backpressure");puts("PASS actual USB driver: diagnostic requests intercepted without updater dispatch, reset/realtime framing, original loader parser preserved in mock,185-byte reply USB-MIDI packetization exact; no devices opened");}
