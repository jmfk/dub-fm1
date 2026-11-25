void fm1_alnk0_irq(void)                       /* via isr_alnk0 (hal/fm1_isr.S) */
{
    uint8_t p;
    in_audio = 1;
    p = fm1_audio_pending();
    fm1_audio_ack_aux(p);
    if (p & FM1_AUDIO_HALF) {
#ifdef APP_AUDIO_BEGIN
        APP_AUDIO_BEGIN();
#endif
        int32_t *o = &abuf[fm1_audio_free_half() * HALF_WORDS];
        uint32_t i, b;
        for (i = 0; i < HALF_WORDS; i++)
            o[i] = 0;
        UAC_ONLY(uac_render_start());
        app_audio(o, HALF_FRAMES);
#ifdef APP_AUDIO_TAP
        APP_AUDIO_TAP(0, o, HALF_FRAMES);
#endif
        for (i = 0; i < HALF_WORDS; i++) {
            int32_t v = (o[i] * master_q12) >> 12;
            o[i] = v > 32767 ? 32767 : v < -32767 ? -32767 : v;
        }
        for (b = 0; b < HALF_FRAMES; b += 32u)
            UAC_ONLY(uac_tap(o + 2u * b, 32u));   /* the computer records what the speaker plays */
#ifdef APP_AUDIO_TAP
        APP_AUDIO_TAP(1, o, HALF_FRAMES);
#endif
#if FM1_USB_PLAYBACK
        up_mix(o, HALF_FRAMES, master_q12);       /* monitor host AFTER synth-only capture: no USB echo */
#endif
#ifdef APP_AUDIO_TAP
        APP_AUDIO_TAP(2, o, HALF_FRAMES);
#endif
        for (i = 0; i < HALF_WORDS; i++)
            o[i] *= 128;                        /* Q15 -> DAC, defined for negative samples too */
        fm1_audio_ack_half();
#ifdef APP_AUDIO_END
        APP_AUDIO_END();
#endif
    }
    in_audio = 0;
}
