/* SPDX-License-Identifier: GPL-3.0-only; generated parameter contract. */
#pragma once
enum {P_MODE,P_CUT,P_RES,P_ENV_AMT,P_LFO_RATE,P_LFO_AMT,P_LFO_SHAPE,P_KEYTRACK,P_LEVEL,P_PAN,P_DLY_A,P_DLY_B,P_REVERB,P_DRIVE,P_MUTE,P_CHANNEL,P_LFO_TARGET,P_LFO2_RATE,P_LFO2_AMT,P_LFO2_SHAPE,P_LFO2_TARGET,P_LFO3_RATE,P_LFO3_AMT,P_LFO3_SHAPE,P_LFO3_TARGET,P_COUNT};
static const int16_t p_default[25]={0,72,32,88,32,12,0,0,90,64,34,0,30,8,0,1,0,48,0,1,0,64,0,2,0};
static const int16_t p_max[25]={3,127,120,127,8191,127,2,127,127,127,127,127,127,127,1,16,50,8191,127,2,50,8191,127,2,50};
static const char *const p_name[]={"MODE","CUT","RES","ENV AMT","LFO RATE","LFO AMT","LFO SHAPE","KEYTRACK","LEVEL","PAN","DLY A","DLY B","REVERB","DRIVE","MUTE","CHANNEL","LFO TARGET","LFO2 RATE","LFO2 AMT","LFO2 SHAPE","LFO2 TARGET","LFO3 RATE","LFO3 AMT","LFO3 SHAPE","LFO3 TARGET"};
enum {G_TEMPO,G_MASTER,G_DLY_A_RET,G_DLY_B_RET,G_REV_RET,G_COUNT};
static const int16_t g_default[5]={112,96,80,70,80};
static const int16_t g_max[5]={240,127,127,127,127};
static const char *const g_name[]={"TEMPO","MASTER","DLY A RET","DLY B RET","REV RET"};
enum {D_MODE,D_TIME_MS,D_SYNC,D_FEEDBACK,D_TONE,D_DRIVE,D_MOD,D_ROUTE,D_WIDTH,D_TO_REV,D_COUNT};
static const int16_t d_default[10]={1,402,0,40,70,12,14,2,100,32};
static const int16_t d_max[10]={1,32000,16,94,127,127,127,2,127,127};
static const char *const d_name[]={"MODE","TIME MS","SYNC","FEEDBACK","TONE","DRIVE","MOD","ROUTE","WIDTH","TO REV"};
enum {V_DECAY,V_DAMPING,V_DIFFUSION,V_MOD,V_LOW_CUT,V_PREDELAY,V_COUNT};
static const int16_t v_default[6]={88,85,90,22,32,18};
static const int16_t v_max[6]={100,127,127,127,127,80};
static const char *const v_name[]={"DECAY","DAMPING","DIFFUSION","MOD","LOW CUT","PREDELAY"};
