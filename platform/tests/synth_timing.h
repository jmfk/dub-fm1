static void synth_usb_audio_begin(void) { synth_usb_started = fm1_ticks();ad_synth_last=ad_fx_last=ad_fx_ticks=0; }
static void synth_usb_audio_end(void) {
    if(!ad_ready||ad_pause)return;
    ad_publish(); /* included in the timing measurement */
    cpu_last = fm1_ticks() - synth_usb_started;
    if (cpu_last > cpu_peak)cpu_peak = cpu_last;
    /* Running integer mean avoids a long-run summation overflow. */
    if(ad_cpu_n<1000000u)ad_cpu_n++;
    if(cpu_last>=ad_cpu_sum)ad_cpu_sum+=(cpu_last-ad_cpu_sum)/ad_cpu_n;
    else ad_cpu_sum-=(ad_cpu_sum-cpu_last)/ad_cpu_n;
    if(cpu_last>256u*80000u/147u){cpu_late++;ad_consecutive++;}else ad_consecutive=0;
    if(ad_consecutive>=3){panel_fault=1;if(ad_state==1)ad_state=3;}
}
