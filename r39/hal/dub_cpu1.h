/* SPDX-License-Identifier: GPL-3.0-only; DUB adapter over SLOOP2.8.5 hardened X0X HAL. */
#pragma once
#include "fm1_cpu1.h"
#define D_DEVICE_CORES 1
#define D_CORE_SCRATCH __attribute__((section(".c1_bss")))
static uint32_t d_core_state,d_core_jobs,d_core_wait_ticks,d_core_job_ticks,d_core_peak_ticks,d_core_timeouts;
static volatile uint32_t d_core_test_result;
static uint32_t d_core_lcg(uint32_t x){for(unsigned i=0;i<64;i++)x=x*1664525u+1013904223u;return x;}
static void d_core_test(uint32_t x){d_core_test_result=d_core_lcg(x);}
static void d_core_boot(void){
 d_core_state=1;fm1_guard_pc_open();int rc=fm1_cpu1_start();fm1_guard_enable(FM1_GUARD_PC);if(rc)return;
 if(!fm1_cpu1_run(d_core_test,20261125u)||fm1_cpu1_wait()||d_core_test_result!=d_core_lcg(20261125u)){fm1_cpu1_hold();d_core_state=2;return;}d_core_state=3;
}
static int d_core_run(void (*fn)(uint32_t),uint32_t arg){return d_core_state==3?fm1_cpu1_run(fn,arg):0;}
static int d_core_wait(void){uint32_t start=fm1_ticks();int rc=fm1_cpu1_wait();d_core_wait_ticks+=fm1_ticks()-start;if(rc){d_core_state=2;d_core_timeouts++;}else d_core_jobs++;return rc;}
static uint32_t d_core_ticks(void){return fm1_ticks();}
