static void app_audio(int32_t *out, uint32_t n) {
    if(!ad_ready)return;
    if(ad_pause){ad_pause_ack=1;return;}
    ad_schedule();
    if (!panel_fault){
#ifndef DINGHY_HOST
        uint32_t start=fm1_ticks(),fx_before=ad_fx_ticks;
#endif
        a_render(out, n);if(!(a_frame&255u))ad_render_blocks++;
#ifndef DINGHY_HOST
        ad_fx_last=ad_fx_ticks;ad_synth_last+=fm1_ticks()-start-(ad_fx_ticks-fx_before);
        if(ad_synth_last>ad_synth_peak)ad_synth_peak=ad_synth_last;
        if(ad_fx_last>ad_fx_peak)ad_fx_peak=ad_fx_last;
#endif
    }
}
