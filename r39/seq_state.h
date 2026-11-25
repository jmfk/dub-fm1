/* SPDX-License-Identifier: GPL-3.0-only; fixed, validated16-pattern store. */
#pragma once
#define DS_SOURCE 4
#define DS_MAGIC 0x31514553u
#define DS_PATTERNS 16
#define DS_STEPS 128
#define DS_STOP 16
enum {DS_ALWAYS,DS_EVERY,DS_CHANCE,DS_TRIG,DS_NOT_TRIG,DS_FILL,DS_NOT_FILL};
enum {DS_NOTE,DS_VEL,DS_LENGTH,DS_COND,DS_VALUE,DS_TRACK_LENGTH,DS_GRID,DS_DRUM,DS_NEXT,DS_REPEAT,DS_MUTE,DS_HITS,DS_ON,DS_PHASE,DS_KEY};
enum {DS_PLAY,DS_PAUSE,DS_TOGGLE,DS_PATTERN,DS_SET_FILL,DS_SET_TRIG,DS_SAVE,DS_CLEAR,DS_CONTINUE};
typedef struct {uint8_t note[4],velocity,length,condition,value;} ds_step;
typedef struct {ds_step step[128];uint8_t length,grid,mute,reserved;} ds_track;
typedef struct {ds_track track[4];uint16_t hits[128];uint8_t drums,repeats,next,reserved;} ds_pattern;
typedef struct {uint32_t magic,index,checksum;ds_pattern pattern[2];} ds_bank;
_Static_assert(sizeof(ds_bank)<=16372,"Sequence bank fits one native slot");
static ds_pattern ds_low[10];static ds_pattern ds_high[6] D_POOL;
static ds_pattern *ds_pattern_at(int i){return i<10?&ds_low[i]:&ds_high[i-10];}
static ds_bank ds_work;
static struct {uint32_t tick,phase,rng,elapsed,repeat,loop[4],step[4],play[4],due[4],off[4][128],pulses;volatile uint8_t pattern,playing,started,fill,trig;uint8_t sub,external;volatile int8_t saving;volatile uint8_t error;volatile uint32_t revision;} ds;
static volatile int ds_io=-1,ds_claim,ds_ack;
static int d_seq_is_drum(void){return ds_pattern_at(ds.pattern)->drums;}
static void d_seq_edit(int pattern,int field,int value,int note);
static void d_seq_control(int action,int value,int step,int track);
static void d_seq_sample(void),d_seq_clock_pulse(void),d_seq_init(void),d_seq_stop(void),d_seq_service(void);
static void d_seq_midi_transport(int status,int source);
