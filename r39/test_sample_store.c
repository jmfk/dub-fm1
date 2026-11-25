/* Device-free NOR/USB-codec/playback gate; no MIDI or physical flash access. */
#include <assert.h>
#include <stdio.h>
#include <setjmp.h>
#include "engine.h"
#include "panel.h"
static uint32_t fm1_ms;
static uint8_t flash[0x100000],baseline[0x100000],captured[160];
static unsigned captured_n,ops;static int cut=-1,streaming,queue_note;static jmp_buf power;
static int d_sample_store_available(void){return 1;}
static int d_sample_audio_streaming(void){return streaming;}
static const uint8_t *d_sample_store_ptr(uint32_t off){assert(off>=D_SAMPLE_BASE&&off<D_SAMPLE_END);return flash+off;}
static int d_sample_store_erase(uint32_t off){assert(!(off&4095u)&&off>=D_SAMPLE_BASE&&off+4096<=D_SAMPLE_END);int fail=cut>=0&&(int)ops==cut;ops++;memset(flash+off,255,fail?2048:4096);if(fail)longjmp(power,1);return 0;}
static int d_sample_store_write(uint32_t off,const uint8_t *p,uint32_t n){assert(off>=D_SAMPLE_BASE&&off<=D_SAMPLE_END&&n<=D_SAMPLE_END-off);int fail=cut>=0&&(int)ops==cut;ops++;unsigned count=fail?n/2:n;for(unsigned i=0;i<count;i++){assert((flash[off+i]&p[i])==p[i]);flash[off+i]&=p[i];}if(fail)longjmp(power,1);return 0;}
static int d_sample_wire_send(const uint8_t *p,uint32_t n){assert(n<=sizeof captured);memcpy(captured,p,n);captured_n=n;return 0;}
#define D_SAMPLE_EXTERNAL_IO 1
#include "sample_transfer.h"
static uint8_t sequence,request[160],result[116];static unsigned request_n,result_n;
static int send_cmd(unsigned cmd,const uint8_t *body,unsigned n){
 uint8_t raw[116];assert(n+4<=sizeof raw);if(n)memcpy(raw,body,n);uint8_t id=(sequence++)&127u;d_sample_wr36(raw+n,d_sample_message_crc((uint8_t)cmd,id,raw,n));memcpy(request,d_sample_prefix,5);request[5]=(uint8_t)cmd;request[6]=id;request_n=7+d_sample_pack(raw,n+4,request+7);captured_n=0;
 assert(d_sample_sysex(request,request_n)==1);
 if(d_sample_transfer.phase==1&&d_sample_pause==1){int32_t silence[2];if(queue_note){queue_note=0;d_send(DC_NOTE,0,60,90,0);}(void)d_sample_audio_gate(silence,1,streaming);}
 for(int tries=0;!captured_n&&tries<30;tries++){fm1_ms++;d_sample_service();}
 assert(captured_n>=15&&captured[0]==240&&captured[captured_n-1]==247&&captured[6]==(cmd|64)&&captured[7]==id);
 int decoded=d_sample_unpack(captured+8,captured_n-9,result,sizeof result);assert(decoded>=5);result_n=(unsigned)decoded-5;
 assert(d_sample_rd32(result+decoded-4)==d_sample_message_crc((uint8_t)(cmd|64),id,result,(unsigned)decoded-4));return result[0];
}
static unsigned make_blob(uint8_t *out,unsigned frames,int loop,const char *name){unsigned n=512+frames*2;memset(out,0,n);memcpy(out,"FSB1",4);d_sample_wr36(out+4,1);d_sample_wr36(out+8,11);d_sample_wr36(out+12,n);memcpy(out+16,name,strlen(name));d_sample_wr36(out+16+28,frames);d_sample_wr36(out+16+32,22050);d_sample_wr36(out+16+36,(unsigned)loop);for(unsigned i=0;i<frames;i++){int16_t x=(int16_t)((int)(i*997u%20000u)-10000);out[512+i*2]=(uint8_t)x;out[513+i*2]=(uint8_t)((uint16_t)x>>8);}assert(d_sample_catalog_valid(out,n));return n;}
static uint32_t begin_blob(const uint8_t *blob,unsigned n){uint8_t body[12];d_sample_wr36(body,d_sample_generation);d_sample_wr36(body+4,n);d_sample_wr36(body+8,d_sample_crc(blob,n));assert(send_cmd(4,body,12)==DSP_OK&&result_n==8&&d_sample_pause==2);return d_sample_rd32(result+1);}
static void upload_blob(const uint8_t *blob,unsigned n){uint32_t token=begin_blob(blob,n);uint8_t body[104];for(unsigned at=0;at<n;){unsigned count=n-at>96?96:n-at;d_sample_wr36(body,token);d_sample_wr36(body+4,at);memcpy(body+8,blob+at,count);assert(send_cmd(5,body,8+count)==DSP_OK);at+=count;assert(result_n==4&&d_sample_rd32(result+1)==at);}d_sample_wr36(body,token);assert(send_cmd(6,body,4)==DSP_OK&&result_n==8&&d_sample_pause==0);}
static void boot(void){d_init();panel_init();d_sample_store_init();captured_n=0;streaming=0;}
int main(int argc,char **argv){
 memset(flash,0xa5,sizeof flash);memset(flash+D_SAMPLE_BASE,255,D_SAMPLE_END-D_SAMPLE_BASE);boot();assert(!d_sample_generation&&!d_sample_payload);
 assert(send_cmd(1,0,0)==DSP_OK&&result_n==36);assert(d_sample_rd32(result+1+16)==D_SAMPLE_PCM_MAX);
 if(argc>1){FILE *f=fopen(argv[1],"wb");assert(f);fwrite(captured,1,captured_n,f);fclose(f);}
 uint8_t body[104];d_sample_wr36(body,0);d_sample_wr36(body+4,0);body[8]=96;assert(send_cmd(3,body,9)==DSP_OK&&result_n==100);assert(!memcmp(result+5,"FSB1",4));
 uint8_t old[1024],next[8192];unsigned old_n=make_blob(old,64,0,"OLD"),next_n=make_blob(next,3400,1,"NEW LOOP");
 upload_blob(old,old_n);upload_blob(old,old_n);assert(d_sample_generation==2&&d_sample_active_bank==1);memcpy(baseline,flash,sizeof flash);
 // Cached/repeated replies do not reprogram data.
 uint32_t token=begin_blob(next,next_n);unsigned before=ops;captured_n=0;assert(d_sample_sysex(request,request_n));d_sample_service();assert(captured_n&&ops==before);
 d_sample_wr36(body,token);d_sample_wr36(body+4,0);memcpy(body+8,next,96);assert(send_cmd(5,body,104)==DSP_OK);before=ops;captured_n=0;assert(d_sample_sysex(request,request_n));d_sample_service();assert(ops==before&&captured_n);
 assert(send_cmd(1,0,0)==DSP_OK);assert(send_cmd(5,body,104)==DSP_OK&&ops==before); // duplicate after cache eviction
 d_sample_wr36(body,token+1);assert(send_cmd(7,body,4)==DSP_TOKEN&&d_sample_transfer.phase==3);d_sample_wr36(body,token);assert(send_cmd(7,body,4)==DSP_OK&&!d_sample_pause&&d_sample_generation==2);
 token=begin_blob(next,next_n);(void)token;fm1_ms+=30001;d_sample_service();assert(!d_sample_pause&&!d_sample_transfer.phase&&d_sample_generation==2);
 token=begin_blob(next,next_n);d_sample_abort_requested=1;d_sample_service();assert(!d_sample_pause&&!d_sample_transfer.phase&&d_sample_generation==2);
 // Stale/active-source/streaming/wrong-token/frame corruption errors.
 d_sample_wr36(body,1);d_sample_wr36(body+4,next_n);d_sample_wr36(body+8,d_sample_crc(next,next_n));assert(send_cmd(4,body,12)==DSP_STALE);
 d_sample_wr36(body,2);d_bench_active=1;assert(send_cmd(4,body,12)==DSP_BUSY);d_bench_active=0;d_note_on(0,48,80,0);assert(send_cmd(4,body,12)==DSP_BUSY);d_full_panic();streaming=1;assert(send_cmd(4,body,12)==DSP_BUSY);streaming=0;queue_note=1;assert(send_cmd(4,body,12)==DSP_BUSY&&d_slot_gates(&dp[0]));d_full_panic();
 d_sample_wr36(body,0);assert(send_cmd(6,body,4)==DSP_TOKEN);
 assert(send_cmd(1,0,0)==DSP_OK);request[request_n-1]^=1;captured_n=0;assert(d_sample_sysex(request,request_n));d_sample_service();assert(captured_n);int dec=d_sample_unpack(captured+8,captured_n-9,result,sizeof result);assert(dec>=5&&result[0]==DSP_BAD_CRC);
 // Full valid upload and replayed commit after another reply.
 upload_blob(next,next_n);if(argc>2){FILE *f=fopen(argv[2],"wb");assert(f);fwrite(next,1,next_n,f);fclose(f);}assert(d_sample_generation==3&&d_sample_views[0].loop&&d_sample_views[0].frames==3400);uint32_t committed=d_sample_transfer.committed_token;assert(send_cmd(1,0,0)==DSP_OK);d_sample_wr36(body,committed);before=ops;assert(send_cmd(6,body,4)==DSP_OK&&d_sample_generation==3&&ops==before);
 // Power loss at EVERY erase/program operation; keep all non-sample bytes intact.
 memcpy(flash,baseline,sizeof flash);boot();ops=0;upload_blob(next,next_n);unsigned operations=ops;boot();assert(d_sample_generation==3&&d_sample_payload_bytes==next_n&&!memcmp(d_sample_payload,next,next_n));
 for(volatile int at=0;at<(int)operations;at++){
  memcpy(flash,baseline,sizeof flash);cut=-1;boot();ops=0;cut=at;
  if(!setjmp(power)){upload_blob(next,next_n);assert(!"power cut was not injected");}
  cut=-1;boot();assert(d_sample_generation==2||d_sample_generation==3);
  const uint8_t *expected=d_sample_generation==2?old:next;unsigned n=d_sample_generation==2?old_n:next_n;assert(d_sample_payload_bytes==n&&!memcmp(d_sample_payload,expected,n));
  assert(!memcmp(flash,baseline,D_SAMPLE_BASE));assert(!memcmp(flash+D_SAMPLE_END,baseline+D_SAMPLE_END,sizeof flash-D_SAMPLE_END));
 }
 // Cache/loop/retrigger ownership and one-shot behavior from actual custom views.
 memcpy(flash,baseline,sizeof flash);boot();upload_blob(next,next_n);ds_pattern_at(0)->drums=1;
 for(unsigned pos=0;pos<6800;pos++){int32_t expected=(int16_t)(next[512+(pos>>1)*2]|(uint16_t)next[513+(pos>>1)*2]<<8);if(pos&1){unsigned b=(pos>>1)+1;if(b==3400)b=0;int32_t y=(int16_t)(next[512+b*2]|(uint16_t)next[513+b*2]<<8);expected=(expected+y)/2;}assert(d_drum_read(0,pos,3400)==expected);}
 d_drum_on(36,100,0);for(int i=0;i<14000;i++)d_drum_tick();assert(d_drum_voice[0].active&&d_drum_voice[0].position<6800);
 d_drum_on(36,100,0);d_drum_off(36,0);assert(!d_drum_voice[0].release);d_drum_off(36,0);assert(d_drum_voice[0].release);for(int i=0;i<256;i++)d_drum_tick();assert(!d_drum_voice[0].active);
 d_drum_on(36,100,4);d_drum_on(36,100,4);d_drum_off(36,4);assert(d_drum_voice[0].release);
 d_drum_clear();d_drum_on(38,100,0);d_drum_off(38,0);assert(!d_drum_voice[1].release);for(unsigned i=0;i<d_drum_length(1)*2+2;i++)d_drum_tick();assert(!d_drum_voice[1].active);
 if(argc>3){uint8_t wire[160];FILE *f=fopen(argv[3],"rb");assert(f);unsigned n=(unsigned)fread(wire,1,sizeof wire,f);fclose(f);assert(n>14&&wire[0]==240&&wire[n-1]==247);captured_n=0;assert(d_sample_sysex(wire+1,n-2));d_sample_service();assert(captured_n);int decoded=d_sample_unpack(captured+8,captured_n-9,result,sizeof result);assert(decoded>=5&&result[0]==DSP_TOKEN);}
 printf("PASS sample protocol/read/retry/stale/abort/timeout/busy/CRC, %u partial NOR power cuts with region sentinels/restart, PCM cache/loop seam/gate ownership and factory one-shot\n",operations);
 return 0;
}
