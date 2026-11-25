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
        /* Publish each control block before computing the next one. USB's
         * nested consumer must not wait for a whole expensive 256-frame synth. */
        for(b=0;b<HALF_FRAMES;b+=32u){
            int32_t *q=o+2u*b;
            app_audio(q,32u);
#ifdef APP_AUDIO_TAP
            APP_AUDIO_TAP(0,q,32u);
#endif
            for(i=0;i<64u;i++){
                int32_t v=(q[i]*master_q12)>>12;
                q[i]=v>32767?32767:v< -32767?-32767:v;
            }
            UAC_ONLY(uac_tap(q,32u));
#ifdef APP_AUDIO_TAP
            APP_AUDIO_TAP(1,q,32u);
#endif
        }
#if FM1_USB_PLAYBACK
        /* Preserve the proven playback feedback update once per 256 frames. */
        up_mix(o,HALF_FRAMES,master_q12);
#endif
#ifdef APP_AUDIO_TAP
        APP_AUDIO_TAP(2,o,HALF_FRAMES);
#endif
        for(i=0;i<HALF_WORDS;i++)o[i]*=128;
        fm1_audio_ack_half();
#ifdef APP_AUDIO_END
        APP_AUDIO_END();
#endif
    }
    in_audio = 0;
}
