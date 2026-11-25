/* Offline physical panel contract. */
#define DINGHY_HOST
#define main unused_host_entry
#include "app.c"
#undef main
#include <assert.h>
static void block(void) {
    int32_t out[512] = {0};
    app_audio(out, 256);
}
int main(void) {
    ad_ready=0;block();assert(!ad_render_blocks);
    a_init();
    panel_init();
    ad_ready=1;
    palette_set(4);
    ap[0].p[P_CUT]=23;ad_request=3;ad_control();block();ad_control();
    assert(ad_ready&&!ad_pause&&ap[0].p[P_CUT]==23);ap[0].p[P_CUT]=48;
    panel_fault=1;ad_recover=2;block();
    assert(!panel_fault && a_song_play && ad_recoveries==1);
    ad_recover=1;block();assert(!a_song_play);
    ad_wanted_case=1;ad_wanted_run=123;ad_request=1;
    ad_control();assert(ad_pause && !ad_pause_ack);
    block();assert(ad_pause_ack);ad_control();
    assert(ad_ready && ad_case==1 && ad_run==123 && ad_state==1);
    fm1_ms=ad_start_ms+4001;block();assert(ad_state==2);
    ad_request=2;ad_control();block();ad_control();assert(ad_state==0);
    end_ms = 1000000;
    host_out = "/tmp/analog-panel-r2";
    panel_button(0, 1, 1000);
    assert(panel.page[0] == 1);
    panel_button(7, 1, 1010);
    assert(panel.bank[0] == 1 && panel.page[0] == 1);
    panel_button(1, 1, 1020);
    assert(panel.group == 1 && panel.page[1] == 0);
    panel_button(0, 1, 1030);
    assert(panel.page[0] == 1);
    panel_turn(6, 3);
    assert(panel.track == 3);
    panel_key(0, 1);
    block();
    assert(ap[3].voice[0].gate);
    panel_turn(6, 1);
    panel_key(0, 0);
    block();
    assert(!ap[3].voice[0].gate && !ap[4].voice[0].gate);
    panel_button(5, 1, 2000);
    assert(panel_shifted());
    panel_button(6, 1, 2100);
    assert(panel.modal);
    panel_button(5, 0, 2200);
    panel_turn(0, 2);
    assert(panel.candidate == 6);
    panel_turn(6, -1);
    assert(panel.track == 3 && panel.candidate == ap[3].program);
    panel_button(4, 1, 2300);
    block();
    assert(!panel.modal);
    panel_button(5, 1, 3000);
    panel_button(5, 0, 3050);
    panel_button(5, 1, 3100);
    panel_button(5, 0, 3150);
    assert(panel.shift_latch);
    panel_button(5, 1, 3400);
    panel_button(5, 0, 3450);
    assert(!panel.shift_latch);
    for (uint32_t g = 0; g < 5; g++)
        for (uint32_t b = 0; b < 4; b++)
            for (uint32_t p = 0; p < 2; p++)
                for (uint32_t s = 0; s < 2; s++) {
                    panel.group = g;
                    panel.bank[g] = b;
                    panel.page[g] = p;
                    panel.shift_down = s;
                    panel_draw();
                    for (uint32_t k = 2; k < 6; k++)
                        panel_turn(k, 1);
                    block();
                }
    panel.shift_down = 0;
    panel_button(10, 1, 4000);
    block();
    assert(a_song_play);
    panel_button(11, 1, 4100);
    block();
    assert(!a_song_play);
    for (int i = 0; i < 4; i++)
        assert(!a_drums[i].left);
    puts("PASS: shared KEEL panel, independent pages, track/key ownership, Shift/latch/Back, patch "
         "selection, all160 page/shift renders, PLAY/HUSH.");
    return 0;
}
