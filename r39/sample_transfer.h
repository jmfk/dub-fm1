/* SPDX-License-Identifier: GPL-3.0-only; main-loop SMP1, inactive-bank writes only. */
#pragma once
#define D_SAMPLE_FLASH_MAGIC 0x31534650u /* PFS1 */
enum {DSP_OK,DSP_BUSY,DSP_BAD_DATA,DSP_BAD_CRC,DSP_OFFSET,DSP_FLASH,DSP_TOKEN,DSP_UNSUPPORTED,DSP_STALE};
static const uint8_t d_sample_prefix[5]={0x7d,'S','M','P',1};
static struct {
 uint32_t phase,bank,length,crc,received,token,last_at,erase_at,erase_end,counter;
 uint32_t begin_hash,last_hash,committed_token,committed_generation;
 uint8_t begin_cmd,begin_id,last_cmd,last_id,reply_len,reply_pending;
 uint8_t reply[160];
} d_sample_transfer D_POOL;
static int d_sample_active_bank=-1;
#if defined(D_SAMPLE_EXTERNAL_IO)
/* A future device-free NOR harness supplies the five d_sample_* IO functions. */
#elif defined(D_PLATFORM_MEMORY) && !defined(DINGHY_HOST)
_Static_assert(!BOARD_SMK37,"DUB sample store is FM-1 only");
_Static_assert(D_SAMPLE_BASE==SLOTS_AT+NSLOT*SLOT_SECT*0x1000u,"Sample bank overlaps sequence slots");
_Static_assert(D_SAMPLE_END<=FL_DATA_HI&&D_SAMPLE_END<=FL_OTA_LO&&D_SAMPLE_END<=BLOBS_AT,"Sample bank overlaps updater/presets");
static int d_sample_store_available(void){return flash_ok;}
static int d_sample_audio_streaming(void){return uac.flowing;}
static const uint8_t *d_sample_store_ptr(uint32_t off){return fm1_xip_ptr(off);}
static int d_sample_store_erase(uint32_t off){uint32_t took;return fl_erase4k(off,&took);}
static int d_sample_store_write(uint32_t off,const uint8_t *p,uint32_t n){int rc=fl_write(off,p,n);return rc?rc:fl_verify_xip(off,p,n);}
static int d_sample_wire_send(const uint8_t *p,uint32_t n){return ota_wire_send(p,n);}
#else
/* Simulator has no physical sample store; factory playback still works. */
static int d_sample_store_available(void){return 0;}
static int d_sample_audio_streaming(void){return 0;}
static const uint8_t *d_sample_store_ptr(uint32_t off){(void)off;return 0;}
static int d_sample_store_erase(uint32_t off){(void)off;return -1;}
static int d_sample_store_write(uint32_t off,const uint8_t *p,uint32_t n){(void)off;(void)p;(void)n;return -1;}
static int d_sample_wire_send(const uint8_t *p,uint32_t n){(void)p;(void)n;return -1;}
#endif
static uint32_t d_sample_message_crc(uint8_t cmd,uint8_t id,const uint8_t *p,uint32_t n){uint8_t h[2]={cmd,id};return ~d_sample_crc_more(d_sample_crc_more(0xffffffffu,h,2),p,n);}
static void d_sample_reply(uint8_t cmd,uint8_t id,uint32_t hash,uint8_t status,const uint8_t *p,uint32_t n){
 if(n>108)return;uint8_t body[116];body[0]=status;if(n)memcpy(body+1,p,n);d_sample_wr36(body+1+n,d_sample_message_crc(cmd|0x40u,id,body,n+1));
 uint8_t *out=d_sample_transfer.reply;out[0]=0xf0;memcpy(out+1,d_sample_prefix,5);out[6]=cmd|0x40u;out[7]=id;
 unsigned length=8+d_sample_pack(body,n+5,out+8);out[length++]=0xf7;
 d_sample_transfer.reply_len=(uint8_t)length;d_sample_transfer.reply_pending=1;d_sample_transfer.last_cmd=cmd;d_sample_transfer.last_id=id;d_sample_transfer.last_hash=hash;
}
static const uint8_t *d_sample_bank_ptr(unsigned bank){return d_sample_store_ptr(D_SAMPLE_BASE+bank*D_SAMPLE_BANK_BYTES);}
static int d_sample_bank_valid(unsigned bank,uint32_t *generation,uint32_t *length){
 const uint8_t *h=d_sample_bank_ptr(bank);if(!h)return 0;
 if(d_sample_rd32(h)!=D_SAMPLE_FLASH_MAGIC||d_sample_rd32(h+4)!=1||d_sample_rd32(h+28)!=~D_SAMPLE_FLASH_MAGIC||d_sample_rd32(h+24)||d_sample_crc(h,20)!=d_sample_rd32(h+20))return 0;
 uint32_t n=d_sample_rd32(h+12);if(n<D_SAMPLE_CATALOG_BYTES||n>D_SAMPLE_BLOB_MAX)return 0;
 const uint8_t *payload=h+D_SAMPLE_HEADER_BYTES;
 if(d_sample_crc(payload,n)!=d_sample_rd32(h+16)||!d_sample_catalog_valid(payload,n))return 0;
 *generation=d_sample_rd32(h+8);*length=n;return 1;
}
static void d_sample_store_init(void){
 d_sample_active_bank=-1;memset(&d_sample_transfer,0,sizeof d_sample_transfer);if(!d_sample_store_available())return;
 uint32_t generation=0,length=0;
 for(unsigned bank=0;bank<2;bank++){uint32_t g,n;if(d_sample_bank_valid(bank,&g,&n)&&(d_sample_active_bank<0||(int32_t)(g-generation)>0)){d_sample_active_bank=(int)bank;generation=g;length=n;}}
 if(d_sample_active_bank>=0){d_sample_publish(d_sample_bank_ptr((unsigned)d_sample_active_bank)+D_SAMPLE_HEADER_BYTES,length,generation);d_drum_cache_init();}
}
static void d_sample_transfer_cancel(void){d_sample_transfer.phase=0;if(d_sample_pause==2)d_qr=d_qw;__asm__ volatile("":::"memory");d_sample_pause=0;}
static uint8_t d_sample_empty_byte(uint32_t offset){
 if(offset==0)return 'F';if(offset==1)return 'S';if(offset==2)return 'B';if(offset==3)return '1';if(offset==4)return 1;if(offset==8)return 11;if(offset==13)return 2;return 0;
}
static int d_sample_sysex(const uint8_t *p,uint32_t n){
 if(n<5||memcmp(p,d_sample_prefix,5))return 0; /* owned namespace never reaches updater */
 if(n<12||p[5]<1||p[5]>8||p[6]>127)return 1;
 uint8_t cmd=p[5],id=p[6],body[116],out[108];int decoded=d_sample_unpack(p+7,n-7,body,sizeof body);if(decoded<4)return 1;
 uint32_t size=(uint32_t)decoded-4,hash=d_sample_message_crc(cmd,id,body,size);
 if(hash!=d_sample_rd32(body+size)){d_sample_reply(cmd,id,hash^0x80000000u,DSP_BAD_CRC,0,0);return 1;}
 if(d_sample_transfer.reply_pending)return 1;
 if(d_sample_transfer.reply_len&&cmd==d_sample_transfer.last_cmd&&id==d_sample_transfer.last_id&&hash==d_sample_transfer.last_hash){d_sample_transfer.reply_pending=1;return 1;}
 if(d_sample_transfer.phase==1||d_sample_transfer.phase==2)return 1; /* pending BEGIN */
 uint8_t status=DSP_OK;uint32_t length=0;d_sample_transfer.last_at=fm1_ms;
 if(cmd==1){
  if(size)status=DSP_BAD_DATA;else{
   uint32_t values[9]={1,D_SAMPLE_RATE,11,D_SAMPLE_BLOB_MAX,D_SAMPLE_PCM_MAX,d_sample_payload_bytes?d_sample_payload_bytes-D_SAMPLE_CATALOG_BYTES:0,d_sample_generation,d_sample_payload_bytes?d_sample_payload_bytes:D_SAMPLE_CATALOG_BYTES,(uint32_t)(d_sample_store_available()?1:0)|(d_sample_transfer.phase?2u:0u)};
   for(int i=0;i<9;i++)d_sample_wr36(out+i*4,values[i]);length=36;
  }
 }else if(cmd==2){
  if(size!=1||body[0]>=11)status=DSP_BAD_DATA;else{
   int pad=body[0];out[0]=(uint8_t)pad;out[1]=d_sample_views[pad].pcm?1:0;memset(out+2,0,40);
   if(out[1])memcpy(out+2,d_sample_payload+16+pad*40,40);else{
    const char *name=d_drum_names[pad];for(int k=0;k<23&&name[k];k++)out[2+k]=(uint8_t)name[k];
    d_sample_wr36(out+2+28,d_drum_length(pad));d_sample_wr36(out+2+32,D_SAMPLE_RATE);
   }length=42;
  }
 }else if(cmd==3){
  if(size!=9||!body[8]||body[8]>D_SAMPLE_CHUNK)status=DSP_BAD_DATA;
  else if(d_sample_rd32(body)!=d_sample_generation)status=DSP_STALE;
  else{uint32_t offset=d_sample_rd32(body+4),total=d_sample_payload_bytes?d_sample_payload_bytes:D_SAMPLE_CATALOG_BYTES,count=body[8];
   if(offset>total||count>total-offset)status=DSP_OFFSET;else{d_sample_wr36(out,offset);for(uint32_t i=0;i<count;i++)out[4+i]=d_sample_payload?d_sample_payload[offset+i]:d_sample_empty_byte(offset+i);length=4+count;}
  }
 }else if(cmd==4){
  if(size!=12)status=DSP_BAD_DATA;
  else if(d_sample_transfer.phase)status=DSP_BUSY;
  else if(!d_sample_store_available())status=DSP_UNSUPPORTED;
  else if(d_sample_rd32(body)!=d_sample_generation)status=DSP_STALE;
  else if(d_sample_rd32(body+4)<D_SAMPLE_CATALOG_BYTES||d_sample_rd32(body+4)>D_SAMPLE_BLOB_MAX)status=DSP_BAD_DATA;
  else if(d_bench_active||d_sample_audio_streaming()||ds.playing||d_osc_active_count()||d_sample_active_count()||preset_preview||preset_busy||preset_io>=0||ds.saving>=0||d_mix_io)status=DSP_BUSY;
  else{
   d_sample_transfer.bank=d_sample_active_bank<0?0u:(unsigned)d_sample_active_bank^1u;
   d_sample_transfer.length=d_sample_rd32(body+4);d_sample_transfer.crc=d_sample_rd32(body+8);d_sample_transfer.received=d_sample_transfer.erase_at=0;
   d_sample_transfer.erase_end=(D_SAMPLE_HEADER_BYTES+d_sample_transfer.length+4095u)&~4095u;
   d_sample_transfer.token=(++d_sample_transfer.counter)^fm1_ms^0x534d5001u;if(!d_sample_transfer.token)d_sample_transfer.token=1;
   d_sample_progress=0;d_sample_abort_requested=0;d_sample_transfer.begin_cmd=cmd;d_sample_transfer.begin_id=id;d_sample_transfer.begin_hash=hash;d_sample_transfer.phase=1;__asm__ volatile("":::"memory");d_sample_pause=1;return 1;
  }
 }else if(cmd==5){
  if(size<9||size>8+D_SAMPLE_CHUNK)status=DSP_BAD_DATA;
  else if(d_sample_transfer.phase!=3||d_sample_rd32(body)!=d_sample_transfer.token)status=DSP_TOKEN;
  else{uint32_t offset=d_sample_rd32(body+4),count=size-8,base=D_SAMPLE_BASE+d_sample_transfer.bank*D_SAMPLE_BANK_BYTES+D_SAMPLE_HEADER_BYTES;
   if(offset<d_sample_transfer.received&&count<=d_sample_transfer.received-offset){if(memcmp(d_sample_store_ptr(base+offset),body+8,count))status=DSP_OFFSET;}
   else if(offset!=d_sample_transfer.received||count>d_sample_transfer.length-offset)status=DSP_OFFSET;
   else if(d_sample_store_write(base+offset,body+8,count)){status=DSP_FLASH;d_sample_transfer_cancel();}
   else{d_sample_transfer.received+=count;d_sample_progress=25u+d_sample_transfer.received*70u/d_sample_transfer.length;}
   d_sample_wr36(out,d_sample_transfer.received);length=4;
  }
 }else if(cmd==6){
  if(size!=4)status=DSP_BAD_DATA;
  else if(!d_sample_transfer.phase&&d_sample_transfer.committed_token&&d_sample_rd32(body)==d_sample_transfer.committed_token){d_sample_wr36(out,d_sample_transfer.committed_generation);d_sample_wr36(out+4,d_sample_payload_bytes);length=8;}
  else if(d_sample_transfer.phase!=3||d_sample_rd32(body)!=d_sample_transfer.token)status=DSP_TOKEN;
  else if(d_sample_transfer.received!=d_sample_transfer.length)status=DSP_OFFSET;
  else{
   const uint8_t *payload=d_sample_bank_ptr(d_sample_transfer.bank)+D_SAMPLE_HEADER_BYTES;
   if(d_sample_crc(payload,d_sample_transfer.length)!=d_sample_transfer.crc)status=DSP_BAD_CRC;
   else if(!d_sample_catalog_valid(payload,d_sample_transfer.length))status=DSP_BAD_DATA;
   else{
    uint8_t header[32];memset(header,0,32);uint32_t generation=d_sample_generation+1u;
    d_sample_wr36(header,D_SAMPLE_FLASH_MAGIC);d_sample_wr36(header+4,1);d_sample_wr36(header+8,generation);d_sample_wr36(header+12,d_sample_transfer.length);d_sample_wr36(header+16,d_sample_transfer.crc);d_sample_wr36(header+20,d_sample_crc(header,20));d_sample_wr36(header+28,~D_SAMPLE_FLASH_MAGIC);
    if(d_sample_store_write(D_SAMPLE_BASE+d_sample_transfer.bank*D_SAMPLE_BANK_BYTES,header,32))status=DSP_FLASH;
    else{
     d_sample_publish(payload,d_sample_transfer.length,generation);d_sample_active_bank=(int)d_sample_transfer.bank;d_drum_clear();d_drum_cache_init();
     d_sample_transfer.committed_token=d_sample_transfer.token;d_sample_transfer.committed_generation=generation;
     d_sample_wr36(out,generation);d_sample_wr36(out+4,d_sample_transfer.length);length=8;
    }
   }d_sample_transfer_cancel();
  }
 }else if(cmd==7){
  if(size!=4)status=DSP_BAD_DATA;else if(d_sample_transfer.phase&&d_sample_rd32(body)!=d_sample_transfer.token)status=DSP_TOKEN;else d_sample_transfer_cancel();
 }else if(cmd==8){
  if(size!=10||body[0]>=11||!body[9]||body[9]>D_SAMPLE_CHUNK)status=DSP_BAD_DATA;
  else if(d_sample_rd32(body+1)!=d_sample_generation)status=DSP_STALE;
  else{unsigned pad=body[0];uint32_t offset=d_sample_rd32(body+5),count=body[9],total=d_drum_length(pad)*2u;
   if(offset>total||count>total-offset)status=DSP_OFFSET;else{out[0]=(uint8_t)pad;d_sample_wr36(out+1,offset);memcpy(out+5,(const uint8_t *)d_drum_source((int)pad)+offset,count);length=5+count;}
  }
 }
 d_sample_reply(cmd,id,hash,status,out,length);return 1;
}
static void d_sample_service(void){
 if(d_sample_abort_requested){d_sample_abort_requested=0;d_sample_transfer_cancel();d_sample_transfer.reply_len=d_sample_transfer.reply_pending=0;}
 if(d_sample_transfer.phase&&((uint32_t)(fm1_ms-d_sample_transfer.last_at)>30000u)){d_sample_transfer_cancel();d_sample_transfer.reply_len=d_sample_transfer.reply_pending=0;}
 if(d_sample_transfer.phase==1){
  if(d_sample_pause==3){d_sample_transfer_cancel();d_sample_reply(d_sample_transfer.begin_cmd,d_sample_transfer.begin_id,d_sample_transfer.begin_hash,DSP_BUSY,0,0);}
  else if(d_sample_pause==2)d_sample_transfer.phase=2;
 }
 if(d_sample_transfer.phase==2){
  if(d_sample_store_erase(D_SAMPLE_BASE+d_sample_transfer.bank*D_SAMPLE_BANK_BYTES+d_sample_transfer.erase_at)){
   d_sample_transfer_cancel();d_sample_reply(d_sample_transfer.begin_cmd,d_sample_transfer.begin_id,d_sample_transfer.begin_hash,DSP_FLASH,0,0);
  }else{
   d_sample_transfer.erase_at+=4096u;d_sample_progress=d_sample_transfer.erase_at*25u/d_sample_transfer.erase_end;d_sample_transfer.last_at=fm1_ms;
   if(d_sample_transfer.erase_at==d_sample_transfer.erase_end){uint8_t out[8];d_sample_wr36(out,d_sample_transfer.token);d_sample_wr36(out+4,0);d_sample_transfer.phase=3;d_sample_reply(d_sample_transfer.begin_cmd,d_sample_transfer.begin_id,d_sample_transfer.begin_hash,DSP_OK,out,8);}
  }
 }
 if(d_sample_transfer.reply_pending&&!d_sample_wire_send(d_sample_transfer.reply,d_sample_transfer.reply_len))d_sample_transfer.reply_pending=0;
}
