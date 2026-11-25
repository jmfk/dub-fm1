/* Optional USB recording state; session-only, synthesis stays at 44100 Hz. */
#pragma once
static volatile unsigned char d_usb_mode,d_capture_active,d_usb_groups=7;
static int d_capture_main_gain=4096;
#ifndef D_CAPTURE_HOOK
#define D_CAPTURE_HOOK(frame) ((void)(frame))
#endif
