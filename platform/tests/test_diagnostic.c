#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "diagnostic.h"
static uint8_t reply[256];static uint32_t reply_n;
static int ad_wire_send(const uint8_t *p,uint32_t n){memcpy(reply,p,n);reply_n=n;return 1;}
static void call(uint32_t cmd,uint32_t a,uint32_t b){uint8_t p[16]={0x7d,0x41,0x4c,1,(uint8_t)cmd,17};for(int i=0;i<5;i++){p[6+i]=(a>>(7*i))&127;p[11+i]=(b>>(7*i))&127;}assert(ad_sysex(p,16));assert(reply_n>=9&&reply[0]==0xf0&&reply[reply_n-1]==0xf7);}
int main(void){
 assert(ad_crc("123456789",9)==0xcbf43926u);
 ad_live[AD_MAGIC_I]=AD_MAGIC;ad_live[AD_BUILD]=AD_BUILD_TAG;ad_live[AD_CONFIG_I]=AD_CONFIG;
 ad_log_count=64;ad_log_w=90;
 for(uint32_t i=0;i<64;i++)ad_log[(26+i)%64][0]=i;
 call(2,0,0);assert(!reply[7]&&ad_generation==1&&ad_frozen_size==4480);
 assert(ad_snapshot[96]==0&&ad_snapshot[96+63*16]==63);
 uint32_t crc=ad_frozen_crc;ad_live[0]=1;call(3,0,192);assert(!reply[7]&&ad_frozen_crc==crc&&ad_snapshot[0]==AD_MAGIC);
 call(3,4480,1);assert(reply[7]==2);call(3,0,193);assert(reply[7]==2);
 ad_epoch=1;call(2,0,0);assert(reply[7]==1&&ad_generation==1);ad_epoch=2;
 call(4,9,1);assert(reply[7]==2&&!ad_request);
 call(4,1,123);assert(!reply[7]&&ad_request==1&&ad_wanted_run==123);
 ad_request=0;ad_case=1;ad_run=123;ad_state=2;call(4,1,123);assert(!ad_request);
 call(5,0,124);assert(reply[7]==3&&!ad_request);
 call(6,3,65);assert(!reply[7]&&ad_echo_until==1);call(6,3,65);assert(reply[7]==2);
 uint8_t foreign[]={0x7d,0x42,0x4c,1};assert(!ad_sysex(foreign,4));
 puts("PASS: diagnostic CRC, immutable chronological ring, chunk bounds, seqlock busy, idempotence, token checks, MIDI probe and foreign namespace");
}
